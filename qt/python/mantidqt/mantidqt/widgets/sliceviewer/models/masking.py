from abc import ABC, abstractmethod
from dataclasses import dataclass
from math import floor, ceil, sqrt
from mantid.api import WorkspaceFactory, AnalysisDataService, AlgorithmManager
from sys import float_info


@dataclass
class TableRow:
    spec_list: str
    x_min: float
    x_max: float


class CursorInfoBase(ABC):
    def __init__(self):
        self._table_rows = None

    @abstractmethod
    def generate_table_rows(self):
        pass

    @property
    def table_rows(self):
        return self._table_rows


@dataclass
class Line:
    start: float
    end: float
    m: float
    c: float


class RectCursorInfoBase(CursorInfoBase, ABC):
    def __init__(self, click, release):
        super().__init__()
        self._click = click
        self._release = release

    def get_xy_data(self):
        y_data = sorted([self._click.data[1], self._release.data[1]])
        x_data = sorted([self._click.data[0], self._release.data[0]])
        return x_data, y_data


class RectCursorInfo(RectCursorInfoBase):
    def __init__(self, click, release):
        super().__init__(click, release)

    def generate_table_rows(self):
        x_data, y_data = self.get_xy_data()
        row = TableRow(spec_list=f"{floor(y_data[0])}-{ceil(y_data[-1])}", x_min=x_data[0], x_max=x_data[-1])
        return [row]


class ElliCursorInfo(RectCursorInfoBase):
    def __init__(self, click, release):
        super().__init__(click, release)

    def generate_table_rows(self):
        x_data, y_data = self.get_xy_data()
        y_min = floor(y_data[0])  # Need to consider numeric axes
        y_max = ceil(y_data[-1])
        x_min, x_max = x_data[0], x_data[-1]
        a = (x_max - x_min) / 2
        b = (y_max - y_min) / 2
        h = x_min + a
        k = y_min + b
        rows = []
        for y in range(y_min, y_max + 1):
            x_min, x_max = self._calc_x_val(y, a, b, h, k)
            rows.append(TableRow(spec_list=str(y), x_min=x_min, x_max=x_max))
        return rows

    def _calc_x_val(self, y, a, b, h, k):
        return (h - self._calc_sqrt_portion(y, a, b, k)), (h + self._calc_sqrt_portion(y, a, b, k))

    @staticmethod
    def _calc_sqrt_portion(y, a, b, k):
        return sqrt((a**2) * (1 - ((y - k) ** 2) / (b**2)))


class PolyCursorInfo(CursorInfoBase):
    def __init__(self, nodes):
        super().__init__()
        self._nodes = nodes

    def generate_table_rows(self):
        rows = []
        lines = self._generate_lines()
        intersecting_lines = self._check_intersecting_lines(lines)
        if intersecting_lines > 1:
            # TODO: Remove offending shape.
            raise RuntimeError("Polygon shapes with more than 1 intersection point are not supported.")
        y_min, y_max = self._extract_global_y_limits(lines)
        for y in range(floor(y_min), ceil(y_max) + 1):
            x_val_pairs = self._calculate_relevant_x_value_pairs(lines, y)
            for x_min, x_max in x_val_pairs:
                rows.append(TableRow(spec_list=str(y), x_min=x_min, x_max=x_max))
        return rows

    @staticmethod
    def _calculate_relevant_x_value_pairs(lines, y):
        x_vals = []
        for line in lines:
            y_bounds = sorted([line.start[1], line.end[1]])
            if y_bounds[1] > y > y_bounds[0]:
                x = (y - line.c) / line.m
                x_vals.append(x)
        x_vals.sort()
        if not len(x_vals) % 2 == 0:
            raise ValueError("To form a close bounded shape, each spectra must have an even number of points.")
        open_close_pairs = []
        for i in range(0, len(x_vals), 2):
            open_close_pairs.append((x_vals[i], x_vals[i + 1]))
        return open_close_pairs

    @staticmethod
    def _extract_global_y_limits(lines):
        y_min = float_info.max
        y_max = float_info.min
        for y_val in [y for line in lines for y in (line.start[1], line.end[1])]:
            if y_val < y_min:
                y_min = y_val
            if y_val > y_max:
                y_max = y_val
        return y_min, y_max

    def _generate_lines(self):
        node_count = len(self._nodes)
        lines = []
        for i in range(node_count):
            line = (self._nodes[i].data, self._nodes[i + 1].data) if i < node_count - 1 else (self._nodes[i].data, self._nodes[0].data)
            lines.append(self._generate_line(*line))
        return lines

    @staticmethod
    def _generate_line(start, end):
        m = (start[1] - end[1]) / (start[0] - end[0])
        c = start[1] - m * start[0]
        return Line(start=start, end=end, m=m, c=c)

    def _check_intersecting_lines(self, lines):
        line_count = len(lines)
        cache = []
        intersecting_lines = 0
        for i in range(line_count):
            for j in range(line_count):
                pair = sorted([i, j])
                if i != j and pair not in cache:
                    cache.append(pair)
                    if self._intersecting_line(lines[i], lines[j]):
                        intersecting_lines += 1
        return intersecting_lines

    @staticmethod
    def _intersecting_line(line_1, line_2):
        # if gradients are equal, or if lines intersect at a node
        if line_1.m == line_2.m or (line_1.start == line_2.end or line_2.start == line_1.end):
            return False
        x = (line_1.c - line_2.c) / (line_2.m - line_1.m)
        x_data = sorted([line_1.start[0], line_1.end[0], line_2.start[0], line_2.end[0]])
        if not (x_data[1] < x < x_data[2]):
            return False

        y = line_1.m * x + line_1.c
        y_data = sorted([line_1.start[1], line_1.end[1], line_2.start[1], line_2.end[1]])
        if not (y_data[1] < y < y_data[2]):
            return False
        return True


class MaskingModel:
    def __init__(self, ws_name):
        self._active_mask = None
        self._masks = []
        self._ws_name = ws_name

    def update_active_mask(self, mask):
        self._active_mask = mask

    def clear_active_mask(self):
        self._active_mask = None

    def store_active_mask(self):
        self._masks.append(self._active_mask)
        self._active_mask = None

    def clear_stored_masks(self):
        self._masks = []

    def add_rect_cursor_info(self, click, release):
        self.update_active_mask(RectCursorInfo(click=click, release=release))

    def add_elli_cursor_info(self, click, release):
        self.update_active_mask(ElliCursorInfo(click=click, release=release))

    def add_poly_cursor_info(self, nodes):
        self.update_active_mask(PolyCursorInfo(nodes=nodes))

    def create_table_workspace_from_rows(self, table_rows, store_in_ads):
        # create table ws_from rows
        table_ws = WorkspaceFactory.createTable()
        table_ws.addColumn("str", "SpectraList")
        table_ws.addColumn("double", "XMin")
        table_ws.addColumn("double", "XMax")
        for row in table_rows:
            if not row.x_min == row.x_max:  # the min and max of the ellipse
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
