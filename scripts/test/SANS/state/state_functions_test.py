import unittest
import mantid

from mantid.api import AnalysisDataService
from sans.state.state_functions import (get_output_workspace_name, is_pure_none_or_not_none, one_is_none,
                                            validation_message, is_not_none_and_first_larger_than_second,
                                            write_hash_into_reduced_can_workspace, get_reduced_can_workspace_from_ads)
from test_director import TestDirector
from sans.state.data import StateData
from sans.common.enums import (ReductionDimensionality, ISISReductionMode, OutputParts)
from sans.common.general_functions import create_unmanaged_algorithm


class StateFunctionsTest(unittest.TestCase):
    @staticmethod
    def _get_state():
        test_director = TestDirector()
        state = test_director.construct()

        state.data.sample_scatter_run_number = 12345
        state.data.sample_scatter_period = StateData.ALL_PERIODS

        state.reduction.dimensionality = ReductionDimensionality.OneDim

        state.wavelength.wavelength_low = 12.0
        state.wavelength.wavelength_high = 34.0

        state.mask.phi_min = 12.0
        state.mask.phi_max = 56.0

        state.slice.start_time = [4.56778]
        state.slice.end_time = [12.373938]
        return state

    @staticmethod
    def _prepare_workspaces(number_of_workspaces, tagged_workspace_names=None, state=None):
        create_name = "CreateSampleWorkspace"
        create_options = {"OutputWorkspace": "test",
                          "NumBanks": 1,
                          "BankPixelWidth": 2,
                          "XMin": 1,
                          "XMax": 10,
                          "BinWidth": 2}
        create_alg = create_unmanaged_algorithm(create_name, **create_options)

        for index in range(number_of_workspaces):
            create_alg.execute()
            workspace = create_alg.getProperty("OutputWorkspace").value
            workspace_name = "test" + "_" + str(index)
            AnalysisDataService.addOrReplace(workspace_name, workspace)

        if tagged_workspace_names is not None:
            for key, value in tagged_workspace_names.items():
                create_alg.execute()
                workspace = create_alg.getProperty("OutputWorkspace").value
                AnalysisDataService.addOrReplace(value, workspace)
                write_hash_into_reduced_can_workspace(state, workspace, key)

    @staticmethod
    def _remove_workspaces():
        for element in AnalysisDataService.getObjectNames():
            AnalysisDataService.remove(element)

    def test_that_unknown_reduction_mode_raises(self):
        # Arrange
        state = StateFunctionsTest._get_state()

        # Act + Assert
        try:
            get_output_workspace_name(state, ISISReductionMode.All)
            did_raise = False
        except RuntimeError:
            did_raise = True
        self.assertTrue(did_raise)

    def test_that_creates_correct_workspace_name_for_1D(self):
        # Arrange
        state = StateFunctionsTest._get_state()
        # Act
        output_workspace = get_output_workspace_name(state, ISISReductionMode.LAB)
        # Assert
        self.assertTrue("12345rear_1D12.0_34.0Phi12.0_56.0_t4.57_T12.37" == output_workspace)

    def test_that_detects_if_all_entries_are_none_or_not_none_as_true(self):
        self.assertFalse(is_pure_none_or_not_none(["test", None, "test"]))
        self.assertTrue(is_pure_none_or_not_none([None, None, None]))
        self.assertTrue(is_pure_none_or_not_none(["test", "test", "test"]))
        self.assertTrue(is_pure_none_or_not_none([]))

    def test_that_detects_if_one_is_none(self):
        self.assertTrue(one_is_none(["test", None, "test"]))
        self.assertFalse(one_is_none([]))
        self.assertFalse(one_is_none(["test", "test", "test"]))

    def test_test_that_can_detect_when_first_is_larger_than_second(self):
        self.assertTrue(is_not_none_and_first_larger_than_second([1, 2, 3]))
        self.assertTrue(is_not_none_and_first_larger_than_second([2, 1]))
        self.assertFalse(is_not_none_and_first_larger_than_second([1, 2]))

    def test_that_produces_correct_validation_message(self):
        # Arrange
        error_message = "test message."
        instruction = "do this."
        variables = {"var1": 12,
                     "var2": "test"}
        # Act
        val_message = validation_message(error_message, instruction, variables)
        # Assert
        expected_text = "var1: 12\n" \
                        "var2: test\n" \
                        "" + instruction
        self.assertTrue(val_message.keys()[0] == error_message)
        self.assertTrue(val_message[error_message] == expected_text)

    def test_that_can_find_can_reduction_if_it_exists(self):
        # Arrange
        test_director = TestDirector()
        state = test_director.construct()
        tagged_workspace_names = {None: "test_ws",
                                  OutputParts.Count: "test_ws_count",
                                  OutputParts.Norm: "test_ws_norm"}
        StateFunctionsTest._prepare_workspaces(number_of_workspaces=4,
                                               tagged_workspace_names=tagged_workspace_names,
                                               state=state)
        # Act
        workspace, workspace_count, workspace_norm = get_reduced_can_workspace_from_ads(state, output_parts=True)

        # Assert
        self.assertTrue(workspace is not None)
        self.assertTrue(workspace.name() == AnalysisDataService.retrieve("test_ws").name())
        self.assertTrue(workspace_count is not None)
        self.assertTrue(workspace_count.name() == AnalysisDataService.retrieve("test_ws_count").name())
        self.assertTrue(workspace_norm is not None)
        self.assertTrue(workspace_norm.name() == AnalysisDataService.retrieve("test_ws_norm").name())

        # Clean up
        StateFunctionsTest._remove_workspaces()

    def test_that_returns_none_if_it_does_not_exist(self):
        # Arrange
        test_director = TestDirector()
        state = test_director.construct()
        StateFunctionsTest._prepare_workspaces(number_of_workspaces=4, tagged_workspace_names=None, state=state)

        # Act
        workspace, workspace_count, workspace_norm = get_reduced_can_workspace_from_ads(state, output_parts=False)

        # Assert
        self.assertTrue(workspace is None)
        self.assertTrue(workspace_count is None)
        self.assertTrue(workspace_norm is None)

        # Clean up
        StateFunctionsTest._remove_workspaces()

if __name__ == '__main__':
    unittest.main()
