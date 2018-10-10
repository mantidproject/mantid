from . import ui_save_other_dialog as ui_save_other_dialog
from PyQt4 import QtGui


class SANSSaveOtherDialog(QtGui.QDialog, ui_save_other_dialog.Ui_SaveOtherDialog):
    def __init__(self):
        super(SANSSaveOtherDialog, self).__init__()
        self.setupUi(self)
        self.subscribers = []

        self.filename_lineEdit.textChanged.connect(self.on_file_name_changed)
        self.browse_pushButton.pressed.connect(self.on_browse_clicked)
        self.save_button.pressed.connect(self.on_save_clicked)
        self.cancel_button.pressed.connect(self.on_cancel_clicked)

    def subscribe(self, subscriber):
        self.subscribers.append(subscriber)

    def on_file_name_changed(self):
        file_name = str(self.filename_lineEdit.text())
        for subscriber in self.subscribers:
            subscriber.on_file_name_changed(file_name)

    def on_browse_clicked(self):
        for subscriber in self.subscribers:
            subscriber.on_browse_clicked()

    def on_save_clicked(self):
        for subscriber in self.subscribers:
            subscriber.on_save_clicked()

    def on_cancel_clicked(self):
        for subscriber in self.subscribers:
            subscriber.on_cancel_clicked()

    def update_workspace_list(self):
        pass

    def get_selected_workspaces(self):
        pass

    def get_save_options(self):
        pass