# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QObject, QEvent, Qt, Signal
from qtpy.QtGui import QIcon, QKeyEvent
from qtpy.QtWidgets import QCompleter, QDialog, QLineEdit, QCheckBox

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
    function_added = Signal(str, bool)

    # private
    _key_filter = None

    def __init__(self, parent=None, function_names=None, default_function_name=None, default_checkbox=False):
        """
        :param parent: An optional parent widget
        :param function_names: A list of function names to add to the box
        :param default_function_name: An optional default name to display as a placeholder
        :param default_checkbox: Whether to add a checkbox for setting the function as the global application default
        """
        super(AddFunctionDialogView, self).__init__(parent)
        self.setWindowIcon(QIcon(':/images/MantidIcon.ico'))
        self.setAttribute(Qt.WA_DeleteOnClose, True)
        self._setup_ui(function_names, default_function_name, default_checkbox)

    def is_text_in_function_list(self, function: str) -> bool:
        """Return True if the given str is in the function list"""
        return self.ui.functionBox.findText(function, Qt.MatchExactly) != -1

    def set_error_message(self, text: str):
        """Show the message as an error on the dialog"""
        self.ui.errorMessage.setText("<span style='color:red'> %s </span>" % text)
        self.ui.errorMessage.show()

    def is_set_global_default(self):
        if hasattr(self, '_default_checkbox'):
            return self._default_checkbox.checkState() == Qt.Checked
        return False

    # private api
    def _setup_ui(self, function_names, default_function_name, default_checkbox):
        self.ui = load_ui(__file__, 'add_function_dialog.ui', self)
        functionBox = self.ui.functionBox
        if function_names:
            functionBox.addItems(function_names)
        if default_function_name is not None and default_function_name in function_names:
            functionBox.lineEdit().setPlaceholderText(default_function_name)
        functionBox.clearEditText()
        functionBox.completer().setCompletionMode(QCompleter.PopupCompletion)
        functionBox.completer().setFilterMode(Qt.MatchContains)
        self._key_filter = ActivateCompleterOnReturn(functionBox)
        functionBox.installEventFilter(self._key_filter)
        self.ui.errorMessage.hide()
        # Add a checkbox so user is able to explicitly update the global settings.
        if default_checkbox:
            self._default_checkbox = QCheckBox("Set as global default")
            self.ui.verticalLayout_2.addWidget(self._default_checkbox)
