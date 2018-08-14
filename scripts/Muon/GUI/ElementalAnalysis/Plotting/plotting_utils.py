from PyQt4 import QtGui


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
