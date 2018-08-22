from PyQt4 import QtGui, QtCore


class AxisChangerView(QtGui.QWidget):
    sig_bounds_changed = QtCore.pyqtSignal(object)

    def __init__(self, label):
        super(AxisChangerView, self).__init__()
        layout = QtGui.QHBoxLayout()
        layout.addWidget(QtGui.QLabel(label))

        self.lower_bound = QtGui.QLineEdit()
        self.lower_bound.setValidator(QtGui.QDoubleValidator())
        self.lower_bound.returnPressed.connect(self._bounds_changed)

        self.upper_bound = QtGui.QLineEdit()
        self.upper_bound.setValidator(QtGui.QDoubleValidator())
        self.upper_bound.returnPressed.connect(self._bounds_changed)

        layout.addWidget(self.lower_bound)
        layout.addWidget(QtGui.QLabel("to"))
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

    def _bounds_changed(self):
        self.sig_bounds_changed.emit(self.get_bounds())

    def on_bounds_changed(self, slot):
        self.sig_bounds_changed.connect(slot)

    def unreg_on_bounds_changed(self, slot):
        self.sig_bounds_changed.disconnect(slot)
