# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from dataclasses import dataclass
from typing import Dict, Tuple, Optional, Sequence, Type
from enum import Enum


class ENGINX_GROUP(Enum):
    def __new__(self, value, banks):
        obj = object.__new__(self)
        obj._value_ = value  # overwrite value to be first arg
        obj.banks = banks  # set attribute bank
        return obj

    """Base Group extended for ENGINX specific groups"""
    # value of enum is the file suffix of .prm (for easy creation)
    #       value,  banks
    BOTH = "banks", [1, 2]
    NORTH = "1", [1]
    SOUTH = "2", [2]
    CROPPED = "Cropped", []  # pdcal results will be saved with grouping file with same suffix
    CUSTOM = "Custom", []  # pdcal results will be saved with grouping file with same suffix
    TEXTURE20 = "Texture20", [1, 2]
    TEXTURE30 = "Texture30", [1, 2]


class IMAT_GROUP(Enum):
    def __new__(self, value, banks):
        obj = object.__new__(self)
        obj._value_ = value  # overwrite value to be first arg
        obj.banks = banks  # set attribute bank
        return obj

    """Base Group extended for IMAT specific groups"""
    # value of enum is the file suffix of .prm (for easy creation)
    #       value,  banks
    BOTH = "banks", [1, 2]
    NORTH = "1", [1]
    SOUTH = "2", [2]
    CROPPED = "Cropped", []  # pdcal results will be saved with grouping file with same suffix
    CUSTOM = "Custom", []  # pdcal results will be saved with grouping file with same suffix
    MODULE1 = "Module1", [1, 2]
    MODULE4 = "Module4", [1, 2]
    ROW1 = "Row1", [1, 2]
    ROW4 = "Row4", [1, 2]


###### ~~~~~~~~~~~~~~~~~~~~ IMPORTANT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# if a new instrument is added, add its config key to this list and group to groups (used for tests)
SUPPORTED_INSTRUMENTS = ("ENGINX", "IMAT")
GROUPS = (ENGINX_GROUP, IMAT_GROUP)


@dataclass(frozen=True)
class InstrumentConfig:
    name: str

    # full instrument calibration path
    full_instr_calib: str

    # ROI/grouping
    group: Type[Enum]
    grouping_files: Dict[Enum, str]
    texture_groups: Sequence[Enum]
    group_bank_args: Dict[Enum, str]
    group_descriptions: Dict[Enum, str]
    group_ws_names: Dict[Enum, str]
    group_suffix: Dict[Enum, str]
    group_foc_ws_suffix: Dict[Enum, str]
    interactive_grouping_options: Sequence[Tuple[Enum, str, bool, bool]]  # group, description, has file input, has text input

    # PDCalibration defaults
    calibration_tof_binning: Tuple[float, float, float]

    # GSAS prm
    prm_header_template: str


CONFIGS: Dict[str, InstrumentConfig] = {
    "ENGINX": InstrumentConfig(
        name="ENGINX",
        full_instr_calib="ENGINX_full_instrument_calibration_193749.nxs",
        group=ENGINX_GROUP,
        grouping_files={
            ENGINX_GROUP.BOTH: "ENGINX_NorthAndSouth_grouping.xml",
            ENGINX_GROUP.NORTH: "ENGINX_North_grouping.xml",
            ENGINX_GROUP.SOUTH: "ENGINX_South_grouping.xml",
            ENGINX_GROUP.TEXTURE20: "ENGINX_Texture20_grouping.xml",
            ENGINX_GROUP.TEXTURE30: "ENGINX_Texture30_grouping.xml",
        },
        texture_groups=(ENGINX_GROUP.TEXTURE20, ENGINX_GROUP.TEXTURE30, ENGINX_GROUP.CUSTOM),
        calibration_tof_binning=(12000, -0.0003, 52000),
        prm_header_template="template_ENGINX_prm_header.prm",
        group_bank_args={ENGINX_GROUP.BOTH: "NorthBank,SouthBank", ENGINX_GROUP.NORTH: "NorthBank", ENGINX_GROUP.SOUTH: "SouthBank"},
        group_descriptions={
            ENGINX_GROUP.BOTH: "North and South Banks",
            ENGINX_GROUP.NORTH: "North Bank",
            ENGINX_GROUP.SOUTH: "South Bank",
            ENGINX_GROUP.CROPPED: "Custom spectrum numbers",
            ENGINX_GROUP.CUSTOM: "Custom grouping file",
            ENGINX_GROUP.TEXTURE20: "Texture20",
            ENGINX_GROUP.TEXTURE30: "Texture30",
        },
        group_ws_names={
            ENGINX_GROUP.BOTH: "NorthAndSouthBank_grouping",
            ENGINX_GROUP.NORTH: "NorthBank_grouping",
            ENGINX_GROUP.SOUTH: "SouthBank_grouping",
            ENGINX_GROUP.CROPPED: "Cropped_spectra_grouping",
            ENGINX_GROUP.CUSTOM: "Custom_grouping_file",
            ENGINX_GROUP.TEXTURE20: "Texture20_grouping",
            ENGINX_GROUP.TEXTURE30: "Texture30_grouping",
        },
        group_suffix={
            ENGINX_GROUP.BOTH: "all_banks",
            ENGINX_GROUP.NORTH: "bank_1",
            ENGINX_GROUP.SOUTH: "bank_2",
            ENGINX_GROUP.CROPPED: "Cropped",
            ENGINX_GROUP.CUSTOM: "Custom",
            ENGINX_GROUP.TEXTURE20: "Texture20",
            ENGINX_GROUP.TEXTURE30: "Texture30",
        },
        group_foc_ws_suffix={
            ENGINX_GROUP.BOTH: "bank",
            ENGINX_GROUP.NORTH: "bank_1",
            ENGINX_GROUP.SOUTH: "bank_2",
            ENGINX_GROUP.CROPPED: "Cropped",
            ENGINX_GROUP.CUSTOM: "Custom",
            ENGINX_GROUP.TEXTURE20: "Texture20",
            ENGINX_GROUP.TEXTURE30: "Texture30",
        },
        interactive_grouping_options=(
            (ENGINX_GROUP.CUSTOM, "Custom Grouping File", True, False),
            (ENGINX_GROUP.NORTH, "1 (North)", False, False),
            (ENGINX_GROUP.SOUTH, "2 (South)", False, False),
            (ENGINX_GROUP.CROPPED, "Crop to Spectra", False, True),
            (ENGINX_GROUP.TEXTURE20, "Texture20", False, False),
            (ENGINX_GROUP.TEXTURE30, "Texture30", False, False),
        ),
    ),
    "IMAT": InstrumentConfig(
        name="IMAT",
        group=IMAT_GROUP,
        full_instr_calib="IMAT_full_instrument_calibration_33701.nxs",
        grouping_files={
            IMAT_GROUP.BOTH: "IMAT_NorthAndSouth_grouping.xml",
            IMAT_GROUP.NORTH: "IMAT_North_grouping.xml",
            IMAT_GROUP.SOUTH: "IMAT_South_grouping.xml",
            IMAT_GROUP.MODULE1: "IMAT_Module1_grouping.xml",
            IMAT_GROUP.MODULE4: "IMAT_Module4_grouping.xml",
            IMAT_GROUP.ROW1: "IMAT_Row1_grouping.xml",
            IMAT_GROUP.ROW4: "IMAT_Row4_grouping.xml",
        },
        texture_groups=(IMAT_GROUP.CUSTOM,),
        # initially these are just the same as the ENGINX values, but will be updated in due course
        calibration_tof_binning=(12000, -0.0003, 52000),
        prm_header_template="template_IMAT_prm_header.prm",
        group_bank_args={IMAT_GROUP.BOTH: "NorthBank,SouthBank", IMAT_GROUP.NORTH: "NorthBank", IMAT_GROUP.SOUTH: "SouthBank"},
        group_descriptions={
            IMAT_GROUP.BOTH: "North and South Banks",
            IMAT_GROUP.NORTH: "North Bank",
            IMAT_GROUP.SOUTH: "South Bank",
            IMAT_GROUP.CROPPED: "Custom spectrum numbers",
            IMAT_GROUP.CUSTOM: "Custom grouping file",
            IMAT_GROUP.MODULE1: "1 Group per Module",
            IMAT_GROUP.MODULE4: "4 Groups per Module",
            IMAT_GROUP.ROW1: "1 Group per Row",
            IMAT_GROUP.ROW4: "4 Groups per Row",
        },
        group_ws_names={
            IMAT_GROUP.BOTH: "NorthAndSouthBank_grouping",
            IMAT_GROUP.NORTH: "NorthBank_grouping",
            IMAT_GROUP.SOUTH: "SouthBank_grouping",
            IMAT_GROUP.CROPPED: "Cropped_spectra_grouping",
            IMAT_GROUP.CUSTOM: "Custom_grouping_file",
            IMAT_GROUP.MODULE1: "Module1",
            IMAT_GROUP.MODULE4: "Module4",
            IMAT_GROUP.ROW1: "Row1",
            IMAT_GROUP.ROW4: "Row4",
        },
        group_suffix={
            IMAT_GROUP.BOTH: "all_banks",
            IMAT_GROUP.NORTH: "bank_1",
            IMAT_GROUP.SOUTH: "bank_2",
            IMAT_GROUP.CROPPED: "Cropped",
            IMAT_GROUP.CUSTOM: "Custom",
            IMAT_GROUP.MODULE1: "Module1",
            IMAT_GROUP.MODULE4: "Module4",
            IMAT_GROUP.ROW1: "Row1",
            IMAT_GROUP.ROW4: "Row4",
        },
        group_foc_ws_suffix={
            IMAT_GROUP.BOTH: "bank",
            IMAT_GROUP.NORTH: "bank_1",
            IMAT_GROUP.SOUTH: "bank_2",
            IMAT_GROUP.CROPPED: "Cropped",
            IMAT_GROUP.CUSTOM: "Custom",
            IMAT_GROUP.MODULE1: "Module1",
            IMAT_GROUP.MODULE4: "Module4",
            IMAT_GROUP.ROW1: "Row1",
            IMAT_GROUP.ROW4: "Row4",
        },
        interactive_grouping_options=(
            (IMAT_GROUP.CUSTOM, "Custom Grouping File", True, False),
            (IMAT_GROUP.NORTH, "1 (North)", False, False),
            (IMAT_GROUP.SOUTH, "2 (South)", False, False),
            (IMAT_GROUP.CROPPED, "Crop to Spectra", False, True),
            (IMAT_GROUP.MODULE1, "1 Group per module", False, False),
            (IMAT_GROUP.MODULE4, "4 Groups per module", False, False),
            (IMAT_GROUP.ROW1, "1 Group per row", False, False),
            (IMAT_GROUP.ROW4, "4 Groups per row", False, False),
        ),
    ),
}


def get_instr_config(instrument: Optional[str]) -> Optional[InstrumentConfig]:
    if instrument is None:
        return None
    key = instrument.upper()
    if key not in CONFIGS:
        raise RuntimeError(f"No instrument config registered for instrument='{instrument}'")
    return CONFIGS[key]
