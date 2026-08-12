# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import os
from qtpy.QtWidgets import QMainWindow
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plugins.editor import EditorSettings, MultiFileEditor, TAB_SETTINGS_KEY, ZOOM_LEVEL_KEY


def raise_exception(exception):
    raise exception


@start_qapplication
class MultiFileEditorTest(unittest.TestCase):
    def test_tab_session_restore(self):
        editor = MultiFileEditor(QMainWindow())
        prev_session_tabs = [os.path.join(os.path.dirname(__file__), "__init__.py"), __file__]
        editor.restore_session_tabs(prev_session_tabs)
        self.assertEqual(2, editor.editors.editor_count)

    def test_tab_session_restore_nothing_to_restore(self):
        editor = MultiFileEditor(QMainWindow())
        mock_settings = mock.Mock(get=lambda x: raise_exception(KeyError))
        editor.restoreSettings(editor.readSettings(mock_settings))
        self.assertEqual(1, editor.editors.editor_count)  # default empty tab should be open

    def test_tab_session_restore_path_doesnt_exist(self):
        editor = MultiFileEditor(QMainWindow())
        prev_session_tabs = ["FileDoesntExist"]
        editor.restore_session_tabs(prev_session_tabs)
        self.assertEqual(1, editor.editors.editor_count)  # default empty tab should be open

    def test_read_settings_returns_snapshot_without_restoring_or_writing(self):
        editor = MultiFileEditor(QMainWindow())
        editor.restore_session_tabs = mock.Mock()
        settings = mock.Mock()
        values = {TAB_SETTINGS_KEY: ["one.py", "two.py"], ZOOM_LEVEL_KEY: 3}
        settings.get.side_effect = lambda key, **_: values[key]

        snapshot = editor.readSettings(settings)

        self.assertEqual(EditorSettings(("one.py", "two.py"), 3), snapshot)
        editor.restore_session_tabs.assert_not_called()
        settings.set.assert_not_called()

    def test_read_settings_if_not_done_restores_snapshot_once(self):
        editor = MultiFileEditor(QMainWindow())
        settings = mock.Mock()
        snapshot = EditorSettings(("one.py",), 3)
        editor.readSettings = mock.Mock(return_value=snapshot)
        editor.restoreSettings = mock.Mock()

        editor.readSettingsIfNotDone(settings)
        editor.readSettingsIfNotDone(settings)

        editor.readSettings.assert_called_once_with(settings)
        editor.restoreSettings.assert_called_once_with(snapshot)

    def test_save_settings_writes_an_explicit_snapshot(self):
        editor = MultiFileEditor(QMainWindow())
        settings = mock.Mock()

        editor.saveSettings(settings, EditorSettings(("one.py", "two.py"), 3))

        settings.set.assert_has_calls([mock.call(ZOOM_LEVEL_KEY, 3), mock.call(TAB_SETTINGS_KEY, ["one.py", "two.py"])])


if __name__ == "__main__":
    unittest.main()
