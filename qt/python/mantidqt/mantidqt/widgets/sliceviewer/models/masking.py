from abc import ABC, abstractmethod
from dataclasses import dataclass
from math import ceil, sqrt
from mantid.api import WorkspaceFactory, AnalysisDataService, AlgorithmManager
from sys import float_info
from numpy import searchsorted, where, inf

ALLOWABLE_ERROR = 8


@dataclass
class TableRow:
    spec_list: str
    x_min: float
    x_max: float


class CursorInfoBase(ABC):
    def __init__(self, numeric_axis):
        self._table_rows = None
        self._numeric_axis = numeric_axis
        self._bin_width = 1

    @abstractmethod
    def generate_table_rows(self):
        pass

    def consolidate_table_rows(self, table_rows):
        @dataclass()
        class XVal:
            start: bool
            val: float

        def sort_fn(e):
            return e.val

        consolidated_rows = {}
        for row in table_rows:
            if row.spec_list not in consolidated_rows:
                consolidated_rows[row.spec_list] = []
            consolidated_rows[row.spec_list].extend([XVal(start=True, val=row.x_min), XVal(start=False, val=row.x_max)])

        new_table_rows = []
        for row in consolidated_rows.items():
            row[1].sort(key=sort_fn)
            x_mins = [row[1][0].val]
            x_maxs = []
            found_end = False
            x_max = None
            if len(row[1]) > 2:
                for val in row[1][1:-1]:
                    if not found_end and not val.start:
                        found_end = True
                        x_max = val.val
                    elif found_end and not val.start:
                        x_max = val.val
                    elif found_end and val.start:
                        x_maxs.append(x_max)
                        x_mins.append(val.val)
                        found_end = False
            x_maxs.append(row[1][-1].val)
            for i in range(len(x_mins)):
                new_table_rows.append(TableRow(spec_list=row[0], x_min=x_mins[i], x_max=x_maxs[i]))
        return new_table_rows

    @property
    def table_rows(self):
        return self._table_rows

    def get_y_val_index(self, y_val, apply_floor=False):
        adj = 1 if apply_floor else 0
        return int(searchsorted(self._numeric_axis, y_val)) - adj

    @property
    def numeric_axis(self):
        return self._numeric_axis is not None

    def snap_to_bin_centre(self, val):
        return ceil(val - self._bin_width / 2)


@dataclass
class Line:
    start: tuple[float]
    end: tuple[float]
    m: float
    c: float


class RectCursorInfoBase(CursorInfoBase, ABC):
    def __init__(self, click, release, is_numeric):
        super().__init__(is_numeric)
        self._click = click
        self._release = release

    def get_xy_data(self):
        y_data = sorted([self._click.data[1], self._release.data[1]])
        x_data = sorted([self._click.data[0], self._release.data[0]])
        return x_data, y_data


class RectCursorInfo(RectCursorInfoBase):
    def __init__(self, click, release, is_numeric):
        super().__init__(click, release, is_numeric)

    def generate_table_rows(self):
        x_data, y_data = self.get_xy_data()
        if self.numeric_axis:
            y_min, y_max = self.get_y_val_index(y_data[0]), self.get_y_val_index(y_data[-1]) - 1
        else:
            y_min, y_max = self.snap_to_bin_centre(y_data[0]), self.snap_to_bin_centre(y_data[-1])
        row = TableRow(spec_list=f"{y_min}-{y_max}", x_min=x_data[0], x_max=x_data[-1])
        return [row]


class ElliCursorInfo(RectCursorInfoBase):
    def __init__(self, click, release, is_numeric):
        super().__init__(click, release, is_numeric)

    def generate_table_rows(self):
        x_data, y_data = self.get_xy_data()
        if self.numeric_axis:
            y_min = self._numeric_axis[self.get_y_val_index(y_data[0], True)]
            y_max = self._numeric_axis[self.get_y_val_index(y_data[1])]
            y_range = self._numeric_axis[self.get_y_val_index(y_data[0], True) : self.get_y_val_index(y_data[1]) + 1]
            base_index = where(self._numeric_axis == y_range[0])[0][0]
        else:
            y_min = self.snap_to_bin_centre(y_data[0])
            y_max = self.snap_to_bin_centre(y_data[1])
            y_range = [n / 3 for n in range(y_min * 3, (y_max * 3) + 1)]  # inclusive range with 1/3 step for greater resolution
            base_index = 0

        x_min, x_max = x_data[0], x_data[-1]
        a = (x_max - x_min) / 2
        b = (y_max - y_min) / 2
        h = x_min + a
        k = y_min + b

        rows = []
        for index, y in enumerate(y_range):
            x_min, x_max = self._calc_x_val(y, a, b, h, k)
            ws_index = base_index + index if self.numeric_axis else round(y)
            x_min = x_min - 10**-ALLOWABLE_ERROR if x_min == x_max else x_min  # slightly adjust min value so x vals are different.
            rows.append(TableRow(spec_list=str(ws_index), x_min=x_min, x_max=x_max))
        return self.consolidate_table_rows(rows)

    def _calc_x_val(self, y, a, b, h, k):
        return (h - self._calc_sqrt_portion(y, a, b, k)), (h + self._calc_sqrt_portion(y, a, b, k))

    @staticmethod
    def _calc_sqrt_portion(y, a, b, k):
        return sqrt(round((a**2) * (1 - ((y - k) ** 2) / (b**2)), ALLOWABLE_ERROR))


class PolyCursorInfo(CursorInfoBase):
    def __init__(self, nodes, is_numeric):
        super().__init__(is_numeric)
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
            if y_bounds[1] >= y >= y_bounds[0]:
                x = (y - line.c) / line.m if (abs(line.m) != inf and abs(line.m) != 0) else line.start[0]
                x_vals.append(x)
        x_vals.sort()
        if not len(x_vals) % 2 == 0:
            # To form a close bounded shape, each spectra must have an even number of points.
            # Drop either the first or last x value, as this must correspond to a node in a line-node pair.
            x_vals = x_vals[1:] if len(set(x_vals[:2])) == 1 else x_vals[:-1]
        open_close_pairs = []
        for i in range(0, len(x_vals), 2):
            x_min, x_max = x_vals[i], x_vals[i + 1]
            x_min = x_min - 10**-ALLOWABLE_ERROR if x_min == x_max else x_min  # slightly adjust min value so x vals are different.
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
        start_y = self.snap_to_bin_centre(start[1])
        end_y = self.snap_to_bin_centre(end[1])
        start_x = start[0]
        end_x = end[0]
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


class MaskingModel:
    def __init__(self, ws_name, numeric_axis):
        self._active_mask = None
        self._masks = []
        self._ws_name = ws_name
        self._numeric_axis = numeric_axis

    def update_active_mask(self, mask):
        self._active_mask = mask

    def clear_active_mask(self):
        self._active_mask = None

    def store_active_mask(self):
        if self._active_mask:
            self._masks.append(self._active_mask)
            self._active_mask = None

    def clear_stored_masks(self):
        self._masks = []

    def add_rect_cursor_info(self, click, release):
        self.update_active_mask(RectCursorInfo(click=click, release=release, is_numeric=self._numeric_axis))

    def add_elli_cursor_info(self, click, release):
        self.update_active_mask(ElliCursorInfo(click=click, release=release, is_numeric=self._numeric_axis))

    def add_poly_cursor_info(self, nodes):
        self.update_active_mask(PolyCursorInfo(nodes=nodes, is_numeric=self._numeric_axis))

    @staticmethod
    def create_table_workspace_from_rows(table_rows, store_in_ads):
        # create table ws_from rows
        table_ws = WorkspaceFactory.createTable()
        table_ws.addColumn("str", "SpectraList")
        table_ws.addColumn("double", "XMin")
        table_ws.addColumn("double", "XMax")
        for row in table_rows:
            # if not row.x_min == row.x_max:  # the min and max of the ellipse
            table_ws.addRow([row.spec_list, row.x_min, row.x_max])
        if store_in_ads:
            AnalysisDataService.addOrReplace("svmask_ws", table_ws)
        return table_ws

    def generate_mask_table_ws(self, store_in_ads=True):
        table_rows = []
        for info in self._masks:
            table_rows.extend(info.generate_table_rows())
        return self.create_table_workspace_from_rows(table_rows, store_in_ads)

    def export_selectors(self):
        _ = self.generate_mask_table_ws()

    def apply_selectors(self):
        mask_ws = self.generate_mask_table_ws(store_in_ads=False)
        alg = AlgorithmManager.create("MaskBinsFromTable")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("InputWorkspace", self._ws_name)
        alg.setProperty("OutputWorkspace", self._ws_name)
        alg.setProperty("MaskingInformation", mask_ws)
        alg.execute()
