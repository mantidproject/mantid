# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore, QtWidgets

from mantidqt.utils.qt import load_ui
from mantidqt.widgets.workspacewidget import workspacetreewidget

from mantid import UsageService
from mantid.kernel import FeatureType
from sans.common.enums import SaveType


Ui_SaveOtherDialog, _ = load_ui(__file__, "save_other_dialog.ui")


class SANSSaveOtherDialog(QtWidgets.QDialog, Ui_SaveOtherDialog):
    def __init__(self, parent_widget=None):
        super(QtWidgets.QDialog, self).__init__(parent=parent_widget)
        self.subscribers = []
        self.setup_view()

        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Save Other Tab"], False)

    def setup_view(self):
        self.setupUi(self)
        self.filename_lineEdit.textChanged.connect(self.on_file_name_changed)
        self.browse_pushButton.pressed.connect(self.on_browse_clicked)
        self.save_button.pressed.connect(self.on_save_clicked)
        self.help_button.clicked.connect(self._on_help_button_clicked)
        self.cancel_button.pressed.connect(self.on_cancel_clicked)
        self.directory_lineEdit.textChanged.connect(self.on_directory_changed)
        self.nxcansas_checkBox.setChecked(True)
        self.ads_widget = workspacetreewidget.WorkspaceTreeWidget(True, self)
        self.ads_widget.treeSelectionChanged.connect(self.on_item_selection_changed)
        self.ads_widget.refreshWorkspaces()
        self.ads_widget.installEventFilter(self)
        self.workspace_list_layout.addWidget(self.ads_widget, 0, 1, 4, 1)
        self.progress_bar_value = 0

    def eventFilter(self, source, event):
        if event.type() == QtCore.QEvent.KeyPress:
            return True
        return QtWidgets.QWidget.eventFilter(self, source, event)

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

    def get_selected_workspaces(self):
        return [str(x) for x in self.ads_widget.getSelectedWorkspaceNames()]

    def get_save_options(self):
        save_types = []
        if self.RKH_checkBox.isChecked():
            save_types.append(SaveType.RKH)
        if self.nxcansas_checkBox.isChecked():
            save_types.append(SaveType.NX_CAN_SAS)
        if self.CanSAS_checkBox.isChecked():
            save_types.append(SaveType.CAN_SAS)
        return save_types

    def launch_file_browser(self, current_directory):
        filename = QtWidgets.QFileDialog.getExistingDirectory(
            self, "Select Directory", current_directory, QtWidgets.QFileDialog.ShowDirsOnly
        )
        return filename

    def rename_filebox(self, name):
        self.filename_label.setText(name)

    def _on_help_button_clicked(self):
        try:
            import mantidqt

            mantidqt.interfacemanager.InterfaceManager().showCustomInterfaceHelp("sans_save_other", "isis_sans")
        except:
            pass

    @property
    def progress_bar_minimum(self):
        return self.progress_bar.minimum()

    @progress_bar_minimum.setter
    def progress_bar_minimum(self, value):
        self.progress_bar.setMinimum(value)

    @property
    def progress_bar_maximum(self):
        return self.progress_bar.maximum()

    @progress_bar_maximum.setter
    def progress_bar_maximum(self, value):
        self.progress_bar.setMaximum(value)

    @property
    def progress_bar_value(self):
        return self.progress_bar.value()

    @progress_bar_value.setter
    def progress_bar_value(self, progress):
        self.progress_bar.setValue(progress)

    def increment_progress(self):
        self.progress_bar_value += 1

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
