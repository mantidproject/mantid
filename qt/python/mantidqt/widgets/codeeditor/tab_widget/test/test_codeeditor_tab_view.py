# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import

from qtpy.QtWidgets import QAction, QApplication, QWidget

from mantidqt.utils.qt.testing import GuiTest
from mantidqt.utils.qt.testing.qt_assertions_helper import QtAssertionsHelper
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.utils.testing.strict_mock import StrictMock
from mantidqt.widgets.codeeditor.tab_widget.codeeditor_tab_view import CodeEditorTabWidget


class MockMultiFileInterpreter(QWidget):
    def __init__(self):
        super(MockMultiFileInterpreter, self).__init__(None)
        self.execute_current = StrictMock()
        self.abort_current = StrictMock()
        self.close_tab = StrictMock()
        self.execute_current_async = StrictMock()
        self.execute_async = StrictMock()
        self.abort_current = StrictMock()
        self.toggle_find_replace_dialog = StrictMock()
        self.toggle_comment_current = StrictMock()
        self.tabs_to_spaces_current = StrictMock()
        self.spaces_to_tabs_current = StrictMock()
        self.toggle_whitespace_visible_all = StrictMock()
        self.plus_button_clicked = StrictMock()

    def closeEvent(self, event):
        self.deleteLater()
        super(MockMultiFileInterpreter, self).closeEvent(event)


class CodeEditorTabWidgetTest(GuiTest, QtWidgetFinder, QtAssertionsHelper):

    def test_deleted_on_close(self):
        mock_mfp = MockMultiFileInterpreter()
        view = CodeEditorTabWidget(mock_mfp)
        self.assert_widget_created()

        view.close()

        QApplication.processEvents()
        self.assert_widget_not_present(CodeEditorTabWidget.__name__)

        # closing our local mock should leave the QApplication without any widgets
        mock_mfp.close()
        QApplication.processEvents()
        self.assert_no_toplevel_widgets()

    def test_widget_connections_exist(self):
        mock_mfp = MockMultiFileInterpreter()
        view = CodeEditorTabWidget(mock_mfp)
        self.assert_widget_created()

        self.assert_object_connected_once(view, view.SHOW_IN_EXPLORER_ACTION_OBJECT_NAME, QAction, "triggered")
        self.assert_object_connected_once(view, view.ABORT_BUTTON_OBJECT_NAME)
        self.assert_object_connected_once(view, view.NEW_EDITOR_PLUS_BTN_OBJECT_NAME)
        self.assert_object_connected_once(view, view.RUN_BUTTON_OBJECT_NAME)

        # options button is not connected because it uses the internal Qt triggers
        # to show the popup menu
        self.assert_object_not_connected(view, view.OPTIONS_BUTTON_OBJECT_NAME)

        view.close()

        QApplication.processEvents()
        self.assert_widget_not_present(CodeEditorTabWidget.__name__)

        # closing our local mock should leave the QApplication without any widgets
        mock_mfp.close()
        QApplication.processEvents()
        self.assert_no_toplevel_widgets()
