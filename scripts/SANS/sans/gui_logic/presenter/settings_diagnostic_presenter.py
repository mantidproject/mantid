""" The settings diagnostic tab which visualizes the SANS state object. """

from ui.sans_isis.settings_diagnostic_tab import SettingsDiagnosticTab


class SettingsDiagnosticPresenter(object):
    class ConcreteSettingsDiagnosticTabListener(SettingsDiagnosticTab.SettingsDiagnosticTabListener):
        def __init__(self, presenter):
            super(SettingsDiagnosticPresenter.ConcreteSettingsDiagnosticTabListener, self).__init__()
            self._presenter = presenter

        def on_row_changed(self):
            self._presenter.on_row_changed()

        def on_update_rows(self):
            self._presenter.on_update_rows()

        def on_collapse(self):
            self._presenter.on_collapse()

        def on_expand(self):
            self._presenter.on_expand()

    def __init__(self, parent_presenter):
        super(SettingsDiagnosticPresenter, self).__init__()
        self._view = None
        self._parent_presenter = parent_presenter

    def on_collapse(self):
        self._view.collapse()

    def on_expand(self):
        self._view.expand()

    def on_row_changed(self):
        row_index = self._view.get_current_row()
        state = self.get_state(row_index)
        if state:
            self.display_state_diagnostic_tree(state)

    def on_update_rows(self):
        """
        Update the row selection in the combobox
        """
        current_row_index = self._view.get_current_row()
        valid_row_indices = self._parent_presenter.get_row_indices()

        new_row_index = -1
        if current_row_index in valid_row_indices:
            new_row_index = current_row_index
        elif len(valid_row_indices) > 0:
            new_row_index = valid_row_indices[0]

        self._view.update_rows(valid_row_indices)

        if new_row_index != -1:
            self.set_row(new_row_index)
            self.on_row_changed()

    def set_row(self, index):
        self._view.set_row(index)

    def set_view(self, view):
        if view:
            self._view = view

            # Set up row selection listener
            listener = SettingsDiagnosticPresenter.ConcreteSettingsDiagnosticTabListener(self)
            self._view.add_listener(listener)

            # Set the default gui
            self._set_default_gui()

    def _set_default_gui(self):
        self._view.update_rows([])
        self.display_state_diagnostic_tree(state=None)

    def get_state(self, index):
        return self._parent_presenter.get_state_for_row(index)

    def display_state_diagnostic_tree(self, state):
        # Convert to dict before passing the state to the view
        if state is not None:
            state = state.property_manager
        self._view.set_tree(state)
