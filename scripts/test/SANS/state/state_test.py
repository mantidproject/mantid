from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.state import (State)
from sans.state.data import (StateData)
from sans.state.move import (StateMove)
from sans.state.reduction_mode import (StateReductionMode)
from sans.state.slice_event import (StateSliceEvent)
from sans.state.mask import (StateMask)
from sans.state.wavelength import (StateWavelength)
from sans.state.save import (StateSave)
from sans.state.normalize_to_monitor import (StateNormalizeToMonitor)
from sans.state.scale import (StateScale)
from sans.state.calculate_transmission import (StateCalculateTransmission)
from sans.state.wavelength_and_pixel_adjustment import (StateWavelengthAndPixelAdjustment)
from sans.state.adjustment import (StateAdjustment)
from sans.state.convert_to_q import (StateConvertToQ)

from state_test_helper import assert_validate_error, assert_raises_nothing
from sans.common.enums import SANSInstrument

# ----------------------------------------------------------------------------------------------------------------------
#  State
# ----------------------------------------------------------------------------------------------------------------------
class MockStateData(StateData):
    def validate(self):
        pass


class MockStateMove(StateMove):
    def validate(self):
        pass


class MockStateReduction(StateReductionMode):
    def get_merge_strategy(self):
        pass

    def get_detector_name_for_reduction_mode(self, reduction_mode):
        pass

    def get_all_reduction_modes(self):
        pass

    def validate(self):
        pass


class MockStateSliceEvent(StateSliceEvent):
    def validate(self):
        pass


class MockStateMask(StateMask):
    def validate(self):
        pass


class MockStateWavelength(StateWavelength):
    def validate(self):
        pass


class MockStateSave(StateSave):
    def validate(self):
        pass


class MockStateNormalizeToMonitor(StateNormalizeToMonitor):
    def validate(self):
        pass


class MockStateScale(StateScale):
    def validate(self):
        pass


class MockStateCalculateTransmission(StateCalculateTransmission):
    def validate(self):
        pass


class MockStateWavelengthAndPixelAdjustment(StateWavelengthAndPixelAdjustment):
    def validate(self):
        pass


class MockStateAdjustment(StateAdjustment):
    def validate(self):
        pass


class MockStateConvertToQ(StateConvertToQ):
    def validate(self):
        pass


class StateTest(unittest.TestCase):
    @staticmethod
    def _get_state(entries):
        state = State()
        default_entries = {"data": MockStateData(), "move": MockStateMove(), "reduction": MockStateReduction(),
                           "slice": MockStateSliceEvent(), "mask": MockStateMask(), "wavelength": MockStateWavelength(),
                           "save": MockStateSave(), "scale": MockStateScale(), "adjustment": MockStateAdjustment(),
                           "convert_to_q": MockStateConvertToQ()}
        default_entries["data"].instrument = SANSInstrument.LARMOR

        for key, value in list(default_entries.items()):
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
