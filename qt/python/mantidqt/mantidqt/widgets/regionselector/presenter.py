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


class RegionSelector(ObservingPresenter, SliceViewerBasePresenter):
    def __init__(self, ws, parent=None, view=None):
        if WorkspaceInfo.get_ws_type(ws) != WS_TYPE.MATRIX:
            raise NotImplementedError("Only Matrix Workspaces are currently supported by the region selector.")

        self.view = view if view else RegionSelectorView(self, parent,
                                                         dims_info=Dimensions.get_dimensions_info(ws))
        super().__init__(ws, self.view._data_view)

        # TODO clear up private access
        self.view._data_view.create_axes_orthogonal(
            redraw_on_zoom=not WorkspaceInfo.can_support_dynamic_rebinning(ws))
        self.view._data_view.image_info_widget.setWorkspace(ws)
        self.new_plot()

    def dimensions_changed(self) -> None:
        self.new_plot()

    def slicepoint_changed(self) -> None:
        pass

    def canvas_clicked(self, event) -> None:
        pass

    def zoom_pan_clicked(self, active) -> None:
        pass

    def new_plot(self, *args, **kwargs):
        self.new_plot_matrix()
