# pylint: disable=too-few-public-methods

"""State describing the masking behaviour of the SANS reduction."""

import json
from SANS2.State.SANSStateBase import (SANSStateBase, BoolParameter, StringListParameter, StringParameter,
                                       PositiveFloatParameter, FloatParameter, FloatListParameter,
                                       DictParameter, PositiveIntegerListParameter, sans_parameters)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSFileInformation import find_full_file_path


# ------------------------------------------------
# SANSStateData
# ------------------------------------------------
class SANSStateMask(object):
    pass


@sans_parameters
class SANSStateMaskDetectorISIS(SANSStateBase, SANSStateMask):
    # Vertical strip masks
    single_vertical_strip_mask = PositiveIntegerListParameter()
    range_vertical_strip_start = PositiveIntegerListParameter()
    range_vertical_strip_stop = PositiveIntegerListParameter()

    # Horizontal strip masks
    single_horizontal_strip_mask = PositiveIntegerListParameter()
    range_horizontal_strip_start = PositiveIntegerListParameter()
    range_horizontal_strip_stop = PositiveIntegerListParameter()

    # Spectrum Block
    block_horizontal_start = PositiveIntegerListParameter()
    block_horizontal_stop = PositiveIntegerListParameter()
    block_vertical_start = PositiveIntegerListParameter()
    block_vertical_stop = PositiveIntegerListParameter()

    # Spectrum block cross
    block_cross_horizontal = PositiveIntegerListParameter()
    block_cross_vertical = PositiveIntegerListParameter()

    # Time/Bin mask
    bin_mask_start = FloatListParameter()
    bin_mask_stop = FloatListParameter()

    # Name of the detector
    detector_name = StringParameter()
    detector_name_short = StringParameter()

    def __init__(self):
        super(SANSStateMaskDetectorISIS, self).__init__()

    def validate(self):
        is_invalid = {}
        if not self.detector_name:
            is_invalid.update({"detector_name": "The detector name has not been specified."})
        if not self.detector_name_short:
            is_invalid.update({"detector_name_short": "The short detector name has not been specified."})
        if is_invalid:
            raise ValueError("SANSStateMoveDetectorISIS: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# -----------------------------------------------
#  SANSStateData Setup for ISIS
# -----------------------------------------------
@sans_parameters
class SANSStateMaskISIS(SANSStateBase, SANSStateMask):
    # Radius Mask
    radius_min = FloatParameter()
    radius_max = FloatParameter()

    # Bin mask
    bin_mask_general_start = FloatListParameter()
    bin_mask_general_stop = FloatListParameter()

    # Mask files
    mask_files = StringListParameter()

    # Angle masking
    phi_min = FloatParameter()
    phi_max = FloatParameter()
    use_mask_phi_mirror = BoolParameter()

    # Beam stop
    beam_stop_arm_width = PositiveFloatParameter()
    beam_stop_arm_angle = FloatParameter()
    beam_stop_arm_pos1 = FloatParameter()
    beam_stop_arm_pos2 = FloatParameter()

    # Clear commands
    clear = BoolParameter()
    clear_time = BoolParameter()

    # Single Spectra
    single_spectra = PositiveIntegerListParameter()

    # Spectrum Range
    spectrum_range_start = PositiveIntegerListParameter()
    spectrum_range_stop = PositiveIntegerListParameter()

    # The detector dependent masks
    detectors = DictParameter()

    # The idf path of the instrument
    idf_path = StringParameter()

    def __init__(self):
        super(SANSStateMaskISIS, self).__init__()
        # Setup the detectors
        self.detectors = {SANSConstants.low_angle_bank: SANSStateMaskDetectorISIS(),
                          SANSConstants.high_angle_bank: SANSStateMaskDetectorISIS()}

        # IDF Path
        self.idf_path = ""

    def validate(self):
        is_invalid = dict()

        # Radius mask rule: If there is a minimum, then we also want a maximum
        if self.radius_max and not self.radius_min or not self.radius_max and self.radius_min:
            is_invalid.update({"radius mask": "You have to either specify radius_min AND radius_max or none"})

        # Radius mask rule: the min radius must be less or equal to the max radius
        if self.radius_max and self.radius_min and (self.radius_min > self.radius_max):
            is_invalid.update({"radius mask": "radius_min has to be smaller than radius_max."})

        # Bin mask general rule: If there is a start, then there also has to be a stop
        if self.bin_mask_general_start and not self.bin_mask_general_stop or \
                        not self.bin_mask_general_start and self.bin_mask_general_stop:
            is_invalid.update({"bin mask general": "You have to either specify bin_mask_general_start AND"
                                                   "bin_mask_general_stop or none"})

        # Make sure that the mask files are valid and exist
        if self.mask_files:
            for mask_file in self.mask_files:
                if not find_full_file_path(mask_file):
                    is_invalid.update({"mask_files": "The mask file {0} cannot be found. Make sure it is "
                                                     "visible to the Mantid path.".format(mask_file)})

        if is_invalid:
            raise ValueError("SANSStateMask: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))

# -----------------------------------------------
# SANSStateMask setup for other facilities/techniques/scenarios.
# Needs to derive from SANSStateMask and SANSStateBase and fulfill its contract.
# -----------------------------------------------
