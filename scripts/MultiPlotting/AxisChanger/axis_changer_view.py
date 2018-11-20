# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtGui, QtCore, QtWidgets


class AxisChangerView(QtWidgets.QWidget):
    sig_bound_changed = QtCore.Signal(object)

    def __init__(self, label):
        super(AxisChangerView, self).__init__()
        layout = QtWidgets.QHBoxLayout()
        layout.addWidget(QtWidgets.QLabel(label))

        self.lower_bound = QtWidgets.QLineEdit()
        self.lower_bound.setValidator(QtGui.QDoubleValidator())
        self.lower_bound.returnPressed.connect(self._bound_changed)

        self.upper_bound = QtWidgets.QLineEdit()
        self.upper_bound.setValidator(QtGui.QDoubleValidator())
        self.upper_bound.returnPressed.connect(self._bound_changed)

        layout.addWidget(self.lower_bound)
        layout.addWidget(QtWidgets.QLabel("to"))
        layout.addWidget(self.upper_bound)
        self.setLayout(layout)

    def get_bounds(self):
        bounds = [self.lower_bound, self.upper_bound]
        return [float(str(bound.text())) if bound.text()
                else 0 for bound in bounds]

    def set_bounds(self, bounds):
        lower, upper = [str(bound) for bound in bounds]
        self.lower_bound.setText(lower)
        self.upper_bound.setText(upper)

    def clear_bounds(self):
        self.lower_bound.clear()
        self.upper_bound.clear()

    def _bound_changed(self):
        self.sig_bound_changed.emit(self.get_bounds())

    def on_bound_changed(self, slot):
        self.sig_bound_changed.connect(slot)

    def unreg_bound_changed(self, slot):
        self.sig_bound_changed.disconnect(slot)

 