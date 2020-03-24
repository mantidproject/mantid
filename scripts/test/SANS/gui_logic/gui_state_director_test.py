# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import os
import unittest

from sans.common.enums import SANSFacility
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel
from sans.gui_logic.presenter.gui_state_director import GuiStateDirector
from sans.state.AllStates import AllStates
from sans.test_helper.user_file_test_helper import create_user_file, sample_user_file
from sans.user_file.user_file_reader import UserFileReader


class GuiStateDirectorTest(unittest.TestCase):
    @staticmethod
    def _get_row_entry(option_string="", sample_thickness=8.0):
        row_entry = RowEntries(sample_scatter="SANS2D00022024",
                               sample_thickness=sample_thickness)
        row_entry.options.set_user_options(option_string)
        return row_entry

    @staticmethod
    def _get_state_gui_model():
        user_file_path = create_user_file(sample_user_file)
        user_file_reader = UserFileReader(user_file_path)
        user_file_items = user_file_reader.read_user_file()
        if os.path.exists(user_file_path):
            os.remove(user_file_path)
        return StateGuiModel(user_file_items)

    def test_that_can_construct_state_from_models(self):
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(state_model, SANSFacility.ISIS)
        state = director.create_state(self._get_row_entry())
        self.assertTrue(isinstance(state, AllStates))
        try:
            state.validate()
            has_raised = False
        except ValueError:
            has_raised = True
        self.assertFalse(has_raised)
        self.assertEqual(state.wavelength.wavelength_low,  [1.5])
        self.assertEqual(state.wavelength.wavelength_high,  [12.5])

    def test_that_will_raise_when_models_are_incomplete(self):
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(state_model, SANSFacility.ISIS)
        with self.assertRaises(ValueError):
            director.create_state(RowEntries())

    def test_that_column_options_are_set_on_state(self):
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(state_model, SANSFacility.ISIS)

        row_entry = self._get_row_entry(option_string="WavelengthMin=3.14,WavelengthMax=10.3")
        state = director.create_state(row_entry)
        self.assertTrue(isinstance(state, AllStates))
        self.assertEqual(state.wavelength.wavelength_low,  [3.14])
        self.assertEqual(state.wavelength.wavelength_high,  [10.3])

    def test_that_shift_and_scale_set_on_state_from_options_column(self):
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(state_model, SANSFacility.ISIS)

        state = self._get_row_entry(option_string="MergeScale=1.2,MergeShift=0.5")
        state = director.create_state(state)
        self.assertTrue(isinstance(state, AllStates))
        self.assertEqual(state.reduction.merge_scale, 1.2)
        self.assertEqual(state.reduction.merge_shift, 0.5)


if __name__ == '__main__':
    unittest.main()
