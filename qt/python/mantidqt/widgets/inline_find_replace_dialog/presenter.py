# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from mantidqt.widgets.inline_find_replace_dialog.view import InlineFindReplaceDialogView


class InlineFindReplaceDialog(object):
    def __init__(self, parent, editor, view=None):
        self.view = view if view else InlineFindReplaceDialogView(parent, self)
        self.editor = editor
        self.visible = False
        self.find_in_progress = False

    def clear_search(self, _):
        self.find_in_progress = False

    def show(self):
        self.view.show()
        self.visible = True
        self.focus_find_field()

    def hide(self):
        self.view.hide()
        self.visible = False
        self.editor.setFocus()

    def toggle(self):
        if not self.visible:
            self.show()
        else:
            self.hide()

    def focus_find_field(self):
        self.view.find.setFocus()

    def action_next(self):
        searchString = self.view.find.currentText()

        if searchString == "":
            self.focus_find_field()

        self.add_to_field_history(self.view.find, searchString)

        if not self.find_in_progress:
            options = self.view.get_options()
            self.find_in_progress = self.editor.findFirst(searchString,
                                                          options.regex,
                                                          options.match_case,
                                                          options.words,
                                                          options.wrap_around,
                                                          not options.search_backwards)
        else:
            self.find_in_progress = self.editor.findNext()

    def action_replace(self):
        searchString = self.view.find.currentText()

        if searchString == "":
            self.focus_find_field()

        # if the editor hasn't been searched before, run a find first, but do not replace anything
        # this allows the user to see where the first match is, instead of having _one_ of the matches
        # being replaced to the replacement string, without knowing which one will be done
        if not self.editor.hasSelectedText() or self.editor.selectedText() != searchString:
            self.action_next()
            return

        replaceString = self.view.replace.currentText()
        self.editor.replace(replaceString)
        self.action_next()

        self.add_to_field_history(self.view.replace, replaceString)

    def action_replace_all(self):
        searchString = self.view.find.currentText()

        if searchString == "":
            self.focus_find_field()

        self.add_to_field_history(self.view.find, searchString)

        replaceString = self.view.replace.currentText()

        self.add_to_field_history(self.view.replace, replaceString)

        options = self.view.get_options()
        self.editor.replaceAll(searchString, replaceString,
                               options.regex,
                               options.match_case,
                               options.words,
                               options.wrap_around,
                               not options.search_backwards)

    def add_to_field_history(self, field, search):
        if field.findText(search) == -1:
            field.addItem(search)
