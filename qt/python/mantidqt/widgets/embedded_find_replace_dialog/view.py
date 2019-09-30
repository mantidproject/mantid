# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from qtpy.QtCore import Qt

from mantidqt.icons import get_icon
from mantidqt.utils.qt import create_action, load_ui

DialogBase, DialogForm = load_ui(__file__, 'dialog.ui')


class FindReplaceOptions(object):
    def __init__(self, match_case, words, regex, wrap_around):
        self.match_case = match_case
        self.words = words
        self.regex = regex
        self.wrap_around = wrap_around


class EmbeddedFindReplaceDialogView(DialogBase, DialogForm):
    def __init__(self, parent, presenter):
        super(EmbeddedFindReplaceDialogView, self).__init__(parent)
        self.setupUi(self)
        self.resize(self.width(), 100)

        self.find.completer().setCaseSensitivity(Qt.CaseSensitive)
        self.replace.completer().setCaseSensitivity(Qt.CaseSensitive)

        self.presenter = presenter

        self.hide_find_replace = create_action(self, '',
                                               on_triggered=self.presenter.hide,
                                               shortcut=Qt.Key_Escape)
        self.addAction(self.hide_find_replace)

        self.enter_to_search = create_action(self, '',
                                             on_triggered=self.presenter.action_next,
                                             shortcut=[Qt.Key_Return, Qt.Key_Enter])
        self.addAction(self.enter_to_search)

        self.shift_enter_to_search_backwards = create_action(self, '',
                                                             on_triggered=self.presenter.action_previous,
                                                             shortcut=["Shift+Enter", "Shift+Return"])
        self.addAction(self.shift_enter_to_search_backwards)

        self.find.lineEdit().setPlaceholderText("Find")
        self.replace.lineEdit().setPlaceholderText("Replace with")

        self.setAttribute(Qt.WA_DeleteOnClose, True)

        arrow_up_icon = get_icon("mdi.arrow-up")
        arrow_down_icon = get_icon("mdi.arrow-down")
        x_icon = get_icon("mdi.close")

        self.next_button.setIcon(arrow_down_icon)
        self.previous_button.setIcon(arrow_up_icon)
        self.close_button.setIcon(x_icon)

        self.next_button.clicked.connect(self.presenter.action_next)
        self.previous_button.clicked.connect(self.presenter.action_previous)

        self.close_button.clicked.connect(self.presenter.action_close_button)

        self.replace_button.clicked.connect(self.presenter.action_replace)
        self.replace_all_button.clicked.connect(self.presenter.action_replace_all)

        self.find.currentTextChanged.connect(self.presenter.clear_search)
        self.match_case.stateChanged.connect(self.presenter.clear_search)
        self.words.stateChanged.connect(self.presenter.clear_search)
        self.regex.stateChanged.connect(self.presenter.clear_search)
        self.wrap_around.stateChanged.connect(self.presenter.clear_search)

    def closeEvent(self, event):
        self.deleteLater()
        super(EmbeddedFindReplaceDialogView, self).closeEvent(event)

    def get_options(self):
        return FindReplaceOptions(match_case=self.match_case.isChecked(),
                                  words=self.words.isChecked(),
                                  regex=self.regex.isChecked(),
                                  wrap_around=self.wrap_around.isChecked())
