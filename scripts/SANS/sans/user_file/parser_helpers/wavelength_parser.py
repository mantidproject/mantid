# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Tuple

from mantid.py36compat import dataclass
from sans.common.enums import RangeStepType
from sans.common.general_functions import get_ranges_from_event_slice_setting
from sans.state.StateObjects.StateCalculateTransmission import StateCalculateTransmission
from sans.state.StateObjects.StateWavelength import StateWavelength
from sans.state.StateObjects.StateWavelengthAndPixelAdjustment import StateWavelengthAndPixelAdjustment
from sans.state.StateObjects.wavelength_interval import WavRangePairs, WavRange
from sans.user_file.parser_helpers.toml_parser_impl_base import TomlParserImplBase


@dataclass(init=True)
class DuplicateWavelengthStates:
    """
    These classes contain duplicated attributes, so this POD
    class ties them together for subsequent setters
    """

    transmission: StateCalculateTransmission
    wavelength: StateWavelength
    pixel: StateWavelengthAndPixelAdjustment

    def iterate_fields(self):
        return [self.transmission, self.wavelength, self.pixel]


def parse_range_wavelength(wavelength_range: str) -> Tuple[WavRange, WavRangePairs]:
    """
    Parses a wavelength range and outputs a tuple of wavelength_start
    and wavelength_stop in the format StateObjects expect.
    This method exists to unify the GUI and TOML file parsing into a single place
    return: full wavelength as tuple - (min, max), wavelength pairs - e.g. [(1., 2.), (2., 4.)]
    """
    wavelength_pairs = get_ranges_from_event_slice_setting(wavelength_range)
    full_wavelength = (min(wavelength_pairs, key=lambda v: v[0])[0], max(wavelength_pairs, key=lambda v: v[1])[1])
    wavelength_pairs.append(full_wavelength)
    return full_wavelength, wavelength_pairs


class WavelengthTomlParser(TomlParserImplBase):
    def __init__(self, toml_dict):
        super(WavelengthTomlParser, self).__init__(toml_dict)

    def set_wavelength_details(self, state_objs: DuplicateWavelengthStates):
        binning_dict = self.get_val(["binning"])

        if not self.get_val("wavelength", binning_dict):
            return

        step_type = RangeStepType(self.get_mandatory_val(["binning", "wavelength", "type"]))

        wavelength_step = float(self.get_mandatory_val(["binning", "wavelength", "step"]))
        for i in state_objs.iterate_fields():
            i.wavelength_interval.wavelength_step = wavelength_step
            i.wavelength_step_type = step_type

        if step_type in (RangeStepType.RANGE_LIN, RangeStepType.RANGE_LOG):
            self._parse_range_wavelength(state_objs)
        else:
            assert step_type in (RangeStepType.LIN, RangeStepType.LOG)
            self._parse_linear_wavelength(state_objs)

    def _parse_range_wavelength(self, state_objs):
        range_binning = self.get_mandatory_val(["binning", "wavelength", "binning"])
        full_range, pairs = parse_range_wavelength(range_binning)

        for state_obj in state_objs.iterate_fields():
            state_obj.wavelength_interval.wavelength_full_range = full_range
            state_obj.wavelength_interval.selected_ranges = pairs

    def _parse_linear_wavelength(self, state_objs):
        wavelength_start = self.get_mandatory_val(["binning", "wavelength", "start"])
        wavelength_stop = self.get_mandatory_val(["binning", "wavelength", "stop"])

        for state_obj in state_objs.iterate_fields():
            state_obj.wavelength_interval.wavelength_full_range = (wavelength_start, wavelength_stop)
