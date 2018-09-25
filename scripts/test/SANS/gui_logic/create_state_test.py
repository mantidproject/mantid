from __future__ import (absolute_import, division, print_function)

import unittest
import sys
import mantid

from sans.gui_logic.models.create_state import (create_states, create_gui_state_from_userfile)
from sans.common.enums import (SANSInstrument, ISISReductionMode, SANSFacility, SaveType)
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel, TableIndexModel
from sans.state.state import State

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

class GuiCommonTest(unittest.TestCase):
    def setUp(self):
        self.table_model = TableModel()
        self.state_gui_model = StateGuiModel({})
        table_index_model_0 = TableIndexModel('LOQ74044', '', '', '', '', '', '', '', '', '', '', '')
        table_index_model_1 = TableIndexModel('LOQ74044', '', '', '', '', '', '', '', '', '', '', '')
        self.table_model.add_table_entry(0, table_index_model_0)
        self.table_model.add_table_entry(1, table_index_model_1)

        self.fake_state = mock.MagicMock(spec=State)
        self.gui_state_director_instance = mock.MagicMock()
        self.gui_state_director_instance.create_state.return_value = self.fake_state
        self.patcher = mock.patch('sans.gui_logic.models.create_state.GuiStateDirector')
        self.addCleanup(self.patcher.stop)
        self.gui_state_director = self.patcher.start()
        self.gui_state_director.return_value = self.gui_state_director_instance

    def test_create_states_returns_correct_number_of_states(self):
        states, errors = create_states(self.state_gui_model, self.table_model, SANSInstrument.LOQ, SANSFacility.ISIS,
                               row_index=[0,1])

        self.assertEqual(len(states), 2)

    def test_create_states_returns_correct_number_of_states_for_specified_row_index(self):

        states, errors = create_states(self.state_gui_model, self.table_model, SANSInstrument.LOQ, SANSFacility.ISIS,
                               row_index=[1])

        self.assertEqual(len(states), 1)

    def test_skips_empty_rows(self):
        table_index_model = TableIndexModel('', '', '', '', '', '', '', '', '', '', '', '')
        self.table_model.add_table_entry(1, table_index_model)

        states, errors = create_states(self.state_gui_model, self.table_model, SANSInstrument.LOQ, SANSFacility.ISIS,
                               row_index=[0,1, 2])

        self.assertEqual(len(states), 2)

    @mock.patch('sans.gui_logic.models.create_state.create_gui_state_from_userfile')
    def test_create_state_from_user_file_if_specified(self, create_gui_state_mock):
        create_gui_state_mock.returns = StateGuiModel({})
        table_index_model = TableIndexModel('LOQ74044', '', '', '', '', '', '', '', '', '', '', '',
                                            user_file='MaskLOQData.txt')
        table_model = TableModel()
        table_model.add_table_entry(0, table_index_model)

        states, errors = create_states(self.state_gui_model, table_model, SANSInstrument.LOQ, SANSFacility.ISIS,
                               row_index=[0,1, 2])

        self.assertEqual(len(states), 1)
        create_gui_state_mock.assert_called_once_with('MaskLOQData.txt', self.state_gui_model)

    def test_create_gui_state_from_userfile_adds_save_format_from_gui(self):
        gui_state = StateGuiModel({})
        gui_state.save_types = [SaveType.NXcanSAS]

        row_state = create_gui_state_from_userfile('MaskLOQData.txt', gui_state)

        self.assertEqual(gui_state.save_types, row_state.save_types)


if __name__ == '__main__':
    unittest.main()