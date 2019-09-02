# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, unicode_literals)

from qtpy import QtCore, QtWidgets


class Checkbox(QtWidgets.QCheckBox):
    checked = QtCore.Signal(object)
    unchecked = QtCore.Signal(object)

    def __init__(self, name):
        super(QtWidgets.QCheckBox, self).__init__(name)
        self.name = name
        self.stateChanged.connect(self._check_state)

    def _check_state(self, state):
        if state == 2:
            self.checked.emit(self)
        elif state == 1:
            pass  # partially checked; not implemented, future...?
        elif not state:
            self.unchecked.emit(self)

    def on_checkbox_checked(self, slot):
        self.checked.connect(slot)

    def on_checkbox_unchecked(self, slot):
        self.unchecked.connect(slot)

    def unreg_on_checkbox_checked(self, slot):
        try:
            self.checked.disconnect(slot)
        except TypeError:
            return

    def unreg_on_checkbox_unchecked(self, slot):
        try:
            self.unchecked.disconnect(slot)
        except TypeError:
            return
