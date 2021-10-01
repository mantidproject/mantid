# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator

MAXIMUM_BOUND_PRECISION = 7


class AxisChangerView(QtWidgets.QWidget):
    sig_bound_changed = QtCore.Signal(object)

    def __init__(self, label, parent):
        super(AxisChangerView, self).__init__(parent)
        layout = QtWidgets.QHBoxLayout()
        self._label = QtWidgets.QLabel(label)

        self._min_label = QtWidgets.QLabel(label+" min")
        self.lower_bound = QtWidgets.QLineEdit()
        self.lower_bound.setMaxLength(MAXIMUM_BOUND_PRECISION)
        self.lower_bound_validator = LineEditDoubleValidator(self.lower_bound, 0.0)
        self.lower_bound.setValidator(self.lower_bound_validator)
        self.lower_bound.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Fixed)
        self.lower_bound.returnPressed.connect(self._bound_changed)

        self._max_label =  QtWidgets.QLabel(label+" max")
        self.upper_bound = QtWidgets.QLineEdit()
        self.upper_bound.setMaxLength(MAXIMUM_BOUND_PRECISION)
        self.upper_bound_validator = LineEditDoubleValidator(self.upper_bound, 15.0)
        self.upper_bound.setValidator(self.upper_bound_validator)
        self.upper_bound.setSizePolicy(QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Fixed)
        self.upper_bound.returnPressed.connect(self._bound_changed)

        layout.addWidget(self._min_label)
        layout.addWidget(self.lower_bound)
        layout.addWidget(self._max_label)
        layout.addWidget(self.upper_bound)
        self.setLayout(layout)

    def get_limits(self):
        return [float(self.lower_bound.text()), float(self.upper_bound.text())]

    def set_limits(self, limits):
        self.lower_bound.setText(str(limits[0]))
        self.upper_bound.setText(str(limits[1]))

    def _bound_changed(self):
        self.sig_bound_changed.emit(self.get_limits())

    def on_range_changed(self, slot):
        self.sig_bound_changed.connect(slot)
