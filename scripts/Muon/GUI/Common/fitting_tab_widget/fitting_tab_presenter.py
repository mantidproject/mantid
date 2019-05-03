from Muon.GUI.Common.fitting_tab_widget.workspace_selector_view import WorkspaceSelectorView

class FittingTabPresenter(object):
    def __init__(self, view, context):
        self.view = view
        self.context = context
        self._selected_data = []

    def handle_select_fit_data_clicked(self):
        selected_data, dialog_return = WorkspaceSelectorView.get_selected_data(self.context.data_context.current_runs,
                                                                               self.context.data_context.instrument,
                                                                               self.selected_data,
                                                                               self.view)

        if dialog_return:
            self.selected_data = selected_data

    def handle_display_workspace_changed(self):
        pass

    @property
    def selected_data(self):
        return self._selected_data

    @selected_data.setter
    def selected_data(self, value):
        self._selected_data = value
        self.view.update_displayed_data_combo_box(value)
