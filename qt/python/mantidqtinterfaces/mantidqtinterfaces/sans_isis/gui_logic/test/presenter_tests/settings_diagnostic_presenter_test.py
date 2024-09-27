# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile
import unittest
from unittest import mock

from mantidqtinterfaces.sans_isis.gui_logic.presenter.settings_diagnostic_presenter import SettingsDiagnosticPresenter
from sans_core.state.Serializer import Serializer
from mantidqtinterfaces.sans_isis.gui_logic.test.mock_objects import (
    create_run_tab_presenter_mock,
    FakeState,
    create_mock_settings_diagnostic_tab,
)


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
        self.assertEqual(view.set_tree.call_count, 1)
        presenter.on_row_changed()
        self.assertEqual(view.set_tree.call_count, 2)

    def test_that_updates_rows_when_triggered(self):
        parent_presenter = create_run_tab_presenter_mock()
        view = create_mock_settings_diagnostic_tab()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        presenter.set_view(view)
        self.assertEqual(view.update_rows.call_count, 1)
        presenter.on_update_rows()
        self.assertEqual(view.update_rows.call_count, 2)

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

        obj = Serializer.load_file(dummy_file_path)
        self.assertEqual("dummy_state", obj.dummy_state)

        if os.path.exists(dummy_file_path):
            os.remove(dummy_file_path)

    def test_catches_exception_when_cant_find_file(self):
        parent_presenter = create_run_tab_presenter_mock()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        view = mock.MagicMock()
        view.get_current_row.result = 1
        presenter.set_view(view)
        parent_presenter.get_state_for_row = mock.MagicMock()
        parent_presenter.get_state_for_row.side_effect = RuntimeError("Test Error")

        presenter.on_row_changed()

        parent_presenter.display_warning_box.assert_called_once_with("Warning", "Unable to find files.", "Test Error")


if __name__ == "__main__":
    unittest.main()
