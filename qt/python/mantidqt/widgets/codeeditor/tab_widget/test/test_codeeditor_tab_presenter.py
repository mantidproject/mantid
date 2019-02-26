# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

import os
from unittest import TestCase

from mantid.py3compat.mock import patch
from mantidqt.utils.testing.mocks.mock_qt import MockQWidget
from mantidqt.utils.testing.strict_mock import StrictPropertyMock
from mantidqt.widgets.codeeditor.tab_widget.codeeditor_tab_presenter import CodeEditorTabPresenter
from mantidqt.utils.testing.mocks.mock_codeeditor import MockCodeEditor


class MockCodeEditorTab(MockQWidget):
    """
    Represents a single tab within the TabView.
    Contains an editor that would display the code
    """

    def __init__(self):
        super(MockCodeEditorTab, self).__init__()
        self.editor = MockCodeEditor()


class MockCodeEditorTabView(MockQWidget):
    """
    Represents the QTabView used to contain all tabs
    """

    def __init__(self):
        super(MockCodeEditorTabView, self).__init__()
        self.last_tab_clicked = StrictPropertyMock()
        self.mock_code_editor_tab = MockCodeEditorTab()

        self.widget.return_value = self.mock_code_editor_tab


class CodeEditorTabPresenterTest(TestCase):
    SHOW_IN_EXPLORER_PACKAGE = "mantidqt.widgets.codeeditor.tab_widget.codeeditor_tab_presenter.ShowInExplorer"

    def test_action_show_in_explorer_no_widget(self):
        """
        Test when the view does not have the widget that has been requested and returns None
        """
        view = MockCodeEditorTabView()
        presenter = CodeEditorTabPresenter(view)
        view.widget.return_value = None

        presenter.action_show_in_explorer()
        view.widget.assert_called_once_with(view.last_tab_clicked)

    def test_action_show_in_explorer_no_filepath(self):
        """
        Test when the codeeditor widget exists but does not have a filepath, this
        happens when the code has not been saved to a file
        """
        view = MockCodeEditorTabView()
        presenter = CodeEditorTabPresenter(view)
        view.mock_code_editor_tab.editor.fileName.return_value = ""

        presenter.action_show_in_explorer()

        view.widget.assert_called_once_with(view.last_tab_clicked)
        view.mock_code_editor_tab.editor.fileName.assert_called_once_with()

    @patch(SHOW_IN_EXPLORER_PACKAGE)
    def test_action_show_in_explorer(self, mock_ShowInExplorer):
        """
        Test when the codeeditor widget has been saved to a file, and
        successfully returns a filepath.
        :param mock_ShowInExplorer: Mock of the ShowInExplorer class used to perform the action
        """
        view = MockCodeEditorTabView()
        presenter = CodeEditorTabPresenter(view)
        path = "/some/path"
        view.mock_code_editor_tab.editor.fileName.return_value = path

        presenter.action_show_in_explorer()
        view.widget.assert_called_once_with(view.last_tab_clicked)
        view.mock_code_editor_tab.editor.fileName.assert_called_once_with()
        mock_ShowInExplorer.open_directory.assert_called_once_with(os.path.dirname(path))
