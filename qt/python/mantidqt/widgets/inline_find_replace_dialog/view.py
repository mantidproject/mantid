# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mantidqt.utils.qt import load_ui

DialogBase, DialogForm = load_ui(__file__, 'dialog.ui')


class FindReplaceOptions(object):
    def __init__(self, match_case, search_backwards, words, regex, wrap_around):
        self.match_case = match_case
        self.search_backwards = search_backwards
        self.words = words
        self.regex = regex
        self.wrap_around = wrap_around


class InlineFindReplaceDialogView(DialogBase, DialogForm):
    def __init__(self, parent, presenter):
        super(InlineFindReplaceDialogView, self).__init__(parent)
        self.setupUi(self)
        self.resize(self.width(), 100)

        self.presenter = presenter

        self.find.lineEdit().setPlaceholderText("Find")
        self.replace.lineEdit().setPlaceholderText("Replace with")

        self.button_next.clicked.connect(self.presenter.action_next)
        self.button_replace.clicked.connect(self.presenter.action_replace)
        self.button_replace_all.clicked.connect(self.presenter.action_replace_all)

        self.find.currentTextChanged.connect(self.presenter.clear_search)
        self.match_case.stateChanged.connect(self.presenter.clear_search)
        self.search_backwards.stateChanged.connect(self.presenter.clear_search)
        self.words.stateChanged.connect(self.presenter.clear_search)
        self.regex.stateChanged.connect(self.presenter.clear_search)
        self.wrap_around.stateChanged.connect(self.presenter.clear_search)

    def get_options(self):
        return FindReplaceOptions(match_case=self.match_case.isChecked(),
                                  search_backwards=self.search_backwards.isChecked(),
                                  words=self.words.isChecked(),
                                  regex=self.regex.isChecked(),
                                  wrap_around=self.wrap_around.isChecked())
