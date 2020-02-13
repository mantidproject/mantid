# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt, Signal
from qtpy.QtWidgets import QDialog

from .view import AddFunctionDialogView


class AddFunctionDialog(QDialog):
    """
    Dialog to add function to fit property browser
    """
    function_added = Signal(str)

    def __init__(self, parent = None, function_names = None, view=None):
        super(AddFunctionDialog, self).__init__(parent)
        self.view = view if view else AddFunctionDialogView(parent, function_names)
        self.view.ui.buttonBox.accepted.connect(self.action_add_function)

    def action_add_function(self):
        current_function = self.view.ui.functionBox.currentText()
        if self.view.ui.functionBox.findText(current_function, Qt.MatchExactly) != -1:
            self.function_added.emit(current_function)
            self.view.accept()
        else:
            self.view.set_error_message("Function %s not found " % current_function)
