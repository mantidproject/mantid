from __future__ import absolute_import

from qtpy import QtWidgets


# from mantidqtpython import MantidQt


class LoadView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(LoadView, self).__init__(parent)

        self.browse_button = QtWidgets.QPushButton("User Dirs", self)
        self.browse_button.clicked.connect(self.show_directory_manager)

        self.co_button = QtWidgets.QPushButton("Co-Add", self)
        self.load_button = QtWidgets.QPushButton("Load", self)

        self.spinbox = QtWidgets.QSpinBox(self)
        self.spinbox.setRange(0, 99999)
        self.spinbox.setValue(0)
        self.last_spinbox_val = 0

        self.grid = QtWidgets.QVBoxLayout()
        self.grid.addWidget(self.spinbox)
        self.grid.addWidget(self.load_button)
        self.grid.addWidget(self.co_button)
        self.grid.addWidget(self.browse_button)
        self.setLayout(self.grid)

    def show_directory_manager(self):
        MantidQt.API.ManageUserDirectories.openUserDirsDialog(self)

    def disable_buttons(self):
        self.spinbox.setEnabled(False)
        self.load_button.setEnabled(False)
        self.co_button.setEnabled(False)

    def enable_buttons(self):
        self.spinbox.setEnabled(True)
        self.load_button.setEnabled(True)
        self.co_button.setEnabled(True)

    def on_load_clicked(self, slot):
        self.load_button.clicked.connect(slot)

    def unreg_on_load_clicked(self, slot):
        try:
            self.load_button.clicked.disconnect(slot)
        except TypeError:
            return

    def on_co_add_clicked(self, slot):
        self.co_button.clicked.connect(slot)

    def unreg_on_co_add_clicked(self, slot):
        try:
            self.co_button.clicked.disconnect(slot)
        except TypeError:
            return

    def on_spinbox_changed(self, slot):
        self.spinbox.valueChanged.connect(slot)

    def unreg_on_spinbox_changed(self, slot):
        try:
            self.spinbox.valueChanged.disconnect(slot)
        except TypeError:
            return
