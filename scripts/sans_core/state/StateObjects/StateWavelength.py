# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines the state of the event slices which should be reduced."""

import json
import copy

from sans_core.state.JsonSerializable import JsonSerializable
from sans_core.common.enums import RangeStepType, SANSFacility
from sans_core.state.StateObjects.wavelength_interval import WavelengthInterval
from sans_core.state.automatic_setters import automatic_setters
from sans_core.state.state_functions import one_is_none, validation_message


class StateWavelength(metaclass=JsonSerializable):
    def __init__(self):
        super(StateWavelength, self).__init__()
        self.wavelength_interval: WavelengthInterval = WavelengthInterval()
        self.wavelength_step_type = RangeStepType.NOT_SET

    @property
    def wavelength_step_type_lin_log(self):
        # Return the wavelength step type, converting RANGE_LIN/RANGE_LOG to
        # LIN/LOG. This is not ideal but is required for workflow algorithms
        # which only accept a subset of the values in the enum
        value = self.wavelength_step_type
        result = (
            RangeStepType.LIN
            if value in [RangeStepType.LIN, RangeStepType.RANGE_LIN]
            else RangeStepType.LOG
            if value in [RangeStepType.LOG, RangeStepType.RANGE_LOG]
            else RangeStepType.NOT_SET
        )
        return result

    @property
    def wavelength_step_type_range(self):
        # Return the wavelength step type, converting LIN/LOG to
        # RANGE_LIN/RANGE_LOG.
        value = self.wavelength_step_type
        result = (
            RangeStepType.RANGE_LIN
            if value in [RangeStepType.LIN, RangeStepType.RANGE_LIN]
            else RangeStepType.RANGE_LOG
            if value in [RangeStepType.LOG, RangeStepType.RANGE_LOG]
            else RangeStepType.NOT_SET
        )
        return result

    def validate(self):
        is_invalid = dict()
        if one_is_none([self.wavelength_interval]):
            entry = validation_message(
                "A wavelength entry has not been set.",
                "Make sure that all entries for the wavelength are set.",
                {"wavelength_binning": self.wavelength_interval},
            )
            is_invalid.update(entry)

        if is_invalid:
            raise ValueError("StateWavelength: The provided inputs are illegal. Please see: {0}".format(json.dumps(is_invalid, indent=4)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthBuilder(object):
    @automatic_setters(StateWavelength)
    def __init__(self):
        super(StateWavelengthBuilder, self).__init__()
        self.state = StateWavelength()

    def build(self):
        # Make sure that the product is in a valid state, ie not incomplete
        self.state.validate()
        return copy.copy(self.state)

    def set_wavelength_step_type(self, val):
        self.state.wavelength_step_type = val


def get_wavelength_builder(data_info):
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateWavelengthBuilder()
    else:
        raise NotImplementedError(
            f"StateWavelengthBuilder: Could not find any valid wavelength builder for the specified StateData object {str(data_info)}"
        )
