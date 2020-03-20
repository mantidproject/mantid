# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from sans.common.enums import (SANSFacility, SaveType)
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.create_state import (create_states, create_gui_state_from_userfile)
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.state.AllStates import AllStates
from sans.gui_logic.models.table_model import TableModel


class GuiCommonTest(unittest.TestCase):
    def setUp(self):
        self.state_gui_model = StateGuiModel({})
        self._good_row_one = RowEntries(sample_scatter='LOQ74044')
        self._good_row_two = RowEntries(sample_scatter='LOQ74044')

        self.fake_state = mock.MagicMock(spec=AllStates)
        self.gui_state_director_instance = mock.MagicMock()
        self.gui_state_director_instance.create_state.return_value = self.fake_state
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

    @mock.patch('sans.gui_logic.models.create_state.create_gui_state_from_userfile')
    def test_create_state_from_user_file_if_specified(self, create_gui_state_mock):
        create_gui_state_mock.returns = StateGuiModel({})

        rows = [RowEntries(sample_scatter="LOQ74044", user_file="MaskLOQData.txt"),
                RowEntries(), RowEntries()]

        states, errors = create_states(self.state_gui_model, row_entries=rows, facility=SANSFacility.ISIS)

        self.assertEqual(len(states), 1)
        create_gui_state_mock.assert_called_once_with('MaskLOQData.txt', self.state_gui_model)

    def test_create_gui_state_from_userfile_adds_save_format_from_gui(self):
        gui_state = StateGuiModel({})
        gui_state.save_types = [SaveType.NX_CAN_SAS]

        row_state = create_gui_state_from_userfile('MaskLOQData.txt', gui_state)

        self.assertEqual(gui_state.save_types, row_state.save_types)


if __name__ == '__main__':
    unittest.main()
