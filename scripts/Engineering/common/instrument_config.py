# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from dataclasses import dataclass
from typing import Dict, Tuple, Sequence, Type
from enum import Enum


class ENGINX_GROUP(Enum):
    """Detector groups for ENGINX. The enum value is the file suffix of the .prm (for easy creation)."""

    BOTH = "banks"
    NORTH = "1"
    SOUTH = "2"
    CROPPED = "Cropped"  # pdcal results will be saved with grouping file with same suffix
    CUSTOM = "Custom"  # pdcal results will be saved with grouping file with same suffix
    TEXTURE20 = "Texture20"
    TEXTURE30 = "Texture30"


class IMAT_GROUP(Enum):
    """Detector groups for IMAT. The enum value is the file suffix of the .prm (for easy creation)."""

    BOTH = "banks"
    NORTH = "1"
    SOUTH = "2"
    CROPPED = "Cropped"  # pdcal results will be saved with grouping file with same suffix
    CUSTOM = "Custom"  # pdcal results will be saved with grouping file with same suffix
    MODULE1 = "Module1"
    MODULE4 = "Module4"
    ROW1 = "Row1"
    ROW4 = "Row4"


INSTRUMENT_GROUP = ENGINX_GROUP | IMAT_GROUP

###### ~~~~~~~~~~~~~~~~~~~~ IMPORTANT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# if a new instrument is added, add its config key to this list and group to groups (used for tests)
SUPPORTED_INSTRUMENTS = ("ENGINX", "IMAT")
GROUPS = (ENGINX_GROUP, IMAT_GROUP)
PSEUDONYMS = {"ENGIN-X": "ENGINX"}


@dataclass(frozen=True)
class DetectorGroupInfo:
    """All per-group configuration for a single detector group of an instrument."""

    banks: Sequence[int]
    description: str
    ws_name: str
    suffix: str
    foc_ws_suffix: str
    grouping_file: str | None = None  # only banks/texture groups ship with a grouping file
    bank_args: str | None = None  # GroupNames arg for CreateGroupingWorkspace (bank groups only)


@dataclass(frozen=True)
class InstrumentConfig:
    name: str

    # full instrument calibration path
    full_instr_calib: str

    # ROI/grouping
    group: Type[INSTRUMENT_GROUP]
    group_info: Dict[INSTRUMENT_GROUP, DetectorGroupInfo]
    texture_groups: Sequence[INSTRUMENT_GROUP]
    interactive_grouping_options: Sequence[Tuple[INSTRUMENT_GROUP, str, bool, bool]]  # group, description, has file input, has text input
    peak_func: str
    funcs_to_keep_fixed: Sequence[str]

    # PDCalibration defaults
    calibration_tof_binning: Tuple[float, float, float]

    # GSAS prm
    prm_header_template: str


CONFIGS: Dict[str, InstrumentConfig] = {
    "ENGINX": InstrumentConfig(
        name="ENGINX",
        full_instr_calib="ENGINX_full_instrument_calibration_193749.nxs",
        group=ENGINX_GROUP,
        group_info={
            ENGINX_GROUP.BOTH: DetectorGroupInfo(
                banks=[1, 2],
                description="North and South Banks",
                ws_name="NorthAndSouthBank_grouping",
                suffix="all_banks",
                foc_ws_suffix="bank",
                grouping_file="ENGINX_NorthAndSouth_grouping.xml",
                bank_args="NorthBank,SouthBank",
            ),
            ENGINX_GROUP.NORTH: DetectorGroupInfo(
                banks=[1],
                description="North Bank",
                ws_name="NorthBank_grouping",
                suffix="bank_1",
                foc_ws_suffix="bank_1",
                grouping_file="ENGINX_North_grouping.xml",
                bank_args="NorthBank",
            ),
            ENGINX_GROUP.SOUTH: DetectorGroupInfo(
                banks=[2],
                description="South Bank",
                ws_name="SouthBank_grouping",
                suffix="bank_2",
                foc_ws_suffix="bank_2",
                grouping_file="ENGINX_South_grouping.xml",
                bank_args="SouthBank",
            ),
            ENGINX_GROUP.CROPPED: DetectorGroupInfo(
                banks=[],
                description="Custom spectrum numbers",
                ws_name="Cropped_spectra_grouping",
                suffix="Cropped",
                foc_ws_suffix="Cropped",
            ),
            ENGINX_GROUP.CUSTOM: DetectorGroupInfo(
                banks=[],
                description="Custom grouping file",
                ws_name="Custom_grouping_file",
                suffix="Custom",
                foc_ws_suffix="Custom",
            ),
            ENGINX_GROUP.TEXTURE20: DetectorGroupInfo(
                banks=[1, 2],
                description="Texture20",
                ws_name="Texture20_grouping",
                suffix="Texture20",
                foc_ws_suffix="Texture20",
                grouping_file="ENGINX_Texture20_grouping.xml",
            ),
            ENGINX_GROUP.TEXTURE30: DetectorGroupInfo(
                banks=[1, 2],
                description="Texture30",
                ws_name="Texture30_grouping",
                suffix="Texture30",
                foc_ws_suffix="Texture30",
                grouping_file="ENGINX_Texture30_grouping.xml",
            ),
        },
        texture_groups=(ENGINX_GROUP.TEXTURE20, ENGINX_GROUP.TEXTURE30, ENGINX_GROUP.CUSTOM),
        calibration_tof_binning=(12000, -0.0003, 52000),
        peak_func="BackToBackExponential",
        funcs_to_keep_fixed=("IkedaCarpenterPV",),
        prm_header_template="template_ENGINX_prm_header.prm",
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
        full_instr_calib="IMAT_full_instrument_calibration_36792.nxs",
        group=IMAT_GROUP,
        group_info={
            IMAT_GROUP.BOTH: DetectorGroupInfo(
                banks=[1, 2],
                description="North and South Banks",
                ws_name="NorthAndSouthBank_grouping",
                suffix="all_banks",
                foc_ws_suffix="bank",
                grouping_file="IMAT_NorthAndSouth_grouping.xml",
                bank_args="NorthBank,SouthBank",
            ),
            IMAT_GROUP.NORTH: DetectorGroupInfo(
                banks=[1],
                description="North Bank",
                ws_name="NorthBank_grouping",
                suffix="bank_1",
                foc_ws_suffix="bank_1",
                grouping_file="IMAT_North_grouping.xml",
                bank_args="NorthBank",
            ),
            IMAT_GROUP.SOUTH: DetectorGroupInfo(
                banks=[2],
                description="South Bank",
                ws_name="SouthBank_grouping",
                suffix="bank_2",
                foc_ws_suffix="bank_2",
                grouping_file="IMAT_South_grouping.xml",
                bank_args="SouthBank",
            ),
            IMAT_GROUP.CROPPED: DetectorGroupInfo(
                banks=[],
                description="Custom spectrum numbers",
                ws_name="Cropped_spectra_grouping",
                suffix="Cropped",
                foc_ws_suffix="Cropped",
            ),
            IMAT_GROUP.CUSTOM: DetectorGroupInfo(
                banks=[],
                description="Custom grouping file",
                ws_name="Custom_grouping_file",
                suffix="Custom",
                foc_ws_suffix="Custom",
            ),
            IMAT_GROUP.MODULE1: DetectorGroupInfo(
                banks=[1, 2],
                description="1 Group per Module",
                ws_name="Module1",
                suffix="Module1",
                foc_ws_suffix="Module1",
                grouping_file="IMAT_Module1_grouping.xml",
            ),
            IMAT_GROUP.MODULE4: DetectorGroupInfo(
                banks=[1, 2],
                description="4 Groups per Module",
                ws_name="Module4",
                suffix="Module4",
                foc_ws_suffix="Module4",
                grouping_file="IMAT_Module4_grouping.xml",
            ),
            IMAT_GROUP.ROW1: DetectorGroupInfo(
                banks=[1, 2],
                description="1 Group per Row",
                ws_name="Row1",
                suffix="Row1",
                foc_ws_suffix="Row1",
                grouping_file="IMAT_Row1_grouping.xml",
            ),
            IMAT_GROUP.ROW4: DetectorGroupInfo(
                banks=[1, 2],
                description="4 Groups per Row",
                ws_name="Row4",
                suffix="Row4",
                foc_ws_suffix="Row4",
                grouping_file="IMAT_Row4_grouping.xml",
            ),
        },
        texture_groups=(IMAT_GROUP.CUSTOM, IMAT_GROUP.MODULE1, IMAT_GROUP.MODULE4, IMAT_GROUP.ROW1, IMAT_GROUP.ROW4),
        calibration_tof_binning=(10000, -0.0003, 80000),
        peak_func="IkedaCarpenterPV",
        funcs_to_keep_fixed=("IkedaCarpenterPV",),
        prm_header_template="template_IMAT_prm_header.prm",
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


def get_instr_config(instrument: str | None) -> InstrumentConfig | None:
    if instrument is None:
        return None
    key = instrument.upper()
    key = PSEUDONYMS.get(key, key)
    if key not in CONFIGS:
        raise RuntimeError(f"No instrument config registered for instrument='{instrument}'")
    return CONFIGS[key]
