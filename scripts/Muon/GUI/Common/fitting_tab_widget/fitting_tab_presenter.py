from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView
from Muon.GUI.Common.observer_pattern import GenericObserver


class FittingTabPresenter(object):
    def __init__(self, view, context):
        self.view = view
        self.context = context
        self._selected_data = []
        self.manual_selection_made = False
        self.update_selected_workspace_guess()
        self.gui_context_observer = GenericObserver(self.update_selected_workspace_guess)
        self.run_changed_observer = GenericObserver(self.update_selected_workspace_guess)

    def handle_select_fit_data_clicked(self):
        selected_data, dialog_return = WorkspaceSelectorView.get_selected_data(self.context.data_context.current_runs,
                                                                               self.context.data_context.instrument,
                                                                               self.selected_data,
                                                                               self.view.fit_to_raw,
                                                                               self.context,
                                                                               self.view)

        if dialog_return:
            self.selected_data = selected_data
            self.manual_selection_made = True

    def update_selected_workspace_guess(self):
        if not self.manual_selection_made:
            guess_selection = self.context.get_names_of_workspaces_to_fit(runs='All',
                                                                          group_and_pair=self.context.gui_context[
                                                                              'selected_group_pair'], phasequad=False,
                                                                          rebin=not self.view.fit_to_raw)
            self.selected_data = guess_selection

    def handle_display_workspace_changed(self):
        pass

    def handle_use_rebin_changed(self):
        if not self.manual_selection_made:
            self.update_selected_workspace_guess()
        else:
            self.selected_data = self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents(self.selected_data)

    @property
    def selected_data(self):
        return self._selected_data

    @selected_data.setter
    def selected_data(self, value):
        self._selected_data = value
        self.view.update_displayed_data_combo_box(value)
