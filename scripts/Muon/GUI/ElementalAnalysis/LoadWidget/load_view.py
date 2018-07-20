from __future__ import absolute_import

from PyQt4 import QtGui, QtCore


class LoadView(QtGui.QWidget):
    sig_spinbox_submit = QtCore.pyqtSignal(int)

    def __init__(self, parent=None):
        super(LoadView, self).__init__(parent)

        self.load_button = QtGui.QPushButton("Browse", self)

        self.spinbox = QtGui.QSpinBox(self)
        self.spinbox.setRange(0, 1e7)
        self.last_spinbox_val = 0

        self.on_spinbox_val_changed(self.update_last_spinbox_val)

        self.grid = QtGui.QHBoxLayout()
        self.grid.addWidget(self.spinbox)
        self.grid.addWidget(self.load_button)
        self.setLayout(self.grid)

    def update_last_spinbox_val(self, val):
        if self.last_spinbox_val == val:
            self.sig_spinbox_submit.emit(val)
        self.last_spinbox_val = val

    def on_button_clicked(self, slot):
        self.load_button.clicked.connect(slot)

    def unreg_on_button_clicked(self, slot):
        self.load_button.clicked.disconnect(slot)

    def on_spinbox_val_changed(self, slot):
        self.spinbox.valueChanged.connect(slot)

    def unreg_on_spinbox_val_changed(self, slot):
        self.spinbox.valueChanged.disconnect(slot)

    def on_spinbox_submit(self, slot):
        self.sig_spinbox_submit.connect(slot)

    def unreg_on_spinbox_submit(self, slot):
        self.sig_spinbox_submit.disconnect(slot)
