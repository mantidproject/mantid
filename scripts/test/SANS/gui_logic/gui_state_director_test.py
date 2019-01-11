# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import os

from sans.gui_logic.presenter.gui_state_director import GuiStateDirector
from sans.gui_logic.models.table_model import (TableModel, TableIndexModel)
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.user_file.user_file_reader import UserFileReader
from sans.common.enums import SANSFacility
from sans.state.state import State
from sans.test_helper.user_file_test_helper import create_user_file, sample_user_file


class GuiStateDirectorTest(unittest.TestCase):
    @staticmethod
    def _get_table_model(option_string="", sample_thickness=8.0):
        table_index_model = TableIndexModel("SANS2D00022024", "", "", "", "", "", "", "", "",
                                            "", "", "",options_column_string=option_string,
                                            sample_thickness=sample_thickness)
        table_model = TableModel()
        table_model.add_table_entry_no_thread_or_signal(0, table_index_model)
        return table_model

    @staticmethod
    def _get_state_gui_model():
        user_file_path = create_user_file(sample_user_file)
        user_file_reader = UserFileReader(user_file_path)
        user_file_items = user_file_reader.read_user_file()
        if os.path.exists(user_file_path):
            os.remove(user_file_path)
        return StateGuiModel(user_file_items)

    def test_that_can_construct_state_from_models(self):
        table_model = self._get_table_model()
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(table_model, state_model, SANSFacility.ISIS)
        state = director.create_state(0)
        self.assertTrue(isinstance(state, State))
        try:
            state.validate()
            has_raised = False
        except ValueError:
            has_raised = True
        self.assertFalse(has_raised)
        self.assertTrue(state.wavelength.wavelength_low == [1.5])
        self.assertTrue(state.wavelength.wavelength_high == [12.5])

    def test_that_will_raise_when_models_are_incomplete(self):
        table_index_model = TableIndexModel(0, "", "", "", "", "", "",
                                               "", "", "", "", "", "")
        table_model = TableModel()
        table_model.add_table_entry(0, table_index_model)
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(table_model, state_model, SANSFacility.ISIS)
        self.assertRaises(ValueError, director.create_state, 0)

    def test_that_column_options_are_set_on_state(self):
        table_model = self._get_table_model(option_string="WavelengthMin=3.14,WavelengthMax=10.3")
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(table_model, state_model, SANSFacility.ISIS)

        state = director.create_state(0)
        self.assertTrue(isinstance(state, State))
        self.assertTrue(state.wavelength.wavelength_low == [3.14])
        self.assertTrue(state.wavelength.wavelength_high == [10.3])

    def test_that_shift_and_scale_set_on_state_from_options_column(self):
        table_model = self._get_table_model(option_string="MergeScale=1.2,MergeShift=0.5")
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(table_model, state_model, SANSFacility.ISIS)

        state = director.create_state(0)
        self.assertTrue(isinstance(state, State))
        self.assertEqual(state.reduction.merge_scale, 1.2)
        self.assertEqual(state.reduction.merge_shift, 0.5)

    def test_that_sample_thickness_set_on_state(self):
        table_model = self._get_table_model()
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(table_model, state_model, SANSFacility.ISIS)

        state = director.create_state(0)
        self.assertTrue(isinstance(state, State))

        self.assertEqual(state.scale.thickness, 1.0)

if __name__ == '__main__':
    unittest.main()
