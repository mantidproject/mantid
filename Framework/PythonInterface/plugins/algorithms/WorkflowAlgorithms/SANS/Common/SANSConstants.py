
class SANSConstants(object):
    monitor_suffix = "_monitors"
    input_workspace = "InputWorkspace"

    file_name = "Filename"

    output_workspace = "OutputWorkspace"
    output_workspace_group = output_workspace + "_"

    output_monitor_workspace = "MonitorWorkspace"
    output_monitor_workspace_group = output_monitor_workspace + "_"

    sans_suffix = "sans"
    trans_suffix = "trans"

    class Calibration(object):
        calibration_workspace_tag = "sans_applied_calibration_file"