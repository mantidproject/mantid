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
from mantidqt.widgets.sliceviewer.views.dataview import SliceViewerDataView
from mantidqt.widgets.sliceviewer.views.dataviewsubscriber import IDataViewSubscriber


class SliceViewerBasePresenter(IDataViewSubscriber, ABC):
    def __init__(self, ws, data_view: SliceViewerDataView, model: SliceViewerBaseModel = None):
        self.model = model if model else SliceViewerBaseModel(ws)
        self._data_view : SliceViewerDataView = data_view
        self.normalization = False

    def show_all_data_clicked(self):
        """Instructs the view to show all data"""
        if WorkspaceInfo.is_ragged_matrix_workspace(self.model.ws):
            # get limits from full extent of image (which was calculated by looping over all spectra excl. monitors)
            x0, x1, y0, y1 = self._data_view.get_full_extent()
            limits = ((x0, x1), (y0, y1))
        else:
            # otherwise query data model based on slice info and transpose
            limits = Dimensions.get_dim_limits(self.model.ws, self.get_slicepoint(), self._data_view.dimensions.transpose)
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

    def get_slicepoint(self):
        """Returns the current slicepoint as a list of 3 elements.
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
