# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import (absolute_import, division, print_function)

from mantid.py3compat import Enum
from mantidqt.widgets.embedded_find_replace_dialog.view import EmbeddedFindReplaceDialogView


class SearchDirection(Enum):
    FORWARD = 0
    BACKWARD = 1


class EmbeddedFindReplaceDialog(object):
    def __init__(self, parent, editor, view=None):
        self.view = view if view else EmbeddedFindReplaceDialogView(parent, self)
        self.editor = editor

        self.visible = False
        self.find_in_progress = False
        self.current_direction = SearchDirection.FORWARD

    def close(self):
        self.view.close()

    def set_text_in_find(self, text):
        self.view.find.setCurrentText(text)

    def clear_search(self, _=None):
        self.find_in_progress = False

    def show(self):
        if self.editor.hasSelectedText():
            self.set_text_in_find(self.editor.selectedText())
        self.view.show()
        self.visible = True
        self.focus_find_field()

    def hide(self):
        self.view.hide()
        self.visible = False
        self.editor.setFocus()

    def focus_find_field(self):
        self.view.find.setFocus()

    def action_close_button(self):
        self.hide()

    def action_enter_pressed(self):
        if self.current_direction == SearchDirection.FORWARD:
            self.action_next()
        else:
            self.action_previous()

    def action_next(self):
        if self.current_direction == SearchDirection.FORWARD:
            self._do_search(True)
        else:
            # if not going in the correct direction,
            # change the current direction and reset the search
            self.current_direction = SearchDirection.FORWARD
            self.find_in_progress = False
            # then run the search as normal in the correct direction
            self._do_search(True)

    def action_previous(self):
        if self.current_direction == SearchDirection.BACKWARD:
            self._do_search(False)
        else:
            # if not going in the correct direction,
            # change the current direction and reset the search
            self.current_direction = SearchDirection.BACKWARD
            self.find_in_progress = False
            # then run the search as normal in the correct direction
            self._do_search(False)
            # when switching to backwards we search an additional time,
            # otherwise the selection does not change from going forwards
            # as they both have matched the same text
            self.find_in_progress = self.editor.findNext()

    def _do_search(self, forwards):
        search_string = self.view.find.currentText()

        if search_string == "":
            self.focus_find_field()
            return

        self.add_to_field_history(self.view.find, search_string)

        if not self.find_in_progress:
            options = self.view.get_options()
            self.find_in_progress = self.editor.findFirst(search_string,
                                                          options.regex,
                                                          options.match_case,
                                                          options.words,
                                                          options.wrap_around,
                                                          forwards)
        else:
            self.find_in_progress = self.editor.findNext()

    def action_replace(self):
        search_string = self.view.find.currentText()

        if search_string == "":
            self.focus_find_field()
            return

        options = self.view.get_options()

        # if the editor hasn't been searched before, run a find first, but do not replace anything
        # this allows the user to see where the first match is, instead of having _one_ of the matches
        # being replaced to the replacement string, without knowing which one will be done

        if not self.editor.hasSelectedText() or self.strings_different(self.editor.selectedText(), search_string,
                                                                       options.match_case):
            self.action_next()
            return

        replace_string = self.view.replace.currentText()
        self.editor.replace(replace_string)
        self.action_next()

        self.add_to_field_history(self.view.replace, replace_string)

    def strings_different(self, string1, string2, case_sensitive):
        """

        :param case_sensitive: If case_sensitive is NOT selected, then both strings will be made lowercase
                               Else the strings will be compared as they are without changes
        """
        return (not case_sensitive and string1.lower() != string2.lower()) or (case_sensitive and string1 != string2)

    def action_replace_all(self):
        search_string = self.view.find.currentText()

        if search_string == "":
            self.focus_find_field()
            return

        self.add_to_field_history(self.view.find, search_string)

        replaceString = self.view.replace.currentText()

        self.add_to_field_history(self.view.replace, replaceString)

        options = self.view.get_options()
        self.editor.replaceAll(search_string, replaceString,
                               options.regex,
                               options.match_case,
                               options.words,
                               options.wrap_around,
                               True)

    def add_to_field_history(self, field, search_string):
        if field.findText(search_string) == -1:
            field.addItem(search_string)
