from __future__ import (absolute_import, division, print_function)

import unittest
import sys

import mantid

from sans.gui_logic.presenter.masking_table_presenter import (MaskingTablePresenter, masking_information)
from sans.test_helper.mock_objects import (FakeParentPresenter, FakeState, create_mock_masking_table, create_run_tab_presenter_mock)
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class MaskingTablePresenterTest(unittest.TestCase):
    def test_that_can_get_state_for_index(self):
        parent_presenter = create_run_tab_presenter_mock()
        presenter = MaskingTablePresenter(parent_presenter)
        state = presenter.get_state(3)
        self.assertTrue(isinstance(state, FakeState))

    def test_that_sets_table_when_row_changes(self):
        # Arrange
        parent_presenter = create_run_tab_presenter_mock(use_fake_state=False)
        view = create_mock_masking_table()
        presenter = MaskingTablePresenter(parent_presenter)
        # Act + Assert
        presenter.set_view(view)
        self.assertTrue(view.set_table.call_count == 1)
        presenter.on_row_changed()
        self.assertTrue(view.set_table.call_count == 2)
        first_call = mock.call([])
        second_call = mock.call([masking_information(first='Beam stop', second='', third='infinite-cylinder, r = 10.0'),
                                 masking_information(first='Corners', second='', third='infinite-cylinder, r = 20.0'),
                                 masking_information(first='Phi', second='', third='L/PHI -90.0 90.0')])  # noqa
        view.set_table.assert_has_calls([first_call, second_call])


if __name__ == '__main__':
    unittest.main()
