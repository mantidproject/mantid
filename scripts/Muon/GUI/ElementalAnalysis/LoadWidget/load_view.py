from __future__ import absolute_import

from PyQt4 import QtGui


class LoadView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(LoadView, self).__init__(parent)

        self.load_button = QtGui.QPushButton("Browse", self)
        self.load_button.clicked.connect(self.button_clicked)

        self.spinbox = QtGui.QSpinBox(self)
        self.spinbox.setRange(0, 1e7)
        self.spinbox.valueChanged.connect(self.spinbox_val_changed)

        self.grid = QtGui.QHBoxLayout()
        self.grid.addWidget(self.spinbox)
        self.grid.addWidget(self.load_button)
        self.setLayout(self.grid)

    def on_button_clicked(self, slot):
        self.load_button.clicked.connect(slot)

    def unreg_on_button_clicked(self, slot):
        self.load_button.clicked.disconnect(slot)

    def on_spinbox_val_changed(self, slot):
        self.spinbox.valueChanged.connect(slot)

    def unreg_on_spinbox_val_changed(self, slot):
        self.spinbox.valueChanged.disconnect(slot)
