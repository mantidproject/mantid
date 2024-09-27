# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
import unittest
from unittest import mock

from sans_core.common.enums import SANSFacility
from sans_core.common.RowEntries import RowEntries
from mantidqtinterfaces.sans_isis.gui_logic.models.state_gui_model import StateGuiModel
from sans_core.state.StateObjects.StateSave import StateSave
from mantidqtinterfaces.sans_isis.gui_logic.presenter.gui_state_director import GuiStateDirector
from sans_core.test_helper.user_file_test_helper import create_user_file, sample_user_file
from sans_core.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter


class GuiStateDirectorTest(unittest.TestCase):
    @staticmethod
    def _get_row_entry(option_string="", sample_thickness=8.0, background_workspace="test", scale_factor=1.1, output_name=None):
        row_entry = RowEntries(
            sample_scatter="SANS2D00022024",
            sample_thickness=sample_thickness,
            background_ws=background_workspace,
            scale_factor=scale_factor,
            output_name=output_name,
        )
        row_entry.options.set_user_options(option_string)
        return row_entry

    @staticmethod
    def _get_state_gui_model():
        user_file_path = create_user_file(sample_user_file)
        adapter = UserFileReaderAdapter(file_information=None, user_file_name=user_file_path)
        return StateGuiModel(adapter.get_all_states(file_information=None))

    def test_that_can_construct_state_from_models(self):
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(state_model, SANSFacility.ISIS)
        state = director.create_state(self._get_row_entry())
        self.assertTrue(isinstance(state, StateGuiModel))
        state = state.all_states
        try:
            state.validate()
            has_raised = False
        except ValueError:
            has_raised = True
        self.assertFalse(has_raised)
        self.assertEqual(state.wavelength.wavelength_interval.wavelength_full_range, (1.5, 12.5))

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
        self.assertTrue(isinstance(state, StateGuiModel))
        self.assertEqual(state.all_states.wavelength.wavelength_interval.wavelength_full_range, (3.14, 10.3))

    def test_that_shift_and_scale_set_on_state_from_options_column(self):
        state_model = self._get_state_gui_model()
        director = GuiStateDirector(state_model, SANSFacility.ISIS)

        state = self._get_row_entry(option_string="MergeScale=1.2,MergeShift=0.5")
        state = director.create_state(state)
        self.assertTrue(isinstance(state, StateGuiModel))
        self.assertEqual(state.all_states.reduction.merge_scale, 1.2)
        self.assertEqual(state.all_states.reduction.merge_shift, 0.5)

    def test_reduction_dim_copied_from_gui(self):
        state_model = mock.Mock(spec=self._get_state_gui_model())
        expected_dim = mock.NonCallableMock()
        # This is set by the user on the main GUI, so should be copied in with a custom user file still
        state_model.reduction_dimensionality = expected_dim

        # Copy the top level model and reset dim rather than load a file
        copied_state = copy.deepcopy(state_model)
        copied_state.reduction_dimensionality = None

        director = GuiStateDirector(state_model, SANSFacility.ISIS)
        director._load_current_state = mock.Mock(return_value=copied_state)
        state = director.create_state(self._get_row_entry(), row_user_file="NotThere.txt")

        self.assertEqual(expected_dim, state.reduction_dimensionality)

    def test_save_settings_copied_from_gui(self):
        state_model = mock.Mock(spec=self._get_state_gui_model())
        expected_output_name = "user_specified_name"
        state_save = StateSave()
        state_save.user_specified_output_name = expected_output_name
        state_model.all_states.save = state_save

        # Copy the top level model and reset save rather than load a file
        copied_state = copy.deepcopy(state_model)
        copied_state.all_states.save = None

        director = GuiStateDirector(state_model, SANSFacility.ISIS)
        director._load_current_state = mock.Mock(return_value=copied_state)
        new_state = director.create_state(self._get_row_entry(), row_user_file="NotThere.txt")

        self.assertEqual(expected_output_name, new_state.all_states.save.user_specified_output_name)

    def test_output_name_set_correctly_on_row_state(self):
        test_cases = [(None, None), (None, "row_output_name"), ("state_output_name", None), ("state_output_name", "row_output_name")]

        for state_model_output_name, row_output_name in test_cases:
            with self.subTest(state_model_output_name=state_model_output_name, row_output_name=row_output_name):
                state_model = self._get_state_gui_model()
                state_save = StateSave()
                state_save.user_specified_output_name = state_model_output_name
                state_model.all_states.save = state_save

                director = GuiStateDirector(state_model, SANSFacility.ISIS)
                new_state = director.create_state(self._get_row_entry(output_name=row_output_name))

                self.assertEqual(row_output_name, new_state.all_states.save.user_specified_output_name)


if __name__ == "__main__":
    unittest.main()
