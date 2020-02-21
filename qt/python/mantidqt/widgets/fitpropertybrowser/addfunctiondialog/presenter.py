# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from .view import AddFunctionDialogView
from mantidqt.interfacemanager import InterfaceManager


class AddFunctionDialog(object):
    """
    Dialog to add function to fit property browser
    """

    def __init__(self, parent = None, function_names = None, view=None):
        self.view = view if view else AddFunctionDialogView(parent, function_names)
        self.view.ui.buttonBox.accepted.connect(lambda: self.action_add_function())
        self.view.ui.helpButton.clicked.connect(self.function_help_dialog)

    def action_add_function(self):
        current_function = self.view.ui.functionBox.currentText()
        if self.view.is_text_in_function_list(current_function):
            self.view.function_added.emit(current_function)
            self.view.accept()
        else:
            self.view.set_error_message("Function %s not found " % current_function)

    def function_help_dialog(self):
        InterfaceManager().showFitFunctionHelp(self.view.ui.functionBox.currentText())
