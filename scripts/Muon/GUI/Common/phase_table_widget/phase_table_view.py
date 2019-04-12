from PyQt4 import QtGui


class PhaseTableView(QtGui.QWidget):
    def __init__(self, parent=None):
        super(PhaseTableView, self).__init__(parent)

    @property
    def first_good_time(self):
        return self._first_good_time

    @first_good_time.setter
    def first_good_time(self, value):
        self._first_good_time = value

    @property
    def last_good_time(self):
        return self._last_good_time

    @last_good_time.setter
    def last_good_time(self, value):
        self._last_good_time = value

    @property
    def input_workspace(self):
        return self._input_workspace

    @input_workspace.setter
    def input_workspace(self, value):
        self._input_workspace = value

    @property
    def forward_group(self):
        return self._forward_group

    @forward_group.setter
    def forward_group(self, value):
        self._forward_group = value

    @property
    def backward_group(self):
        return self._backward_group

    @backward_group.setter
    def backward_group(self, value):
        self._backward_group = value
