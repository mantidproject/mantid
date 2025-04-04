# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from typing import Callable, List, Tuple
import sys

from mantid.kernel import Logger, SpecialCoordinateSystem
from qtpy.QtCore import Qt
from qtpy.QtGui import QCursor

from mantidqt.interfacemanager import InterfaceManager
from mantidqt.widgets.observers.observing_presenter import ObservingPresenter
from mantidqt.widgets.sliceviewer.models.adsobsever import SliceViewerADSObserver
from mantidqt.widgets.sliceviewer.models.dimensions import Dimensions
from mantidqt.widgets.sliceviewer.models.model import SliceViewerModel, WS_TYPE
from mantidqt.widgets.sliceviewer.models.sliceinfo import SliceInfo
from mantidqt.widgets.sliceviewer.models.workspaceinfo import WorkspaceInfo
from mantidqt.widgets.sliceviewer.cutviewer.presenter import CutViewerPresenter
from mantidqt.widgets.sliceviewer.cutviewer.view import CutViewerView
from mantidqt.widgets.sliceviewer.cutviewer.model import CutViewerModel
from mantidqt.widgets.sliceviewer.peaksviewer import PeaksViewerPresenter, PeaksViewerCollectionPresenter
from mantidqt.widgets.sliceviewer.presenters.base_presenter import SliceViewerBasePresenter
from mantidqt.widgets.sliceviewer.views.toolbar import ToolItemText
from mantidqt.widgets.sliceviewer.views.view import SliceViewerView

from workbench.plotting.propertiesdialog import XAxisEditor, YAxisEditor

DBLMAX = sys.float_info.max


class SliceViewer(ObservingPresenter, SliceViewerBasePresenter):
    TEMPORARY_STATUS_TIMEOUT = 2000

    def __init__(self, ws, parent=None, window_flags=Qt.Window, model=None, view=None, conf=None):
        """
        Create a presenter for controlling the slice display for a workspace
        :param ws: Workspace containing data to display and slice
        :param parent: An optional parent widget
        :param window_flags: An optional set of window flags
        :param model: A model to define slicing operations. If None uses SliceViewerModel
        :param view: A view to display the operations. If None uses SliceViewerView
        """
        model: SliceViewerModel = model if model else SliceViewerModel(ws)
        self.view = (
            view
            if view
            else SliceViewerView(self, Dimensions.get_dimensions_info(ws), model.can_normalize_workspace(), parent, window_flags, conf)
        )
        super().__init__(ws, self.view.data_view, model)
        self._logger = Logger("SliceViewer")
        self._peaks_presenter: PeaksViewerCollectionPresenter = None
        self._cutviewer_presenter = None
        self.conf = conf

        # Acts as a 'time capsule' to the properties of the model at this
        # point in the execution. By the time the ADS observer calls self.replace_workspace,
        # the workspace associated with self.model has already been changed.
        self.initial_model_properties = model.get_properties()
        self._new_plot_method, self.update_plot_data = self._decide_plot_update_methods()

        self.view.setWindowTitle(self.model.get_title())
        self.view.data_view.create_axes_orthogonal(redraw_on_zoom=not WorkspaceInfo.can_support_dynamic_rebinning(self.model.ws))

        if self.model.can_normalize_workspace():
            self.view.data_view.set_normalization(ws)
            self.view.data_view.norm_opts.currentTextChanged.connect(self.normalization_changed)
        if not self.model.can_support_peaks_overlays():
            self.view.data_view.disable_tool_button(ToolItemText.OVERLAY_PEAKS)
        # check whether to enable non-orthog view
        # don't know whether can always assume init with display indices (0,1) - so get sliceinfo
        sliceinfo = self.get_sliceinfo()
        if not sliceinfo.can_support_nonorthogonal_axes():
            self.view.data_view.disable_tool_button(ToolItemText.NONORTHOGONAL_AXES)
        if not self.model.can_support_non_axis_cuts():
            self.view.data_view.disable_tool_button(ToolItemText.NONAXISALIGNEDCUTS)

        self.view.data_view.help_button.clicked.connect(self.action_open_help_window)

        self.refresh_view()

        # Start the GUI with zoom selected.
        self.view.data_view.activate_tool(ToolItemText.ZOOM)

        self.ads_observer = SliceViewerADSObserver(self.replace_workspace, self.rename_workspace, self.ADS_cleared, self.delete_workspace)

        # simulate clicking on the home button, which will force all signal and slot connections
        # properly set.
        # NOTE: Some part of the connections are not set in the correct, resulting in a strange behavior
        #       where the colorbar and view is not updated with switch between different scales.
        #       This is a ducktape fix and should be revisited once we have a better way to do this.
        # NOTE: This workaround solve the problem, but it leads to a failure in
        #       projectroot.qt.python.mantidqt_qt5.test_sliceviewer_presenter.test_sliceviewer_presenter
        #       Given that this issue is not of high priority, we are leaving it as is for now.
        # self.show_all_data_clicked()

    def new_plot(self, *args, **kwargs):
        self._new_plot_method(*args, **kwargs)

    def new_plot_MDH(self, dimensions_transposing=False, dimensions_changing=False):
        """
        Tell the view to display a new plot of an MDHistoWorkspace
        """
        data_view = self.view.data_view
        limits = data_view.get_data_limits_to_fill_current_axes()

        if limits is None or not WorkspaceInfo.can_support_dynamic_rebinning(self.model.ws):
            data_view.plot_MDH(self.model.get_ws(), slicepoint=self.get_slicepoint())
            self._call_peaks_presenter_if_created("notify", PeaksViewerPresenter.Event.OverlayPeaks)
        else:
            self.new_plot_MDE(dimensions_transposing, dimensions_changing)

    def new_plot_MDE(self, dimensions_transposing=False, dimensions_changing=False):
        """
        Tell the view to display a new plot of an MDEventWorkspace
        """
        data_view = self.view.data_view
        limits = data_view.get_data_limits_to_fill_current_axes()

        # The value at the i'th index of this tells us that the axis with that value (0 or 1) will display dimension i
        dimension_indices = self.view.dimensions.get_states()

        if dimensions_transposing:
            # Since the dimensions are transposing, the limits we have from the view are the wrong way around
            # with respect to the axes the dimensions are about to be displayed, so get the previous dimension states.
            dimension_indices = self.view.dimensions.get_previous_states()
        elif dimensions_changing:
            # If we are changing which dimensions are to be displayed, the limits we got from the view are stale
            # as they refer to the previous two dimensions that were displayed.
            limits = None

        data_view.plot_MDH(
            self.model.get_ws_MDE(
                slicepoint=self.get_slicepoint(),
                bin_params=data_view.dimensions.get_bin_params(),
                limits=limits,
                dimension_indices=dimension_indices,
            )
        )
        self._call_peaks_presenter_if_created("notify", PeaksViewerPresenter.Event.OverlayPeaks)

    def update_plot_data_MDH(self):
        """
        Update the view to display an updated MDHistoWorkspace slice/cut
        """
        self.view.data_view.update_plot_data(self.model.get_data(self.get_slicepoint(), transpose=self.view.data_view.dimensions.transpose))

    def update_plot_data_MDE(self):
        """
        Update the view to display an updated MDEventWorkspace slice/cut
        """
        data_view = self.view.data_view
        data_view.update_plot_data(
            self.model.get_data(
                self.get_slicepoint(),
                bin_params=data_view.dimensions.get_bin_params(),
                dimension_indices=data_view.dimensions.get_states(),
                limits=data_view.get_data_limits_to_fill_current_axes(),
                transpose=self.view.data_view.dimensions.transpose,
            )
        )

    def update_plot_data_matrix(self):
        # should never be called, since this workspace type is only 2D the plot dimensions never change
        pass

    def get_frame(self) -> SpecialCoordinateSystem:
        """Returns frame of workspace - require access for adding a peak in peaksviewer"""
        return self.model.get_frame()

    def get_sliceinfo(self, force_nonortho_mode: bool = False):
        """
        :param force_nonortho_mode: if True then don't use orthogonal angles even if non_ortho mode == False - this
            is necessary because when non-ortho view is toggled the data_view is not updated at the point a new
            SliceInfo is created
        :return: a SliceInfo object describing the current slice and transform (which by default will be orthogonal
                 if non-ortho mode is False)
        """
        dimensions = self.view.data_view.dimensions
        non_ortho_mode = True if force_nonortho_mode else self.view.data_view.nonorthogonal_mode
        axes_angles = self.model.get_axes_angles(force_orthogonal=not non_ortho_mode)  # None if can't support transform
        return SliceInfo(
            point=dimensions.get_slicepoint(),
            transpose=dimensions.transpose,
            range=dimensions.get_slicerange(),
            qflags=dimensions.qflags,
            axes_angles=axes_angles,
            proj_matrix=self.get_proj_matrix(),
        )

    def get_proj_matrix(self):
        return self.model.get_proj_matrix()

    def get_data_limits_to_fill_current_axes(self):
        return self.view.data_view.get_data_limits_to_fill_current_axes()

    def dimensions_changed(self):
        """Indicates that the dimensions have changed"""
        data_view = self._data_view
        sliceinfo = self.get_sliceinfo()
        if data_view.nonorthogonal_mode:
            if sliceinfo.can_support_nonorthogonal_axes():
                # axes need to be recreated to have the correct transform associated
                data_view.create_axes_nonorthogonal(sliceinfo.get_northogonal_transform())
            else:
                data_view.disable_tool_button(ToolItemText.NONORTHOGONAL_AXES)
                data_view.create_axes_orthogonal()
        else:
            if sliceinfo.can_support_nonorthogonal_axes():
                data_view.enable_tool_button(ToolItemText.NONORTHOGONAL_AXES)
            else:
                data_view.disable_tool_button(ToolItemText.NONORTHOGONAL_AXES)

        ws_type = WorkspaceInfo.get_ws_type(self.model.ws)
        if ws_type == WS_TYPE.MDH or ws_type == WS_TYPE.MDE:
            if (
                self.model.get_number_dimensions() > 2
                and sliceinfo.slicepoint[data_view.dimensions.get_previous_states().index(None)] is None
            ):
                # The dimension of the slicepoint has changed
                self.new_plot(dimensions_changing=True)
            else:
                self.new_plot(dimensions_transposing=True)
        else:
            self.new_plot()
        self._call_cutviewer_presenter_if_created("on_dimension_changed")
        if self.view.data_view.nonorthogonal_mode:
            self.show_all_data_clicked()

    def slicepoint_changed(self):
        """Indicates the slicepoint has been updated"""
        self._call_peaks_presenter_if_created("notify", PeaksViewerPresenter.Event.SlicePointChanged)
        self._call_cutviewer_presenter_if_created("on_slicepoint_changed")
        self.update_plot_data()

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
                    transpose=data_view.dimensions.transpose,
                    dimension_indices=data_view.dimensions.get_states(),
                )
            )
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message("Error exporting ROI")

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
                    dimension_indices=data_view.dimensions.get_states(),
                    cut=cut_type,
                )
            )
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message("Error exporting roi cut")

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
                    dimension_indices=data_view.dimensions.get_states(),
                    axis=axis,
                )
            )
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message("Error exporting single-pixel cut")

    def perform_non_axis_aligned_cut(self, vectors, extents, nbins):
        try:
            wscut_name = self.model.perform_non_axis_aligned_cut_to_workspace(vectors, extents, nbins)
            self._call_cutviewer_presenter_if_created("on_cut_done", wscut_name)
        except Exception as exc:
            self._logger.error(str(exc))
            self._show_status_message("Error exporting single-pixel cut")

    def nonorthogonal_axes(self, state: bool):
        """
        Toggle non-orthogonal axes on current view
        :param state: If true a request is being made to turn them on, else they should be turned off
        """
        data_view = self.view.data_view
        data_view.image_info_widget.setShowSignal(not state)
        if state:
            data_view.deactivate_and_disable_tool(ToolItemText.REGIONSELECTION)
            data_view.disable_tool_button(ToolItemText.LINEPLOTS)
            # set transform from sliceinfo but ignore view as non-ortho state not set yet
            data_view.create_axes_nonorthogonal(self.get_sliceinfo(force_nonortho_mode=True).get_northogonal_transform())
            self.show_all_data_clicked()
        else:
            data_view.create_axes_orthogonal()
            data_view.enable_tool_button(ToolItemText.LINEPLOTS)
            data_view.enable_tool_button(ToolItemText.REGIONSELECTION)

        self.new_plot()

        # replot the cut if one was displayed before
        if self._cutviewer_presenter is not None and self._cutviewer_presenter.view.cut_rep is not None:
            self._cutviewer_presenter.show_view()
            self._cutviewer_presenter.update_cut()

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
            self.view.data_view.on_resize()
        else:
            self.view.peaks_view.hide()

    def non_axis_aligned_cut(self, state):
        data_view = self._data_view
        if state:
            if self._cutviewer_presenter is None:
                cutviewer_view = CutViewerView(data_view.canvas, self.get_frame())
                cutviewer_model = CutViewerModel(self.get_proj_matrix())
                self._cutviewer_presenter = CutViewerPresenter(self, cutviewer_model, cutviewer_view)
                self.view.add_widget_to_splitter(self._cutviewer_presenter.get_view())
            self._cutviewer_presenter.show_view()
            data_view.deactivate_tool(ToolItemText.ZOOM)
            for tool in [ToolItemText.REGIONSELECTION, ToolItemText.LINEPLOTS]:
                data_view.deactivate_and_disable_tool(tool)
            # turn off cursor tracking as this causes plot to resize interfering with interactive cutting tool
            data_view.track_cursor.setChecked(False)  # on_track_cursor_state_change(False)
        else:
            self._cutviewer_presenter.hide_view()
            for tool in [ToolItemText.REGIONSELECTION, ToolItemText.LINEPLOTS]:
                data_view.enable_tool_button(tool)
            if self.get_sliceinfo().can_support_nonorthogonal_axes():
                data_view.enable_tool_button(ToolItemText.NONORTHOGONAL_AXES)
            self._cutviewer_presenter = None

    def replace_workspace(self, workspace_name, workspace):
        """
        Called when the SliceViewerADSObserver has detected that a workspace has changed
        @param workspace_name: the name of the workspace that has changed
        @param workspace: the workspace that has changed
        """

        try:
            if self.model.check_for_removed_original_workspace():
                self._close_view_with_message("Original workspace has been replaced: Closing Slice Viewer")
                return
        except RuntimeError:
            # can't check for original workspace if existing models workspace has been replaced
            pass

        if not self.model.workspace_equals(workspace_name):
            # TODO this is a dead branch, since the ADS observer will call this if the
            # names are the same, but the model "workspace_equals" simply checks for the same name
            return
        try:
            candidate_model = SliceViewerModel(workspace)
            candidate_model_properties = candidate_model.get_properties()
            for property, value in candidate_model_properties.items():
                if self.initial_model_properties[property] != value:
                    raise ValueError(f"The property {property} is different on the new workspace.")

            # New model is OK, proceed with updating Slice Viewer
            self.model = candidate_model
            self._new_plot_method, self.update_plot_data = self._decide_plot_update_methods()
            self.view.delayed_refresh()
        except ValueError as err:
            self._close_view_with_message(f"Closing Sliceviewer as the underlying workspace was changed: {str(err)}")
            return

    def refresh_view(self):
        """
        Updates the view to enable/disable certain options depending on the model.
        """
        if not self.view:
            return

        self.view.refresh_queued = False
        # we don't want to use model.get_ws for the image info widget as this needs
        # extra arguments depending on workspace type.
        ws = self.model.ws
        ws.readLock()
        try:
            self.view.data_view.image_info_widget.setWorkspace(ws)
            self.new_plot()
        finally:
            ws.unlock()

    def show_view(self):
        self.view.show()

    def rename_workspace(self, old_name, new_name):
        if self.model.workspace_equals(old_name):
            self.model.set_ws_name(new_name)
            self.view.emit_rename(self.model.get_title(new_name))

    def delete_workspace(self, ws_name):
        if self.model.workspace_equals(ws_name):
            self.view.emit_close()
        elif self.model.check_for_removed_original_workspace():
            self._close_view_with_message("Original workspace has been deleted: Closing Slice Viewer")

    def ADS_cleared(self):
        if self.view:
            self.view.emit_close()

    def clear_observer(self):
        """Called by ObservingView on close event"""
        self.ads_observer = None
        if self._peaks_presenter is not None:
            self._peaks_presenter.clear_observer()

    def canvas_clicked(self, event):
        data_view = self.view.data_view
        if self._peaks_presenter is not None and event.inaxes:
            sliceinfo = self.get_sliceinfo()
            if sliceinfo.can_support_peak_overlay():
                self._logger.debug(f"Coordinates selected x={event.xdata} y={event.ydata} z={sliceinfo.z_value}")
                pos = sliceinfo.inverse_transform([event.xdata, event.ydata, sliceinfo.z_value])
                self._logger.debug(f"Coordinates transformed into {self.get_frame()} frame, pos={pos}")
                self._peaks_presenter.add_delete_peak(pos)
                self.view.data_view.canvas.draw_idle()
        elif event.dblclick and event.button == data_view.canvas.buttond.get(Qt.LeftButton) and not data_view.nonorthogonal_mode:
            if data_view.ax.xaxis.contains(event)[0] or any(tick.contains(event)[0] for tick in data_view.ax.get_xticklabels()):
                editor = SliceViewXAxisEditor(data_view.canvas, data_view.ax, self.dimensions_changed)
                editor.move(QCursor.pos())
                editor.exec_()
            elif data_view.ax.yaxis.contains(event)[0] or any(tick.contains(event)[0] for tick in data_view.ax.get_yticklabels()):
                editor = SliceViewYAxisEditor(data_view.canvas, data_view.ax, self.dimensions_changed)
                editor.move(QCursor.pos())
                editor.exec_()

    def key_pressed(self, event) -> None:
        pass

    def mouse_moved(self, event) -> None:
        pass

    def deactivate_zoom_pan(self):
        self.view.data_view.deactivate_zoom_pan()

    def zoom_pan_clicked(self, active):
        if active and self._peaks_presenter is not None:
            self._peaks_presenter.deactivate_peak_add_delete()

    # private api
    def _create_peaks_presenter_if_necessary(self):
        if self._peaks_presenter is None:
            self._peaks_presenter = PeaksViewerCollectionPresenter(self.view.peaks_view)
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

    def _call_cutviewer_presenter_if_created(self, attr, *args, **kwargs):
        """
        Call a method on the peaks presenter if it has been created
        :param attr: The attribute to call
        :param *args: Positional-arguments to pass to call
        :param **kwargs Keyword-arguments to pass to call
        """
        if self._cutviewer_presenter is not None:
            getattr(self._cutviewer_presenter, attr)(*args, **kwargs)

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

    def _decide_plot_update_methods(self) -> Tuple[Callable, Callable]:
        """
        Checks the type of workspace in self.model and decides which of the
        new_plot and update_plot_data methods to use
        :return: the new_plot method to use
        """
        # TODO get rid of private access here
        ws_type = WorkspaceInfo.get_ws_type(self.model.ws)
        if ws_type == WS_TYPE.MDH:
            return self.new_plot_MDH, self.update_plot_data_MDH
        elif ws_type == WS_TYPE.MDE:
            return self.new_plot_MDE, self.update_plot_data_MDE
        else:
            return self.new_plot_matrix, self.update_plot_data_matrix

    def _close_view_with_message(self, message: str):
        self.view.emit_close()  # inherited from ObservingView
        self._logger.warning(message)

    def notify_close(self):
        self.view = None

    def action_open_help_window(self):
        InterfaceManager().showHelpPage("qthelp://org.mantidproject/doc/workbench/sliceviewer.html")

    def is_integer_frame(self):
        if self.get_frame() != SpecialCoordinateSystem.HKL:
            return (False, False)
        else:
            return self.get_sliceinfo().is_xy_q_frame()

    def get_extra_image_info_columns(self, xdata, ydata):
        if self.view is None:
            return {"H": "0", "K": "0", "L": "0"}

        qdims = [i for i, v in enumerate(self.view.data_view.dimensions.qflags) if v]

        if len(qdims) != 3 or self.get_frame() != SpecialCoordinateSystem.HKL:
            return {}

        if xdata == DBLMAX and ydata == DBLMAX:
            return {"H": "-", "K": "-", "L": "-"}

        slice_point = self.get_slicepoint()
        xdim, ydim = WorkspaceInfo.display_indices(slice_point, self.view.data_view.dimensions.transpose)
        full_point = self._get_full_point(slice_point, xdata, ydata, xdim, ydim)

        hkl = self.model.get_hkl_from_full_point(full_point, qdims)

        hkl_as_strings = [f"{element:.4f}" for element in hkl]
        return {"H": hkl_as_strings[0], "K": hkl_as_strings[1], "L": hkl_as_strings[2]}

    @staticmethod
    def _get_full_point(slice_point: List[float], xdata: float, ydata: float, xdim: int, ydim: int) -> List[float]:
        """Construct a list containing the full cursor coordinate i.e. x, y, z, and any other dimensions"""
        full_point = slice_point.copy()
        full_point[xdim] = xdata
        full_point[ydim] = ydata
        return full_point


class SliceViewXAxisEditor(XAxisEditor):
    def __init__(self, canvas, axes, dimensions_changed):
        super(SliceViewXAxisEditor, self).__init__(canvas, axes)
        self.dimensions_changed = dimensions_changed
        self.ui.logBox.hide()
        self.ui.gridBox.hide()

    def on_ok(self):
        super(SliceViewXAxisEditor, self).on_ok()
        self.dimensions_changed()


class SliceViewYAxisEditor(YAxisEditor):
    def __init__(self, canvas, axes, dimensions_changed):
        super(SliceViewYAxisEditor, self).__init__(canvas, axes)
        self.dimensions_changed = dimensions_changed
        self.ui.logBox.hide()
        self.ui.gridBox.hide()

    def on_ok(self):
        super(SliceViewYAxisEditor, self).on_ok()
        self.dimensions_changed()
