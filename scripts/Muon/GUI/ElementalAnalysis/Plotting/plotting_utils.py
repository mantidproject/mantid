from PyQt4 import QtGui


def get_layout(num):
    return layout[num - 1]()


def get_layout_four():
    return [[0, 0], [0, 1], [1, 0], [1, 1]]


def get_layout_three():
    return [[0, 0], [1, 0], [2, 0]]


def get_layout_two():
    return [[0, 0], [0, 1]]


def get_layout_one():
    return [[0, 0]]


layout = [get_layout_one, get_layout_two, get_layout_three, get_layout_four]


class AxisChanger(QtGui.QWidget):
    def __init__(self, label):
        super(AxisChanger, self).__init__()
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
