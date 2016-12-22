import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm

from SANS2.State.SANSStateReduction import (SANSStateReduction, SANSStateReductionISIS)
from SANS2.Common.SANSEnumerations import (ISISReductionMode, ReductionDimensionality, FitModeForMerge)
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSConstants import SANSConstants


class SANSStateReductionTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateReductionISIS()
        self.assertTrue(isinstance(state, SANSStateReduction))

    def test_that_reduction_state_gets_and_sets(self):
        # Arrange
        state = SANSStateReductionISIS()

        # Act
        state.reduction_mode = ISISReductionMode.Merged
        state.dimensionality = ReductionDimensionality.TwoDim
        state.merge_shift = 12.65
        state.merge_scale = 34.6
        state.merge_fit_mode = FitModeForMerge.ShiftOnly

        # Assert
        self.assertTrue(state.reduction_mode is ISISReductionMode.Merged)
        self.assertTrue(state.dimensionality is ReductionDimensionality.TwoDim)
        self.assertTrue(state.merge_shift == 12.65)
        self.assertTrue(state.merge_scale == 34.6)
        self.assertTrue(state.merge_fit_mode == FitModeForMerge.ShiftOnly)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateReductionISIS()

        # Act and Assert
        try:
            state.merge_fit_mode = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_that_dict_can_be_generated_from_state_object_and_property_manager_read_in(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass

        # Arrange
        state = SANSStateReductionISIS()
        state.reduction_mode = ISISReductionMode.Hab
        state.dimensionality = ReductionDimensionality.TwoDim
        state.merge_shift = 12.65
        state.merge_scale = 34.6
        state.merge_fit_mode = FitModeForMerge.ShiftOnly

        # Act
        serialized = state.property_manager
        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        self.assertTrue(type(serialized) == dict)
        self.assertTrue(type(property_manager) == PropertyManager)
        state_2 = create_deserialized_sans_state_from_property_manager(property_manager)
        state_2.property_manager = property_manager

        self.assertTrue(state_2.reduction_mode == ISISReductionMode.Hab)
        self.assertTrue(state_2.dimensionality == ReductionDimensionality.TwoDim)
        self.assertTrue(state_2.merge_shift == 12.65)
        self.assertTrue(state_2.merge_scale == 34.6)
        self.assertTrue(state_2.merge_fit_mode == FitModeForMerge.ShiftOnly)

    def test_that_converter_methods_work(self):
        # Arrange
        state = SANSStateReductionISIS()

        state.reduction_mode = ISISReductionMode.Merged
        state.dimensionality = ReductionDimensionality.TwoDim
        state.merge_shift = 12.65
        state.merge_scale = 34.6
        state.merge_fit_mode = FitModeForMerge.ShiftOnly

        state.detector_names[SANSConstants.low_angle_bank] = "Test1"
        state.detector_names[SANSConstants.high_angle_bank] = "Test2"

        # Assert
        merge_strategy = state.get_merge_strategy()
        self.assertTrue(merge_strategy[0] is ISISReductionMode.Lab)
        self.assertTrue(merge_strategy[1] is ISISReductionMode.Hab)

        all_reductions = state.get_all_reduction_modes()
        self.assertTrue(len(all_reductions) == 2)
        self.assertTrue(all_reductions[0] is ISISReductionMode.Lab)
        self.assertTrue(all_reductions[1] is ISISReductionMode.Hab)

        result_lab = state.get_detector_name_for_reduction_mode(ISISReductionMode.Lab)
        self.assertTrue(result_lab == "Test1")
        result_hab = state.get_detector_name_for_reduction_mode(ISISReductionMode.Hab)
        self.assertTrue(result_hab == "Test2")

        self.assertRaises(RuntimeError, state.get_detector_name_for_reduction_mode, "non_sense")


if __name__ == '__main__':
    unittest.main()
