from matplotlib.widgets import RectangleSelector, EllipseSelector, PolygonSelector
from matplotlib.axes import Axes
from abc import ABC, abstractmethod

from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText
from .selector import make_selector_class, cursor_info


class Masking:
    """
    Manages Masking
    """

    def __init__(self, ax):
        self._selectors = []
        self._active_selector = None
        self._ax = ax

    def _new_selector(self, selector_type):
        if self._active_selector and self._active_selector.mask_drawn:
            self._selectors.append(self._active_selector)
            self._active_selector.set_active(False)
        self._active_selector = selector_type(self._ax)
        self._active_selector.set_active(True)

    def new_selector(self, text: str):
        match text:
            case ToolItemText.RECT_MASKING:
                self._new_selector(RectangleSelectionMasking)
            case ToolItemText.ELLI_MASKING:
                self._new_selector(EllipticalSelectionMasking)
            case ToolItemText.POLY_MASKING:
                self._new_selector(PolygonSelectionMasking)

    def export_selectors(self):
        pass

    def apply_selectors(self):
        pass

    def clear(self):
        self._active_selector.set_active(False)
        for selector in self._selectors:
            selector.clear()


class SelectionMaskingBase(ABC):
    def __init__(self, ax):
        self._img = ax.images[0]
        self._cursor_info = None
        self.mask_drawn = False
        self._selector = None

    @abstractmethod
    def _on_selected(self, eclick, erelease):
        pass

    def set_active(self, state):
        self._selector.set_active(state)

    def clear(self):
        for artist in self._selector.artists:
            artist.remove()


class RectangleSelectionMaskingBase(SelectionMaskingBase):
    """
    Base class for a selector with a `RectangleSelector` base.
    """

    def __init__(self, ax: Axes, selector):
        super().__init__(ax)
        self._selector = make_selector_class(selector)(
            ax,
            self._on_selected,
            useblit=False,  # rectangle persists on button release
            button=[1],
            minspanx=5,
            minspany=5,
            spancoords="pixels",
            interactive=True,
        )

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
        self._cursor_info = (cinfo_click, cinfo_release)
        self.mask_drawn = True


class RectangleSelectionMasking(RectangleSelectionMaskingBase):
    """
    Draws a mask from a rectangular selection
    """

    def __init__(self, ax):
        super().__init__(ax, RectangleSelector)


class EllipticalSelectionMasking(RectangleSelectionMaskingBase):
    """
    Draws a mask from an elliptical selection
    """

    def __init__(self, ax):
        super().__init__(ax, EllipseSelector)


class PolygonSelectionMasking(SelectionMaskingBase):
    """
    Draws a mask from a polygon selection
    """

    def __init__(self, ax: Axes):
        super().__init__(ax)
        self._selector = make_selector_class(PolygonSelector)(
            ax,
            self._on_selected,
            useblit=False,  # rectangle persists on button release
        )

    def _on_selected(self, eclick, erelease=None):
        """
        Callback when a polgyon shape has been draw on the axes
        :param eclick: Event marking where the mouse was clicked
        :param erelease: None for polygon selector
        """
        self.mask_drawn = True
