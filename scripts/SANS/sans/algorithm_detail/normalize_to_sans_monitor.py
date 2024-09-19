# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""SANSNormalizeToMonitor algorithm calculates the normalization to the monitor."""

import json

from SANS.sans.common.constants import EMPTY_NAME
from SANS.sans.common.general_functions import create_unmanaged_algorithm
from SANS.sans.state.StateObjects.StateNormalizeToMonitor import StateNormalizeToMonitor
from SANS.sans.state.StateObjects.StateWavelength import StateWavelength


def normalize_to_monitor(
    normalize_to_monitor: StateNormalizeToMonitor, wavelengths: StateWavelength, workspace, wav_range, scale_factor=1.0
):
    # 1. Extract the spectrum of the incident monitor
    incident_monitor_spectrum_number = normalize_to_monitor.incident_monitor
    workspace = _extract_monitor(workspace=workspace, spectrum_number=incident_monitor_spectrum_number)

    # 2. Multiply the workspace by the specified scaling factor.
    if scale_factor != 1.0:
        workspace = _scale(workspace, scale_factor)

    # 3. Remove the prompt peak (if it exists)
    workspace = _perform_prompt_peak_correction(workspace, normalize_to_monitor)

    # 4. Perform a flat background correction
    workspace = _perform_flat_background_correction(workspace, normalize_to_monitor)

    # 5. Convert to wavelength with the specified bin settings.
    workspace = _convert_to_wavelength(workspace, wavelengths, normalize_to_monitor, wav_range)

    return workspace


def _scale(workspace, factor):
    """
    The incident monitor is scaled by a factor.

    When we work with sliced event data, then we need to slice the monitor data set accordingly. The monitor
    needs to be scaled by the slice factor which one gets when operating slice_sans_event. If this was not performed,
    then we would be comparing the full monitor data with only parts of the detector data.
    :param workspace: the workspace to scale.
    :param factor: the scaling factor.
    :return: a scaled workspace.
    """
    scale_name = "Scale"
    scale_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME, "Factor": factor, "Operation": "Multiply"}
    scale_alg = create_unmanaged_algorithm(scale_name, **scale_options)
    scale_alg.execute()
    return scale_alg.getProperty("OutputWorkspace").value


def _extract_monitor(workspace, spectrum_number):
    """
    The extracts a single spectrum from the input workspace.

    We are only interested in the incident monitor here.
    :param workspace: The workspace to extract from
    :param spectrum_number: the spectrum number of the incident beam monitor.
    :return: a workspace which only contains the incident beam spectrum.
    """
    workspace_index = workspace.getIndexFromSpectrumNumber(spectrum_number)
    extract_name = "ExtractSingleSpectrum"
    extract_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME, "WorkspaceIndex": workspace_index}
    extract_alg = create_unmanaged_algorithm(extract_name, **extract_options)
    extract_alg.execute()
    return extract_alg.getProperty("OutputWorkspace").value


def _perform_prompt_peak_correction(workspace, normalize_to_monitor_state):
    """
    Performs a prompt peak correction.

    A prompt peak can occur when very fast neutrons shoot through the measurement. This can happen when working
    with two time regimes. Prompt peaks are prominent peaks which stand out from usual data. They occur frequently
    on LOQ, but are now also a possibility on other instruments. We deal with them, by removing the data and
    interpolating between the edge data points. If the user does not specify a start and stop time for the
    prompt peak, then this correction is not performed.
    :param workspace: the workspace which is to be corrected.
    :param normalize_to_monitor_state: a SANSStateNormalizeToMonitor object.
    :return: the corrected workspace.
    """
    prompt_peak_correction_start = normalize_to_monitor_state.prompt_peak_correction_min
    prompt_peak_correction_stop = normalize_to_monitor_state.prompt_peak_correction_max
    prompt_peak_correction_enabled = normalize_to_monitor_state.prompt_peak_correction_enabled
    # We perform only a prompt peak correction if the start and stop values of the bins we want to remove,
    # were explicitly set. Some instruments require it, others don't.
    if prompt_peak_correction_enabled and prompt_peak_correction_start is not None and prompt_peak_correction_stop is not None:
        remove_name = "RemoveBins"
        remove_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "XMin": prompt_peak_correction_start,
            "XMax": prompt_peak_correction_stop,
            "Interpolation": "Linear",
        }
        remove_alg = create_unmanaged_algorithm(remove_name, **remove_options)
        remove_alg.execute()
        workspace = remove_alg.getProperty("OutputWorkspace").value
    return workspace


def _perform_flat_background_correction(workspace, normalize_to_monitor_state):
    """
    Removes an offset from the monitor data.

    A certain region of the data set is selected which corresponds to only background data. This data is averaged
    which results in a mean background value which is subtracted from the data.
    :param workspace: the workspace which is to be corrected.
    :param normalize_to_monitor_state: a SANSStateNormalizeToMonitor object.
    :return: the corrected workspace.
    """

    # Get the time range for the for the background calculation. First check if there is an entry for the
    # incident monitor.
    incident_monitor_spectrum_number = normalize_to_monitor_state.incident_monitor
    incident_monitor_spectrum_as_string = str(incident_monitor_spectrum_number)
    background_tof_monitor_start = normalize_to_monitor_state.background_TOF_monitor_start
    background_tof_monitor_stop = normalize_to_monitor_state.background_TOF_monitor_stop

    if incident_monitor_spectrum_as_string in list(background_tof_monitor_start.keys()) and incident_monitor_spectrum_as_string in list(
        background_tof_monitor_stop.keys()
    ):
        start_tof = background_tof_monitor_start[incident_monitor_spectrum_as_string]
        stop_tof = background_tof_monitor_stop[incident_monitor_spectrum_as_string]
    else:
        start_tof = normalize_to_monitor_state.background_TOF_general_start
        stop_tof = normalize_to_monitor_state.background_TOF_general_stop

    # Only if a TOF range was set, do we have to perform a correction
    if start_tof and stop_tof:
        flat_name = "CalculateFlatBackground"
        flat_options = {"InputWorkspace": workspace, "OutputWorkspace": EMPTY_NAME, "StartX": start_tof, "EndX": stop_tof, "Mode": "Mean"}
        flat_alg = create_unmanaged_algorithm(flat_name, **flat_options)
        flat_alg.execute()
        workspace = flat_alg.getProperty("OutputWorkspace").value
    return workspace


def _convert_to_wavelength(workspace, wavelength_state: StateWavelength, norm_state: StateNormalizeToMonitor, wav_range):
    """
    Converts the workspace from time-of-flight units to wavelength units

    :param workspace: a time-of-flight workspace.
    :param wavelength_state: the current wavelength state
    :param norm_state: the current monitor normalization params
    :return: a wavelength workspace.
    """
    wavelength_step = wavelength_state.wavelength_interval.wavelength_step
    wavelength_step_type = wavelength_state.wavelength_step_type_lin_log
    convert_name = "SANSConvertToWavelengthAndRebin"
    convert_options = {
        "InputWorkspace": workspace,
        "WavelengthPairs": json.dumps([(wav_range[0], wav_range[1])]),
        "WavelengthStep": wavelength_step,
        "WavelengthStepType": wavelength_step_type.value,
        "RebinMode": norm_state.rebin_type.value,
    }

    convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
    convert_alg.setPropertyValue("OutputWorkspace", EMPTY_NAME)
    convert_alg.setProperty("OutputWorkspace", workspace)
    convert_alg.execute()
    group_ws = convert_alg.getProperty("OutputWorkspace").value
    return group_ws.getItem(0)
