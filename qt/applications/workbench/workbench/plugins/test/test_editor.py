# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
import os
from qtpy.QtWidgets import QMainWindow
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from workbench.plugins.editor import MultiFileEditor


def raise_exception(exception):
    raise exception


@start_qapplication
class MultiFileEditorTest(unittest.TestCase):

    def test_tab_session_restore(self):
        editor = MultiFileEditor(QMainWindow())
        prev_session_tabs = [
            os.path.join(os.path.dirname(__file__), '__init__.py'),
            __file__]
        editor.restore_session_tabs(prev_session_tabs)
        self.assertEqual(2, editor.editors.editor_count)

    def test_tab_session_restore_nothing_to_restore(self):
        editor = MultiFileEditor(QMainWindow())
        mock_settings = mock.Mock(get=lambda x: raise_exception(KeyError))
        editor.readSettings(mock_settings)
        self.assertEqual(1, editor.editors.editor_count)  # default empty tab should be open

    def test_tab_session_restore_path_doesnt_exist(self):
        editor = MultiFileEditor(QMainWindow())
        prev_session_tabs = ['FileDoesntExist']
        editor.restore_session_tabs(prev_session_tabs)
        self.assertEqual(1, editor.editors.editor_count)  # default empty tab should be open


if __name__ == '__main__':
    unittest.main()
