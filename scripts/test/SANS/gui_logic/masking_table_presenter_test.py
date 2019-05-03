# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from sans.gui_logic.presenter.masking_table_presenter import (MaskingTablePresenter, masking_information)
from sans.test_helper.mock_objects import (FakeParentPresenter, FakeState, create_mock_masking_table, create_run_tab_presenter_mock)


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

    def test_that_checks_display_mask_is_reenabled_after_error(self):
        # Arrange
        parent_presenter = create_run_tab_presenter_mock(use_fake_state=False)
        presenter = MaskingTablePresenter(parent_presenter)

        presenter.on_processing_error_masking_display = mock.MagicMock()
        presenter._view = mock.MagicMock()
        presenter._view.set_display_mask_button_to_processing = mock.MagicMock()
        presenter._view.get_current_row.side_effect = RuntimeError("Mock get_current_row failure")

        try:
            presenter.on_display()
        except Exception as e:
            self.assertEqual(str(e), "Mock get_current_row failure")
        else:
            self.assertFalse(True)  # As we expect an error to be raised

        # Confirm that on_processing_error_masking_display was called
        self.assertEqual(
            presenter.on_processing_error_masking_display.call_count, 1,
            "Expected on_processing_error_masking_display to have been called. Called {} times.".format(
                presenter.on_processing_error_masking_display.call_count))


if __name__ == '__main__':
    unittest.main()
