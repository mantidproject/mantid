from mantidqt.widgets.settings.view import SettingsView


class SettingsPresenter(object):
    ASK_BEFORE_CLOSE_TITLE = "Confirm exit"
    ASK_BEFORE_CLOSE_MESSAGE = "Are you sure you want to exit without applying the settings?"

    def __init__(self, parent, view=None):
        self.view = view if view else SettingsView(parent, self)

        self.ask_before_close = False

    def action_current_row_changed(self, new_pos):
        print("Row changed to", new_pos)
        self.view.current.hide()
        if 0 == new_pos:
            new_view = self.view.general_settings.view
        else:  # 1 == new_pos
            new_view = self.view.plots_settings_view
        # elif 2 == new_pos:
        #     new_view = self.view.plots_settings_view
        #     pass

        if self.view.current != new_view:
            print("Changing widget.")
            self.view.container.replaceWidget(self.view.current, new_view)
            self.view.current = new_view

        self.view.current.show()

    def action_cancel_button(self):
        if not self.ask_before_close or self.view.ask_before_close():
            self.view.close()

    def action_ok_button(self):
        self.ask_before_close = False
        # TODO save stuff
        self.view.close()
        print("OK Button")

    def action_apply_button(self):
        self.ask_before_close = False
        print("Apply Button")
