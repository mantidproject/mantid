# Mantid Repository : https://github.com/mantidproject/mantid#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from .view import RegionSelectorView
from ..observers.observing_presenter import ObservingPresenter
from ..sliceviewer.models.dimensions import Dimensions
from ..sliceviewer.models.workspaceinfo import WorkspaceInfo, WS_TYPE
from ..sliceviewer.presenters.base_presenter import SliceViewerBasePresenter
from mantid.api import RegionSelectorObserver

# 3rd party imports
from matplotlib.widgets import RectangleSelector


class RegionSelector(ObservingPresenter, SliceViewerBasePresenter):
    def __init__(self, ws=None, parent=None, view=None):
        if ws and WorkspaceInfo.get_ws_type(ws) != WS_TYPE.MATRIX:
            raise NotImplementedError("Only Matrix Workspaces are currently supported by the region selector.")

        self.notifyee = None
        self.view = view if view else RegionSelectorView(self, parent)
        super().__init__(ws, self.view._data_view)
        self._selection: list[float] = None

        if ws:
            self._initialise_dimensions(ws)
            self._set_workspace(ws)

    def subscribe(self, notifyee: RegionSelectorObserver):
        self.notifyee = notifyee

    def dimensions_changed(self) -> None:
        self.new_plot()

    def slicepoint_changed(self) -> None:
        pass

    def canvas_clicked(self, event) -> None:
        pass

    def zoom_pan_clicked(self, active) -> None:
        pass

    def new_plot(self, *args, **kwargs):
        if self.model.ws:
            self.new_plot_matrix()

    def nonorthogonal_axes(self, state: bool) -> None:
        pass

    def update_workspace(self, workspace) -> None:
        if WorkspaceInfo.get_ws_type(workspace) != WS_TYPE.MATRIX:
            raise NotImplementedError("Only Matrix Workspaces are currently supported by the region selector.")

        if not self.model.ws:
            self._initialise_dimensions(workspace)

        self._set_workspace(workspace)

    def add_rectangular_region(self):
        """
        Toggle the rectangular region selection tool.
        """
        self._selector = RectangleSelector(
            self.view._data_view.ax,
            self._on_rectangle_selected,
            useblit=False,  # rectangle persists on button release
            button=[1],
            minspanx=5,
            minspany=5,
            spancoords='pixels',
            interactive=True)

    def get_region(self):
        return self._selection

    def _initialise_dimensions(self, workspace):
        self.view.create_dimensions(dims_info=Dimensions.get_dimensions_info(workspace))
        self.view.create_axes_orthogonal(
            redraw_on_zoom=not WorkspaceInfo.can_support_dynamic_rebinning(workspace))

    def _set_workspace(self, workspace):
        self.model.ws = workspace
        self.view.set_workspace(workspace)
        self.new_plot()

    def _on_rectangle_selected(self, eclick, erelease):
        """
        Callback when a rectangle has been draw on the axes
        :param eclick: Event marking where the mouse was clicked
        :param erelease: Event marking where the mouse was released
        """
        # extents contains x1, x2, y1, y2. Just store y (spectra) for now
        self._selection = [self._selector.extents[2], self._selector.extents[3]]
        if self.notifyee:
            self.notifyee.notifyRegionChanged()
