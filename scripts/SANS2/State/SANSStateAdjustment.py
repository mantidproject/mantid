# pylint: disable=too-few-public-methods

"""State describing the adjustment workspace creation of the SANS reduction."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, TypedParameter, sans_parameters, BoolParameter,
                                       validator_sub_state)
from SANS2.State.SANSStateCalculateTransmission import SANSStateCalculateTransmission
from SANS2.State.SANSStateNormalizeToMonitor import SANSStateNormalizeToMonitor
from SANS2.State.SANSStateWavelengthAndPixelAdjustment import SANSStateWavelengthAndPixelAdjustment


# ------------------------------------------------
# SANSStateAdjustment
# ------------------------------------------------
class SANSStateAdjustment(object):
    pass


@sans_parameters
class SANSStateAdjustmentISIS(SANSStateBase, SANSStateAdjustment):
    calculate_transmission = TypedParameter(SANSStateCalculateTransmission, validator_sub_state)
    normalize_to_monitor = TypedParameter(SANSStateNormalizeToMonitor, validator_sub_state)
    wavelength_and_pixel_adjustment = TypedParameter(SANSStateWavelengthAndPixelAdjustment, validator_sub_state)
    wide_angle_correction = BoolParameter()

    def __init__(self):
        super(SANSStateAdjustmentISIS, self).__init__()
        self.wide_angle_correction = False

    def validate(self):
        is_invalid = {}

        # Calculate transmission
        if not self.calculate_transmission:
            is_invalid.update({"SANSStateAdjustment": "The SANSStateCalculateTransmission object is missing."})
        if self.calculate_transmission:
            try:
                self.calculate_transmission.validate()
            except ValueError as e:
                is_invalid.update({"SANSStateAdjustment": "The sub-CalculateTransmission state is invalid,"
                                                          " see here {0}".format(str(e))})

        # Normalize to monitor
        if not self.normalize_to_monitor:
            is_invalid.update({"SANSStateAdjustment": "The SANSStateNormalizeToMonitor object is missing."})
        if self.normalize_to_monitor:
            try:
                self.normalize_to_monitor.validate()
            except ValueError as e:
                is_invalid.update({"SANSStateAdjustment": "The sub-NormalizeToMonitor state is invalid,"
                                                          " see here {0}".format(str(e))})

        # Wavelength and pixel adjustment
        if not self.wavelength_and_pixel_adjustment:
            is_invalid.update({"SANSStateAdjustment": "The SANSStateWavelengthAndPixelAdjustment object is missing."})
        if self.wavelength_and_pixel_adjustment:
            try:
                self.wavelength_and_pixel_adjustment.validate()
            except ValueError as e:
                is_invalid.update({"SANSStateAdjustment": "The sub-WavelengthAndPixelAdjustment state is invalid,"
                                                          " see here {0}".format(str(e))})
        if is_invalid:
            raise ValueError("SANSStateAdjustmentISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# -----------------------------------------------
# SANSStateAdjustment setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateAdjustment and SANSStateBase and fulfill its contract.
# -----------------------------------------------
