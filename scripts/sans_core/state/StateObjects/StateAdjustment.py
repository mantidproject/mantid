# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the adjustment workspace creation of the SANS reduction."""

import copy

from sans_core.common.enums import SANSFacility
from sans_core.state.JsonSerializable import JsonSerializable
from sans_core.state.StateObjects.StateCalculateTransmission import StateCalculateTransmission
from sans_core.state.StateObjects.StateNormalizeToMonitor import StateNormalizeToMonitor
from sans_core.state.StateObjects.StateWavelengthAndPixelAdjustment import StateWavelengthAndPixelAdjustment
from sans_core.state.automatic_setters import automatic_setters


class StateAdjustment(metaclass=JsonSerializable):
    def __init__(self):
        super(StateAdjustment, self).__init__()
        self.calculate_transmission: StateCalculateTransmission = StateCalculateTransmission()
        self.normalize_to_monitor: StateNormalizeToMonitor = StateNormalizeToMonitor()
        self.wavelength_and_pixel_adjustment: StateWavelengthAndPixelAdjustment = StateWavelengthAndPixelAdjustment()
        self.wide_angle_correction: bool = False

        self.calibration = None  # : Str()

    def validate(self):
        is_invalid = {}

        # Calculate transmission
        if not self.calculate_transmission:
            is_invalid.update({"StateAdjustment": "The StateCalculateTransmission object is missing."})
        if self.calculate_transmission:
            try:
                self.calculate_transmission.validate()
            except ValueError as e:
                is_invalid.update({"StateAdjustment": "The sub-CalculateTransmission state is invalid, see here " + str(e)})

        # Normalize to monitor
        if not self.normalize_to_monitor:
            is_invalid.update({"StateAdjustment": "The StateNormalizeToMonitor object is missing."})
        if self.normalize_to_monitor:
            try:
                self.normalize_to_monitor.validate()
            except ValueError as e:
                is_invalid.update({"StateAdjustment": f"The sub-NormalizeToMonitor state is invalid, see here {str(e)}"})

        # Wavelength and pixel adjustment
        if not self.wavelength_and_pixel_adjustment:
            is_invalid.update({"StateAdjustment": "The StateWavelengthAndPixelAdjustment object is missing."})
        if self.wavelength_and_pixel_adjustment:
            try:
                self.wavelength_and_pixel_adjustment.validate()
            except ValueError as e:
                is_invalid.update({"StateAdjustment": f"The sub-WavelengthAndPixelAdjustment state is invalid, see here {str(e)}"})
        if is_invalid:
            raise ValueError("StateAdjustment: The provided inputs are illegal. Please see: {json.dumps(is_invalid)}")


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateAdjustmentBuilder(object):
    @automatic_setters(StateAdjustment)
    def __init__(self):
        super(StateAdjustmentBuilder, self).__init__()
        self.state = StateAdjustment()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_adjustment_builder(data_info):
    # The data state has most of the information that we require to define the adjustment. For the factory method, only
    # the instrument is of relevance.
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateAdjustmentBuilder()
    else:
        raise NotImplementedError(
            f"StateAdjustmentBuilder: Could not find any valid adjustment builder for the specified StateData object {str(data_info)}"
        )
