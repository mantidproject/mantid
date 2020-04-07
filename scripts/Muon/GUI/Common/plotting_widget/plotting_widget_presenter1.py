# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
from time import sleep

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from Muon.GUI.Common.plotting_widget.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plotting_widget.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plotting_widget.plotting_canvas_presenter_interface import PlottingCanvasPresenterInterface
from Muon.GUI.Common.plotting_widget.plotting_widget_view1 import PlotWidgetView1
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS
from Muon.GUI.Common.plotting_widget.workspace_finder import WorkspaceFinder
from Muon.GUI.Common.ADSHandler.workspace_naming import TF_ASYMMETRY_PREFIX


class PlotWidgetPresenterCommmon(HomeTabSubWidget):

    def __init__(self, view, model, context, figure_presenter: PlottingCanvasPresenterInterface):
        """
        :param view: A reference to the QWidget object for plotting
        :param model: A reference to a model which contains the plotting logic
        :param context: A reference to the Muon context object
        """
        if not isinstance(figure_presenter, PlottingCanvasPresenterInterface):
            raise TypeError("Parameter figure_presenter must be of type PlottingCanvasPresenterInterface")

        self._view = view
        self._model = model
        self.context = context
        self.workspace_finder = WorkspaceFinder(self.context)

        # figure presenter - the common presenter talks to this through an interface
        self._figure_presenter = figure_presenter

        # observers
        self.input_workspace_observer = GenericObserver(self.handle_data_updated)
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)

        # Connect to the view
        self._view.on_plot_tiled_checkbox_changed(self.handle_plot_tiled_state_changed)
        self._view.on_tiled_by_type_changed(self.handle_tiled_by_type_changed)

    def handle_data_updated(self):
        """
        Handles the group, pair calculation finishing
        """
        if self._view.tiled_plot_checkbox.isChecked():
            tiled_by = self._view.tiled_by()
            keys = self._create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)

        workspace_list = self.workspace_finder.get_workspace_list_to_plot(True, 'Asymmetry')
        indices = [0 for _ in range(len(workspace_list))]
        self._figure_presenter.plot_workspaces(workspace_list, indices, False)

    def handle_workspace_deleted_from_ads(self, workspace):
        """
        Handles the workspace being deleted from ads
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.remove_workspace_from_plot(workspace)

    def handle_workspace_replaced_in_ads(self, workspace):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in ads
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.replace_workspace_in_plot(workspace)

    def handle_plot_type_changed(self):
        pass

    def handle_plot_tiled_state_changed(self):
        if self._view.tiled_plot_checkbox.isChecked():
            tiled_by = self._view.tiled_by()
            keys = self._create_tiled_keys(tiled_by)
            self._figure_presenter.convert_to_tiled_plot(keys, tiled_by)
        else:
            self._figure_presenter.convert_to_single_plot()

    def handle_tiled_by_type_changed(self):
        if not self._view.tiled_plot_checkbox.isChecked():
            return
        tiled_by = self._view.tiled_by()
        keys = self._create_tiled_keys(tiled_by)
        self._figure_presenter.convert_to_tiled_plot(keys, tiled_by)

    def handle_rebin_options_changed(self):
        pass

    def handle_ads_observer_stuff(self):
        pass

    def _create_tiled_keys(self, tiled_by):
        if tiled_by == "Group/Pair":
            keys = self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs
        else:
            keys = [str(item) for sublist in self.context.data_context.current_runs for item in sublist]
        return keys
