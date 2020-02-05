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
from .view import SliceViewerView
from .peaksviewer.presenter import PeaksViewerCollectionPresenter


class SliceViewer(object):
    def __init__(self, ws, parent=None, model=None, view=None):
        """
        Create a presenter for controlling the slice display for a workspace
        :param ws: Workspace containing data to display and slice
        :param parent: An optinal parent widget
        :param model: A model to define slicing operations. If None uses SliceViewerModel
        :param view: A view to display the operations. If None uses SliceViewerView
        """
        self._peaks_tool_presenter = None
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
            self.view.norm_opts.currentTextChanged.connect(self.normalization_changed)
            self.view.set_normalization(ws)

        self.new_plot()

    def new_plot_MDH(self):
        """
        Tell the view to display a new plot of an MDHistoWorkspace
        """
        self.view.plot_MDH(self.model.get_ws(), slicepoint=self.view.dimensions.get_slicepoint())

    def new_plot_MDE(self):
        """
        Tell the view to display a new plot of an MDEventWorkspace
        """
        self.view.plot_MDH(self.model.get_ws(slicepoint=self.view.dimensions.get_slicepoint(),
                                             bin_params=self.view.dimensions.get_bin_params()))

    def new_plot_matrix(self):
        """
        Tell the view to display a new plot of an MatrixWorkspace
        """
        self.view.plot_matrix(self.model.get_ws(), normalize=self.normalization)

    def update_plot_data_MDH(self):
        """
        Update the view to display an updated MDHistoWorkspace slice/cut
        """
        self.view.update_plot_data(self.model.get_data(self.view.dimensions.get_slicepoint(),
                                                       self.view.dimensions.transpose))

    def update_plot_data_MDE(self):
        """
        Update the view to display an updated MDEventWorkspace slice/cut
        """
        self.view.update_plot_data(self.model.get_data(slicepoint=self.view.dimensions.get_slicepoint(),
                                                       bin_params=self.view.dimensions.get_bin_params(),
                                                       transpose=self.view.dimensions.transpose))

    def update_plot_data_matrix(self):
        # should never be called, since this workspace type is only 2D the plot dimensions never change
        pass

    def line_plots(self):
        """
        Display the attached line plots for the integrated signal over each dimension for the current cursor
        position
        """
        self.view.create_axes()
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

    def overlay_peaks_workspace(self):
        """
        Request activation of peak overlay tools.
          - Asks user to select peaks workspace(s)
          - Attaches peaks table viewer/tools
          - Displays peaks on data display
        """
        peaks_workspaces = [
            self.model.get_peaksworkspace(name)
            for name in self.view.query_peaks_to_overlay()
        ]
        self._create_peaks_tool_presenter(peaks_workspaces)
        peak_info = self._peaks_tool_presenter.peaks_info()
        view = self.view
        for peak in peak_info:
            center = peak.center
            view.draw_peak(center.X(), center.Y(),
                           peak.alpha, peak.color)

    # private api
    def _create_peaks_tool_presenter(self, workspaces):
        """
        Create the presenter for the PeaksViewer view
        :param workspaces: A list of workspaces
        """
        self._peaks_tool_presenter = \
            PeaksViewerCollectionPresenter(workspaces,
                                           self.view.attach_peaks_tools())
