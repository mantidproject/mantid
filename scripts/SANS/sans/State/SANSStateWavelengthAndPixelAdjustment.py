# pylint: disable=too-few-public-methods

"""State describing the creation of pixel and wavelength adjustment workspaces for SANS reduction."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, sans_parameters, StringParameter,
                                       ClassTypeParameter, PositiveFloatParameter, DictParameter)
from SANS2.State.SANSStateFunctions import (is_not_none_and_first_larger_than_second, one_is_none, validation_message)
from SANS2.Common.SANSType import (RangeStepType, DetectorType, convert_detector_type_to_string)


# ------------------------------------------------
# SANSStateAdjustment
# ------------------------------------------------
class SANSStateWavelengthAndPixelAdjustment(object):
    pass


@sans_parameters
class SANSStateAdjustmentFiles(SANSStateBase):
    pixel_adjustment_file = StringParameter()
    wavelength_adjustment_file = StringParameter()

    def __init__(self):
        super(SANSStateAdjustmentFiles, self).__init__()

    def validate(self):
        is_invalid = {}
        # TODO if a file was specified then make sure that its existence is checked.

        if is_invalid:
            raise ValueError("SANSStateAdjustmentFiles: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@sans_parameters
class SANSStateWavelengthAndPixelAdjustmentISIS(SANSStateBase, SANSStateWavelengthAndPixelAdjustment):
    wavelength_low = PositiveFloatParameter()
    wavelength_high = PositiveFloatParameter()
    wavelength_step = PositiveFloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    adjustment_files = DictParameter()

    def __init__(self):
        super(SANSStateWavelengthAndPixelAdjustmentISIS, self).__init__()
        self.adjustment_files = {convert_detector_type_to_string(DetectorType.Lab): SANSStateAdjustmentFiles(),
                                 convert_detector_type_to_string(DetectorType.Hab): SANSStateAdjustmentFiles()}

    def validate(self):
        is_invalid = {}

        if one_is_none([self.wavelength_low, self.wavelength_high, self.wavelength_step, self.wavelength_step_type]):
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high,
                                        "wavelength_step": self.wavelength_step,
                                        "wavelength_step_type": self.wavelength_step_type})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Incorrect wavelength bounds.",
                                       "Make sure that lower wavelength bound is smaller then upper bound.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
            is_invalid.update(entry)

        try:
            self.adjustment_files[convert_detector_type_to_string(DetectorType.Lab)].validate()
            self.adjustment_files[convert_detector_type_to_string(DetectorType.Hab)].validate()
        except ValueError as e:
            is_invalid.update({"adjustment_files": str(e)})

        if is_invalid:
            raise ValueError("SANSStateWavelengthAndPixelAdjustmentISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# -----------------------------------------------------------------------------------------
# SANSStateWavelengthAndPixelAdjustment setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateWavelengthAndPixelAdjustment and SANSStateBase and fulfill its contract.
# -------------------------------------------------------------------------------------------
