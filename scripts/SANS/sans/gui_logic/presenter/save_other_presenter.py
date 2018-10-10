from ui.sans_isis.SANSSaveOtherWindow import SANSSaveOtherDialog


class SaveOtherPresenter():
    def __init__(self, parent_presenter=None, view=SANSSaveOtherDialog()):
        self._parent_presenter = parent_presenter
        self._view = view
        self._view.subscribe(self)

    def on_file_name_changed(self, file_name):
        pass

    def on_browse_clicked(self):
        pass

    def on_save_clicked(self):
        pass

    def on_cancel_clicked(self):
        self._view.done(0)