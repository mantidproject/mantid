import unittest
import mantid

from SANS2.State.SANSState import (SANSStateISIS, SANSState)
from SANS2.State.SANSStateData import (SANSStateData)
from SANS2.State.SANSStateMove import (SANSStateMove)
from SANS2.State.SANSStateReduction import (SANSStateReduction)
from SANS2.State.SANSStateSliceEvent import (SANSStateSliceEvent)
from SANS2.State.SANSStateMask import (SANSStateMask)
from SANS2.State.SANSStateWavelength import (SANSStateWavelength)
from SANS2.State.SANSStateSave import (SANSStateSave)
from SANS2.State.SANSStateNormalizeToMonitor import (SANSStateNormalizeToMonitor)
from SANS2.State.SANSStateScale import (SANSStateScale)
from SANS2.State.SANSStateCalculateTransmission import (SANSStateCalculateTransmission)
from SANS2.State.SANSStateWavelengthAndPixelAdjustment import (SANSStateWavelengthAndPixelAdjustment)
from SANS2.State.SANSStateAdjustment import (SANSStateAdjustment)
from SANS2.State.SANSStateConvertToQ import (SANSStateConvertToQ)

from StateTestHelper import assert_validate_error, assert_raises_nothing


class MockState(object):
    def validate(self):
        pass


class MockStateData(SANSStateData, MockState):
    pass


class MockStateMove(SANSStateMove, MockState):
    pass


class MockStateReduction(SANSStateReduction, MockState):
    def get_merge_strategy(self):
        pass

    def get_detector_name_for_reduction_mode(self, reduction_mode):
        pass

    def get_all_reduction_modes(self):
        pass


class MockStateSliceEvent(SANSStateSliceEvent, MockState):
    pass


class MockStateMask(SANSStateMask, MockState):
    pass


class MockStateWavelength(SANSStateWavelength, MockState):
    pass


class MockStateSave(SANSStateSave, MockState):
    pass


class MockStateNormalizeToMonitor(SANSStateNormalizeToMonitor, MockState):
    pass


class MockStateScale(SANSStateScale, MockState):
    pass


class MockStateCalculateTransmission(SANSStateCalculateTransmission, MockState):
    pass


class MockStateWavelengthAndPixelAdjustment(SANSStateWavelengthAndPixelAdjustment, MockState):
    pass


class MockStateAdjustment(SANSStateAdjustment, MockState):
    pass


class MockStateConvertToQ(SANSStateConvertToQ, MockState):
    pass


class SANSStateTest(unittest.TestCase):
    @staticmethod
    def _get_state(entries):
        state = SANSStateISIS()
        default_entries = {"data": MockStateData(), "move": MockStateMove(), "reduction": MockStateReduction(),
                           "slice": MockStateSliceEvent(), "mask": MockStateMask(), "wavelength": MockStateWavelength(),
                           "save": MockStateSave(), "scale": MockStateScale(), "adjustment": MockStateAdjustment(),
                           "convert_to_q": MockStateConvertToQ()}

        for key, value in default_entries.items():
            if key in entries:
                value = entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)
        return state

    def check_bad_and_good_values(self, bad_state, good_state):
        # Bad values
        state = self._get_state(bad_state)
        assert_validate_error(self, ValueError, state)

        # Good values
        state = self._get_state(good_state)
        assert_raises_nothing(self, state)

    def test_that_is_sans_state_object(self):
        state = SANSStateISIS()
        self.assertTrue(isinstance(state, SANSState))

    def test_that_raises_when_data_has_not_been_set(self):
        self.check_bad_and_good_values({"data": None}, {"data": MockStateData()})

    def test_that_raises_when_move_has_not_been_set(self):
        self.check_bad_and_good_values({"move": None}, {"move": MockStateMove()})

    def test_that_raises_when_reduction_has_not_been_set(self):
        self.check_bad_and_good_values({"reduction": None}, {"reduction": MockStateReduction()})

    def test_that_raises_when_slice_has_not_been_set(self):
        self.check_bad_and_good_values({"slice": None}, {"slice": MockStateSliceEvent()})

    def test_that_raises_when_mask_has_not_been_set(self):
        self.check_bad_and_good_values({"mask": None}, {"mask": MockStateMask()})

    def test_that_raises_when_wavelength_has_not_been_set(self):
        self.check_bad_and_good_values({"wavelength": None}, {"wavelength": MockStateWavelength()})

    def test_that_raises_when_save_has_not_been_set(self):
        self.check_bad_and_good_values({"save": None}, {"save": MockStateSave()})

    def test_that_raises_when_scale_has_not_been_set(self):
        self.check_bad_and_good_values({"scale": None}, {"scale": MockStateScale()})

    def test_that_raises_when_adjustment_has_not_been_set(self):
        self.check_bad_and_good_values({"adjustment": None}, {"adjustment": MockStateAdjustment()})

    def test_that_raises_when_convert_to_q_has_not_been_set(self):
        self.check_bad_and_good_values({"convert_to_q": None}, {"convert_to_q": MockStateConvertToQ()})


if __name__ == '__main__':
    unittest.main()
