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
        self._label = QtWidgets.QLabel(label)
        layout.addWidget(self._label)

        self.lower_bound = QtWidgets.QLineEdit()
        self.lower_bound.setValidator(QtGui.QDoubleValidator())
        self.lower_bound.returnPressed.connect(self._bound_changed)

        self.upper_bound = QtWidgets.QLineEdit()
        self.upper_bound.setValidator(QtGui.QDoubleValidator())
        self.upper_bound.returnPressed.connect(self._bound_changed)

        layout.addWidget(self.lower_bound)
        self._to = QtWidgets.QLabel("to")
        layout.addWidget(self._to)
        layout.addWidget(self.upper_bound)
        self.setLayout(layout)

    def set_enabled(self, state):
        self.lower_bound.setDisabled(state)
        self.upper_bound.setDisabled(state)

    def show(self):
        self.lower_bound.show()
        self.upper_bound.show()
        self._label.show()
        self._to.show()

    def hide(self):
        self.lower_bound.hide()
        self.upper_bound.hide()
        self._label.hide()
        self._to.hide()

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
