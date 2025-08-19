from abc import ABC, abstractmethod
from dataclasses import dataclass
from math import floor, ceil, sqrt
from mantid.api import WorkspaceFactory, AnalysisDataService


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


class RectCursorInfoBase(CursorInfoBase, ABC):
    def __init__(self, click, release):
        super().__init__()
        self._click = click
        self._release = release

    def get_xy_data(self):
        y_data = [self._click.data[1], self._release.data[1]]
        x_data = [self._click.data[0], self._release.data[0]]
        y_data.sort()
        x_data.sort()
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
        return []


class MaskingModel:
    def __init__(self):
        self._active_mask = None
        self._masks = []

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
        # apply mask ws to underlying workspace
