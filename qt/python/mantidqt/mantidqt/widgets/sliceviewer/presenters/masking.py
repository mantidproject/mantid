from matplotlib.widgets import RectangleSelector, EllipseSelector, PolygonSelector
from abc import ABC, abstractmethod

from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText
from .selector import make_selector_class, cursor_info
from mantidqt.widgets.sliceviewer.models.masking import MaskingModel


SHAPE_STYLE = {"alpha": 0.5, "linewidth": 1.75, "color": "black", "linestyle": "-"}
HANDLE_STYLE = {"alpha": 0.5, "markerfacecolor": "gray", "markersize": 4, "markeredgecolor": "gray"}
INACTIVE_HANDLE_STYLE = {"alpha": 0.5, "markerfacecolor": "gray", "markersize": 4, "markeredgecolor": "gray"}
INACTIVE_SHAPE_STYLE = {"alpha": 0.5, "linewidth": 1.75, "color": "gray", "linestyle": "-"}


class SelectionMaskingBase(ABC):
    def __init__(self, dataview, model):
        self._mask_drawn = False
        self._selector = None
        self._dataview = dataview
        self._model = model

    @abstractmethod
    def _on_selected(self, eclick, erelease):
        pass

    def set_active(self, state):
        self._selector.set_active(state)

    def clear(self):
        for artist in self._selector.artists:
            artist.remove()

    def disconnect(self):
        self._selector.disconnect_events()

    @abstractmethod
    def set_inactive_color(self):
        pass

    @property
    def _img(self):
        return self._dataview.ax.images[0]

    @property
    def mask_drawn(self):
        return self._mask_drawn


class RectangleSelectionMaskingBase(SelectionMaskingBase, ABC):
    """
    Base class for a selector with a `RectangleSelector` base.
    """

    def __init__(self, dataview, model, selector):
        super().__init__(dataview, model)
        self._selector = make_selector_class(selector)(
            dataview.ax,
            self._on_selected,
            useblit=False,  # rectangle persists on button release
            button=[1],
            minspanx=5,
            minspany=5,
            spancoords="pixels",
            interactive=True,
            props={"fill": False, **SHAPE_STYLE},
            handle_props=HANDLE_STYLE,
            ignore_event_outside=True,
        )

    @abstractmethod
    def add_cursor_info(self, click, release):
        pass

    def _on_selected(self, eclick, erelease):
        """
        Callback when a shape has been draw on the axes
        :param eclick: Event marking where the mouse was clicked
        :param erelease: Event marking where the mouse was released
        """
        cinfo_click = cursor_info(self._img, eclick.xdata, eclick.ydata)
        if cinfo_click is None:
            return
        cinfo_release = cursor_info(self._img, erelease.xdata, erelease.ydata)
        if cinfo_release is None:
            return

        # Add shape object to collection
        self._mask_drawn = True
        self._dataview.mpl_toolbar.set_action_checked(SELECTOR_TO_TOOL_ITEM_TEXT[self.__class__], False, trigger=False)
        self.add_cursor_info(cinfo_click, cinfo_release)

    def set_inactive_color(self):
        self._selector.set_props(fill=False, **INACTIVE_SHAPE_STYLE)
        self._selector.set_handle_props(**INACTIVE_HANDLE_STYLE)


class RectangleSelectionMasking(RectangleSelectionMaskingBase):
    """
    Draws a mask from a rectangular selection
    """

    def __init__(self, dataview, model):
        super().__init__(dataview, model, RectangleSelector)

    def add_cursor_info(self, click, release):
        self._model.add_rect_cursor_info(click=click, release=release)


class EllipticalSelectionMasking(RectangleSelectionMaskingBase):
    """
    Draws a mask from an elliptical selection
    """

    def __init__(self, dataview, model):
        super().__init__(dataview, model, EllipseSelector)

    def add_cursor_info(self, click, release):
        self._model.add_elli_cursor_info(click=click, release=release)


class PolygonSelectionMasking(SelectionMaskingBase):
    """
    Draws a mask from a polygon selection
    """

    def __init__(self, dataview, model):
        super().__init__(dataview, model)
        self._selector = make_selector_class(PolygonSelector)(
            dataview.ax,
            self._on_selected,
            useblit=False,  # rectangle persists on button release
            props=SHAPE_STYLE,
            handle_props=HANDLE_STYLE,
        )

    def _on_selected(self, eclick, erelease=None):
        """
        Callback when a polgyon shape has been draw on the axes
        :param eclick: Event marking where the mouse was clicked
        :param erelease: None for polygon selector
        """
        self._mask_drawn = True
        self._dataview.mpl_toolbar.set_action_checked(SELECTOR_TO_TOOL_ITEM_TEXT[self.__class__], False, trigger=False)
        nodes = [cursor_info(self._img, node[0], node[1]) for node in eclick]
        self._model.add_poly_cursor_info(nodes)

    def set_inactive_color(self):
        self._selector.set_props(**INACTIVE_SHAPE_STYLE)
        self._selector.set_handle_props(**INACTIVE_HANDLE_STYLE)


TOOL_ITEM_TEXT_TO_SELECTOR = {
    ToolItemText.RECT_MASKING: RectangleSelectionMasking,
    ToolItemText.ELLI_MASKING: EllipticalSelectionMasking,
    ToolItemText.POLY_MASKING: PolygonSelectionMasking,
}
SELECTOR_TO_TOOL_ITEM_TEXT = {v: k for k, v in TOOL_ITEM_TEXT_TO_SELECTOR.items()}


class Masking:
    """
    Manages Masking
    """

    def __init__(self, dataview):
        self._selectors = []
        self._active_selector = None
        self._dataview = dataview
        self._model = MaskingModel()

    def _reset_active_selector(self):
        if self._active_selector:
            self._active_selector.set_active(False)
            self._active_selector.disconnect()
            if self._active_selector.mask_drawn:
                self._active_selector.set_inactive_color()
                self._selectors.append(self._active_selector)
                self._model.store_active_mask()
            else:
                self._active_selector.clear()
                self._model.clear_active_mask()
            self._active_selector = None

    def _new_selector(self, selector_type):
        self._reset_active_selector()
        self._active_selector = selector_type(self._dataview, self._model)
        self._active_selector.set_active(True)

    def new_selector(self, text: str):
        self._new_selector(TOOL_ITEM_TEXT_TO_SELECTOR[text])

    def export_selectors(self):
        self._reset_active_selector()
        self._model.export_selectors()

    def apply_selectors(self):
        self._reset_active_selector()
        self._model.apply_selectors()

    def clear_and_disconnect(self):
        if self._active_selector:
            self._active_selector.set_active(False)
            self._active_selector.disconnect()
            if self._active_selector.mask_drawn:
                self._active_selector.clear()
        for selector in self._selectors:
            selector.clear()

    def clear_model(self):
        self._model.clear_active_mask()
        self._model.clear_stored_masks()
