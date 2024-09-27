# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans_core.common.enums import SANSInstrument, SANSFacility
from sans_core.state.StateObjects.StateAdjustment import StateAdjustment
from sans_core.state.StateObjects.StateCalculateTransmission import StateCalculateTransmission
from sans_core.state.StateObjects.StateConvertToQ import StateConvertToQ
from sans_core.state.StateObjects.StateData import StateData
from sans_core.state.StateObjects.StateMaskDetectors import StateMask
from sans_core.state.StateObjects.StateMoveDetectors import StateMove
from sans_core.state.StateObjects.StateNormalizeToMonitor import StateNormalizeToMonitor
from sans_core.state.StateObjects.StateReductionMode import StateReductionMode
from sans_core.state.StateObjects.StateSave import StateSave
from sans_core.state.StateObjects.StateScale import StateScale
from sans_core.state.StateObjects.StateSliceEvent import StateSliceEvent
from sans_core.state.AllStates import AllStates
from sans_core.state.StateObjects.StateWavelength import StateWavelength
from sans_core.state.StateObjects.StateWavelengthAndPixelAdjustment import StateWavelengthAndPixelAdjustment


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
        state = AllStates()
        default_entries = {
            "data": MockStateData(),
            "move": MockStateMove(),
            "reduction": MockStateReduction(),
            "slice": MockStateSliceEvent(),
            "mask": MockStateMask(),
            "wavelength": MockStateWavelength(),
            "save": MockStateSave(),
            "scale": MockStateScale(),
            "adjustment": MockStateAdjustment(),
            "convert_to_q": MockStateConvertToQ(),
        }
        default_entries["data"].instrument = SANSInstrument.LARMOR
        default_entries["data"].facility = SANSFacility.ISIS

        for key, value in list(default_entries.items()):
            if key in entries:
                value = entries[key]
            if value is not None:  # If the value is None, then don't set it
                setattr(state, key, value)
        return state

    def check_bad_and_good_values(self, bad_state, good_state):
        # Bad values
        state = self._get_state(bad_state)
        with self.assertRaises(ValueError):
            state.validate()

        # Good values
        state = self._get_state(good_state)
        self.assertIsNone(state.validate())


if __name__ == "__main__":
    unittest.main()
