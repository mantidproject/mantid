# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore, QtGui
import qtpy

maximum_bound_precision = 7


class AxisChangerView(QtWidgets.QWidget):
    sig_bound_changed = QtCore.Signal(object)
    def __init__(self,label, parent):#, context):
        super(AxisChangerView, self).__init__(parent)
        layout = QtWidgets.QHBoxLayout()
        self._label = QtWidgets.QLabel(label)

        self._min_label =  QtWidgets.QLabel(label+" min")
        self.lower_bound = QtWidgets.QLineEdit()
        self.lower_bound.setMaxLength(maximum_bound_precision)
        self.lower_bound.setValidator(QtGui.QDoubleValidator())
        #self.lower_bound.returnPressed.connect(self._bound_changed)

        self._max_label =  QtWidgets.QLabel(label+" max")
        self.upper_bound = QtWidgets.QLineEdit()
        self.upper_bound.setMaxLength(maximum_bound_precision)
        self.upper_bound.setValidator(QtGui.QDoubleValidator())
        #self.upper_bound.returnPressed.connect(self._bound_changed)

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

    def on_range_changed(self,slot):
        self.sig_bound_changed.emit(self.get_limits())
