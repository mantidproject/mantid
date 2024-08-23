# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import os.path
import tempfile
import unittest

from qtpy.QtWidgets import QApplication
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter

MANTID_API_IMPORT = "from mantid.simpleapi import *\n"
PERMISSION_BOX_FUNC = "mantidqt.widgets.codeeditor.scriptcompatibility.permission_box_to_prepend_import"


@start_qapplication
class MultiPythonFileInterpreterTest(unittest.TestCase, QtWidgetFinder):
    def test_default_contains_single_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)

    def test_add_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(2, widget.editor_count)

    def test_open_file_in_new_tab_import_added(self):
        test_string = "Test file\nConvertUnits()"
        widget = MultiPythonFileInterpreter()
        mock_open_func = mock.mock_open(read_data=test_string)
        with mock.patch(widget.__module__ + ".open", mock_open_func, create=True):
            with mock.patch(PERMISSION_BOX_FUNC, lambda: True):
                widget.open_file_in_new_tab(test_string)
        self.assertEqual(widget.current_editor().editor.isModified(), True, msg="Script not marked as modified.")
        self.assertIn(MANTID_API_IMPORT, widget.current_editor().editor.text(), msg="'simpleapi' import not added to script.")

    def test_open_file_in_new_tab_no_import_added(self):
        test_string = "Test file\n"
        widget = MultiPythonFileInterpreter()
        mock_open_func = mock.mock_open(read_data=test_string)
        with mock.patch(widget.__module__ + ".open", mock_open_func, create=True):
            with mock.patch(PERMISSION_BOX_FUNC, lambda: True):
                widget.open_file_in_new_tab(test_string)
        self.assertNotIn(MANTID_API_IMPORT, widget.current_editor().editor.text())

    def test_open_same_file_twice_no_extra_tab(self):
        widget = MultiPythonFileInterpreter()
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = os.path.join(temp_dir, "test_open_same_file_twice_no_extra_tab")
            with open(filename, "w") as f:
                f.write("Test")
            with mock.patch("mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename", lambda s: filename):
                widget.open_file_in_new_tab(filename)
                widget.open_file_in_new_tab(filename)
        self.assertEqual(2, widget.editor_count, msg="Should be the original tab, plus one (not two) tabs for the file")

    def test_open_same_file_twice_no_tab_even_if_modified(self):
        widget = MultiPythonFileInterpreter()
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = os.path.join(temp_dir, "test_open_same_file_twice_no_tab_even_if_modified")
            with open(filename, "w") as f:
                f.write("Test")
            with mock.patch("mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename", lambda s: filename):
                widget.open_file_in_new_tab(filename)
                widget.current_editor().editor.append("Some text")
                widget.open_file_in_new_tab(filename)
        self.assertEqual(2, widget.editor_count, msg="Should be the original tab, plus one (not two) tabs for the file")
        # File should still be modified after attempted reopening because it was modified before reopening and we don't want to lose changes
        self.assertTrue(widget.current_editor().editor.isModified(), msg="File should still be modified after attempted reopening")

    def test_save_as_closes_the_original_tab(self):
        test_string = "Test file\n"
        widget = MultiPythonFileInterpreter()
        mock_open_func = mock.mock_open(read_data=test_string)
        ask_for_filename = "mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename"
        test_file_name = "test_save_as_leaves_original_marked_unmodified"
        # fmt: off
        with mock.patch(widget.__module__ + ".open", mock_open_func, create=True), \
                mock.patch(PERMISSION_BOX_FUNC, lambda: True), \
                tempfile.TemporaryDirectory() as temp_dir, \
                mock.patch(ask_for_filename, lambda s: os.path.join(temp_dir, test_file_name)):
            widget.open_file_in_new_tab(test_string)
            widget.save_current_file_as()
        # fmt: on
        # There's one tab when you create the editor, then another one was added when opening the mock file. Clicking
        # 'Save Script as' should close the original tab with the old name, and leave you with a tab with the new name.
        self.assertEqual(2, widget.editor_count)
        self.assertFalse(widget.current_editor().editor.isModified(), msg="Tab should be marked as not modified")

    def test_file_added_to_watcher_on_opening_tab(self):
        widget = MultiPythonFileInterpreter()
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = os.path.join(temp_dir, "test_file_added_to_watcher_on_opening_tab")
            with open(filename, "w") as f:
                f.write("Test")
            with mock.patch("mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename", lambda s: filename):
                widget.open_file_in_new_tab(filename)
            watcher_files = widget.file_watcher.files()
            self.assertEqual(1, len(watcher_files), "Should be one file being watched")
            self.assertEqual(filename, watcher_files[0], "Single file being watched should be the correct file")

    def test_file_removed_from_watcher_on_closing_tab(self):
        widget = MultiPythonFileInterpreter()
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = os.path.join(temp_dir, "test_file_removed_from_watcher_on_closing_tab")
            with open(filename, "w") as f:
                f.write("Test")
            with mock.patch("mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename", lambda s: filename):
                widget.open_file_in_new_tab(filename)
            widget.close_all()
            files = widget.file_watcher.files()
            self.assertEqual(0, len(files), "Tab is closed so should be removed from the file watcher")

    def test_saving_file_adds_events_to_ignore_list(self):
        widget = MultiPythonFileInterpreter()
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = os.path.join(temp_dir, "test_saving_file_adds_events_to_ignore_list")
            with open(filename, "w") as f:
                f.write("Test")
            with mock.patch("mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename", lambda s: filename):
                widget.open_file_in_new_tab(filename)
            editor = widget.current_editor()
            editor.editor.setText("New text")
            widget.save_current_file()
            QApplication.instance().processEvents()
            self.assertEqual(0, len(widget.files_changed_unhandled), "Saving the file should not generate events")

    def test_open_file_in_new_tab_with_utf8_content(self):
        widget = MultiPythonFileInterpreter()
        with tempfile.TemporaryDirectory() as temp_dir:
            filename = os.path.join(temp_dir, "utf8_characters")
            with open(filename, "w", encoding="utf_8") as f:
                f.write("输出坐标系")
            with mock.patch("mantidqt.widgets.codeeditor.interpreter.EditorIO.ask_for_filename", lambda s: filename):
                # Test that we can open a utf-8 file with no exceptions
                widget.open_file_in_new_tab(filename)
            self.assertEqual(2, widget.editor_count, msg="Should be the original tab, plus one (not two) tabs for the file")

    def test_open_file_in_new_tab_handles_missing_file(self):
        widget = MultiPythonFileInterpreter()
        missing_file = "fake/file/path.txt"
        # Should catch the filenotfound and return early
        widget.open_file_in_new_tab(missing_file)
        self.assertEqual(1, widget.editor_count, msg="No new file should have been opened")

    def test_cancelled_save_does_not_add_file_to_watcher(self):
        widget = MultiPythonFileInterpreter()
        widget.current_editor = mock.MagicMock(autospec=True)
        widget.current_editor().filename = ""
        widget.save_current_file()
        self.assertEqual(0, len(widget.file_watcher.files()), "File watcher should be empty")
        self.assertEqual(0, len(widget.files_that_we_have_changed), "We haven't changed any files so this should be empty")


if __name__ == "__main__":
    unittest.main()
