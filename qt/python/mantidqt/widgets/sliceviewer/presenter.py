# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#

# 3rdparty imports
import mantid.api

# local imports
from .model import SliceViewerModel, WS_TYPE
from .sliceinfo import SliceInfo
from .view import SliceViewerView
from .peaksviewer import PeaksViewerPresenter, PeaksViewerCollectionPresenter


class SliceViewer(object):
    def __init__(self, ws, parent=None, model=None, view=None):
        """
        Create a presenter for controlling the slice display for a workspace
        :param ws: Workspace containing data to display and slice
        :param parent: An optinal parent widget
        :param model: A model to define slicing operations. If None uses SliceViewerModel
        :param view: A view to display the operations. If None uses SliceViewerView
        """
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

        self.normalization = mantid.api.MDNormalization.NoNormalization

        self.view = view if view else SliceViewerView(self, self.model.get_dimensions_info(),
                                                      self.model.can_normalize_workspace(), parent)
        if self.model.can_normalize_workspace():
            self.view.data_view.set_normalization(ws)
            self.view.data_view.norm_opts.currentTextChanged.connect(self.normalization_changed)
        if not self.model.can_support_peaks_overlays():
            self.view.data_view.disable_peaks_button()
        if not self.model.can_support_nonorthogonal_axes():
            self.view.data_view.disable_nonorthogonal_axes_button()

        self.view.setWindowTitle(f"SliceViewer - {self.model.get_ws_name()}")
        self.new_plot()

    def new_plot_MDH(self):
        """
        Tell the view to display a new plot of an MDHistoWorkspace
        """
        self.view.data_view.plot_MDH(self.model.get_ws(), slicepoint=self.get_slicepoint())

    def new_plot_MDE(self):
        """
        Tell the view to display a new plot of an MDEventWorkspace
        """
        self.view.data_view.plot_MDH(
            self.model.get_ws(slicepoint=self.get_slicepoint(),
                              bin_params=self.view.data_view.dimensions.get_bin_params()))

    def new_plot_matrix(self):
        """Tell the view to display a new plot of an MatrixWorkspace"""
        self.view.data_view.plot_matrix(self.model.get_ws(), normalize=self.normalization)

    def get_sliceinfo(self):
        """Returns a SliceInfo object describing the current slice"""
        dimensions = self.view.data_view.dimensions
        return SliceInfo(frame=self.model.get_frame(),
                         point=dimensions.get_slicepoint(),
                         transpose=dimensions.transpose,
                         range=dimensions.get_slicerange(),
                         qflags=dimensions.qflags)

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
                data_view.disable_nonorthogonal_axes_button()
                data_view.create_axes_orthogonal()
        else:
            if sliceinfo.can_support_nonorthogonal_axes():
                data_view.enable_nonorthogonal_axes_button()
            else:
                data_view.disable_nonorthogonal_axes_button()

        self.new_plot()
        self._call_peaks_presenter_if_created("notify", PeaksViewerPresenter.Event.OverlayPeaks)

    def slicepoint_changed(self):
        """Indicates the slicepoint has been updated"""
        self._call_peaks_presenter_if_created("notify",
                                              PeaksViewerPresenter.Event.SlicePointChanged)
        self.update_plot_data()

    def show_all_data_requested(self):
        """Instructs the view to show all data"""
        self.view.data_view.set_axes_limits(*self.model.get_dim_limits(
            self.get_slicepoint(), self.view.data_view.dimensions.transpose))

    def update_plot_data_MDH(self):
        """
        Update the view to display an updated MDHistoWorkspace slice/cut
        """
        self.view.data_view.update_plot_data(
            self.model.get_data(self.get_slicepoint(), self.view.data_view.dimensions.transpose))

    def update_plot_data_MDE(self):
        """
        Update the view to display an updated MDEventWorkspace slice/cut
        """
        self.view.data_view.update_plot_data(
            self.model.get_data(self.get_slicepoint(),
                                bin_params=self.view.data_view.dimensions.get_bin_params(),
                                transpose=self.view.data_view.dimensions.transpose))

    def update_plot_data_matrix(self):
        # should never be called, since this workspace type is only 2D the plot dimensions never change
        pass

    def line_plots(self, state):
        """
        Toggle the attached line plots for the integrated signal over each dimension for the current cursor
        position
        """
        if state:
            self.view.data_view.add_line_plots()
        else:
            self.view.data_view.remove_line_plots()

    def nonorthogonal_axes(self, state: bool):
        """
        Toggle non-orthogonal axes on current view
        :param state: If true a request is being made to turn them on, else they should be turned off
        """
        data_view = self.view.data_view
        data_view.remove_line_plots()
        if state:
            data_view.disable_lineplots_button()
            data_view.disable_peaks_button()
            data_view.create_axes_nonorthogonal(
                self.model.create_nonorthogonal_transform(self.get_sliceinfo()))
            self.new_plot()
        else:
            data_view.create_axes_orthogonal()
            data_view.enable_lineplots_button()
            data_view.enable_peaks_button()
            self.new_plot()

    def normalization_changed(self, norm_type):
        """
        Notify the presenter that the type of normalization has changed.
        :param norm_type: "By bin width" = volume normalization else no normalization
        """
        if norm_type == "By bin width":
            self.normalization = mantid.api.MDNormalization.VolumeNormalization
        else:
            self.normalization = mantid.api.MDNormalization.NoNormalization
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

    def _overlayed_peaks_workspaces(self):
        """
        :return: A list of names of the current PeaksWorkspaces overlayed
        """
        current_workspaces = []
        if self._peaks_presenter is not None:
            current_workspaces = self._peaks_presenter.workspace_names()

        return current_workspaces
