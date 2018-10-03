# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import tempfile
import unittest
import sys
import os
import json

import mantid

from sans.gui_logic.presenter.settings_diagnostic_presenter import SettingsDiagnosticPresenter
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock, FakeState, create_mock_settings_diagnostic_tab)
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class SettingsDiagnosticPresenterTest(unittest.TestCase):
    def test_that_can_get_state_for_index(self):
        parent_presenter = create_run_tab_presenter_mock()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        state = presenter.get_state(3)
        self.assertTrue(isinstance(state, FakeState))

    def test_that_updates_tree_view_when_row_selection_changes(self):
        parent_presenter = create_run_tab_presenter_mock()
        view = create_mock_settings_diagnostic_tab()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        presenter.set_view(view)
        self.assertTrue(view.set_tree.call_count == 1)
        presenter.on_row_changed()
        self.assertTrue(view.set_tree.call_count == 2)

    def test_that_updates_rows_when_triggered(self):
        parent_presenter = create_run_tab_presenter_mock()
        view = create_mock_settings_diagnostic_tab()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        presenter.set_view(view)
        self.assertTrue(view.update_rows.call_count == 1)
        presenter.on_update_rows()
        self.assertTrue(view.update_rows.call_count == 2)

    def test_that_can_save_out_state(self):
        # Arrange
        parent_presenter = create_run_tab_presenter_mock()
        view = create_mock_settings_diagnostic_tab()
        dummy_file_path = os.path.join(tempfile.gettempdir(), "sans_settings_diag_test.json")
        print(dummy_file_path)
        view.get_save_location = mock.MagicMock(return_value=dummy_file_path)
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        presenter.set_view(view)

        # Act
        presenter.on_save_state()

        # Assert
        self.assertTrue(os.path.exists(dummy_file_path))

        with open(dummy_file_path) as f:
            data = json.load(f)
        self.assertTrue(data == "dummy_state")

        if os.path.exists(dummy_file_path):
            os.remove(dummy_file_path)

    def test_catches_exception_when_cant_find_file(self):
        parent_presenter = create_run_tab_presenter_mock()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        view = mock.MagicMock()
        view.get_current_row.result = 1
        presenter.set_view(view)
        parent_presenter.get_state_for_row = mock.MagicMock()
        parent_presenter.get_state_for_row.side_effect = RuntimeError('Test Error')

        presenter.on_row_changed()

        parent_presenter.display_warning_box.assert_called_once_with('Warning', 'Unable to find files.', 'Test Error')

if __name__ == '__main__':
    unittest.main()
