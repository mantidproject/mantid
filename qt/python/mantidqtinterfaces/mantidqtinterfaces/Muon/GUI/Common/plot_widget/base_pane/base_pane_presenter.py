# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.external_plotting.external_plotting_model import ExternalPlottingModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.external_plotting.external_plotting_view import ExternalPlottingView
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from mantid.dataobjects import Workspace2D


class BasePanePresenter:
    def __init__(self, view, model, context, figure_presenter, external_plotting_view=None, external_plotting_model=None):
        self._name = model.name
        self._data_type = [""]
        self._sort_by = [""]
        self._view = view
        self._model = model
        self.context = context
        self._figure_presenter = figure_presenter
        self._x_data_range = context.plot_panes_context[self._name].default_xlims
        self._external_plotting_view = external_plotting_view if external_plotting_view else ExternalPlottingView()
        self._external_plotting_model = external_plotting_model if external_plotting_model else ExternalPlottingModel()
        self.add_or_remove_plot_observer = GenericObserverWithArgPassing(self.handle_added_or_removed_plot)
        self._view.on_rebin_options_changed(self.handle_use_raw_workspaces_changed)
        self.workspace_replaced_in_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_replaced_in_ads)
        self.workspace_deleted_from_ads_observer = GenericObserverWithArgPassing(self.handle_workspace_deleted_from_ads)
        self.clear_plot_observer = GenericObserver(self.create_empty_plot)

        self._setup_view_connections()

    def _setup_view_connections(self):
        # tile plots
        self._view.on_tiled_by_type_changed(self.handle_tiled_by_type_changed)
        self._view.on_plot_tiled_checkbox_changed(self.handle_plot_tiled_state_changed)
        # external plot
        self._view.on_external_plot_pressed(self.handle_external_plot_requested)
        self._view.on_plot_type_changed(self.handle_data_type_changed)
        # rebin
        self.rebin_options_set_observer = GenericObserver(self.handle_rebin_options_changed)
        # data changed
        self.data_changed_observer = GenericObserver(self.handle_data_updated)

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        return

    def handle_data_updated(self):
        """
        Handles the data being updated
        """
        return

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

    def handle_workspace_replaced_in_ads(self, workspace):
        """
        Handles the use raw workspaces being changed (e.g rebinned) in the ADS
        and updates to the values from recalculation.
        :param workspace: workspace 2D object
        """
        workspace_name = workspace.name()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.replace_workspace_in_plot(workspace)

    def handle_workspace_deleted_from_ads(self, workspace: Workspace2D):
        """
        Handles a workspace being deleted from ads by removing the workspace from the plot
        :param workspace: workspace 2D object
        """
        # this is to allow both MA and FDA to be open at the same time
        # without this an error is thrown as the ws name is passed as a string
        workspace_name = None
        if type(workspace) is Workspace2D:
            workspace_name = workspace.name()
        else:
            return

        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        if workspace_name in plotted_workspaces:
            self._figure_presenter.remove_workspace_from_plot(workspace)

    def handle_added_or_removed_plot(self, plot_info):
        """
        Handles a group or pair being added or removed from
        the grouping widget analysis table
        :param group_pair_info: A dictionary containing information on the removed group/pair
        """
        is_added = plot_info["is_added"]
        name = plot_info["name"]
        if is_added:
            self.add_list_to_plot([name], [0])
        else:
            self.remove_list_from_plot([name])

    def add_list_to_plot(self, ws_names, indices, hold=False, autoscale=False):
        self._update_tile_plot()
        plotted_workspaces, _ = self._figure_presenter.get_plotted_workspaces_and_indices()
        self._figure_presenter.plot_workspaces(ws_names, indices, hold, autoscale)
        if plotted_workspaces == []:
            self._figure_presenter.force_autoscale()

    def remove_list_from_plot(self, names):
        self._update_tile_plot()
        self._figure_presenter.remove_workspace_names_from_plot(names)

    def update_view(self):
        self._view.setup_plot_type_options(self._data_type)
        self._view.setup_tiled_by_options(self._sort_by)

    @property
    def view(self):
        return self._view

    @property
    def name(self):
        return self._name

    @property
    def data_types(self):
        return self._data_types

    @property
    def sort_by(self):
        return self._sort_by

    def update_range(self, range):
        self._x_data_range = range

    @property
    def get_range(self):
        return self._x_data_range

    def hide(self):
        self._view.hide()

    def show(self):
        self._view.show()

    def handle_rebin_options_changed(self):
        """
        Handles rebin options changed on the home tab
        """
        if self.context._do_rebin():
            self._view.set_raw_checkbox_state(False)
        else:
            self._view.set_raw_checkbox_state(True)

    def handle_use_raw_workspaces_changed(self):
        self.check_if_can_use_rebin()

    def check_if_can_use_rebin(self):
        if not self._view.is_raw_plot() and not self.context._do_rebin():
            self._view.set_raw_checkbox_state(True)
            self._view.warning_popup("No rebin options specified")
            return False
        return True

    def handle_plot_tiled_state_changed(self):
        """
        Handles the tiled plots checkbox being changed in the view. This leads to two behaviors:
        If switching to tiled plot, create a new figure based on the number of tiles and replot the data
        If switching from a tiled plot, create a new single figure and replot the data
        """
        self.context.plot_panes_context[self.name].settings.set_tiled(self._view.is_tiled_plot())
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            self.context.plot_panes_context[self.name].settings.set_tiled_by(tiled_by)
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.convert_plot_to_tiled_plot(keys)
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
        self.context.plot_panes_context[self.name].settings.set_tiled_by(tiled_by)
        keys = self._model.create_tiled_keys(tiled_by)
        self._figure_presenter.convert_plot_to_tiled_plot(keys)

    def _update_tile_plot(self):
        if self._view.is_tiled_plot():
            tiled_by = self._view.tiled_by()
            self.context.plot_panes_context[self.name].settings.set_tiled_by(tiled_by)
            keys = self._model.create_tiled_keys(tiled_by)
            self._figure_presenter.create_tiled_plot(keys)
            self._figure_presenter._handle_autoscale_y_axes()

    def clear_subplots(self):
        self._figure_presenter.clear_subplots()

    def create_empty_plot(self):
        self._figure_presenter.create_single_plot()
