""" These constants  are used in the SANS reducer framework. We want a central place for them."""

# pylint: disable=too-few-public-methods

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
EMPTY_NAME = "dummy"


# ----------------------------------------
# Other
# ---------------------------------------
SANS_SUFFIX = "sans"
TRANS_SUFFIX = "trans"

high_angle_bank = "HAB"
low_angle_bank = "LAB"

SANS2D = "SANS2D"
LARMOR = "LARMOR"
LOQ = "LOQ"

REDUCED_WORKSPACE_NAME_IN_LOGS = "reduced_workspace_name"
SANS_FILE_TAG = "sans_file_tag"
REDUCED_CAN_TAG = "reduced_can_hash"

ALL_PERIODS = 0

CALIBRATION_WORKSPACE_TAG = "sans_applied_calibration_file"
