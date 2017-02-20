# pylint: disable=too-few-public-methods

"""State describing the creation of pixel and wavelength adjustment workspaces for SANS reduction."""

import json
import copy
from sans.state.state_base import (StateBase, rename_descriptor_names, StringParameter,
                                   ClassTypeParameter, PositiveFloatParameter, DictParameter)
from sans.state.state_functions import (is_not_none_and_first_larger_than_second, one_is_none, validation_message)
from sans.common.enums import (RangeStepType, DetectorType, SANSInstrument)
from sans.state.automatic_setters import (automatic_setters)


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
@rename_descriptor_names
class StateAdjustmentFiles(StateBase):
    pixel_adjustment_file = StringParameter()
    wavelength_adjustment_file = StringParameter()

    def __init__(self):
        super(StateAdjustmentFiles, self).__init__()

    def validate(self):
        is_invalid = {}
        # TODO if a file was specified then make sure that its existence is checked.

        if is_invalid:
            raise ValueError("StateAdjustmentFiles: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


@rename_descriptor_names
class StateWavelengthAndPixelAdjustment(StateBase):
    wavelength_low = PositiveFloatParameter()
    wavelength_high = PositiveFloatParameter()
    wavelength_step = PositiveFloatParameter()
    wavelength_step_type = ClassTypeParameter(RangeStepType)

    adjustment_files = DictParameter()

    def __init__(self):
        super(StateWavelengthAndPixelAdjustment, self).__init__()
        self.adjustment_files = {DetectorType.to_string(DetectorType.LAB): StateAdjustmentFiles(),
                                 DetectorType.to_string(DetectorType.HAB): StateAdjustmentFiles()}

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
            self.adjustment_files[DetectorType.to_string(DetectorType.LAB)].validate()
            self.adjustment_files[DetectorType.to_string(DetectorType.HAB)].validate()
        except ValueError as e:
            is_invalid.update({"adjustment_files": str(e)})

        if is_invalid:
            raise ValueError("StateWavelengthAndPixelAdjustment: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthAndPixelAdjustmentBuilder(object):
    @automatic_setters(StateWavelengthAndPixelAdjustment)
    def __init__(self):
        super(StateWavelengthAndPixelAdjustmentBuilder, self).__init__()
        self.state = StateWavelengthAndPixelAdjustment()

    def build(self):
        self.state.validate()
        return copy.copy(self.state)


def get_wavelength_and_pixel_adjustment_builder(data_info):
    instrument = data_info.instrument
    if instrument is SANSInstrument.LARMOR or instrument is SANSInstrument.SANS2D or instrument is SANSInstrument.LOQ:
        return StateWavelengthAndPixelAdjustmentBuilder()
    else:
        raise NotImplementedError("StateWavelengthAndPixelAdjustmentBuilder: Could not find any valid "
                                  "wavelength and pixel adjustment builder for the specified "
                                  "StateData object {0}".format(str(data_info)))
