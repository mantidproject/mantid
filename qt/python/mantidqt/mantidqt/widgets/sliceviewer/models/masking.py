from abc import ABC, abstractmethod
from dataclasses import dataclass
from math import ceil, sqrt
from mantid.api import WorkspaceFactory, AnalysisDataService, AlgorithmManager
from sys import float_info
from numpy import inf


ALLOWABLE_ERROR_SIG_FIGS = 8


@dataclass
class TableRow:
    spec_list: str
    x_min: float
    x_max: float


class CursorInfoBase(ABC):
    def __init__(self, transpose):
        self._table_rows = None
        self._bin_width = 1
        self._transpose = transpose

    @abstractmethod
    def generate_table_rows(self):
        pass

    @abstractmethod
    def generate_inverted_table_rows(self):
        pass

    @staticmethod
    def create_consolidated_dict(table_rows):
        @dataclass()
        class XVal:
            start: bool
            val: float

        consolidated_rows = {}
        for row in table_rows:
            consolidated_rows.setdefault(row.spec_list, []).extend([XVal(start=True, val=row.x_min), XVal(start=False, val=row.x_max)])
        return consolidated_rows

    @staticmethod
    def consolidate_table_rows(table_rows):
        consolidated_rows = CursorInfoBase.create_consolidated_dict(table_rows)

        def sort_fn(e):
            return e.val

        new_table_rows = []
        for spec, xvals in consolidated_rows.items():
            xvals.sort(key=sort_fn)
            x_mins = [xvals[0].val]
            x_maxs = []
            found_end = False
            x_max = None
            if len(xvals) > 2:
                for val in xvals[1:-1]:
                    if not found_end and not val.start:
                        found_end = True
                        x_max = val.val
                    elif found_end and not val.start:
                        x_max = val.val
                    elif found_end and val.start:
                        x_maxs.append(x_max)
                        x_mins.append(val.val)
                        found_end = False
            x_maxs.append(xvals[-1].val)
            for i in range(len(x_mins)):
                new_table_rows.append(TableRow(spec_list=spec, x_min=x_mins[i], x_max=x_maxs[i]))
        return new_table_rows

    @staticmethod
    def consolidate_inverted_table_rows(table_rows):
        unpacked_table_rows = CursorInfoBase.unpack_table_rows(table_rows)
        consolidated_rows = CursorInfoBase.create_consolidated_dict(unpacked_table_rows)
        new_table_rows = []
        for spec, xvals in consolidated_rows.items():
            starts = set()
            stops = set()
            for val in xvals:
                if val.start:
                    starts.add(val.val)
                else:
                    stops.add(val.val)
            starts = sorted(starts)
            stops = sorted(stops)
            for start, stop in zip(starts, stops):
                if start < stop:
                    new_table_rows.append(TableRow(spec_list=spec, x_min=start, x_max=stop))

        packed_new_table_rows = CursorInfoBase.pack_table_rows(new_table_rows)
        return packed_new_table_rows

    @staticmethod
    def unpack_table_rows(table_rows):
        unpacked_table_rows = []
        for row in table_rows:
            if "-" not in row.spec_list:
                unpacked_table_rows.append(row)
                continue
            start, end = map(int, row.spec_list.split("-", 1))
            for spec in range(start, end + 1):
                unpacked_table_rows.append(TableRow(spec_list=str(spec), x_min=row.x_min, x_max=row.x_max))
        return unpacked_table_rows

    @staticmethod
    def pack_table_rows(table_rows):
        packed_table_rows = []
        packing_summary = {}
        for row in table_rows:
            key = (row.x_min, row.x_max)
            packing_summary.setdefault(key, []).extend([int(row.spec_list)])

        for (x_min, x_max), specs in packing_summary.items():
            ranges = CursorInfoBase.find_ranges(specs)
            for spec_list in ranges:
                packed_table_rows.append(
                    TableRow(
                        spec_list=spec_list,
                        x_min=x_min,
                        x_max=x_max,
                    )
                )
        return packed_table_rows

    @staticmethod
    def find_ranges(specs):
        specs = sorted(set(specs))
        ranges = []
        start = prev = specs[0]
        for spec in specs[1:]:
            if spec == prev + 1:
                prev = spec
                continue
            if start == prev:
                ranges.append(str(start))
            else:
                ranges.append(f"{start}-{prev}")
            start = prev = spec
        if start == prev:
            ranges.append(str(start))
        else:
            ranges.append(f"{start}-{prev}")
        return ranges

    @property
    def table_rows(self):
        return self._table_rows

    def snap_to_bin_centre(self, val):
        return ceil(val - self._bin_width / 2)


@dataclass
class Line:
    start: tuple[float]
    end: tuple[float]
    m: float
    c: float


class RectCursorInfoBase(CursorInfoBase, ABC):
    def __init__(self, click, release, transpose):
        super().__init__(transpose)
        self._click = click
        self._release = release

    def get_xy_data(self):
        y_data = sorted([self._click.data[1], self._release.data[1]])
        x_data = sorted([self._click.data[0], self._release.data[0]])
        return (x_data, y_data) if not self._transpose else (y_data, x_data)

    def get_image_axis_boundaries(self, click):
        x_min, x_max, spec_min, spec_max = click.extent
        # Round the spec min and max
        spec_min = int(ceil(spec_min))
        spec_max = self.snap_to_bin_centre((spec_max))
        return x_min, x_max, spec_min, spec_max

    def get_rows_outside_rect(self):
        # Get the axis limits
        x_min, x_max, spec_min, spec_max = self.get_image_axis_boundaries(self._click)
        x_data, y_data = self.get_xy_data()
        y_min, y_max = self.snap_to_bin_centre(y_data[0]), self.snap_to_bin_centre(y_data[-1])

        # Mask everything outside selected rectangle
        row1 = TableRow(spec_list=f"{spec_min}-{spec_max}", x_min=x_min, x_max=x_data[0])
        row2 = TableRow(spec_list=f"{spec_min}-{y_min}", x_min=x_data[0], x_max=x_data[-1])
        row3 = TableRow(spec_list=f"{y_max}-{spec_max}", x_min=x_data[0], x_max=x_data[-1])
        row4 = TableRow(spec_list=f"{spec_min}-{spec_max}", x_min=x_data[-1], x_max=x_max)
        return [row1, row2, row3, row4]


class RectCursorInfo(RectCursorInfoBase):
    def __init__(self, click, release, transpose):
        super().__init__(click, release, transpose)

    def generate_table_rows(self):
        x_data, y_data = self.get_xy_data()
        y_min, y_max = self.snap_to_bin_centre(y_data[0]), self.snap_to_bin_centre(y_data[-1])
        row = TableRow(spec_list=f"{y_min}-{y_max}", x_min=x_data[0], x_max=x_data[-1])
        return [row]

    def generate_inverted_table_rows(self):
        return self.get_rows_outside_rect()


class ElliCursorInfo(RectCursorInfoBase):
    def __init__(self, click, release, transpose):
        super().__init__(click, release, transpose)

    def generate_table_rows(self):
        x_data, y_data = self.get_xy_data()
        y_range, a, b, h, k = self._process_elli_parameters(x_data, y_data)

        rows = []
        for y in y_range:
            x_min, x_max = self._calc_x_val(y, a, b, h, k)
            x_min = x_min - 10**-ALLOWABLE_ERROR_SIG_FIGS if x_min == x_max else x_min  # slightly adjust min value so x vals are different.
            rows.append(TableRow(spec_list=str(round(y)), x_min=x_min, x_max=x_max))
        return self.consolidate_table_rows(rows)

    def _calc_x_val(self, y, a, b, h, k):
        return (h - self._calc_sqrt_portion(y, a, b, k)), (h + self._calc_sqrt_portion(y, a, b, k))

    @staticmethod
    def _calc_sqrt_portion(y, a, b, k):
        return sqrt(round((a**2) * (1 - ((y - k) ** 2) / (b**2)), ALLOWABLE_ERROR_SIG_FIGS))

    def _process_elli_parameters(self, x_data, y_data):
        y_min = self.snap_to_bin_centre(y_data[0])
        y_max = self.snap_to_bin_centre(y_data[1])
        y_range = [n / 3 for n in range(y_min * 3, (y_max * 3) + 1)]  # inclusive range with 1/3 step for greater resolution

        x_min, x_max = x_data[0], x_data[-1]
        a = (x_max - x_min) / 2
        b = (y_max - y_min) / 2
        h = x_min + a
        k = y_min + b
        return y_range, a, b, h, k

    def generate_inverted_table_rows(self):
        rect_rows = self.get_rows_outside_rect()

        x_data, y_data = self.get_xy_data()
        y_range, a, b, h, k = self._process_elli_parameters(x_data, y_data)

        rows = []
        for y in y_range:
            x_min, x_max = self._calc_x_val(y, a, b, h, k)
            # slightly adjust min value so x vals are different.
            x_min = x_min - 10**-ALLOWABLE_ERROR_SIG_FIGS if x_min == x_max else x_min
            rows.append(TableRow(spec_list=str(round(y)), x_min=x_data[0], x_max=x_min))
            rows.append(TableRow(spec_list=str(round(y)), x_min=x_max, x_max=x_data[-1]))
        rows = self.consolidate_table_rows(rows)
        return rect_rows + rows


class PolyCursorInfo(CursorInfoBase):
    def __init__(self, nodes, transpose, x_limits, y_limits):
        super().__init__(transpose)
        self._x_limits = x_limits
        self._y_limits = y_limits

        self._lines = self._generate_lines(nodes)
        if not self._check_intersecting_lines():
            raise RuntimeError("Polygon shapes with more than 1 intersection point are not supported.")

    def generate_table_rows(self):
        rows = []
        y_min, y_max = self._extract_global_y_limits()

        y_range = [n / 3 for n in range(y_min * 3, (y_max * 3) + 1)]  # inclusive range with 1/3 step for greater resolution
        for y in y_range:
            x_val_pairs = self._calculate_relevant_x_value_pairs(y)
            for x_min, x_max in x_val_pairs:
                rows.append(TableRow(spec_list=str(round(y)), x_min=x_min, x_max=x_max))
        return self.consolidate_table_rows(rows)

    def _calculate_relevant_x_value_pairs(self, y):
        x_vals = []
        for line in self._lines:
            y_bounds = sorted([line.start[1], line.end[1]])
            # if line is horizontal at value y.
            if y_bounds[1] >= y > y_bounds[0]:
                # if line is not vertical, else just use the line start x pos
                x = (y - line.c) / line.m if (abs(line.m) != inf and abs(line.m) != 0) else line.start[0]
                x_vals.append(x)
        x_vals.sort()

        open_close_pairs = []
        for i in range(0, len(x_vals), 2):
            x_min, x_max = x_vals[i], x_vals[i + 1]
            x_min = x_min - 10**-ALLOWABLE_ERROR_SIG_FIGS if x_min == x_max else x_min  # slightly adjust min value so x vals are different.
            open_close_pairs.append((x_min, x_max))
        return open_close_pairs

    def _extract_global_y_limits(self):
        y_min = float_info.max
        y_max = float_info.min
        for y_val in [y for line in self._lines for y in (line.start[1], line.end[1])]:
            if y_val < y_min:
                y_min = y_val
            if y_val > y_max:
                y_max = y_val
        return y_min, y_max

    def _generate_lines(self, nodes):
        node_count = len(nodes)
        lines = []
        for i in range(node_count):
            line = (nodes[i].data, nodes[i + 1].data) if i < node_count - 1 else (nodes[i].data, nodes[0].data)
            lines.append(self._generate_line(*line))
        return lines

    def _generate_line(self, start, end):
        y_index = 1 if not self._transpose else 0
        x_index = 0 if not self._transpose else 1
        start_y = self.snap_to_bin_centre(start[y_index])
        end_y = self.snap_to_bin_centre(end[y_index])
        start_x = start[x_index]
        end_x = end[x_index]
        m = (start_y - end_y) / (start_x - end_x)
        c = start_y - m * start_x
        return Line(start=(start_x, start_y), end=(end_x, end_y), m=m, c=c)

    def _check_intersecting_lines(self):
        line_count = len(self._lines)
        cache = []
        intersecting_lines = 0
        for i in range(line_count):
            for j in range(line_count):
                pair = sorted([i, j])
                if i != j and pair not in cache:
                    cache.append(pair)
                    if self._intersecting_line(self._lines[i], self._lines[j]):
                        intersecting_lines += 1
        return intersecting_lines <= 1

    @staticmethod
    def _intersecting_line(line_1, line_2):
        # if gradients are equal, or if lines intersect at a node
        if line_1.m == line_2.m or (line_1.start == line_2.end or line_2.start == line_1.end):
            return False

        x = (line_1.c - line_2.c) / (line_2.m - line_1.m)
        line_1_x = sorted([line_1.start[0], line_1.end[0]])
        line_2_x = sorted([line_2.start[0], line_2.end[0]])
        if not (line_1_x[0] < x < line_1_x[1]) or not (line_2_x[0] < x < line_2_x[1]):
            return False

        y = line_1.m * x + line_1.c
        line_1_y = sorted([line_1.start[1], line_1.end[1]])
        line_2_y = sorted([line_2.start[1], line_2.end[1]])
        if not (line_1_y[0] < y < line_1_y[1]) or not (line_2_y[0] < y < line_2_y[1]):
            return False
        return True

    def generate_inverted_table_rows(self):
        rows = []

        # Get the axis limits to mask eveything outside the selected polygon
        x_axis_min, x_axis_max = self._x_limits
        spec_min, spec_max = self._y_limits
        spec_min = int(ceil(spec_min))
        spec_max = self.snap_to_bin_centre((spec_max))
        y_min, y_max = self._extract_global_y_limits()

        # inclusive range with 1/3 step for greater resolution
        y_range = [n / 3 for n in range(y_min * 3, (y_max * 3) + 1)]

        # Mask the rectangles above and below the polygon
        rows.append(TableRow(spec_list=f"{spec_min}-{str(round(y_range[0]))}", x_min=x_axis_min, x_max=x_axis_max))
        rows.append(TableRow(spec_list=f"{str(round(y_range[-1]))}-{spec_max}", x_min=x_axis_min, x_max=x_axis_max))

        # Mask the area around the polygon
        for y in y_range:
            x_val_pairs = self._calculate_relevant_x_value_pairs(y)
            for x_min, x_max in x_val_pairs:
                rows.append(TableRow(spec_list=str(round(y)), x_min=x_axis_min, x_max=x_min))
                rows.append(TableRow(spec_list=str(round(y)), x_min=x_max, x_max=x_axis_max))
        return self.consolidate_table_rows(rows)


class MaskingModel:
    def __init__(self, ws_name, auto_update_mask_file=False):
        self._active_mask = None
        self._masks = []
        self._ws_name = ws_name or "ws"
        self._apply_inverted_mask = False
        self._auto_update_mask_file = auto_update_mask_file

    def update_active_mask(self, mask):
        self._active_mask = mask
        if self._auto_update_mask_file:
            self._masks.append(self._active_mask)
            self.export_selectors()
            self._masks.remove(self._active_mask)

    def clear_active_mask(self):
        self._active_mask = None

    def store_active_mask(self):
        if self._active_mask:
            self._masks.append(self._active_mask)
            self._active_mask = None

    def clear_stored_masks(self):
        self._masks = []

    def add_rect_cursor_info(self, click, release, transpose):
        self.update_active_mask(RectCursorInfo(click=click, release=release, transpose=transpose))

    def add_elli_cursor_info(self, click, release, transpose):
        self.update_active_mask(ElliCursorInfo(click=click, release=release, transpose=transpose))

    def add_poly_cursor_info(self, nodes, transpose, x_limits, y_limits):
        self.update_active_mask(PolyCursorInfo(nodes=nodes, transpose=transpose, x_limits=x_limits, y_limits=y_limits))

    def create_table_workspace_from_rows(self, table_rows, store_in_ads):
        # create table ws_from rows
        table_ws = WorkspaceFactory.createTable()
        table_ws.addColumn("str", "SpectraList")
        table_ws.addColumn("double", "XMin")
        table_ws.addColumn("double", "XMax")
        for row in table_rows:
            table_ws.addRow([row.spec_list, row.x_min, row.x_max])
        if store_in_ads:
            AnalysisDataService.addOrReplace(f"{self._ws_name}_sv_mask_tbl", table_ws)
        return table_ws

    def generate_mask_table_ws(self, store_in_ads=True):
        table_rows = []
        for info in self._masks:
            if self._apply_inverted_mask:
                table_rows.extend(info.generate_inverted_table_rows())
            else:
                table_rows.extend(info.generate_table_rows())
        if self._apply_inverted_mask and len(self._masks) > 1:
            table_rows = self._masks[0].consolidate_inverted_table_rows(table_rows)
        return self.create_table_workspace_from_rows(table_rows, store_in_ads)

    def export_selectors(self):
        _ = self.generate_mask_table_ws(store_in_ads=True)

    def apply_selectors(self):
        mask_ws = self.generate_mask_table_ws(store_in_ads=False)
        alg = AlgorithmManager.create("MaskBinsFromTable")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", self._ws_name)
        alg.setProperty("OutputWorkspace", self._ws_name)
        alg.setProperty("MaskingInformation", mask_ws)
        alg.setProperty("InputWorkspaceIndexType", "SpectrumNumber")
        alg.execute()

    def invert_masking_clicked(self, active):
        self._apply_inverted_mask = active
        if self._auto_update_mask_file and self._active_mask:
            self._masks.append(self._active_mask)
            self.export_selectors()
            self._masks.remove(self._active_mask)
