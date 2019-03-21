# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the adjustment workspace creation of the SANS reduction."""

from __future__ import (absolute_import, division, print_function)
import json
import copy
from sans.state.state_base import (StateBase, TypedParameter, rename_descriptor_names, BoolParameter,
                                   validator_sub_state)
from sans.state.calculate_transmission import StateCalculateTransmission
from sans.state.normalize_to_monitor import StateNormalizeToMonitor
from sans.state.wavelength_and_pixel_adjustment import StateWavelengthAndPixelAdjustment
from sans.state.automatic_setters import (automatic_setters)
from sans.common.enums import SANSFacility


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateAdjustment(StateBase):
    calculate_transmission = TypedParameter(StateCalculateTransmission, validator_sub_state)
    normalize_to_monitor = TypedParameter(StateNormalizeToMonitor, validator_sub_state)
    wavelength_and_pixel_adjustment = TypedParameter(StateWavelengthAndPixelAdjustment, validator_sub_state)
    wide_angle_correction = BoolParameter()

    def __init__(self):
        super(StateAdjustment, self).__init__()
        self.wide_angle_correction = False

    def validate(self):
        is_invalid = {}

        # Calculate transmission
        if not self.calculate_transmission:
            is_invalid.update({"StateAdjustment": "The StateCalculateTransmission object is missing."})
        if self.calculate_transmission:
            try:
                self.calculate_transmission.validate()
            except ValueError as e:
                is_invalid.update({"StateAdjustment": "The sub-CalculateTransmission state is invalid,"
                                                      " see here {0}".format(str(e))})

        # Normalize to monitor
        if not self.normalize_to_monitor:
            is_invalid.update({"StateAdjustment": "The StateNormalizeToMonitor object is missing."})
        if self.normalize_to_monitor:
            try:
                self.normalize_to_monitor.validate()
            except ValueError as e:
                is_invalid.update({"StateAdjustment": "The sub-NormalizeToMonitor state is invalid,"
                                                      " see here {0}".format(str(e))})

        # Wavelength and pixel adjustment
        if not self.wavelength_and_pixel_adjustment:
            is_invalid.update({"StateAdjustment": "The StateWavelengthAndPixelAdjustment object is missing."})
        if self.wavelength_and_pixel_adjustment:
            try:
                self.wavelength_and_pixel_adjustment.validate()
            except ValueError as e:
                is_invalid.update({"StateAdjustment": "The sub-WavelengthAndPixelAdjustment state is invalid,"
                                                      " see here {0}".format(str(e))})
        if is_invalid:
            raise ValueError("StateAdjustment: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


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
        raise NotImplementedError("StateAdjustmentBuilder: Could not find any valid adjustment builder for the "
                                  "specified StateData object {0}".format(str(data_info)))
