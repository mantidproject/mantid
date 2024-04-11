# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
import abc
from abc import ABC

from mantidqt.widgets.sliceviewer.models.base_model import SliceViewerBaseModel
from mantidqt.widgets.sliceviewer.models.dimensions import Dimensions
from mantidqt.widgets.sliceviewer.models.workspaceinfo import WorkspaceInfo
from mantidqt.widgets.sliceviewer.presenters.lineplots import PixelLinePlot, RectangleSelectionLinePlot
from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView
from mantidqt.widgets.sliceviewer.views.dataviewsubscriber import IDataViewSubscriber
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText


class SliceViewerBasePresenter(IDataViewSubscriber, ABC):
    def __init__(self, ws, data_view: SliceViewerDataView, model: SliceViewerBaseModel = None):
        self.model = model if model else SliceViewerBaseModel(ws)
        self._data_view: SliceViewerDataView = data_view
        self.normalization = False

    def show_all_data_clicked(self):
        """Instructs the view to show all data"""
        if WorkspaceInfo.is_ragged_matrix_workspace(self.model.ws):
            # get limits from full extent of image (which was calculated by looping over all spectra excl. monitors)
            x0, x1, y0, y1 = self._data_view.get_full_extent()
            limits = ((x0, x1), (y0, y1))
        else:
            # otherwise query data model based on slice info and transpose
            limits = self.get_data_limits()
        self.set_axes_limits(*limits)

    def set_axes_limits(self, xlim, ylim, auto_transform=True):
        """Set the axes limits on the view.
        :param xlim: Limits on the X axis in image coordinates
        :param ylim: Limits on the Y axis in image coordinates
        :param auto_transform: If True transform the given limits to the rectilinear
        coordinate before passing to the view
        """
        if auto_transform and self._data_view.nonorthogonal_mode:
            to_display = self._data_view.nonortho_transform.tr
            xmin_p, ymin_p = to_display(xlim[0], ylim[0])
            xmax_p, ymax_p = to_display(xlim[1], ylim[1])
            xlim, ylim = (xmin_p, xmax_p), (ymin_p, ymax_p)

        self._data_view.set_axes_limits(xlim, ylim)
        self.data_limits_changed()

    def data_limits_changed(self):
        """Notify data limits on image axes have changed"""
        if WorkspaceInfo.can_support_dynamic_rebinning(self.model.ws):
            self.new_plot()  # automatically uses current display limits
        else:
            self._data_view.draw_plot()

    def get_data_limits(self):
        return Dimensions.get_dim_limits(self.model.ws, self.get_slicepoint(), self._data_view.dimensions.transpose)

    def get_dimensions(self):
        return self.view.data_view.dimensions

    def get_slicepoint(self):
        """Returns the current slicepoint as a list of 3 or more elements.
        None indicates that dimension is being displayed"""
        return self._data_view.dimensions.get_slicepoint()

    def set_slicepoint(self, value):
        """Set the slicepoint
        :param value: The value of the slice point
        """
        self._data_view.dimensions.set_slicepoint(value)

    def new_plot_matrix(self):
        """Tell the view to display a new plot of an MatrixWorkspace"""
        self._data_view.plot_matrix(self.model.ws, distribution=not self.normalization)

    @abc.abstractmethod
    def new_plot(self, *args, **kwargs):
        pass

    def line_plots(self, state):
        """
        Toggle the attached line plots for the integrated signal over each dimension for the current cursor
        position
        :param state: If true a request is being made to turn them on, else they should be turned off
        """
        tool = PixelLinePlot
        data_view = self._data_view
        if state:
            data_view.add_line_plots(tool, self)
            if data_view.track_cursor_checked():
                data_view._line_plots.connect()
        else:
            data_view.deactivate_tool(ToolItemText.REGIONSELECTION)
            data_view.remove_line_plots()

    def region_selection(self, state):
        """
        Toggle the region selection tool. If the line plots are disabled then they are enabled.
        :param state: If true a request is being made to turn them on, else they should be turned off
        :param region_selection: If true the region selection rather than single pixel selection should
                                 be enabled.
        """
        data_view = self._data_view
        if state:
            # incompatible with drag zooming/panning as they both require drag selection
            data_view.deactivate_and_disable_tool(ToolItemText.ZOOM)
            data_view.deactivate_and_disable_tool(ToolItemText.PAN)
            data_view.extents_set_enabled(False)
            tool = RectangleSelectionLinePlot
            if data_view.line_plots_active:
                data_view.switch_line_plots_tool(RectangleSelectionLinePlot, self)
            else:
                data_view.add_line_plots(tool, self)
        else:
            data_view.enable_tool_button(ToolItemText.ZOOM)
            data_view.enable_tool_button(ToolItemText.PAN)
            data_view.extents_set_enabled(True)
            data_view.switch_line_plots_tool(PixelLinePlot, self)

    @abc.abstractmethod
    def get_extra_image_info_columns(self, xdata, ydata):
        pass

    @abc.abstractmethod
    def is_integer_frame(self):
        pass
