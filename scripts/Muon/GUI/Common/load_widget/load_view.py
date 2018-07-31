from __future__ import absolute_import

from PyQt4 import QtGui

from mantidqtpython import MantidQt


class LoadView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(LoadView, self).__init__(parent)

        self.browse_button = QtGui.QPushButton("User Dirs", self)
        self.browse_button.clicked.connect(self.show_directory_manager)

        self.co_button = QtGui.QPushButton("Co-Add", self)
        self.load_button = QtGui.QPushButton("Load", self)

        self.spinbox = QtGui.QSpinBox(self)
        self.spinbox.setRange(0, 99999)
        self.spinbox.setValue(0)
        self.last_spinbox_val = 0

        self.grid = QtGui.QVBoxLayout()
        self.grid.addWidget(self.spinbox)
        self.grid.addWidget(self.load_button)
        self.grid.addWidget(self.co_button)
        self.grid.addWidget(self.browse_button)
        self.setLayout(self.grid)

    def show_directory_manager(self):
        MantidQt.API.ManageUserDirectories.openUserDirsDialog(self)

    def on_load_clicked(self, slot):
        self.load_button.clicked.connect(slot)

    def unreg_on_load_clicked(self, slot):
        self.load_button.clicked.disconnect(slot)

    def on_spinbox_changed(self, slot):
        self.spinbox.valueChanged.connect(slot)

    def unreg_on_spinbox_changed(self, slot):
        self.spinbox.valueChanged.disconnect(slot)
