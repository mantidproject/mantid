# pylint: disable=invalid-name

""" Handles calibration of SANS workspaces."""
from os.path import (basename, splitext, isfile)
from mantid.api import (AnalysisDataService)

from SANS2.Common.SANSFileInformation import find_full_file_path
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSLogTagger import (has_tag, get_tag, set_tag)
from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS2.Common.SANSEnumerations import SANSDataType


# -----------------------------
#  Free functions for Calibration
# -----------------------------
def has_calibration_already_been_applied(workspace, full_file_path):
    """
    Checks if particular calibration, defined by the file path has been applied to a workspace.

    :param workspace: The workspace which might have been calibrated
    :param full_file_path: An absolute file path to the calibration file.
    :return: True if the calibration has been applied else False
    """
    has_calibration_applied = False
    if has_tag(SANSConstants.Calibration.calibration_workspace_tag, workspace):
        value = get_tag(SANSConstants.Calibration.calibration_workspace_tag, workspace)
        has_calibration_applied = value == full_file_path
    return has_calibration_applied


def add_calibration_tag_to_workspace(workspace, full_file_path):
    set_tag(SANSConstants.Calibration.calibration_workspace_tag, full_file_path, workspace)


def get_expected_calibration_workspace_name(full_file_path):
    truncated_path = basename(full_file_path)
    file_name, _ = splitext(truncated_path)
    return file_name


def get_already_loaded_calibration_workspace(full_file_path):
    calibration_workspace_name = get_expected_calibration_workspace_name(full_file_path)
    if AnalysisDataService.doesExist(calibration_workspace_name):
        output_ws = AnalysisDataService.retrieve(calibration_workspace_name)
    else:
        output_ws = None
    return output_ws


def get_calibration_workspace(full_file_path, use_loaded):
    """
    Load the calibration workspace from the specified file

    :param full_file_path: Path to the calibration file.
    :param use_loaded: Allows us to check for the calibration file on the ADS
    :return: the calibration workspace
    """
    calibration_workspace = None
    # Here we can avoid reloading of the calibration workspace
    if use_loaded:
        calibration_workspace = get_already_loaded_calibration_workspace(full_file_path)

    if calibration_workspace is None:
        if not isfile(full_file_path):
            raise RuntimeError("SANSCalibration: The file for  {0} does not seem to exist".format(full_file_path))
        loader_name = "LoadNexusProcessed"
        loader_options = {SANSConstants.file_name: full_file_path,
                          SANSConstants.output_workspace: "dummy"}
        loader = create_unmanaged_algorithm(loader_name, **loader_options)
        loader.execute()
        calibration_workspace = loader.getProperty(SANSConstants.output_workspace).value

    return calibration_workspace


def get_cloned_calibration_workspace(calibration_workspace):
    clone_name = "CloneWorkspace"
    clone_options = {SANSConstants.input_workspace: calibration_workspace,
                     SANSConstants.output_workspace: "dummy"}
    alg = create_unmanaged_algorithm(clone_name, **clone_options)
    alg.execute()
    return alg.getProperty(SANSConstants.output_workspace).value


def get_missing_parameters(calibration_workspace, workspace):
    original_parameter_names = workspace.getInstrument().getParameterNames()
    calibration_workspace_instrument = calibration_workspace.getInstrument()
    missing_parameter_names = []
    for parameter in original_parameter_names:
        if not calibration_workspace_instrument.hasParameter(parameter):
            missing_parameter_names.append(parameter)
    return missing_parameter_names


def apply_missing_parameters(calibration_workspace, workspace, missing_parameters):
    instrument = workspace.getInstrument()
    component_name = instrument.getName()
    set_instrument_parameter_options = {"Workspace": calibration_workspace,
                                        "ComponentName": component_name}
    alg = create_unmanaged_algorithm("SetInstrumentParameter", **set_instrument_parameter_options)

    # For now only string, int and double are handled
    type_options = {"string": "String", "int": "Number", "double": "Number"}
    value_options = {"string": instrument.getStringParameter,
                     "int": instrument.getIntParameter,
                     "double": instrument.getNumberParameter}
    try:
        for missing_parameter in missing_parameters:
            parameter_type = instrument.getParameterType(missing_parameter)
            type_to_save = type_options[parameter_type]
            value = value_options[parameter_type](missing_parameter)

            alg.setProperty("ParameterName", missing_parameter)
            alg.setProperty("ParameterType", type_to_save)
            alg.setProperty("Value", str(value[0]))
    except KeyError:
        raise RuntimeError("SANSCalibration: An Instrument Parameter File value of unknown type"
                           "was going to be copied. Cannot handle this currently.")


def calibrate(calibration_workspace, workspace_to_calibrate):
    copy_instrument_name = "CopyInstrumentParameters"
    copy_instrument_options = {SANSConstants.input_workspace: calibration_workspace,
                               SANSConstants.output_workspace: workspace_to_calibrate}
    alg = create_unmanaged_algorithm(copy_instrument_name, **copy_instrument_options)
    alg.execute()


def add_to_ads(calibration_workspace, full_file_path):
    calibration_workspace_name = get_expected_calibration_workspace_name(full_file_path)
    AnalysisDataService.addOrReplace(calibration_workspace_name, calibration_workspace)


def do_apply_calibration(full_file_path, workspaces_to_calibrate, use_loaded, publish_to_ads):
    # Check if the workspace has a calibration already applied to it and if it coincides with the
    # provided calibration file
    for workspaces in list(workspaces_to_calibrate.values()):
        # If it is already calibrated then don't do anything, We only need to check the first element, even for
        # GroupWorkspaces
        if has_calibration_already_been_applied(workspaces[0], full_file_path):
            continue

        # Load calibration workspace
        calibration_workspace = get_calibration_workspace(full_file_path, use_loaded)

        # Apply calibration to workspace
        # This means that we copy the Parameter Map (PM) from the calibration workspace to the
        # actual data workspace. Entries which only exist in the data workspace would be lost, hence we need to
        # add the missing entries  to the PM of the calibration workspace. Once the calibration has been
        # applied, we don't want these extra parameters to avoid data-cross-talk between different data sets. If
        # we used a workspace from the ADS or intend to publish to the ADS, we will be working with a cloned calibration
        # workspace in order to avoid this cross-talk. The calibration workspace sizes allow for very fast in-memory
        # cloning.
        has_been_published = False
        for workspace in workspaces:
            if use_loaded or publish_to_ads:
                calibration_workspace_to_use = get_cloned_calibration_workspace(calibration_workspace)
            else:
                calibration_workspace_to_use = calibration_workspace
            missing_parameters = get_missing_parameters(calibration_workspace_to_use, workspace)
            apply_missing_parameters(calibration_workspace_to_use, workspace, missing_parameters)
            calibrate(calibration_workspace_to_use, workspace)

            # Publish to ADS if requested
            if publish_to_ads and not has_been_published:
                add_to_ads(calibration_workspace, full_file_path)
                has_been_published = True

            # Add calibration tag to workspace
            add_calibration_tag_to_workspace(workspace, full_file_path)


def apply_calibration(calibration_file_name, workspaces, monitor_workspaces, use_loaded, publish_to_ads):
    full_file_path = find_full_file_path(calibration_file_name)

    # Check for the sample scatter and the can scatter workspaces
    workspaces_to_calibrate = {}
    if SANSDataType.SampleScatter in workspaces:
        workspaces_to_calibrate.update({SANSDataType.SampleScatter: workspaces[SANSDataType.SampleScatter]})
    if SANSDataType.CanScatter in workspaces:
        workspaces_to_calibrate.update({SANSDataType.CanScatter: workspaces[SANSDataType.CanScatter]})
    do_apply_calibration(full_file_path, workspaces_to_calibrate, use_loaded, publish_to_ads)

    # Check for the sample scatter and the can scatter workspaces monitors
    workspace_monitors_to_calibrate = {}
    if SANSDataType.SampleScatter in monitor_workspaces:
        workspace_monitors_to_calibrate.update({SANSDataType.SampleScatter:
                                                monitor_workspaces[SANSDataType.SampleScatter]})
    if SANSDataType.CanScatter in monitor_workspaces:
        workspace_monitors_to_calibrate.update({SANSDataType.CanScatter:
                                                monitor_workspaces[SANSDataType.CanScatter]})
    do_apply_calibration(full_file_path, workspace_monitors_to_calibrate,
                         use_loaded, publish_to_ads)
