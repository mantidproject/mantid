import unittest
import mantid

from SANS2.State.SANSStateConvertToQ import (SANSStateConvertToQ, SANSStateConvertToQISIS)
from SANS2.Common.SANSType import (ReductionDimensionality, RangeStepType)
from StateTestHelper import (assert_validate_error, assert_raises_nothing)


class SANSStateConvertToQTest(unittest.TestCase):
    @staticmethod
    def _get_convert_to_q_state(convert_to_q_entries):
        state = SANSStateConvertToQISIS()
        default_entries = {"reduction_dimensionality": ReductionDimensionality.OneDim, "use_gravity": True,
                           "gravity_extra_length": 12., "radius_cutoff": 1.5, "wavelength_cutoff": 2.7,
                           "q_min": 0.5, "q_max": 1., "q_step": 1., "q_step_type": RangeStepType.Lin,
                           "q_step2": 1., "q_step_type2": RangeStepType.Lin, "q_mid": 1.,
                           "q_xy_max": 1.4, "q_xy_step": 24.5, "q_xy_step_type": RangeStepType.Lin,
                           "use_q_resolution": True, "q_resolution_collimation_length": 12.,
                           "q_resolution_delta_r": 12., "moderator_file": "test.txt", "q_resolution_a1": 1.,
                           "q_resolution_a2": 2., "q_resolution_h1": 1., "q_resolution_h2": 2., "q_resolution_w1": 1.,
                           "q_resolution_w2": 2.}

        for key, value in default_entries.items():
            if key in convert_to_q_entries:
                value = convert_to_q_entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)
        return state

    def check_bad_and_good_value(self, bad_convert_to_q, good_convert_to_q):
        # Bad values
        state = self._get_convert_to_q_state(bad_convert_to_q)
        assert_validate_error(self, ValueError, state)

        # Good values
        state = self._get_convert_to_q_state(good_convert_to_q)
        assert_raises_nothing(self, state)

    def test_that_is_sans_state_data_object(self):
        state = SANSStateConvertToQISIS()
        self.assertTrue(isinstance(state, SANSStateConvertToQ))

    def test_that_raises_with_inconsistent_1D_q_values(self):
        self.check_bad_and_good_value({"q_min": None, "q_max": 2.}, {"q_min": 1., "q_max": 2.})

    def test_that_raises_when_the_lower_bound_is_larger_than_the_upper_bound_for_q_1D(self):
        self.check_bad_and_good_value({"q_min": 2., "q_max": 1.}, {"q_min": 1., "q_max": 2.})

    def test_that_raises_when_no_q_bounds_are_set_for_explicit_1D_reduction(self):
        self.check_bad_and_good_value({"q_min": None, "q_max": None,
                                       "reduction_dimensionality": ReductionDimensionality.OneDim},
                                      {"q_min": 1., "q_max": 2.,
                                       "reduction_dimensionality": ReductionDimensionality.OneDim})

    def test_that_raises_when_no_q_bounds_are_set_for_explicit_2D_reduction(self):
        self.check_bad_and_good_value({"q_xy_max": None, "q_xy_step": None,
                                       "reduction_dimensionality": ReductionDimensionality.TwoDim},
                                      {"q_xy_max": 1., "q_xy_step": 2.,
                                       "reduction_dimensionality": ReductionDimensionality.TwoDim})

    def test_that_raises_when_inconsistent_circular_values_for_q_resolution_are_specified(self):
        self.check_bad_and_good_value({"use_q_resolution": True, "q_resolution_a1": None,
                                       "q_resolution_a2": 12.},
                                      {"use_q_resolution": True, "q_resolution_a1": 11.,
                                       "q_resolution_a2": 12.})

    def test_that_raises_when_inconsistent_rectangular_values_for_q_resolution_are_specified(self):
        self.check_bad_and_good_value({"use_q_resolution": True, "q_resolution_h1": None,
                                       "q_resolution_h2": 12., "q_resolution_w1": 1., "q_resolution_w2": 2.},
                                      {"use_q_resolution": True, "q_resolution_h1": 1.,
                                       "q_resolution_h2": 12., "q_resolution_w1": 1., "q_resolution_w2": 2.})

    def test_that_raises_when_no_geometry_for_q_resolution_was_specified(self):
        self.check_bad_and_good_value({"use_q_resolution": True, "q_resolution_h1": None, "q_resolution_a1": None,
                                       "q_resolution_a2": None, "q_resolution_h2": None, "q_resolution_w1": None,
                                       "q_resolution_w2": None},
                                      {"use_q_resolution": True, "q_resolution_h1": 1., "q_resolution_a1": 1.,
                                       "q_resolution_a2": 2., "q_resolution_h2": 12., "q_resolution_w1": 1.,
                                       "q_resolution_w2": 2.})

    def test_that_raises_when_moderator_file_has_not_been_set(self):
        self.check_bad_and_good_value({"moderator_file": None}, {"moderator_file": "test"})


if __name__ == '__main__':
    unittest.main()
