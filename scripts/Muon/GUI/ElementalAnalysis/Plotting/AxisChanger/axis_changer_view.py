from PyQt4 import QtGui


class AxisChangerView(QtGui.QWidget):
    def __init__(self, label):
        super(AxisChangerView, self).__init__()
        layout = QtGui.QHBoxLayout()
        layout.addWidget(QtGui.QLabel(label))

        self.lower_bound = QtGui.QLineEdit()
        self.upper_bound = QtGui.QLineEdit()

        layout.addWidget(self.lower_bound)
        layout.addWidget(QtGui.QLabel("to"))
        layout.addWidget(self.upper_bound)
        self.setLayout(layout)

    def on_lbound_return_pressed(self, slot):
        self.lower_bound.returnPressed.connect(slot)

    def unreg_on_lbound_return_pressed(self, slot):
        self.lower_bound.returnPressed.disconnect(slot)

    def on_ubound_return_pressed(self, slot):
        self.upper_bound.returnPressed.connect(slot)

    def unreg_on_ubound_return_pressed(self, slot):
        self.upper_bound.returnPressed.disconnect(slot)
