# Mantid Repository : https://github.com/mantidproject/mantid#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from typing import Callable, Optional

from .view import RegionSelectorView
from ..observers.observing_presenter import ObservingPresenter
from ..sliceviewer.models.dimensions import Dimensions
from ..sliceviewer.models.workspaceinfo import WorkspaceInfo, WS_TYPE
from ..sliceviewer.presenters.base_presenter import SliceViewerBasePresenter
from mantid.api import RegionSelectorObserver

# 3rd party imports
from matplotlib.widgets import RectangleSelector


class Selector(RectangleSelector):
    active_handle_alpha = 0.5
    kwargs = {
        "useblit": False,  # rectangle persists on button release
        "button": [1],
        "minspanx": 5,
        "minspany": 5,
        "spancoords": "pixels",
        "interactive": True,
    }

    def __init__(self, region_type: str, color: str, hatch: str, *args):
        self.kwargs["props"] = dict(facecolor=color, edgecolor=color, alpha=0.4, hatch=hatch, linewidth=3, fill=True)
        self.kwargs["handle_props"] = dict(markersize=6)
        self.kwargs["drag_from_anywhere"] = True
        self.kwargs["ignore_event_outside"] = True

        super().__init__(*args, **self.kwargs)
        self._region_type = region_type

    def region_type(self):
        return self._region_type

    def set_active(self, active: bool) -> None:
        """Hide the handles of a selector if it is not active."""
        self.set_handle_props(alpha=self.active_handle_alpha if active else 0)
        super().set_active(active)


class RegionSelector(ObservingPresenter, SliceViewerBasePresenter):
    def __init__(self, ws=None, parent=None, view=None, image_info_widget=None):
        if ws and WorkspaceInfo.get_ws_type(ws) != WS_TYPE.MATRIX:
            raise NotImplementedError("Only Matrix Workspaces are currently supported by the region selector.")

        self.notifyee = None
        self.view = view if view else RegionSelectorView(self, parent, image_info_widget=image_info_widget)
        super().__init__(ws, self.view.data_view)
        self._selectors: list[Selector] = []
        self._drawing_region = False

        if ws:
            self._initialise_dimensions(ws)
            self._set_workspace(ws)

    def subscribe(self, notifyee: RegionSelectorObserver):
        self.notifyee = notifyee

    def dimensions_changed(self) -> None:
        self.new_plot()

    def slicepoint_changed(self) -> None:
        pass

    def deselect_all_selectors(self) -> None:
        for selector in self._selectors:
            selector.set_active(False)

    def canvas_clicked(self, event) -> None:
        if self._drawing_region:
            return

        for selector in self._selectors:
            selector.set_active(False)

        clicked_selector = self._find_selector_if(lambda x: self._contains_point(x.extents, event.xdata, event.ydata))
        if clicked_selector is not None:
            # Ensure only one selector is active to avoid confusing matplotlib behaviour
            clicked_selector.set_active(True)

    def key_pressed(self, event) -> None:
        """Handles key press events."""
        if event.key == "delete":
            selector = self._find_selector_if(lambda x: x.active)
            if selector is not None:
                self._remove_selector(selector)

    def mouse_moved(self, event) -> None:
        """Handles mouse move events on the canvas."""
        # Find selector if it is active and the mouse is hovering over it
        selector = self._find_selector_if(lambda x: x.active and self._contains_point(x.extents, event.xdata, event.ydata))

        # Set an override cursor if a selector is found
        self.view.set_override_cursor(selector is not None)

    def zoom_pan_clicked(self, active) -> None:
        pass

    def new_plot(self, *args, **kwargs):
        if self.model.ws:
            self.new_plot_matrix()

    def nonorthogonal_axes(self, state: bool) -> None:
        pass

    def clear_workspace(self):
        self._selectors = []
        self.model.ws = None
        self.view.clear_figure()

    def update_workspace(self, workspace) -> None:
        if WorkspaceInfo.get_ws_type(workspace) != WS_TYPE.MATRIX:
            raise NotImplementedError("Only Matrix Workspaces are currently supported by the region selector.")

        if not self.model.ws:
            self._initialise_dimensions(workspace)

        self._set_workspace(workspace, True)

    def cancel_drawing_region(self):
        """
        Cancel drawing a region if a different toolbar option is pressed.
        """
        if self._drawing_region:
            self._selectors.pop()
            self._drawing_region = False

    def add_rectangular_region(self, region_type: str, color: str, hatch: str):
        """
        Add a rectangular region selection tool.
        """
        for selector in self._selectors:
            selector.set_active(False)

        if self._drawing_region:
            self._selectors.pop()

        self._selectors.append(self._create_selector(region_type, color, hatch))

        self._drawing_region = True

    def get_region(self, region_type):
        # extents contains x1, x2, y1, y2. Just store y (spectra) for now
        result = []
        for selector in self._selectors:
            if selector.region_type() == region_type:
                result.extend([selector.extents[2], selector.extents[3]])
        return result

    def display_rectangular_region(self, region_type: str, color: str, hatch: str, y1: float, y2: float):
        """
        Add a rectangular region selector to the given location on the plot, if it does not already exist
        and if it is located within the y-axis bounds.
        This method currently only takes y values and will automatically set some x values.
        """

        def is_same_location(extents):
            return self._equal_within_tolerance(extents[2], y1) and self._equal_within_tolerance(extents[3], y2)

        existing_selector = self._find_selector_if(lambda x: x.region_type() == region_type and is_same_location(x.extents))
        if existing_selector:
            return

        y_min, y_max = self.view.data_view.ax.get_ybound()
        if y1 < y_min or y2 > y_max:
            return

        # Set some values for x1 and x2
        x_min, x_max = self.view.data_view.ax.get_xbound()
        x_width = (x_max - x_min) / 4
        x1 = x_min + x_width
        x2 = x1 + x_width

        selector = self._create_selector(region_type, color, hatch, (x1, x2, y1, y2))
        selector.set_active(False)
        self._selectors.append(selector)

    def _initialise_dimensions(self, workspace):
        self.view.create_dimensions(dims_info=Dimensions.get_dimensions_info(workspace))
        self.view.create_axes_orthogonal(redraw_on_zoom=not WorkspaceInfo.can_support_dynamic_rebinning(workspace))

    def _set_workspace(self, workspace, show_all_data=False):
        self.model.ws = workspace
        self.view.set_workspace(workspace)
        self.new_plot()
        if show_all_data:
            self.show_all_data_clicked()

    def _on_rectangle_selected(self, eclick, erelease):
        """
        Callback when a rectangle has been draw on the axes
        :param eclick: Event marking where the mouse was clicked
        :param erelease: Event marking where the mouse was released
        """
        self._drawing_region = False

        if self.notifyee:
            self.notifyee.notifyRegionChanged()

    def _remove_selector(self, selector: Selector) -> None:
        """
        Removes a selector from the plot
        :param selector: The selector to be removed from the plot
        """
        selector.set_active(False)
        for artist in selector.artists:
            artist.set_visible(False)
        selector.update()
        self._selectors.remove(selector)

        if self.notifyee:
            self.notifyee.notifyRegionChanged()

    def _find_selector_if(self, predicate: Callable) -> Optional[Selector]:
        """
        Find the first selector which agrees with a predicate. Return None if no selector is found
        :param predicate: A callable function or lambda that takes a Selector and returns a boolean
        """
        for selector in self._selectors:
            if predicate(selector):
                return selector
        return None

    @staticmethod
    def _equal_within_tolerance(val1, val2, tolerance=1e-8):
        return abs(val1 - val2) < tolerance

    @staticmethod
    def _contains_point(extents, x, y) -> bool:
        """
        Check if a point given by (x, y) is contained within a box with extents
        :param extents: A list of 4 floats representing the x1, x2, y1 and y2 extents of a box
        :param x: The x position of a point
        :param y: The y position of a point
        """
        if x is None or y is None:
            return False
        return extents[0] <= x <= extents[1] and extents[2] <= y <= extents[3]

    def _create_selector(
        self, region_type: str, color: str, hatch: str, extents: Optional[tuple[float, float, float, float]] = None
    ) -> Selector:
        selector = Selector(region_type, color, hatch, self.view.data_view.ax, self._on_rectangle_selected)
        if extents:
            selector.extents = extents
        return selector

    def get_extra_image_info_columns(self, xdata, ydata):
        return {}

    def is_integer_frame(self):
        return False, False
