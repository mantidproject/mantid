# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the creation of pixel and wavelength adjustment workspaces for SANS reduction."""

import copy
import json

from sans.common.enums import RangeStepType, DetectorType
from sans.state.JsonSerializable import JsonSerializable
from sans.state.StateObjects.wavelength_interval import WavelengthInterval
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import one_is_none, validation_message


class StateAdjustmentFiles(metaclass=JsonSerializable):
    def __init__(self):
        super(StateAdjustmentFiles, self).__init__()
        self.pixel_adjustment_file = None  # : Str()
        self.wavelength_adjustment_file = None  # : Str()

    def validate(self):
        is_invalid = {}
        # TODO: It would be nice to have a typed parameter for files which checks if a file input exists or not.
        #       This is very low priority, but would be nice to have.

        if is_invalid:
            raise ValueError("StateAdjustmentFiles: The provided inputs are illegal. " "Please see: {0}".format(json.dumps(is_invalid)))


class StateWavelengthAndPixelAdjustment(metaclass=JsonSerializable):
    def __init__(self):
        super(StateWavelengthAndPixelAdjustment, self).__init__()
        self.wavelength_interval: WavelengthInterval = WavelengthInterval()
        self.wavelength_step_type = RangeStepType.NOT_SET

        self.idf_path = None  # : Str()

        self.adjustment_files = {DetectorType.LAB.value: StateAdjustmentFiles(), DetectorType.HAB.value: StateAdjustmentFiles()}

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

    def validate(self):
        is_invalid = {}

        if one_is_none([self.wavelength_interval, self.wavelength_step_type]):
            entry = validation_message(
                "A wavelength entry has not been set.",
                "Make sure that all entries are set.",
                {"wavelength_low": self.wavelength_interval, "wavelength_step_type": self.wavelength_step_type},
            )
            is_invalid.update(entry)

        if self.wavelength_step_type is RangeStepType.NOT_SET:
            entry = validation_message(
                "A wavelength entry has not been set.",
                "Make sure that all entries are set.",
                {"wavelength_step_type": self.wavelength_step_type},
            )
            is_invalid.update(entry)

        try:
            self.adjustment_files[DetectorType.LAB.value].validate()
            self.adjustment_files[DetectorType.HAB.value].validate()
        except ValueError as e:
            is_invalid.update({"adjustment_files": str(e)})

        if is_invalid:
            raise ValueError(
                "StateWavelengthAndPixelAdjustment: The provided inputs are illegal. " "Please see: {0}".format(json.dumps(is_invalid))
            )


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthAndPixelAdjustmentBuilder(object):
    @automatic_setters(StateWavelengthAndPixelAdjustment, exclusions=["idf_path"])
    def __init__(self, data_info):
        super(StateWavelengthAndPixelAdjustmentBuilder, self).__init__()
        idf_file_path = data_info.idf_file_path
        self.state = StateWavelengthAndPixelAdjustment()
        self.state.idf_path = idf_file_path

    def build(self):
        return copy.copy(self.state)

    def set_wavelength_step_type(self, val):
        self.state.wavelength_step_type = val


def get_wavelength_and_pixel_adjustment_builder(data_info):
    return StateWavelengthAndPixelAdjustmentBuilder(data_info=data_info)
