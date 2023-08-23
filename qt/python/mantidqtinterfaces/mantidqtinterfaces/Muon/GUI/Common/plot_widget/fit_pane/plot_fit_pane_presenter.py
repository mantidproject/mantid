# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_presenter import BasePanePresenter
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import remove_rebin_from_name, add_rebin_to_name


class PlotFitPanePresenter(BasePanePresenter):
    def __init__(self, view, model, context, fitting_context, figure_presenter):
        """
        Initializes the PlotFitPanePresenter. The fitting_context is a FittingContext that corresponds to this specific
        fit pane. For instance it is a ModelFittingContext for the Model Fitting pane, and is a TFAsymmetryFittingContext
        for the tf asymmetry Fitting pane.
        """
        super().__init__(view, model, context, figure_presenter)
        self._fitting_context = fitting_context
        self._data_type = ["Asymmetry"]
        self._sort_by = ["Group/Pair", "Run"]
        self.update_view()
        self._current_fit_info = None
        self._view.enable_tile_plotting_options()
        self._view.on_plot_diff_checkbox_changed(self.plot_diff_changed)

        self.plot_selected_fit_observer = GenericObserverWithArgPassing(self.handle_plot_selected_fits)
        self.remove_plot_guess_observer = GenericObserver(self.handle_remove_plot_guess)
        self.update_plot_guess_observer = GenericObserver(self.handle_update_plot_guess)

    def plot_diff_changed(self):
        self.handle_plot_selected_fits(self._current_fit_info)

    def handle_plot_selected_fits(self, fit_information_list, autoscale=False):
        """Plots a list of selected fit workspaces (obtained from fit and seq fit tabs).
        :param fit_information_list: List of named tuples each entry of the form (fit, input_workspaces)
        """
        workspace_list = []
        indices = []
        raw = self._view.is_raw_plot()
        with_diff = self._view.is_plot_diff()
        if fit_information_list:
            self._current_fit_info = fit_information_list
            for fit_information in fit_information_list:
                fit = fit_information.fit
                fit_workspaces, fit_indices = self._model.get_fit_workspace_and_indices(fit, with_diff)
                workspace_list += self.match_raw_selection(fit_information.input_workspaces, raw) + fit_workspaces
                indices += [0] * len(fit_information.input_workspaces) + fit_indices
                # dont shade the data but do shade the fit lines
                self._figure_presenter.add_shaded_region(fit_workspaces, fit_indices)
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=autoscale)
        # the data change probably means its the wrong scale
        self._figure_presenter.force_autoscale()

    def match_raw_selection(self, workspace_names, plot_raw):
        ws_list = []
        workspace_list = workspace_names
        if not isinstance(workspace_names, list):
            workspace_list = [workspace_names]
        for workspace_name in workspace_list:
            fit_raw_data = self._fitting_context.fit_to_raw
            # binned data but want raw plot
            if plot_raw and not fit_raw_data:
                ws_list.append(remove_rebin_from_name(workspace_name))
            # raw data but want binned plot
            elif not plot_raw and fit_raw_data:
                ws_list.append(add_rebin_to_name(workspace_name))
            else:
                ws_list.append(workspace_name)
        return ws_list

    def handle_use_raw_workspaces_changed(self):
        if self.check_if_can_use_rebin():
            self.handle_plot_selected_fits(self._current_fit_info)

    def handle_data_updated(self, autoscale=True, hold_on=False):
        self.handle_plot_selected_fits(self._current_fit_info, autoscale)

    def handle_remove_plot_guess(self):
        if self._fitting_context.guess_workspace_name is not None:
            self._figure_presenter.remove_workspace_names_from_plot([self._fitting_context.guess_workspace_name])

    def handle_update_plot_guess(self):
        if self._fitting_context.guess_workspace_name is not None and self._fitting_context.plot_guess:
            self._figure_presenter.plot_guess_workspace(self._fitting_context.guess_workspace_name)

    def create_empty_plot(self):
        self._figure_presenter.create_single_plot()
        # need to do this manually
        self._current_fit_info = None
