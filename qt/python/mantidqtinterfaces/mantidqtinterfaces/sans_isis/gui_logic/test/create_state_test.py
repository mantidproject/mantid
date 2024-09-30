# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from sans_core.common.enums import SANSFacility
from sans_core.common.RowEntries import RowEntries
from mantidqtinterfaces.sans_isis.gui_logic.models.create_state import create_states
from mantidqtinterfaces.sans_isis.gui_logic.models.state_gui_model import StateGuiModel
from sans_core.state.AllStates import AllStates


class GuiCommonTest(unittest.TestCase):
    def setUp(self):
        self.state_gui_model = StateGuiModel(AllStates())
        self._good_row_one = RowEntries(sample_scatter="LOQ74044")
        self._good_row_two = RowEntries(sample_scatter="LOQ74044")

        self.gui_state_director_instance = mock.MagicMock()
        self.gui_state_director_instance.create_state.return_value = self.state_gui_model
        self.patcher = mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.create_state.GuiStateDirector")
        self.addCleanup(self.patcher.stop)
        self.gui_state_director = self.patcher.start()
        self.gui_state_director.return_value = self.gui_state_director_instance

    def test_create_states_returns_correct_number_of_states(self):
        rows = [self._good_row_one, self._good_row_two]
        states, errors = create_states(self.state_gui_model, SANSFacility.ISIS, row_entries=rows)

        self.assertEqual(len(states), 2)

    def test_skips_empty_rows(self):
        rows = [self._good_row_one, RowEntries(), self._good_row_two]
        states, errors = create_states(self.state_gui_model, SANSFacility.ISIS, row_entries=rows)
        self.assertEqual(2, len(states))

    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.models.create_state._get_thickness_for_row")
    def test_create_state_from_user_file_if_specified(self, thickness_mock):
        expected_user_file = "MaskLOQData.txt"

        # Mock out row entry so it does not lookup file information
        mock_row_entry = mock.Mock(spec=RowEntries)
        mock_row_entry.is_empty.return_value = False
        mock_row_entry.user_file = expected_user_file
        rows = [mock_row_entry, RowEntries(), RowEntries()]

        states, errors = create_states(self.state_gui_model, row_entries=rows, facility=SANSFacility.ISIS)

        self.assertEqual(len(states), 1)
        self.gui_state_director_instance.create_state.assert_called_once_with(
            mock_row_entry, file_lookup=mock.ANY, row_user_file=expected_user_file
        )
        thickness_mock.assert_called()


if __name__ == "__main__":
    unittest.main()
