from PyQt4 import QtGui

from functools import partial


class Checkbox(QtGui.QCheckBox):
    def __init__(self, name):
        super(QtGui.QCheckBox, self).__init__(name)
        self.partials = {}


class CheckboxView(QtGui.QWidget):
    def __init__(self, names, title=None, parent=None):
        super(CheckboxView, self).__init__(parent)

        self.checkbox_dict = {}

        self.list = QtGui.QVBoxLayout()
        if title is not None:
            t = QtGui.QLabel(title)
            self.list.addWidget(t)
        for name in names:
            checkbox = Checkbox(name)
            self.list.addWidget(checkbox)
            self.checkbox_dict[name] = checkbox
        self.setLayout(self.list)

    def on_checkbox_changed(self, checkbox, slot):
        p = partial(slot, checkbox)
        # this does, however, mean that a slot can only be connected once (without data loss)
        # this shouldn't be an issue because, e.g. the user can create a
        # function that handles both
        checkbox.partials[slot] = p
        checkbox.stateChanged.connect(p)

    def unreg_on_checkbox_changed(self, checkbox, slot):
        try:
            checkbox.stateChanged.disconnect(checkbox.partials[slot])
        except KeyError:
            return
        del checkbox.partials[slot]
