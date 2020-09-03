# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# 3rdparty imports
import mantid.api
import mantid.kernel

# local imports
from .lineplots import PixelLinePlot, RectangleSelectionLinePlot
from .model import SliceViewerModel, WS_TYPE
from .sliceinfo import SliceInfo
from .toolbar import ToolItemText
from .view import SliceViewerView
from .peaksviewer import PeaksViewerPresenter, PeaksViewerCollectionPresenter


class SliceViewer(object):
    TEMPORARY_STATUS_TIMEOUT = 2000

    def __init__(self, ws, parent=None, model=None, view=None, conf=None):
        """
        Create a presenter for controlling the slice display for a workspace
        :param ws: Workspace containing data to display and slice
        :param parent: An optional parent widget
        :param model: A model to define slicing operations. If None uses SliceViewerModel
        :param view: A view to display the operations. If None uses SliceViewerView
        """
        self._logger = mantid.kernel.Logger("SliceViewer")
        self._peaks_presenter = None
        self.model = model if model else SliceViewerModel(ws)

        if self.model.get_ws_type() == WS_TYPE.MDH:
            self.new_plot = self.new_plot_MDH
            self.update_plot_data = self.update_plot_data_MDH
        elif self.model.get_ws_type() == WS_TYPE.MDE:
            self.new_plot = self.new_plot_MDE
            self.update_plot_data = self.update_plot_data_MDE
        else:
            self.new_plot = self.new_plot_matrix
            self.update_plot_data = self.update_plot_data_matrix

        self.normalization = False

        self.view = view if view else SliceViewerView(self, self.model.get_dimensions_info(),
                                                      self.model.can_normalize_workspace(), parent, conf)
        self.view.data_view.create_axes_orthogonal(
            redraw_on_zoom=not self.model.can_support_dynamic_rebinning())
        self.view.data_view.image_info_widget.setWorkspace(ws)

        if self.model.can_normalize_workspace():
            self.view.data_view.set_normalization(ws)
            self.view.data_view.norm_opts.currentTextChanged.connect(self.normalization_changed)
        if not self.model.can_support_peaks_overlays():
            self.view.data_view.disable_tool_button(ToolItemText.OVERLAY_PEAKS)
        if not self.model.can_support_nonorthogonal_axes():
            self.view.data_view.disable_tool_button(ToolItemText.NONORTHOGONAL_AXES)

        self.view.setWindowTitle(self.model.get_title())
        self.new_plot()

        # Start the GUI with zoom selected.
        self.view.data_view.activate_tool(ToolItemText.ZOOM)

    def new_plot_MDH(self):
        """
        Tell the view to display a new plot of an MDHistoWorkspace
        """
        self.view.data_view.plot_MDH(self.model.get_ws(), slicepoint=self.get_slicepoint())
        self._call_peaks_presenter_if_created("notify", PeaksViewerPresenter.Event.OverlayPeaks)

    def new_plot_MDE(self):
        """
        Tell the view to display a new plot of an MDEventWorkspace
        """
        data_view = self.view.data_view
        limits = data_view.get_axes_limits()

        if limits is not None:
            xlim, ylim = limits
            # view limits are in orthogonal frame. transform to nonorthogonal
            # model frame
            if data_view.nonorthogonal_mode:
                inv_tr = data_view.nonortho_transform.inv_tr
                xmin_p, ymin_p = inv_tr(xlim[0], ylim[0])
                xmax_p, ymax_p = inv_tr(xlim[1], ylim[1])
                xlim, ylim = (xmin_p, xmax_p), (ymin_p, ymax_p)
            if data_view.dimensions.transpose:
                limits = ylim, xlim
            else:
                limits = xlim, ylim

        data_view.plot_MDH(
            self.model.get_ws(
                slicepoint=self.get_slicepoint(),
                bin_params=data_view.dimensions.get_bin_params(),
                limits=limits))
        self._call_peaks_presenter_if_created("notify", PeaksViewerPresenter.Event.OverlayPeaks)

    def new_plot_matrix(self):
        """Tell the view to display a new plot of an MatrixWorkspace"""
        self.view.data_view.plot_matrix(self.model.get_ws(), distribution=not self.normalization)

    def update_plot_data_MDH(self):
        """
        Update the view to display an updated MDHistoWorkspace slice/cut
        """
        self.view.data_view.update_plot_data(
            self.model.get_data(
                self.get_slicepoint(), transpose=self.view.data_view.dimensions.transpose))

    def update_plot_data_MDE(self):
        """
        Update the view to display an updated MDEventWorkspace slice/cut
        """
        data_view = self.view.data_view
        data_view.update_plot_data(
            self.model.get_data(
                self.get_slicepoint(),
                bin_params=data_view.dimensions.get_bin_params(),
                limits=data_view.get_axes_limits(),
                transpose=self.view.data_view.dimensions.transpose))

    def update_plot_data_matrix(self):
        # should never be called, since this workspace type is only 2D the plot dimensions never change
        pass

    def get_sliceinfo(self):
        """Returns a SliceInfo object describing the current slice"""
        dimensions = self.view.data_view.dimensions
        return SliceInfo(frame=self.model.get_frame(),
                         point=dimensions.get_slicepoint(),
                         transpose=dimensions.transpose,
                         range=dimensions.get_slicerange(),
                         qflags=dimensions.qflags,
                         nonortho_transform=self.view.data_view.nonortho_transform)

    def get_slicepoint(self):
        """Returns the current slicepoint as a list of 3 elements.
           None indicates that dimension is being displayed"""
        return self.view.data_view.dimensions.get_slicepoint()

    def set_slicepoint(self, value):
        """Set the slicepoint
        :param value: The value of the slice point
        """
        self.view.data_view.dimensions.set_slicepoint(value)

    def dimensions_changed(self):
        """Indicates that the dimensions have changed"""
        data_view = self.view.data_view
        sliceinfo = self.get_sliceinfo()
        if data_view.nonorthogonal_mode:
            if sliceinfo.can_support_nonorthogonal_axes():
                # axes need to be recreated to have the correct transform associated
                data_view.create_axes_nonorthogonal(
                    self.model.create_nonorthogonal_transform(sliceinfo))
            else:
                data_view.disable_tool_button(ToolItemText.NONORTHOGONAL_AXES)
                data_view.create_axes_orthogonal()
        else:
            if sliceinfo.can_support_nonorthogonal_axes():
                data_view.enable_tool_button(ToolItemText.NONORTHOGONAL_AXES)
            else:
                data_view.disable_tool_button(ToolItemText.NONORTHOGONAL_AXES)

        self.new_plot()

    def slicepoint_changed(self):
        """Indicates the slicepoint has been updated"""
        self._call_peaks_presenter_if_created("notify",
                                              PeaksViewerPresenter.Event.SlicePointChanged)
        self.update_plot_data()

    def data_limits_changed(self):
        """Notify data limits on image axes have changed"""
        data_view = self.view.data_view
        if self.model.can_support_dynamic_rebinning():
            self.new_plot()  # automatically uses current display limits
        else:
            data_view.draw_plot()

    def show_all_data_requested(self):
        """Instructs the view to show all data"""
        self.set_axes_limits(*self.model.get_dim_limits(self.get_slicepoint(),
                                                        self.view.data_view.dimensions.transpose))

    def set_axes_limits(self, xlim, ylim, auto_transform=True):
        """Set the axes limits on the view.
        :param xlim: Limits on the X axis in image coordinates
        :param ylim: Limits on the Y axis in image coordinates
        :param auto_transform: If True transform the given limits to the rectilinear
        coordinate before passing to the view
        """
        data_view = self.view.data_view
        if auto_transform and data_view.nonorthogonal_mode:
            to_display = data_view.nonortho_transform.tr
            xmin_p, ymin_p = to_display(xlim[0], ylim[0])
            xmax_p, ymax_p = to_display(xlim[1], ylim[1])
            xlim, ylim = (xmin_p, xmax_p), (ymin_p, ymax_p)

        data_view.set_axes_limits(xlim, ylim)
        self.data_limits_changed()

    def line_plots(self, state):
        """
        Toggle the attached line plots for the integrated signal over each dimension for the current cursor
        position
        :param state: If true a request is being made to turn them on, else they should be turned off
        """
        tool = PixelLinePlot
        data_view = self.view.data_view
        if state:
            data_view.add_line_plots(tool, self)
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
        data_view = self.view.data_view
        if state:
            # incompatible with drag zooming/panning as they both require drag selection
            data_view.deactivate_and_disable_tool(ToolItemText.ZOOM)
            data_view.deactivate_and_disable_tool(ToolItemText.PAN)
            tool = RectangleSelectionLinePlot
            if data_view.line_plots_active:
                data_view.switch_line_plots_tool(RectangleSelectionLinePlot, self)
            else:
                data_view.add_line_plots(tool, self)
        else:
            data_view.enable_tool_button(ToolItemText.ZOOM)
            data_view.enable_tool_button(ToolItemText.PAN)
            data_view.switch_line_plots_tool(PixelLinePlot, self)

    def export_roi(self, limits):
        """Notify that an roi has been selected for export to a workspace
        :param limits: 2-tuple of ((left, right), (bottom, top)). These are in display order
        """
        data_view = self.view.data_view

        try:
            self._show_status_message(
                self.model.export_roi_to_workspace(
                    self.get_slicepoint(),
                    bin_params=data_view.dimensions.get_bin_params(),
                    limits=limits,
                    transpose=data_view.dimensions.transpose))
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message(f"Error exporting ROI")

    def export_cut(self, limits, cut_type):
        """Notify that an roi has been selected for export to a workspace
        :param limits: 2-tuple of ((left, right), (bottom, top)). These are in display order
        and could be transposed w.r.t to the data
        :param cut: A string indicating the required cut type
        """
        data_view = self.view.data_view

        try:
            self._show_status_message(
                self.model.export_cuts_to_workspace(
                    self.get_slicepoint(),
                    bin_params=data_view.dimensions.get_bin_params(),
                    limits=limits,
                    transpose=data_view.dimensions.transpose,
                    cut=cut_type))
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message(f"Error exporting roi cut")

    def export_pixel_cut(self, pos, axis):
        """Notify a single pixel line plot has been requested from the
        given position in data coordinates.
        :param pos: Position on the image
        :param axis: String indicating the axis the position relates to: 'x' or 'y'
        """
        data_view = self.view.data_view

        try:
            self._show_status_message(
                self.model.export_pixel_cut_to_workspace(
                    self.get_slicepoint(),
                    bin_params=data_view.dimensions.get_bin_params(),
                    pos=pos,
                    transpose=data_view.dimensions.transpose,
                    axis=axis))
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message(f"Error exporting single-pixel cut")

    def nonorthogonal_axes(self, state: bool):
        """
        Toggle non-orthogonal axes on current view
        :param state: If true a request is being made to turn them on, else they should be turned off
        """
        data_view = self.view.data_view
        if state:
            data_view.deactivate_and_disable_tool(ToolItemText.REGIONSELECTION)
            data_view.disable_tool_button(ToolItemText.LINEPLOTS)
            data_view.create_axes_nonorthogonal(
                self.model.create_nonorthogonal_transform(self.get_sliceinfo()))
        else:
            data_view.create_axes_orthogonal()
            data_view.enable_tool_button(ToolItemText.LINEPLOTS)
            data_view.enable_tool_button(ToolItemText.REGIONSELECTION)

        self.new_plot()

    def normalization_changed(self, norm_type):
        """
        Notify the presenter that the type of normalization has changed.
        :param norm_type: "By bin width" = volume normalization else no normalization
        """
        self.normalization = norm_type == "By bin width"
        self.new_plot()

    def overlay_peaks_workspaces(self):
        """
        Request activation of peak overlay tools.
          - Asks user to select peaks workspace(s), taking into account any current selection
          - Attaches peaks table viewer/tools if new workspaces requested. Removes any unselected
          - Displays peaks on data display (if any left to display)
        """
        names_overlayed = self._overlayed_peaks_workspaces()
        names_to_overlay = self.view.query_peaks_to_overlay(names_overlayed)
        if names_to_overlay is None:
            # cancelled
            return
        if names_to_overlay or names_overlayed:
            self._create_peaks_presenter_if_necessary().overlay_peaksworkspaces(names_to_overlay)
        else:
            self.view.peaks_view.hide()

    # private api
    def _create_peaks_presenter_if_necessary(self):
        if self._peaks_presenter is None:
            self._peaks_presenter = \
                PeaksViewerCollectionPresenter(self.view.peaks_view)

        return self._peaks_presenter

    def _call_peaks_presenter_if_created(self, attr, *args, **kwargs):
        """
        Call a method on the peaks presenter if it has been created
        :param attr: The attribute to call
        :param *args: Positional-arguments to pass to call
        :param **kwargs Keyword-arguments to pass to call
        """
        if self._peaks_presenter is not None:
            getattr(self._peaks_presenter, attr)(*args, **kwargs)

    def _show_status_message(self, message: str):
        """
        Show a temporary message in the status of the view
        """
        self.view.data_view.show_temporary_status_message(message, self.TEMPORARY_STATUS_TIMEOUT)

    def _overlayed_peaks_workspaces(self):
        """
        :return: A list of names of the current PeaksWorkspaces overlayed
        """
        current_workspaces = []
        if self._peaks_presenter is not None:
            current_workspaces = self._peaks_presenter.workspace_names()

        return current_workspaces
