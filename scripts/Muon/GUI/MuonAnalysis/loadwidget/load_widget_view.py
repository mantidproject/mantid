from __future__ import (absolute_import, division, print_function)

from PyQt4 import QtCore, QtGui


# from mantidqtpython import MantidQt

class LoadWidgetView(QtGui.QWidget):

    def __init__(self, parent=None, load_run_view=None, load_file_view=None):
        super(LoadWidgetView, self).__init__(parent)

        self.load_run_widget = load_run_view
        self.load_file_widget = load_file_view

        self.setupUi(self)
        self.set_connections()

    def set_connections(self):
        self.on_multiple_loading_check_changed(self.change_multiple_loading_state)

    def on_multiple_loading_check_changed(self, slot):
        self.multiple_loading_check.stateChanged.connect(slot)

    def get_multiple_loading_state(self):
        return self.multiple_loading_check.isChecked()

    def change_multiple_loading_state(self):
        if self.get_multiple_loading_state():
            self.load_behaviour_combo.setEnabled(True)
        else:
            self.load_behaviour_combo.setEnabled(False)

    def on_subwidget_loading_started(self, slot):
        self.load_run_widget.loadingStarted.connect(slot)
        self.load_file_widget.loadingStarted.connect(slot)

    def on_subwidget_loading_finished(self, slot):
        self.load_run_widget.loadingFinished.connect(slot)
        self.load_file_widget.loadingFinished.connect(slot)

    def on_file_widget_data_changed(self, slot):
        self.load_file_widget.dataChanged.connect(slot)

    def on_run_widget_data_changed(self, slot):
        self.load_run_widget.dataChanged.connect(slot)

    def on_clear_button_clicked(self, slot):
        self.clearButton.clicked.connect(slot)

    def setupUi(self, Form):

        self.clearButton = QtGui.QPushButton(Form)
        self.clearButton.setMinimumSize(QtCore.QSize(100, 25))
        self.clearButton.setObjectName("clearButton")
        self.clearButton.setToolTip("Clear the currently loaded data")
        self.clearButton.setText("Clear")

        self.multiple_loading_label = QtGui.QLabel(Form)
        self.multiple_loading_label.setObjectName("multiple_loading_label")
        self.multiple_loading_label.setText("Multiple loading : ")

        self.multiple_loading_check = QtGui.QCheckBox(Form)
        self.multiple_loading_check.setToolTip("Enable/disable loading multiple runs at once")
        self.multiple_loading_check.setChecked(False)

        # self.load_behaviour_label = QtWidgets.QLabel(self)
        # self.load_behaviour_label.setObjectName("load_behaviour_label")
        # self.load_behaviour_label.setText("Load Behaviour : ")

        self.load_behaviour_combo = QtGui.QComboBox(Form)
        self.load_behaviour_combo.setObjectName("load_behaviour_combo")
        self.load_behaviour_combo.addItem("Co-Add")
        self.load_behaviour_combo.addItem("Simultaneous")
        self.load_behaviour_combo.setToolTip("The behaviour of the loaded data in multiple file mode")
        self.load_behaviour_combo.setEnabled(False)

        self.manageDirectoriesButton = QtGui.QPushButton(Form)
        self.manageDirectoriesButton.setMinimumSize(QtCore.QSize(100, 25))
        self.manageDirectoriesButton.setObjectName("manageDirectoriesButton")
        self.manageDirectoriesButton.setText("Manage User Directories")

        self.spacer = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)

        # Set the layout vertically
        self.verticalLayout = QtGui.QVBoxLayout(Form)

        self.control_frame = QtGui.QFrame(self)

        # Set the layout of the tools at the bottom of the widget
        self.horizontalLayout = QtGui.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.horizontalLayout.addWidget(self.clearButton)
        self.horizontalLayout.addWidget(self.multiple_loading_label)
        self.horizontalLayout.addWidget(self.multiple_loading_check)
        # # self.horizontalLayout.addWidget(self.load_behaviour_label)
        self.horizontalLayout.addWidget(self.load_behaviour_combo)
        # self.horizontalLayout.addItem(self.spacer)
        self.horizontalLayout.addWidget(self.manageDirectoriesButton)
        self.horizontalLayout.addItem(self.spacer)

        self.verticalLayout.addWidget(self.load_file_widget)
        self.verticalLayout.addWidget(self.load_run_widget)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.verticalLayout.addStretch(1)

        # self.setLayout(self.verticalLayout)

    def disable_loading(self):
        self.clearButton.setEnabled(False)
        self.multiple_loading_check.setEnabled(False)

    def enable_loading(self):
        self.clearButton.setEnabled(True)
        self.multiple_loading_check.setEnabled(True)

    # def show_directory_manager(self):
    #     MantidQt.API.ManageUserDirectories.openUserDirsDialog(self)
