# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt, Signal
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QDialog, QCompleter

from mantid import logger
from mantidqt.utils.qt import load_ui


class AddFunctionDialog(QDialog):
    """
    Dialog to add function to fit property browser
    """
    function_added = Signal(str)

    def __init__(self, parent = None, function_names = None):
        super(AddFunctionDialog, self).__init__(parent)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self._setup_ui(function_names)
        self.accepted.connect(self.action_add_function)

    def _setup_ui(self, function_names):
        self.ui = load_ui(__file__, 'add_function_dialog.ui', self)
        function_box = self.ui.functionBox
        if function_names:
            self.ui.functionBox.addItems(function_names)
        function_box.completer().setCompletionMode(QCompleter.PopupCompletion)
        function_box.completer().setFilterMode(Qt.MatchContains)

    def action_add_function(self):
        current_function = self.ui.functionBox.currentText()
        if self.ui.functionBox.findText(current_function, Qt.MatchExactly) != -1:
            self.function_added.emit(current_function)
        else:
            logger.warning("Function %s not found" % current_function)
