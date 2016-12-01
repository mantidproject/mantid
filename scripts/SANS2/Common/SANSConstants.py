""" The SANSConstants class holds constants which are used in the SANS reducer framework."""

# pylint: disable=too-few-public-methods


class SANSConstants(object):
    """
    The SANSConstants object is a convenient collection of constants which are used during the SANS reduction.
    """
    monitor_suffix = "_monitors"
    input_workspace = "InputWorkspace"

    file_name = "Filename"

    output_workspace = "OutputWorkspace"
    output_workspace_group = output_workspace + "_"

    output_monitor_workspace = "MonitorWorkspace"
    output_monitor_workspace_group = output_monitor_workspace + "_"

    workspace = "Workspace"

    dummy = "dummy"

    sans_suffix = "sans"
    trans_suffix = "trans"

    high_angle_bank = "HAB"
    low_angle_bank = "LAB"

    sans2d = "SANS2D"
    larmor = "LARMOR"
    loq = "LOQ"

    reduced_workspace_name_in_logs = "reduced_workspace_name"
    sans_file_tag = "sans_file_tag"
    reduced_can_tag = "reduced_can_hash"

    # String names for SANSType
    rebin = "Rebin"
    intperpolating_rebin = "InterpolatingRebin"
    range_step_lin = "lin"
    range_step_log = "log"
    sample = "Sample"
    can = "Can"

    ALL_PERIODS = 0

    class Calibration(object):
        calibration_workspace_tag = "sans_applied_calibration_file"