from __future__ import (absolute_import, division, print_function)

import unittest

import mantid

from sans.gui_logic.presenter.settings_diagnostic_presenter import SettingsDiagnosticPresenter
from sans.test_helper.mock_objects import (create_run_tab_presenter_mock, FakeState, create_mock_settings_diagnostic_tab)


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


if __name__ == '__main__':
    unittest.main()
