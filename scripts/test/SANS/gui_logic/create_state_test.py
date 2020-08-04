# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from sans.common.enums import (SANSFacility, SaveType, SANSInstrument)
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.create_state import (create_states, create_state_from_userfile)
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.state.AllStates import AllStates


class GuiCommonTest(unittest.TestCase):
    def setUp(self):
        self.state_gui_model = StateGuiModel(AllStates())
        self._good_row_one = RowEntries(sample_scatter='LOQ74044')
        self._good_row_two = RowEntries(sample_scatter='LOQ74044')

        self.gui_state_director_instance = mock.MagicMock()
        self.gui_state_director_instance.create_state.return_value = self.state_gui_model
        self.patcher = mock.patch('sans.gui_logic.models.create_state.GuiStateDirector')
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

    @mock.patch('sans.gui_logic.models.create_state.create_state_from_userfile')
    @mock.patch('sans.gui_logic.models.create_state._get_thickness_for_row')
    def test_create_state_from_user_file_if_specified(self, thickness_mock, create_gui_state_mock):
        create_gui_state_mock.return_value = StateGuiModel(AllStates())
        expected_user_file = 'MaskLOQData.txt'

        # Mock out row entry so it does not lookup file information
        mock_row_entry = mock.Mock(spec=RowEntries)
        mock_row_entry.is_empty.return_value = False
        mock_row_entry.user_file = expected_user_file
        rows = [mock_row_entry, RowEntries(), RowEntries()]

        states, errors = create_states(self.state_gui_model, row_entries=rows, facility=SANSFacility.ISIS)

        self.assertEqual(len(states), 1)
        create_gui_state_mock.assert_called_once_with(expected_user_file, existing_state=self.state_gui_model,
                                                      file_information=rows[0].file_information)
        thickness_mock.assert_called()

    def test_create_gui_state_from_userfile_adds_save_format_from_gui(self):
        gui_state = StateGuiModel(AllStates())
        gui_state.save_types = [SaveType.NX_CAN_SAS]

        row_state = create_state_from_userfile('MaskLOQData.txt', existing_state=gui_state,
                                               file_information=None)
        self.assertEqual(gui_state.save_types, row_state.save_types)


if __name__ == '__main__':
    unittest.main()
