from abc import ABC, abstractmethod


class CursorInfoBase(ABC):
    def __init__(self):
        self._table_rows = None

    @abstractmethod
    def generate_table_rows(self):
        pass

    @property
    def table_rows(self):
        return self._table_rows


class RectCursorInfo(CursorInfoBase):
    def __init__(self, click, release):
        self._click = click
        self._release = release

    def generate_table_rows(self):
        return []


class PolyCursorInfo(CursorInfoBase):
    def __init__(self, nodes):
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

    def add_poly_cursor_info(self, nodes):
        self.update_active_mask(PolyCursorInfo(nodes=nodes))

    def create_table_workspace_from_rows(self, table_rows, store_in_ads):
        # create table ws_from rows
        return "ws"

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
