# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

from qtpy.QtCore import Qt, Signal
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QCompleter, QDialog

from mantidqt.utils.qt import load_ui


class AddFunctionDialogView(QDialog):
    function_added = Signal(str)

    def __init__(self, parent = None, function_names = None):
        super(AddFunctionDialogView, self).__init__(parent)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self._setup_ui(function_names)

    def _setup_ui(self, function_names):
        self.ui = load_ui(__file__, 'add_function_dialog.ui', self)
        function_box = self.ui.functionBox
        if function_names:
            self.ui.functionBox.addItems(function_names)
        function_box.completer().setCompletionMode(QCompleter.PopupCompletion)
        function_box.completer().setFilterMode(Qt.MatchContains)
        self.ui.errorMessage.hide()

    def is_text_in_function_list(self, function):
        return self.ui.functionBox.findText(function, Qt.MatchExactly) != -1

    def set_error_message(self, text):
        self.ui.errorMessage.setText("<span style='color:red'> %s </span>" % text)
        self.ui.errorMessage.show()
