# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import types

from qtpy.QtCore import Qt, Signal
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import QCompleter, QDialog

from mantidqt.utils.qt import load_ui


class AddFunctionDialogView(QDialog):
    function_added = Signal(str)

    def __init__(self, parent=None, function_names=None):
        super(AddFunctionDialogView, self).__init__(parent)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self._setup_ui(function_names)

    def _setup_ui(self, function_names):
        self.ui = load_ui(__file__, 'add_function_dialog.ui', self)
        if function_names:
            self.ui.functionBox.addItems(function_names)
        self.ui.functionBox.clearEditText()
        self.ui.functionBox.completer().setCompletionMode(QCompleter.PopupCompletion)
        self.ui.functionBox.completer().setFilterMode(Qt.MatchContains)
        self.ui.errorMessage.hide()
        # Monkey patch the functionBox keyPressEvent to make use of auto-completion.
        self.ui.functionBox.keyPressEvent = types.MethodType(keyPressEvent,
                                                             self.ui.functionBox)

    def is_text_in_function_list(self, function):
        return self.ui.functionBox.findText(function, Qt.MatchExactly) != -1

    def set_error_message(self, text):
        self.ui.errorMessage.setText("<span style='color:red'> %s </span>" % text)
        self.ui.errorMessage.show()


# This method monkey patches the keyPressEvent for the QComboBox function box
# Using this patch, the text will be auto-completed if enter is pressed
# and the number of possible completions is one.
def keyPressEvent(self, event):
    if event.key() == Qt.Key_Return:
        # if completion list is of len = 1, accept the completion else continue
        if self.completer().completionCount() == 1:
            completion_text = self.completer().currentCompletion()
            if completion_text != self.currentText():  # manually emit the code completion signal
                self.completer().activated.emit(completion_text)
    # If up or down pressed, start the completer
    elif event.key() in [Qt.Key_Down, Qt.Key_Up]:
        self.completer().complete()

    self.lineEdit().keyPressEvent(event)
