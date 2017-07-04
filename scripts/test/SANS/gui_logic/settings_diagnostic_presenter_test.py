from __future__ import (absolute_import, division, print_function)

import mantid
import unittest
from sans.gui_logic.presenter.settings_diagnostic_presenter import SettingsDiagnosticPresenter


# ------------
# Test Helper
# ------------
class MockState(object):
    dummy_state = "dummy_state"

    def __init__(self):
        super(MockState, self).__init__()

    @property
    def property_manager(self):
        return self.dummy_state


class MockParentPresenter(object):

    def __init__(self):
        super(MockParentPresenter, self).__init__()

    def get_row_indices(self):
        # We assume that row 2 is empty
        return [0, 1, 3]

    def get_state_for_row(self, row_index):
        return MockState() if row_index == 3 else ""


class MockSettingsDiagnosticTabView(object):
    def __init__(self):
        super(MockSettingsDiagnosticTabView, self).__init__()
        self.number_of_view_updates = 0
        self.number_of_row_updates = 0

    def add_listener(self, listener):
        pass

    def update_rows(self, rows):
        _ = rows
        self.number_of_row_updates += 1

    def set_tree(self, state):
        _ = state
        self.number_of_view_updates += 1

    @staticmethod
    def get_current_row():
        return 3


class SettingsDiagnosticPresenterTest(unittest.TestCase):
    def test_that_can_get_state_for_index(self):
        parent_presenter = MockParentPresenter()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        state = presenter.get_state(3)
        self.assertTrue(isinstance(state, MockState))

    def test_that_updates_tree_view_when_row_selection_changes(self):
        parent_presenter = MockParentPresenter()
        view = MockSettingsDiagnosticTabView()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        presenter.set_view(view)
        self.assertTrue(view.number_of_view_updates == 1)
        presenter.on_row_changed()
        self.assertTrue(view.number_of_view_updates == 2)

    def test_that_updates_rows_when_triggered(self):
        parent_presenter = MockParentPresenter()
        view = MockSettingsDiagnosticTabView()
        presenter = SettingsDiagnosticPresenter(parent_presenter)
        presenter.set_view(view)
        self.assertTrue(view.number_of_row_updates == 1)
        presenter.on_update_rows()
        self.assertTrue(view.number_of_row_updates == 2)


if __name__ == '__main__':
    unittest.main()
