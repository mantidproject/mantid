# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QObject, QEvent, Qt, Signal
from qtpy.QtGui import QIcon, QKeyEvent
from qtpy.QtWidgets import QCompleter, QDialog, QLineEdit

from mantidqt.utils.qt import load_ui


class ActivateCompleterOnReturn(QObject):
    """Event filter to capture key presses from a QLineEdit
    and activate completion popups if enter is pressed
    """
    def __init__(self, lineedit: QLineEdit):
        """
        :param lineedit: The QLineEdit whose key presses should be monitored
        """
        super().__init__()
        self._lineedit = lineedit

    def eventFilter(self, _, event: QEvent) -> bool:
        """Receive events for a QLineEdit and if enter is pressed
        start completion. Always returns False to indicate the
        target object should still process the event.
        """
        if not isinstance(event, QKeyEvent):
            return False

        lineedit = self._lineedit
        completer = lineedit.completer()
        if event.key() == Qt.Key_Return:
            # if completion list is of len = 1, accept the completion else continue
            if completer.completionCount() == 1:
                completion_text = completer.currentCompletion()
                if completion_text != lineedit.currentText():
                    # manually emit the code completion signal
                    completer.activated.emit(completion_text)
        elif event.key() in [Qt.Key_Down, Qt.Key_Up]:
            # If up or down pressed, start the completer
            completer.complete()

        return False


class AddFunctionDialogView(QDialog):
    # public
    function_added = Signal(str)

    # private
    _key_filter = None

    def __init__(self, parent=None, function_names=None):
        super(AddFunctionDialogView, self).__init__(parent)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self._setup_ui(function_names)

    def _setup_ui(self, function_names):
        self.ui = load_ui(__file__, 'add_function_dialog.ui', self)
        functionBox = self.ui.functionBox
        if function_names:
            functionBox.addItems(function_names)
        functionBox.clearEditText()
        functionBox.completer().setCompletionMode(QCompleter.PopupCompletion)
        functionBox.completer().setFilterMode(Qt.MatchContains)
        self._key_filter = ActivateCompleterOnReturn(functionBox)
        functionBox.installEventFilter(self._key_filter)
        self.ui.errorMessage.hide()

    def is_text_in_function_list(self, function):
        return self.ui.functionBox.findText(function, Qt.MatchExactly) != -1

    def set_error_message(self, text):
        self.ui.errorMessage.setText("<span style='color:red'> %s </span>" % text)
        self.ui.errorMessage.show()
