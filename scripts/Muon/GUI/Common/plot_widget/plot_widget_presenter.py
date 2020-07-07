# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, List
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import FitPlotInformation
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel
from Muon.GUI.Common.plot_widget.external_plotting.external_plotting_view import ExternalPlottingView
from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
    PlottingCanvasPresenterInterface
from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
from Muon.GUI.Common.plot_widget.plot_widget_model import PlotWidgetModel
from Muon.GUI.Common.plot_widget.plot_widget_view_interface import PlotWidgetViewInterface
from Muon.GUI.Common.contexts.muon_gui_context import PlotMode
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable
from mantid.dataobjects import Workspace2D

MUON_ANALYSIS_DEFAULT_X_RANGE = [0.0, 15.0]
FREQUENCY_DOMAIN_ANALYSIS_DEFAULT_X_RANGE = [0.0, 1000.0]


class PlotWidgetPresenterCommon(HomeTabSubWidget):

    def __init__(self, view: PlotWidgetViewInterface, model: PlotWidgetModel, context,
                 figure_presenter: PlottingCanvasPresenterInterface, get_selected_fit_workspaces,
                 external_plotting_view=None, external_plotting_model=None):
        """
        :param view: A reference to the QWidget object for plotting
        :param model: A reference to a model which contains the plotting logic
        :param context: A reference to the Muon context object
        :param figure_presenter: A reference to a figure presenter, which is used as an interface to plotting
        :param external_plotting_view: A reference to an external plotting_view - used for mocking
        :param external_plotting_model: A reference to an external plotting model - used for mocking
        """
        if not isinstance(figure_presenter, PlottingCanvasPresenterInterface):
            raise TypeError("Parameter figure_presenter must be of type PlottingCanvasPresenterInterface")

        self._view = view
        self._model = model
        self.context = context
        self._get_selected_fit_workspaces = get_selected_fit_workspaces
        # figure presenter - the common presenter talks to this through an interface
        self._figure_presenter = figure_presenter
        self._external_plotting_view = external_plotting_view if external_plotting_view else ExternalPlottingView()
        self._external_plotting_model = external_plotting_model if external_plotting_model else ExternalPlottingModel()

        # gui observers
        self._setup_gui_observers()
        self._setup_view_connections()

        self.update_view_from_model()

        self.data_plot_range = MUON_ANALYSIS_DEFAULT_X_RANGE
        self.fitting_plot_range = FREQUENCY_DOMAIN_ANALYSIS_DEFAULT_X_RANGE if\
            isinstance(self.context, FrequencyDomainAnalysisContext) else MUON_ANALYSIS_DEFAULT_X_RANGE
        self.data_plot_tiled_state = None

    def update_view_from_model(self):
        """"Updates the view based on data in the model """
        plot_types = self._model.get_plot_types()
        self._view.setup_plot_type_options(plot_types)
        tiled_types = self._model.get_tiled_by_types()
        self._view.setup_tiled_by_options(tiled_types)

    def _setup_gui_observers(self):
        """"Setup GUI observers, e.g fit observers"""
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)
        self.added_group_or_pair_observer = GenericObserverWithArgPassing(
            self.handle_added_or_removed_group_or_pair_to_plot)
        self.instrument_observer = GenericObserver(self.handle_instrument_changed)
        self.plot_selected_fit_observer = GenericObserverWithArgPassing(self.handle_plot_selected_fits)
        self.plot_guess_observer = GenericObserver(self.handle_plot_guess_changed)
        self.rebin_options_set_observer = GenericObserver(self.handle_rebin_options_changed)
        self.plot_type_changed_notifier = GenericObservable()

    def _setup_view_connections(self):
        self._view.on_plot_tiled_checkbox_changed(self.handle_plot_tiled_state_changed)
        self._view.on_tiled_by_type_changed(self.handle_tiled_by_type_changed)
        self._view.on_plot_type_changed(self.handle_plot_type_changed)
        self._view.on_external_plot_pressed(self.handle_external_plot_requested)
        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)
        self._view.on_plot_mode_changed(self.handle_plot_mode_changed_by_user)

    def handle_data_updated(self, autoscale=False):
        """
        Handles the group and pairs calculation finishing by plotting the loaded groups and pairs.
        """
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys, tiled_by)

        self.plot_all_selected_data(autoscale=autoscale, hold_on=False)

    def update_plot(self, autoscale=False):
        if self.context.gui_context['PlotMode'] == PlotMode.Data:
            self.handle_data_updated(autoscale=autoscale)
        elif self.context.gui_context['PlotMode'] == PlotMode.Fitting:  # Plot the displayed workspace
            self.handle_plot_selected_fits(
                self._get_selected_fit_workspaces(), autoscale
            )

    def handle_plot_mode_changed(self, plot_mode : PlotMode):
        if isinstance(self.context, FrequencyDomainAnalysisContext):
            self.handle_plot_mode_changed_for_frequency_domain_analysis(plot_mode)
        else:
            self.handle_plot_mode_changed_for_muon_analysis(plot_mode)

    def handle_plot_mode_changed_for_muon_analysis(self, plot_mode : PlotMode):
        if plot_mode == self.context.gui_context['PlotMode']:
            return

        self.context.gui_context['PlotMode'] = plot_mode

        self._view.set_plot_mode(str(plot_mode))
        if plot_mode == PlotMode.Data:
            self._view.enable_plot_type_combo()
            self.update_plot()
            self.fitting_plot_range = self._figure_presenter.get_plot_x_range()
            self._figure_presenter.set_plot_range(self.data_plot_range)
        elif plot_mode == PlotMode.Fitting:
            self._view.disable_plot_type_combo()
            self.update_plot()
            self.data_plot_range = self._figure_presenter.get_plot_x_range()
            self._figure_presenter.set_plot_range(self.fitting_plot_range)

        self._figure_presenter.autoscale_y_axes()

    def handle_plot_mode_changed_for_frequency_domain_analysis(self, plot_mode : PlotMode):
        if plot_mode == self.context.gui_context['PlotMode']:
            return

        self.context.gui_context['PlotMode'] = plot_mode

        self._view.set_plot_mode(str(plot_mode))
        if plot_mode == PlotMode.Data:
            self._view.enable_plot_type_combo()
            self._view.enable_tile_plotting_options()
            self._view.set_is_tiled_plot(self.data_plot_tiled_state)
            self.update_plot()
            self.fitting_plot_range = self._figure_presenter.get_plot_x_range()
            self._figure_presenter.set_plot_range(self.data_plot_range)
        elif plot_mode == PlotMode.Fitting:
            self._view.disable_plot_type_combo()
            self._view.disable_tile_plotting_options()
            self.data_plot_tiled_state = self._view.is_tiled_plot()
            self._view.set_is_tiled_plot(False)
            self.update_plot()
            self.data_plot_range = self._figure_presenter.get_plot_x_range()
            self._figure_presenter.set_plot_range(self.fitting_plot_range)

        self._figure_presenter.autoscale_y_axes()

    def handle_plot_mode_changed_by_user(self):
        plot_mode = PlotMode.Data if str(PlotMode.Data) == self._view.get_plot_mode() else PlotMode.Fitting

        self.handle_plot_mode_changed(plot_mode)

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
        if self._check_if_counts_and_groups_selected():
            return

        self.plot_all_selected_data(autoscale=True, hold_on=False)
        self.plot_type_changed_notifier.notify_subscribers(self._view.get_plot_type())

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
        :param group_pair_info: A dictionary containing information on the removed group/pair
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
            self.plot_all_selected_data(autoscale=False, hold_on=False)
            return

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair(
            group_or_pair_name, is_raw=self._view.is_raw_plot(), plot_type=self._view.get_plot_type())
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
            self.plot_all_selected_data(hold_on=False, autoscale=False)
            return

        workspace_list, indices = self._model.get_workspace_and_indices_for_group_or_pair(
            group_or_pair_name, is_raw=True, plot_type=self._view.get_plot_type())
        self._figure_presenter.remove_workspace_names_from_plot(workspace_list)

    def handle_external_plot_requested(self):
        """
        Handles an external plot request
        """
        axes = self._figure_presenter.get_plot_axes()
        external_fig = self._external_plotting_view.create_external_plot_window(axes)
        data = self._external_plotting_model.get_plotted_workspaces_and_indices_from_axes(axes)
        self._external_plotting_view.plot_data(external_fig, data)
        self._external_plotting_view.copy_axes_setup(external_fig, axes)
        self._external_plotting_view.show(external_fig)

    def handle_instrument_changed(self):
        """
        Handles the instrument being changed by creating a blank plot window
        """
        self._figure_presenter.create_single_plot()

    def handle_plot_selected_fits(self, fit_information_list: List[FitPlotInformation], autoscale=False):
        """Plots a list of selected fit workspaces (obtained from fit and seq fit tabs).
        :param fit_information_list: List of named tuples each entry of the form (fit, input_workspaces)
        """
        workspace_list = []
        indices = []

        if fit_information_list:
            for fit_information in fit_information_list:
                fit = fit_information.fit
                fit_workspaces, fit_indices = self._model.get_fit_workspace_and_indices(fit)
                workspace_list += fit_information.input_workspaces + fit_workspaces
                indices += [0] * len(fit_information.input_workspaces) + fit_indices

        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=autoscale)

    def handle_plot_guess_changed(self):
        if self.context.fitting_context.guess_ws is None:
            return

        if self.context.fitting_context.plot_guess:
            self._figure_presenter.plot_guess_workspace(self.context.fitting_context.guess_ws)
        else:
            self._figure_presenter.remove_workspace_names_from_plot([self.context.fitting_context.guess_ws])

    def plot_all_selected_data(self, autoscale, hold_on):
        """Plots all selected run data e.g runs and groups
        :param autoscale: Whether to autoscale the graph
        :param hold_on: Whether to keep previous plots
        """
        workspace_list, indices = self._model.get_workspace_list_and_indices_to_plot(self._view.is_raw_plot(),
                                                                                     self._view.get_plot_type())

        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=hold_on, autoscale=autoscale)

    def _check_if_counts_and_groups_selected(self):
        if len(self.context.group_pair_context.selected_pairs) != 0 and \
                self._view.get_plot_type() == self._model.counts_plot:
            self._view.set_plot_type(self._model.asymmetry_plot)
            self._view.warning_popup(
                'Pair workspaces have no counts workspace, plotting Asymmetry')
            return True
        return False
