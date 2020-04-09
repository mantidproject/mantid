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
from Muon.GUI.Common.plotting_widget.plotting_widget_view_interface import PlottingWidgetViewInterface
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserver, GenericObserverWithArgPassing
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.FrequencyDomainAnalysis.frequency_context import FREQUENCY_EXTENSIONS
from Muon.GUI.Common.plotting_widget.workspace_finder import WorkspaceFinder
from Muon.GUI.Common.ADSHandler.workspace_naming import TF_ASYMMETRY_PREFIX

COUNTS_PLOT_TYPE = 'Counts'
ASYMMETRY_PLOT_TYPE = 'Asymmetry'
TILED_BY_GROUP_TYPE = 'Group/Pair'
TILED_BY_RUN_TYPE = 'Run'


class PlotWidgetPresenterCommmon(HomeTabSubWidget):

    def __init__(self, view: PlottingWidgetViewInterface, model, context,
                 figure_presenter: PlottingCanvasPresenterInterface):
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
        self._model = self.workspace_finder

        # figure presenter - the common presenter talks to this through an interface
        self._figure_presenter = figure_presenter

        # gui observers
        self._setup_gui_observers()

        # Connect to the view
        self._view.on_plot_tiled_checkbox_changed(self.handle_plot_tiled_state_changed)
        self._view.on_tiled_by_type_changed(self.handle_tiled_by_type_changed)
        self._view.on_plot_type_changed(self.handle_plot_type_changed)

        # setup view options
        self._view.setup_plot_type_options([ASYMMETRY_PLOT_TYPE, COUNTS_PLOT_TYPE])
        self._view.setup_tiled_by_options(([TILED_BY_GROUP_TYPE, TILED_BY_RUN_TYPE]))

    def _setup_gui_observers(self):
        self.input_workspace_observer = GenericObserver(self.handle_data_updated)
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)

    def handle_data_updated(self):
        """
        Handles the group, pair calculation finishing
        """
        if self._view.tiled_plot_checkbox.isChecked():
            tiled_by = self._view.tiled_by()
            keys = self._create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)

        workspace_list = self.workspace_finder.get_workspace_list_to_plot(True, self._view.get_plot_type())
        indices = [0 for _ in range(len(workspace_list))]
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=False)

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
        """
        Handles the use raw workspaces being changed (e.g rebinned) in ads
        """
        if len(self.context.group_pair_context.selected_pairs) != 0 and self._view.get_selected() == COUNTS_PLOT_TYPE:
            self._view.plot_selector.blockSignals(True)
            self._view.plot_selector.setCurrentText(ASYMMETRY_PLOT_TYPE)
            self._view.plot_selector.blockSignals(False)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')

        workspace_list = self.workspace_finder.get_workspace_list_to_plot(True, self._view.get_plot_type())
        indices = [0 for _ in range(len(workspace_list))]
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=True)

    def handle_plot_tiled_state_changed(self):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in ads
        """
        if self._view.tiled_plot_checkbox.isChecked():
            tiled_by = self._view.tiled_by()
            keys = self._create_tiled_keys(tiled_by)
            self._figure_presenter.convert_plot_to_tiled_plot(keys, tiled_by)
        else:
            self._figure_presenter.convert_plot_to_single_plot()

    def handle_tiled_by_type_changed(self):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in ads
        """
        if not self._view.tiled_plot_checkbox.isChecked():
            return
        tiled_by = self._view.tiled_by()
        keys = self._create_tiled_keys(tiled_by)
        self._figure_presenter.convert_plot_to_tiled_plot(keys, tiled_by)

    def handle_rebin_options_changed(self):
        pass

    def handle_added_or_removed_group_or_pair_to_plot(self, group_pair_info):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        """
        is_added = group_pair_info["is_added"]
        name = group_pair_info["name"]
        if is_added:
            self.handle_added_group_or_pair_to_plot(name)
        else:
            self.handle_removed_group_or_pair_to_plot(name)

    def handle_added_group_or_pair_to_plot(self, group_or_pair_name):
        """
        Handles a group or pair being added from the view
        """
        if len(self.context.group_pair_context.selected_pairs) != 0 and self._view.get_selected() == COUNTS_PLOT_TYPE:
            self._view.plot_selector.blockSignals(True)
            self._view.plot_selector.setCurrentText(ASYMMETRY_PLOT_TYPE)
            self._view.plot_selector.blockSignals(False)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')

        # # if tiled by group, we will need to recreate the tiles
        if self._view.tiled_plot_checkbox.isChecked() and self._view.tiled_by() == TILED_BY_GROUP_TYPE:
            tiled_by = self._view.tiled_by()
            keys = self._create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)
            workspace_list = self.workspace_finder.get_workspace_list_to_plot(True, self._view.get_plot_type())
            indices = [0 for _ in range(len(workspace_list))]
            self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=False)

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair \
            (group_or_pair_name, is_raw=True, plot_type=self._view.get_plot_type())
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=True, autoscale=False)

    def handle_removed_group_or_pair_to_plot(self, group_or_pair_name):
        """
        Handles a group or pair being removed in grouping widget analysis table
        """
        # # if tiled by group, we will need to recreate the tiles
        if self._view.tiled_plot_checkbox.isChecked() and self._view.tiled_by() == TILED_BY_GROUP_TYPE:
            tiled_by = self._view.tiled_by()
            keys = self._create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)
            workspace_list = self.workspace_finder.get_workspace_list_to_plot(True, self._view.get_plot_type())
            indices = [0 for _ in range(len(workspace_list))]
            self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=False)

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair \
            (group_or_pair_name, is_raw=True, plot_type=self._view.get_plot_type())
        self._figure_presenter.remove_workspace_names_from_plot(workspace_list)

    def handle_instrument_changed(self):
        pass

    # Interface to plotting
    # def plot_run_and_fit_workspaces(self, workspaces):
    #     pass
    #     #self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=True, autoscale=False)

    def _create_tiled_keys(self, tiled_by):
        if tiled_by == TILED_BY_GROUP_TYPE:
            keys = self.context.group_pair_context.selected_groups + self.context.group_pair_context.selected_pairs
        else:
            keys = [str(item) for sublist in self.context.data_context.current_runs for item in sublist]
        return keys

