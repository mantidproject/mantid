# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import logger, Load, NormaliseByCurrent, Integration, ReplaceSpecialValues, \
    ApplyDiffCal, ConvertUnits
from mantid.simpleapi import AnalysisDataService as Ads

from Engineering.common import path_handling
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

VANADIUM_INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
PROCESSED_WORKSPACE_NAME = "engggui_vanadium_processed_instrument"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"


def fetch_correction_workspaces(vanadium_path: str, instrument: str):
    # -> Workspace2D, Workspace2D
    """
    Fetch workspaces from the ADS or create new ones.
    :param vanadium_path: The path to the requested vanadium run raw data.
    :param instrument: The instrument the data came from.
    """
    van_run_no = path_handling.get_run_number_from_path(vanadium_path, instrument)
    if not check_workspaces_exist(van_run_no):
        van_integration_ws, van_processed_inst_ws = create_vanadium_corrections(vanadium_path,
                                                                                instrument)
    else:
        van_integration_ws = Ads.retrieve(str(van_run_no) + '_' + INTEGRATED_WORKSPACE_NAME)
        van_processed_inst_ws = Ads.retrieve(str(van_run_no) + '_' + PROCESSED_WORKSPACE_NAME)
    return van_integration_ws, van_processed_inst_ws


def check_workspaces_exist(run_no) -> bool:
    """
    Check the vanadium workspaces are loaded into the ADS
    :return: True if both are present, false otherwise
    """
    return Ads.doesExist(str(run_no) + '_' + PROCESSED_WORKSPACE_NAME) and Ads.doesExist(str(run_no) + '_' + INTEGRATED_WORKSPACE_NAME)


def create_vanadium_corrections(vanadium_path: str, instrument: str):  # -> Workspace, Workspace
    """
    Runs the vanadium correction algorithm.
    :param vanadium_path: The path to the vanadium data.
    :return: The integrated workspace and the processed instrument workspace generated.
    """
    try:
        run_no = path_handling.get_run_number_from_path(vanadium_path, instrument)
        van_ws = Load(Filename=vanadium_path, OutputWorkspace=str(run_no) + '_' + VANADIUM_INPUT_WORKSPACE_NAME)
    except Exception as e:
        logger.error("Error when loading vanadium sample data. "
                     "Could not run Load algorithm with vanadium run number: "
                     + str(vanadium_path) + ". Error description: " + str(e))
        raise RuntimeError
    # get full instrument calibration for instrument processing calculation
    if Ads.doesExist("full_inst_calib"):
        full_calib_ws = Ads.retrieve("full_inst_calib")
    else:
        full_calib_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP,
                                      output_settings.ENGINEERING_PREFIX, "full_calibration")
        try:
            full_calib_ws = Load(full_calib_path, OutputWorkspace="full_inst_calib")
        except ValueError:
            logger.error("Error loading Full instrument calibration - this is set in the interface settings.")
            return
    integral_ws = _calculate_vanadium_integral(van_ws, run_no)
    processed_ws = _calculate_vanadium_processed_instrument(van_ws, full_calib_ws, integral_ws, run_no)
    return integral_ws, processed_ws


def _calculate_vanadium_integral(van_ws, run_no):  # -> Workspace
    """
    Calculate the integral of the normalised vanadium workspace
    :param van_ws: Chosen vanadium run workspace
    :return: Integrated workspace
    """
    ws = NormaliseByCurrent(InputWorkspace=van_ws, OutputWorkspace=str(run_no) + "_" + INTEGRATED_WORKSPACE_NAME)  # create new name ws here
    # sensitivity correction for van
    ws_integ = Integration(InputWorkspace=ws, OutputWorkspace=ws)
    ws_integ /= van_ws.blocksize()
    return ws_integ


def _calculate_vanadium_processed_instrument(van_ws, full_calib_ws, integral_ws, run_no):  # -> Workspace
    """
    Calculate the processed whole-instrument workspace
    :param van_ws: Chosen vanadium run workspace
    :param full_calib_ws: Table workspace from the full instrument calibration (output from PDCalibration)
    :param integral_ws: Vanadium integral workspace
    :return: Whole instrument processed workspace
    """
    ws = NormaliseByCurrent(InputWorkspace=van_ws, OutputWorkspace=str(run_no) + "_" + PROCESSED_WORKSPACE_NAME)  # create new name ws here
    ApplyDiffCal(InstrumentWorkspace=ws, CalibrationWorkspace=full_calib_ws)
    ConvertUnits(InputWorkspace=ws, OutputWorkspace=ws, Target='dSpacing')
    # van sensitivity correction
    ws /= integral_ws
    ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0, InfinityValue=0)
    return ws
