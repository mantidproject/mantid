# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import (absolute_import, division, print_function)

from unittest import TestCase

from mantid.py3compat.mock import Mock, patch

from mantidqt.widgets.embedded_find_replace_dialog.presenter import EmbeddedFindReplaceDialog, SearchDirection
from mantidqt.utils.testing.mocks.mock_codeeditor import MockCodeEditor
from mantidqt.widgets.embedded_find_replace_dialog.view import EmbeddedFindReplaceDialogView, FindReplaceOptions


def with_mock_presenter(func):
    def wrapper(self, *args):
        view = Mock(spec=EmbeddedFindReplaceDialogView)

        editor = MockCodeEditor()
        presenter = EmbeddedFindReplaceDialog(parent=None, editor=editor, view=view)
        return func(self, view, presenter, editor, *args)

    return wrapper


def set_find_text(search_string, replace_string=None):
    """
    This decorator should be used after @with_mock_presenter.
    It expects to receive the function already decorated with view, presenter, editor
    and sets the return value on one of the view's mocks.
    """

    def this_receives_the_function(func):
        def wrapper(self, view, presenter, editor):
            view.find.currentText = Mock(return_value=search_string)
            if replace_string is not None:
                view.replace = Mock()
                view.replace.currentText = Mock(return_value=replace_string)

            return func(self, view, presenter, editor)

        return wrapper

    return this_receives_the_function


def set_editor_text_selection(selected_text):
    """
    This decorator should be used after @with_mock_presenter.
    It expects to receive the function already decorated with view, presenter, editor
    and sets the return value on one of the view's mocks.
    """

    def this_receives_the_function(func):
        def wrapper(self, view, presenter, editor):
            editor.hasSelectedText.return_value = True
            editor.selectedText.return_value = selected_text
            return func(self, view, presenter, editor)

        return wrapper

    return this_receives_the_function


class EmbeddedFindReplaceDialogTest(TestCase):
    default_search_string = "doorway"

    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)

    @with_mock_presenter
    def test_clear_search(self, view, presenter, editor):
        presenter.find_in_progress = True

        presenter.clear_search()
        self.assertFalse(presenter.find_in_progress)

    @with_mock_presenter
    def test_show_without_text(self, view, presenter, editor):
        presenter.show()

        view.show.assert_called_once_with()
        view.find.setFocus.assert_called_once_with()
        self.assertTrue(presenter.visible)

    @with_mock_presenter
    @set_editor_text_selection("apples")
    def test_show_with_selected_text(self, view, presenter, editor):
        presenter.show()

        editor.hasSelectedText.assert_called_once_with()
        editor.selectedText.assert_called_once_with()

        view.show.assert_called_once_with()
        view.find.setFocus.assert_called_once_with()
        view.find.setCurrentText.assert_called_once_with("apples")
        self.assertTrue(presenter.visible)

    @with_mock_presenter
    def test_hide(self, view, presenter, editor):
        presenter.hide()

        view.hide.assert_called_once_with()
        editor.setFocus.assert_called_once_with()
        self.assertFalse(presenter.visible)

    @with_mock_presenter
    def test_focus_find_field(self, view, presenter, editor):
        presenter.focus_find_field()
        view.find.setFocus.assert_called_once_with()

    @with_mock_presenter
    @set_find_text(default_search_string)
    def test_action_next(self, view, presenter, editor):
        view.get_options.return_value = FindReplaceOptions(True, True, True, True)
        presenter.action_next()
        self.assertNotCalled(view.find.setFocus)

        view.get_options.assert_called_once_with()
        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, True)

        presenter.action_next()
        editor.findNext.assert_called_once_with()

        # check that find next is called correctly each time
        for i in range(2, 10):
            presenter.action_next()
            self.assertEqual(i, editor.findNext.call_count)

    @with_mock_presenter
    @set_find_text(default_search_string)
    def test_action_next_reset_search(self, view, presenter, editor):
        view.get_options.return_value = FindReplaceOptions(True, True, True, True)
        presenter.action_next()
        self.assertNotCalled(view.find.setFocus)

        view.get_options.assert_called_once_with()
        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, True)
        self.assertNotCalled(editor.findNext)
        self.assertEqual(presenter.current_direction, SearchDirection.FORWARD)

        presenter.action_next()
        editor.findNext.assert_called_once_with()

        presenter.action_next()
        self.assertEqual(2, editor.findNext.call_count)

        presenter.clear_search()
        view.find.currentText = Mock(return_value="wowowow")

        editor.findFirst.reset_mock()
        editor.findNext.reset_mock()

        presenter.action_next()
        self.assertEqual(presenter.current_direction, SearchDirection.FORWARD)
        editor.findFirst.assert_called_once_with("wowowow", True, True, True, True, True)
        self.assertNotCalled(editor.findNext)

        for i in range(1, 10):
            presenter.action_next()
            self.assertEqual(i, editor.findNext.call_count)

    @with_mock_presenter
    @set_find_text("")
    def test_action_next_no_search_string(self, view, presenter, editor):
        presenter.action_next()
        view.find.setFocus.assert_called_once_with()

        self.assertNotCalled(view.find.findItem)
        self.assertEqual(presenter.current_direction, SearchDirection.FORWARD)

    @with_mock_presenter
    @set_find_text(default_search_string)
    def test_action_previous(self, view, presenter, editor):
        view.get_options.return_value = FindReplaceOptions(True, True, True, True)
        presenter.action_previous()
        view.get_options.assert_called_once_with()
        self.assertEqual(presenter.current_direction, SearchDirection.BACKWARD)
        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, False)
        editor.findNext.assert_called_once_with()

    @with_mock_presenter
    @set_find_text(default_search_string)
    def test_action_replace_no_selected_text_for_replacement(self, view, presenter, editor):
        view.get_options.return_value = FindReplaceOptions(True, True, True, True)
        editor.hasSelectedText = Mock(return_value=False)

        presenter.action_replace()

        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, True)

        # if text is selected, but something else is selected in the editor
        editor.hasSelectedText = Mock(return_value=True)
        editor.selectedText = Mock(return_value="not_a_doorway")

        presenter.action_replace()

        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, True)

    @with_mock_presenter
    @set_find_text(default_search_string, replace_string="apples")
    def test_action_replace(self, view, presenter, editor):
        editor.hasSelectedText = Mock(return_value=True)
        editor.selectedText = Mock(return_value=self.default_search_string)

        presenter.action_replace()

        editor.replace.assert_called_once_with("apples")

    @with_mock_presenter
    @set_find_text(default_search_string, replace_string="")
    def test_action_replace_empty_replace_string(self, view, presenter, editor):
        editor.hasSelectedText = Mock(return_value=True)
        editor.selectedText = Mock(return_value=self.default_search_string)

        presenter.action_replace()

        editor.replace.assert_called_once_with("")

    @with_mock_presenter
    @set_find_text("")
    def test_action_replace_empty_search(self, view, presenter, editor):
        presenter.action_replace()
        view.find.setFocus.assert_called_once_with()

        self.assertNotCalled(view.find.findItem)

    @with_mock_presenter
    @set_find_text("")
    def test_action_replace_all_empty_search(self, view, presenter, editor):
        presenter.action_replace_all()
        view.find.setFocus.assert_called_once_with()

        self.assertNotCalled(view.find.findItem)

    @with_mock_presenter
    @set_find_text(default_search_string, replace_string="tomato")
    def test_action_replace_all(self, view, presenter, editor):
        view.get_options.return_value = FindReplaceOptions(True, True, True, True)

        presenter.action_replace_all()
        editor.replaceAll.assert_called_once_with(self.default_search_string, "tomato", True, True, True, True, True)

    @with_mock_presenter
    def test_strings_different(self, view, presenter, editor):
        str1 = "apples"
        str2 = "potatoes"
        self.assertTrue(presenter.strings_different(str1, str2, case_sensitive=True))

        str2 = "APPles"
        self.assertTrue(presenter.strings_different(str1, str2, case_sensitive=True))

        str2 = "apples"
        self.assertFalse(presenter.strings_different(str1, str2, case_sensitive=True))

    @with_mock_presenter
    def test_strings_different_case_insensitive(self, view, presenter, editor):
        str1 = "APPLES"
        str2 = "potatoes"
        self.assertTrue(presenter.strings_different(str1, str2, case_sensitive=False))

        str2 = "apples"
        self.assertFalse(presenter.strings_different(str1, str2, case_sensitive=False))

    @with_mock_presenter
    @set_find_text(default_search_string)
    def test_next_then_previous_changes_direction(self, view, presenter, editor):
        view.get_options.return_value = FindReplaceOptions(True, True, True, True)

        presenter.action_next()
        self.assertNotCalled(view.find.setFocus)
        self.assertEqual(presenter.current_direction, SearchDirection.FORWARD)

        view.get_options.assert_called_once_with()
        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, True)

        presenter.action_next()
        self.assertEqual(presenter.current_direction, SearchDirection.FORWARD)
        editor.findNext.assert_called_once_with()

        editor.findFirst.reset_mock()
        editor.findNext.reset_mock()
        view.get_options.reset_mock()

        presenter.action_previous()
        view.get_options.assert_called_once_with()
        self.assertEqual(presenter.current_direction, SearchDirection.BACKWARD)
        editor.findFirst.assert_called_once_with(self.default_search_string, True, True, True, True, False)
        editor.findNext.assert_called_once_with()
        presenter.action_previous()
        self.assertEqual(2, editor.findNext.call_count)

    @with_mock_presenter
    @patch('mantidqt.widgets.embedded_find_replace_dialog.presenter.EmbeddedFindReplaceDialog.action_next')
    @patch('mantidqt.widgets.embedded_find_replace_dialog.presenter.EmbeddedFindReplaceDialog.action_previous')
    def test_action_enter_pressed(self, view, presenter, editor, mock_action_previous, mock_action_next):
        presenter.current_direction = SearchDirection.BACKWARD

        presenter.action_enter_pressed()
        mock_action_previous.assert_called_once_with()
        self.assertNotCalled(mock_action_next)

        mock_action_previous.reset_mock()
        presenter.current_direction = SearchDirection.FORWARD
        presenter.action_enter_pressed()
        mock_action_next.assert_called_once_with()
        self.assertNotCalled(mock_action_previous)
