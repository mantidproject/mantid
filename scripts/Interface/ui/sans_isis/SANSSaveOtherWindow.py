from . import ui_save_other_dialog as ui_save_other_dialog
from PyQt4 import QtGui
from sans.common.enums import SaveType

class SANSSaveOtherDialog(QtGui.QDialog, ui_save_other_dialog.Ui_SaveOtherDialog):
    def __init__(self):
        super(QtGui.QDialog, self).__init__()
        self.subscribers = []
        self.setup_view()

    def setup_view(self):
        self.setupUi(self)
        self.filename_lineEdit.textChanged.connect(self.on_file_name_changed)
        self.browse_pushButton.pressed.connect(self.on_browse_clicked)
        self.save_button.pressed.connect(self.on_save_clicked)
        self.cancel_button.pressed.connect(self.on_cancel_clicked)
        self.directory_lineEdit.textChanged.connect(self.on_directory_changed)
        self.NxCanSAS_checkBox.setChecked(True)
        self.workspace_listWidget.itemSelectionChanged.connect(self.on_item_selection_changed)

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

    def on_directory_changed(self):
        for subscriber in self.subscribers:
            subscriber.on_directory_changed(self.current_directory)

    def on_item_selection_changed(self):
        for subscriber in self.subscribers:
            subscriber.on_item_selection_changed()

    def populate_workspace_list(self, workspace_list):
        for workspace in workspace_list:
            self.workspace_listWidget.addItem(workspace)


    def update_workspace_list(self):
        pass

    def get_selected_workspaces(self):
        return [str(x.text()) for x in self.workspace_listWidget.selectedItems()]

    def get_save_options(self):
        save_types = []
        if self.RKH_checkBox.isChecked():
            save_types.append(SaveType.RKH)
        if self.NxCanSAS_checkBox.isChecked():
            save_types.append(SaveType.NXcanSAS)
        if self.CanSAS_checkBox.isChecked():
            save_types.append(SaveType.CanSAS)
        return save_types

    def launch_file_browser(self, current_directory):
        filename = QtGui.QFileDialog.getExistingDirectory(self, 'Select Directory', current_directory,
                                                          QtGui.QFileDialog.ShowDirsOnly)
        return filename

    def disable_filename(self):
        self.filename_lineEdit.setEnabled(False)

    def enable_filename(self):
        self.filename_lineEdit.setEnabled(True)

    @property
    def current_directory(self):
        return str(self.directory_lineEdit.text())

    @current_directory.setter
    def current_directory(self, value):
        self.directory_lineEdit.setText(value)

    @property
    def filename(self):
        return str(self.filename_lineEdit.text())

    @filename.setter
    def filename(self, value):
        self.filename_lineEdit.setText(value)