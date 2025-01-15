# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""Handles calibration of SANS workspaces."""

from os.path import basename, splitext, isfile
from mantid.api import AnalysisDataService

from sans.common.file_information import find_full_file_path
from sans.common.constants import EMPTY_NAME, CALIBRATION_WORKSPACE_TAG
from sans.common.log_tagger import has_tag, get_tag, set_tag
from sans.common.general_functions import create_child_algorithm, sanitise_instrument_name
from sans.common.enums import SANSDataType


# ---------------------------------
#  Free functions for Calibration
# ---------------------------------
def apply_calibration(calibration_file_name, workspaces, monitor_workspaces, use_loaded, publish_to_ads, parent_alg):
    """
    Apply (tube) calibration to scatter workspaces and corresponding monitor workspaces.

    :param calibration_file_name: the file name of the calibration file.
    :param workspaces: a map with scatter workspaces for sample and can
    :param monitor_workspaces: a map with scatter monitor workspaces for sample and can
    :param use_loaded: if calibration file from ADS is to be used (if it exists)
    :param publish_to_ads: if the calibration file should be published to the ADS
    :param parent_alg: a handle to the parent algorithm
    :return:
    """
    full_file_path = find_full_file_path(calibration_file_name)

    if not full_file_path:
        raise IOError(f"{calibration_file_name} was not found")

    # Check for the sample scatter and the can scatter workspaces
    workspaces_to_calibrate = {}
    if SANSDataType.SAMPLE_SCATTER in workspaces:
        workspaces_to_calibrate.update({SANSDataType.SAMPLE_SCATTER: workspaces[SANSDataType.SAMPLE_SCATTER]})
    if SANSDataType.CAN_SCATTER in workspaces:
        workspaces_to_calibrate.update({SANSDataType.CAN_SCATTER: workspaces[SANSDataType.CAN_SCATTER]})
    do_apply_calibration(full_file_path, workspaces_to_calibrate, use_loaded, publish_to_ads, parent_alg)

    # Check for the sample scatter and the can scatter workspaces monitors
    workspace_monitors_to_calibrate = {}
    if SANSDataType.SAMPLE_SCATTER in monitor_workspaces:
        workspace_monitors_to_calibrate.update({SANSDataType.SAMPLE_SCATTER: monitor_workspaces[SANSDataType.SAMPLE_SCATTER]})
    if SANSDataType.CAN_SCATTER in monitor_workspaces:
        workspace_monitors_to_calibrate.update({SANSDataType.CAN_SCATTER: monitor_workspaces[SANSDataType.CAN_SCATTER]})
    do_apply_calibration(full_file_path, workspace_monitors_to_calibrate, use_loaded, publish_to_ads, parent_alg)


def do_apply_calibration(full_file_path, workspaces_to_calibrate, use_loaded, publish_to_ads, parent_alg):
    """
    Applies calibration to a data workspace

    :param full_file_path: the file path to the calibration file.
    :param workspaces_to_calibrate: the workspace which is to be calibrated.
    :param use_loaded: if already loaded calibration workspace is to be used.
    :param publish_to_ads: if calibration workspace is to be added to the ADS.
    :param parent_alg: a handle to the parent algorithm
    """
    # Load calibration workspace
    calibration_workspace = get_calibration_workspace(full_file_path, use_loaded, parent_alg)

    # Check if the workspace has a calibration already applied to it and if it coincides with the
    # provided calibration file
    for workspaces in list(workspaces_to_calibrate.values()):
        # If it is already calibrated then don't do anything, We only need to check the first element, even for
        # GroupWorkspaces
        to_check = workspaces[0]
        if has_calibration_already_been_applied(to_check, full_file_path):
            continue

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
                calibration_workspace_to_use = get_cloned_calibration_workspace(calibration_workspace, parent_alg)
            else:
                calibration_workspace_to_use = calibration_workspace
            missing_parameters = get_missing_parameters(calibration_workspace_to_use, workspace)
            apply_missing_parameters(calibration_workspace_to_use, workspace, missing_parameters, parent_alg)
            calibrate(calibration_workspace_to_use, workspace, parent_alg)

            # Publish to ADS if requested
            if publish_to_ads and not has_been_published:
                add_to_ads(calibration_workspace, full_file_path)
                has_been_published = True

            # Add calibration tag to workspace
            add_calibration_tag_to_workspace(workspace, full_file_path)


def get_calibration_workspace(full_file_path, use_loaded, parent_alg):
    """
    Load the calibration workspace from the specified file

    :param full_file_path: Path to the calibration file.
    :param use_loaded: Allows us to check for the calibration file on the ADS.
    :param parent_alg: a handle to the parent algorithm
    :return: the calibration workspace.
    """
    calibration_workspace = None
    # Here we can avoid reloading of the calibration workspace
    if use_loaded:
        calibration_workspace = get_already_loaded_calibration_workspace(full_file_path)

    if calibration_workspace is None:
        if not isfile(full_file_path):
            raise RuntimeError("SANSCalibration: The file for {0} does not seem to exist".format(full_file_path))
        loader_name = "LoadNexusProcessed"
        loader_options = {"Filename": full_file_path, "OutputWorkspace": "dummy"}
        loader = create_child_algorithm(parent_alg, loader_name, **loader_options)
        loader.execute()
        calibration_workspace = loader.getProperty("OutputWorkspace").value

    return calibration_workspace


def has_calibration_already_been_applied(workspace, full_file_path):
    """
    Checks if particular calibration, defined by the file path has been applied to a workspace.

    :param workspace: The workspace which might have been calibrated
    :param full_file_path: An absolute file path to the calibration file.
    :return: True if the calibration has been applied else False
    """
    has_calibration_applied = False
    if has_tag(CALIBRATION_WORKSPACE_TAG, workspace):
        value = get_tag(CALIBRATION_WORKSPACE_TAG, workspace)
        has_calibration_applied = value == full_file_path
    return has_calibration_applied


def add_calibration_tag_to_workspace(workspace, full_file_path):
    """
    Adds a calibration tag to the workspace, which is the file path to the calibration file

    This is used to determine if a calibration (and if the correct calibration) has been applied to a workspace.
    :param workspace: the workspace to which the calibration tag is added
    :param full_file_path: the full file path to the calibration file
    """
    set_tag(CALIBRATION_WORKSPACE_TAG, full_file_path, workspace)


def get_expected_calibration_workspace_name(full_file_path):
    """
    Gets the name of the calibration file.

    :param full_file_path: the full file path to the calibration file.
    :return: the calibration file name.
    """
    truncated_path = basename(full_file_path)
    file_name, _ = splitext(truncated_path)
    return file_name


def get_already_loaded_calibration_workspace(full_file_path):
    """
    Gets a calibration workspace from the ADS if it exists.

    :param full_file_path: the full file path to the calibration workspace
    :return: a handle to the calibration workspace or None
    """
    calibration_workspace_name = get_expected_calibration_workspace_name(full_file_path)
    if AnalysisDataService.doesExist(calibration_workspace_name):
        output_ws = AnalysisDataService.retrieve(calibration_workspace_name)
    else:
        output_ws = None
    return output_ws


def get_cloned_calibration_workspace(calibration_workspace, parent_alg):
    """
    Creates a clone from a calibration workspace, in order to consume it later.

    :param calibration_workspace: the calibration workspace which is being cloned
    :param parent_alg: a handle to the parent algorithm
    :return: a cloned calibration workspace
    """
    clone_name = "CloneWorkspace"
    clone_options = {"InputWorkspace": calibration_workspace, "OutputWorkspace": EMPTY_NAME}
    alg = create_child_algorithm(parent_alg, clone_name, **clone_options)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def get_missing_parameters(calibration_workspace, workspace):
    """
    Get a list of missing parameter names which are on the data workspace but not on the calibration workspace.

    :param calibration_workspace: the calibration workspace
    :param workspace: the data workspace (which is to be calibrated later on).
    :return: a list of parameters which exist on the data workspace but not on the calibration workspace.
    """
    original_parameter_names = workspace.getInstrument().getParameterNames()
    calibration_workspace_instrument = calibration_workspace.getInstrument()
    missing_parameter_names = []
    for parameter in original_parameter_names:
        if not calibration_workspace_instrument.hasParameter(parameter):
            missing_parameter_names.append(parameter)
    return missing_parameter_names


def apply_missing_parameters(calibration_workspace, workspace, missing_parameters, parent_alg):
    """
    Transfers missing properties from the data workspace to the calibration workspace.

    :param calibration_workspace: the calibration workspace.
    :param workspace: the data workspace.
    :param missing_parameters: a list of missing parameters which exist on the data workspace but not on the calibration
                               workspace.
    :param parent_alg: a handle to the parent algorithm
    """
    instrument = workspace.getInstrument()
    component_name = instrument.getName()
    component_name = sanitise_instrument_name(component_name)
    set_instrument_name = "SetInstrumentParameter"
    set_instrument_parameter_options = {"Workspace": calibration_workspace, "ComponentName": component_name}
    alg = create_child_algorithm(parent_alg, set_instrument_name, **set_instrument_parameter_options)

    # For now only string, int and double are handled
    type_options = {"string": "String", "int": "Number", "double": "Number"}
    value_options = {"string": instrument.getStringParameter, "int": instrument.getIntParameter, "double": instrument.getNumberParameter}
    try:
        for missing_parameter in missing_parameters:
            parameter_type = instrument.getParameterType(missing_parameter)
            type_to_save = type_options[parameter_type]
            value = value_options[parameter_type](missing_parameter)

            alg.setProperty("ParameterName", missing_parameter)
            alg.setProperty("ParameterType", type_to_save)
            alg.setProperty("Value", str(value[0]))
    except KeyError:
        raise RuntimeError(
            "SANSCalibration: An Instrument Parameter File value of unknown type was going to be copied. Cannot handle this currently."
        )


def calibrate(calibration_workspace, workspace_to_calibrate, parent_alg):
    """
    Performs a calibration. The instrument parameters are copied from the calibration workspace to the data workspace.

    :param calibration_workspace: the calibration workspace
    :param workspace_to_calibrate: the workspace which has the calibration applied to it.
    :param parent_alg: a handle to the parent algorithm
    """
    copy_instrument_name = "CopyInstrumentParameters"
    copy_instrument_options = {"InputWorkspace": calibration_workspace, "OutputWorkspace": workspace_to_calibrate}
    alg = create_child_algorithm(parent_alg, copy_instrument_name, **copy_instrument_options)
    alg.execute()


def add_to_ads(calibration_workspace, full_file_path):
    """
    Add the calibration file to the ADS. The file name is used to publish it to the ADS.

    :param calibration_workspace: the calibration file which is to be published.
    :param full_file_path: the file path to the calibration file.
    """
    calibration_workspace_name = get_expected_calibration_workspace_name(full_file_path)
    AnalysisDataService.addOrReplace(calibration_workspace_name, calibration_workspace)
