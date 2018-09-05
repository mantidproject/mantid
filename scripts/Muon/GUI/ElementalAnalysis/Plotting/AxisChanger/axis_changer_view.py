from qtpy import QtGui, QtCore, QtWidgets


class AxisChangerView(QtWidgets.QWidget):
    sig_lower_bound_changed = QtCore.Signal(object)
    sig_upper_bound_changed = QtCore.Signal(object)

    def __init__(self, label):
        super(AxisChangerView, self).__init__()
        layout = QtWidgets.QHBoxLayout()
        layout.addWidget(QtWidgets.QLabel(label))

        self.lower_bound = QtWidgets.QLineEdit()
        self.lower_bound.setValidator(QtGui.QDoubleValidator())
        self.lower_bound.returnPressed.connect(self._lower_bound_changed)

        self.upper_bound = QtWidgets.QLineEdit()
        self.upper_bound.setValidator(QtGui.QDoubleValidator())
        self.upper_bound.returnPressed.connect(self._upper_bound_changed)

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

    def _lower_bound_changed(self):
        self.sig_lower_bound_changed.emit(self.get_bounds()[0])

    def _upper_bound_changed(self):
        self.sig_upper_bound_changed.emit(self.get_bounds()[1])

    def on_lower_bound_changed(self, slot):
        self.sig_lower_bound_changed.connect(slot)

    def on_upper_bound_changed(self, slot):
        self.sig_upper_bound_changed.connect(slot)

    def unreg_on_lower_bound_changed(self, slot):
        self.sig_lower_bound_changed.disconnect(slot)

    def unreg_on_upper_bound_changed(self, slot):
        self.sig_upper_bound_changed.disconnect(slot)
