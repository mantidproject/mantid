# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from os import path, makedirs

from mantid.simpleapi import logger, Load, SaveNexus, NormaliseByCurrent, Integration, ReplaceSpecialValues, \
    ApplyDiffCal, ConvertUnits
from mantid.simpleapi import AnalysisDataService as Ads

from Engineering.common import path_handling
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
PROCESSED_WORKSPACE_NAME = "engggui_vanadium_processed_instrument"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"

VANADIUM_DIRECTORY_NAME = "Vanadium_Runs"

SAVED_FILE_PROCESSED_SUFFIX = "_precalculated_vanadium_run_processed_instrument.nxs"
SAVED_FILE_INTEG_SUFFIX = "_precalculated_vanadium_run_integration.nxs"


def fetch_correction_workspaces(vanadium_path: str, instrument: str, rb_num: str = "", is_load: bool = False):
    # -> Workspace2D, Workspace2D
    """
    Fetch workspaces from the file system or create new ones.
    :param vanadium_path: The path to the requested vanadium run raw data.
    :param instrument: The instrument the data came from.
    :param rb_num: A user identifier, usually an experiment number.
    :param is_load: True if this is being called as part of loading a previous calibration (force_recalc ignored)
    :return: The resultant integration and processed instrument workspaces.
    """
    vanadium_number = path_handling.get_run_number_from_path(vanadium_path, instrument)
    integ_path, processed_path = generate_van_ws_file_paths(vanadium_number, rb_num)
    if is_load:
        force_recalc = False
    else:
        force_recalc = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                   output_settings.ENGINEERING_PREFIX, "recalc_vanadium", return_type=bool)
    if path.exists(processed_path) and path.exists(integ_path) and not force_recalc:  # Check if the cached files exist.
        try:
            integ_workspace = Load(Filename=integ_path, OutputWorkspace=INTEGRATED_WORKSPACE_NAME)
            processed_workspace = Load(Filename=processed_path, OutputWorkspace=PROCESSED_WORKSPACE_NAME)
            if rb_num:
                user_integ_path, user_processed_path = generate_van_ws_file_paths(vanadium_number, rb_num=rb_num)
                if not path.exists(user_integ_path) and not path.exists(user_processed_path):
                    save_van_workspace(integ_workspace, user_integ_path)
                    save_van_workspace(processed_workspace, user_processed_path)
            return integ_workspace, processed_workspace
        except RuntimeError as e:
            logger.error(
                "Problem loading existing vanadium calculations. Creating new files. Description: "
                + str(e))
    (integ_workspace, processed_workspace) = create_vanadium_corrections(vanadium_path)
    save_van_workspace(integ_workspace, integ_path)
    save_van_workspace(processed_workspace, processed_path)
    if rb_num:
        user_integ_path, user_processed_path = generate_van_ws_file_paths(vanadium_number, rb_num=rb_num)
        save_van_workspace(integ_workspace, user_integ_path)
        save_van_workspace(processed_workspace, user_processed_path)
    return integ_workspace, processed_workspace


def check_workspaces_exist() -> bool:
    """
    Check the vanadium workspaces are loaded into the ADS
    :return: True if both are present, false otherwise
    """
    return Ads.doesExist(PROCESSED_WORKSPACE_NAME) and Ads.doesExist(INTEGRATED_WORKSPACE_NAME)


def create_vanadium_corrections(vanadium_path: str):  # -> Workspace, Workspace
    """
    Runs the vanadium correction algorithm.
    :param vanadium_path: The path to the vanadium data.
    :return: The integrated workspace and the processed instrument workspace generated.
    """
    try:
        van_ws = Load(Filename=vanadium_path, OutputWorkspace=VANADIUM_INPUT_WORKSPACE_NAME)
    except Exception as e:
        logger.error("Error when loading vanadium sample data. "
                     "Could not run Load algorithm with vanadium run number: "
                     + str(vanadium_path) + ". Error description: " + str(e))
        raise RuntimeError
    # get full instrument calibration for instrument processing calculation
    full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                  output_settings.ENGINEERING_PREFIX, "full_calibration")
    try:
        full_calib_ws = Load(full_calib_path, OutputWorkspace="full_inst_calib")
    except ValueError:
        logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
        return
    integral_ws = _calculate_vanadium_integral(van_ws)
    processed_ws = _calculate_vanadium_processed_instrument(van_ws, full_calib_ws, integral_ws)
    return integral_ws, processed_ws


def _calculate_vanadium_integral(van_ws):  # -> Workspace
    """
    Calculate the integral of the normalised vanadium workspace
    :param van_ws: Chosen vanadium run workspace
    :return: Integrated workspace
    """
    ws = NormaliseByCurrent(InputWorkspace=van_ws, OutputWorkspace=INTEGRATED_WORKSPACE_NAME)  # create new name ws here
    # sensitivity correction for van
    ws_integ = Integration(InputWorkspace=ws, OutputWorkspace=ws)
    ws_integ /= van_ws.blocksize()
    return ws_integ


def _calculate_vanadium_processed_instrument(van_ws, full_calib_ws, integral_ws):  # -> Workspace
    """
    Calculate the processed whole-instrument workspace
    :param van_ws: Chosen vanadium run workspace
    :param full_calib_ws: Table workspace from the full instrument calibration (output from PDCalibration)
    :param integral_ws: Vanadium integral workspace
    :return: Whole instrument processed workspace
    """
    ws = NormaliseByCurrent(InputWorkspace=van_ws, OutputWorkspace=PROCESSED_WORKSPACE_NAME)  # create new name ws here
    ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_calib_ws)
    ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='dSpacing')
    # van sensitivity correction
    ws /= integral_ws
    ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0, InfinityValue=0)
    return ws


def save_van_workspace(workspace, output_path: str) -> None:
    """
    Attempt to save the created workspaces to the filesystem.
    :param workspace: The vanadium workspace
    :param output_path: The path to save the workspace to.
    """
    try:
        SaveNexus(InputWorkspace=workspace, Filename=output_path)
    except RuntimeError as e:  # If the files cannot be saved, continue with the execution of the algorithm anyway.
        logger.error(
            "Vanadium Integration file could not be saved to the filesystem. Description: " + str(e))
        return


def _generate_vanadium_saves_file_path(rb_num: str = "") -> str:
    """
    Generate file paths based on a vanadium run number.
    :param rb_num: User identifier, usually an experiment number.
    :return: The full path to the file.
    """
    if rb_num:
        vanadium_dir = path.join(output_settings.get_output_path(), "User", rb_num,
                                 VANADIUM_DIRECTORY_NAME)
    else:
        vanadium_dir = path.join(output_settings.get_output_path(), VANADIUM_DIRECTORY_NAME)
    if not path.exists(vanadium_dir):
        makedirs(vanadium_dir)
    return vanadium_dir


def generate_van_ws_file_paths(vanadium_number: str, rb_num: str = "") -> (str, str):
    """
    Generate file path based on a vanadium run number for the vanadium integration workspace.
    :param vanadium_number: The number of the vanadium run.
    :param rb_num: User identifier, usually an experiment number.
    :return: The full path to the file.
    """
    integral_filename = vanadium_number + SAVED_FILE_INTEG_SUFFIX
    processed_filename = vanadium_number + SAVED_FILE_PROCESSED_SUFFIX
    vanadium_dir = _generate_vanadium_saves_file_path(rb_num)
    return path.join(vanadium_dir, integral_filename), path.join(vanadium_dir, processed_filename)
