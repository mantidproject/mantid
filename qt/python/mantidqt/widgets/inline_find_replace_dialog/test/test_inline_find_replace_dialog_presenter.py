# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from unittest import TestCase

from mock import Mock

from mantidqt.widgets.inline_find_replace_dialog.presenter import InlineFindReplaceDialog
from mantidqt.widgets.inline_find_replace_dialog.test.mock_codeeditor import MockCodeEditor
from mantidqt.widgets.inline_find_replace_dialog.view import InlineFindReplaceDialogView


def with_mock_presenter(func):
    def wrapper(self, *args):
        view = Mock(spec=InlineFindReplaceDialogView)
        editor = MockCodeEditor()
        presenter = InlineFindReplaceDialog(parent=None, editor=editor, view=view)
        return func(self, view, presenter, editor, *args)

    return wrapper


class InlineFindReplaceDialogTest(TestCase):
    @with_mock_presenter
    def test_clear_search(self, view, presenter, editor):
        presenter.find_in_progress = True

        presenter.clear_search(None)
        self.assertFalse(presenter.find_in_progress)

    @with_mock_presenter
    def test_show(self, view, presenter, editor):
        presenter.show()

        view.show.assert_called_once_with()
        view.find.setFocus.assert_called_once_with()
        self.assertTrue(presenter.visible)

    @with_mock_presenter
    def test_hide(self, view, presenter, editor):
        presenter.hide()

        view.hide.assert_called_once_with()
        editor.setFocus.assert_called_once_with()
        self.assertFalse(presenter.visible)

    @with_mock_presenter
    def test_toggle(self, view, presenter, editor):
        presenter.toggle()

        self.test_show(view, presenter, editor)

        presenter.toggle()

        self.test_hide()(view, presenter, editor)

    @with_mock_presenter
    def test_focus_find_field(self, view, presenter, editor):
        self.skipTest("Nein")

    @with_mock_presenter
    def test_action_next(self, view, presenter, editor):
        self.skipTest("Nein")

    @with_mock_presenter
    def test_action_replace(self, view, presenter, editor):
        self.skipTest("Nein")

    @with_mock_presenter
    def test_action_replace_all(self, view, presenter, editor):
        self.skipTest("Nein")

    @with_mock_presenter
    def test_add_to_field_history(self, view, presenter, editor):
        self.skipTest("Nein")
