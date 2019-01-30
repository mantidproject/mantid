from mantidqt.widgets.settings.view import SettingsView


class SettingsPresenter(object):
    def __init__(self, parent, view=None):
        self.view = view if view else SettingsView(parent, self)

    def action_current_row_changed(self, new_pos):
        self.view.current.hide()
        if 0 == new_pos:
            if self.view.current != self.view.general_settings_view:
                self.view.container.replaceWidget(self.view.current, self.view.general_settings_view)
                self.view.current = self.view.general_settings_view
        elif 1 == new_pos:
            if self.view.current != self.view.plots_settings_view:
                self.view.container.replaceWidget(self.view.current, self.view.plots_settings_view)
                self.view.current = self.view.plots_settings_view
        elif 2 == new_pos:
            pass

        self.view.current.show()
