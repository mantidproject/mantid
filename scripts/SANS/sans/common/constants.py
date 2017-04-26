""" These constants  are used in the SANS reducer framework. We want a central place for them."""

# pylint: disable=too-few-public-methods

from __future__ import (absolute_import, division, print_function)

# ----------------------------------------
# Proeprty names for Algorithms
# ---------------------------------------
MONITOR_SUFFIX = "_monitors"
INPUT_WORKSPACE = "InputWorkspace"

FILE_NAME = "Filename"

OUTPUT_WORKSPACE = "OutputWorkspace"
OUTPUT_WORKSPACE_GROUP = OUTPUT_WORKSPACE + "_"

OUTPUT_MONITOR_WORKSPACE = "MonitorWorkspace"
OUTPUT_MONITOR_WORKSPACE_GROUP = OUTPUT_MONITOR_WORKSPACE + "_"

WORKSPACE = "Workspace"
EMPTY_NAME = "empty_name"


# ----------------------------------------
# Other
# ---------------------------------------
SANS_SUFFIX = "sans"
TRANS_SUFFIX = "trans"

HIGH_ANGLE_BANK = "HAB"
LOW_ANGLE_BANK = "LAB"

SANS2D = "SANS2D"
LARMOR = "LARMOR"
LOQ = "LOQ"

SANS_FILE_TAG = "sans_file_tag"
REDUCED_CAN_TAG = "reduced_can_hash"

ALL_PERIODS = 0

CALIBRATION_WORKSPACE_TAG = "sans_applied_calibration_file"

# --------------------------------------------
# Workspace name suffixes
# --------------------------------------------
LAB_CAN_SUFFIX = "_lab_can"
LAB_CAN_COUNT_SUFFIX = "_lab_can_count"
LAB_CAN_NORM_SUFFIX = "_lab_can_norm"

HAB_CAN_SUFFIX = "_hab_can"
HAB_CAN_COUNT_SUFFIX = "_hab_can_count"
HAB_CAN_NORM_SUFFIX = "_hab_can_norm"

REDUCED_HAB_AND_LAB_WORKSPACE_FOR_MERGED_REDUCTION = "can_sample_workspaces_from_merged_reduction"
REDUCED_CAN_AND_PARTIAL_CAN_FOR_OPTIMIZATION = "reduced_can_and_partial_can_workspaces"
