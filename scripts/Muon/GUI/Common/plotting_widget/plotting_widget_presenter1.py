# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict

from Muon.GUI.Common.contexts.fitting_context import FitInformation
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from Muon.GUI.Common.plotting_widget.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plotting_widget.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plotting_widget.plotting_canvas_presenter_interface import PlottingCanvasPresenterInterface
from Muon.GUI.Common.plotting_widget.plotting_widget_model1 import PlotWidgetModel
from Muon.GUI.Common.plotting_widget.plotting_widget_view_interface import PlottingWidgetViewInterface
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from mantid.dataobjects import Workspace2D


class PlotWidgetPresenterCommon(HomeTabSubWidget):

    def __init__(self, view: PlottingWidgetViewInterface, model: PlotWidgetModel, context,
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
        # figure presenter - the common presenter talks to this through an interface
        self._figure_presenter = figure_presenter
        self._plotting_view = ExternalPlottingView()
        self._plotting_model = ExternalPlottingModel()

        # gui observers
        self._setup_gui_observers()
        # Connect to the view
        self._setup_view_connections()
        # setup view options
        self.update_view_from_model()

    def update_view_from_model(self):
        """"Updates the view based on data in the model """
        plot_types = self._model.get_plot_types()
        self._view.setup_plot_type_options(plot_types)
        tiled_types = self._model.get_tiled_by_types()
        self._view.setup_tiled_by_options(tiled_types)

    def _setup_gui_observers(self):
        """"Setup GUI observers, e.g fit observers"""
        self.input_workspace_observer = GenericObserver(self.handle_data_updated)
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)
        self.instrument_observer = GenericObserver(self.handle_instrument_changed)
        self.fit_observer = GenericObserverWithArgPassing(self.handle_fit_completed)
        self.fit_removed_observer = GenericObserverWithArgPassing(self.handle_fit_removed)

    def _setup_view_connections(self):
        self._view.on_plot_tiled_checkbox_changed(self.handle_plot_tiled_state_changed)
        self._view.on_tiled_by_type_changed(self.handle_tiled_by_type_changed)
        self._view.on_plot_type_changed(self.handle_plot_type_changed)
        self._view.on_external_plot_pressed(self.handle_external_plot_requested)
        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)

    def handle_data_updated(self):
        """
        Handles the group, pair calculation finishing
        """
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)

        workspace_list, indices = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                     self._view.get_plot_type())

        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=False)

    def handle_workspace_deleted_from_ads(self, workspace: Workspace2D):
        """
        Handles a workspace being deleted from ads by removing the workspace from the plot
        :param workspace: workspace 2D object
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.remove_workspace_from_plot(workspace)

    def handle_workspace_replaced_in_ads(self, workspace: Workspace2D):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in the ADS.
        :param workspace: workspace 2D object
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.replace_workspace_in_plot(workspace)

    def handle_plot_type_changed(self):
        """
        Handles the plot type being changed in the view by plotting the workspaces corresponding to the new plot type
        """
        self._check_if_counts_and_groups_selected()

        workspace_list, indices = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                     self._view.get_plot_type())
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=True)

    def handle_plot_tiled_state_changed(self):
        """
        Handles the tiled plots checkbox being changed in the view. This leads to two behaviors:
        If switching to tiled plot, create a new figure based on the number of tiles and replot the data
        If switching from a tiled plot, create a new single figure and replot the data
        """
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.convert_plot_to_tiled_plot(keys, tiled_by)
        else:
            self._figure_presenter.convert_plot_to_single_plot()

    def handle_tiled_by_type_changed(self):
        """
        Handles the tiled type changing, this will cause the tiles (and the key for each tile) to change.
        This is handled by generating the new keys and replotting the data based on these new tiles.
        """
        if not self._view.is_tiled_plot():
            return
        tiled_by = self._view.tiled_by()
        keys = self._model.create_tiled_keys(tiled_by)
        self._figure_presenter.convert_plot_to_tiled_plot(keys, tiled_by)

    def handle_rebin_options_changed(self):
        """
        Handles rebin options changed on the home tab
        """
        if self.context._do_rebin():
            self._view.set_raw_checkbox_state(False)
        else:
            self._view.set_raw_checkbox_state(True)

    def handle_use_raw_workspaces_changed(self):
        """
        Handles plot raw changed in view
        """
        if not self._view.is_raw_plot() and not self.context._do_rebin():
            self._view.set_raw_checkbox_state(True)
            self._view.warning_popup('No rebin options specified')
            return
        workspace_list, indices = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                     self._view.get_plot_type())
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=False)

    def handle_added_or_removed_group_or_pair_to_plot(self, group_pair_info: Dict):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        :param group_pair_info: A dictionary continue information on the removed group/pair
        """
        is_added = group_pair_info["is_added"]
        name = group_pair_info["name"]
        if is_added:
            self.handle_added_group_or_pair_to_plot(name)
        else:
            self.handle_removed_group_or_pair_to_plot(name)

    def handle_added_group_or_pair_to_plot(self, group_or_pair_name: str):
        """
        Handles a group or pair being added from the view
        :param group_or_pair_name: The group or pair name that was added to the analysis
        """
        self._check_if_counts_and_groups_selected()
        # if tiled by group, we will need to recreate the tiles
        if self._view.is_tiled_plot() and self._view.tiled_by() == self._model.tiled_by_group:
            tiled_by = self._view.tiled_by()
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair \
            (group_or_pair_name, is_raw=True, plot_type=self._view.get_plot_type())
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=True, autoscale=False)

    def handle_removed_group_or_pair_to_plot(self, group_or_pair_name: str):
        """
        Handles a group or pair being removed in grouping widget analysis table
        :param group_or_pair_name: The group or pair name that was removed from the analysis
        """
        # tiled by group, we will need to recreate the tiles
        if self._view.is_tiled_plot() and self._view.tiled_by() == self._model.tiled_by_group:
            tiled_by = self._view.tiled_by()
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair \
                (group_or_pair_name, is_raw=True, plot_type=self._view.get_plot_type())
        self._figure_presenter.remove_workspace_names_from_plot(workspace_list)

    def handle_external_plot_requested(self):
        """
        Handles an external plot request
        """
        axes = self._figure_presenter.get_fig_axes()
        external_fig = self._plotting_view.create_external_plot_window(axes)
        data = self._plotting_model.get_plotted_workspaces_and_indices_from_axes(axes)
        self._plotting_view.plot_data(external_fig, data)
        self._plotting_view.copy_axes_setup(external_fig, axes)
        self._plotting_view.show(external_fig)

    def handle_instrument_changed(self):
        """
        Handles the instrument being changed by creating a blank plot window
        """
        self._figure_presenter.create_single_plot()

    def handle_fit_completed(self, fit: FitInformation):
        """
        Handles a completed fit which is given as an input to the function
        :param fit: The completed fit
        """
        if fit is None:
            return
        workspace_list, indices = self._model.get_fit_workspace_and_indices(fit)
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=True, autoscale=False)

    def handle_fit_removed(self, fits: list[FitInformation]):
        """
        Handles the removal of a fit which is given as an input to the function
        :param fits: A list of fits to remove from the plot
        """
        fit_workspaces_to_remove = []
        for fit in fits:
            fit_workspaces_to_remove.extend(fit.output_workspace_names)

        self._figure_presenter.remove_workspace_names_from_plot(fit_workspaces_to_remove)

    def _check_if_counts_and_groups_selected(self):
        if len(self.context.group_pair_context.selected_pairs) != 0 and \
                self._view.get_selected() == self._model.counts_plot:
            self._view.plot_selector.blockSignals(True)
            self._view.plot_selector.setCurrentText(self._model.asymmetry_plot)
            self._view.plot_selector.blockSignals(False)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')
