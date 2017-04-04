from __future__ import (absolute_import, division, print_function)
import os
import numpy as numpy
import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines import yaml_parser
from isis_powder.routines.RunDetails import RunDetails


def attenuate_workspace(attenuation_file_path, ws_to_correct):
    original_units = ws_to_correct.getAxis(0).getUnit().unitID()
    wc_attenuated = mantid.PearlMCAbsorption(attenuation_file_path)
    wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                        Target=wc_attenuated.getAxis(0).getUnit().unitID())
    wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=ws_to_correct,
                                            OutputWorkspace=wc_attenuated)
    pearl_attenuated_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=wc_attenuated)
    common.remove_intermediate_workspace(workspaces=wc_attenuated)
    pearl_attenuated_ws = mantid.ConvertUnits(InputWorkspace=pearl_attenuated_ws, OutputWorkspace=pearl_attenuated_ws,
                                              Target=original_units)
    return pearl_attenuated_ws


def apply_vanadium_absorb_corrections(van_ws, run_details):
    absorb_ws = mantid.Load(Filename=run_details.vanadium_absorption_path)

    van_original_units = van_ws.getAxis(0).getUnit().unitID()
    absorb_units = absorb_ws.getAxis(0).getUnit().unitID()
    if van_original_units != absorb_units:
        van_ws = mantid.ConvertUnits(InputWorkspace=van_ws, Target=absorb_units, OutputWorkspace=van_ws)

    van_ws = mantid.RebinToWorkspace(WorkspaceToRebin=van_ws, WorkspaceToMatch=absorb_ws, OutputWorkspace=van_ws)
    van_ws = mantid.Divide(LHSWorkspace=van_ws, RHSWorkspace=absorb_ws, OutputWorkspace=van_ws)

    if van_original_units != absorb_units:
        van_ws = mantid.ConvertUnits(InputWorkspace=van_ws, Target=van_original_units, OutputWorkspace=van_ws)

    common.remove_intermediate_workspace(absorb_ws)
    return van_ws


def generate_out_name(run_number_string, absorb_on, long_mode_on, tt_mode):
    output_name = "PRL" + str(run_number_string)
    # Append each mode of operation
    output_name += "_" + str(tt_mode)
    output_name += "_absorb" if absorb_on else ""
    output_name += "_long" if long_mode_on else ""
    return output_name


def generate_vanadium_absorb_corrections(van_ws):
    raise NotImplementedError("Generating absorption corrections needs to be implemented correctly")

    # TODO are these values applicable to all instruments
    shape_ws = mantid.CloneWorkspace(InputWorkspace=van_ws)
    mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                      <radius val="0.005" /> </sphere>')

    calibration_full_paths = None
    absorb_ws = \
        mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                    ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                    NumberOfWavelengthPoints="25", ElementSize="0.05")
    mantid.SaveNexus(Filename=calibration_full_paths["vanadium_absorption"],
                     InputWorkspace=absorb_ws, Append=False)
    common.remove_intermediate_workspace(shape_ws)
    return absorb_ws


def get_run_details(run_number_string, inst_settings):
    first_run_number = common.get_first_run_number(run_number_string=run_number_string)
    mapping_dict = yaml_parser.get_run_dictionary(run_number_string=first_run_number,
                                                  file_path=inst_settings.cal_map_path)

    calibration_file_name = common.cal_map_dictionary_key_helper(mapping_dict, "calibration_file")
    empty_run_numbers = common.cal_map_dictionary_key_helper(mapping_dict, "empty_run_numbers")
    label = common.cal_map_dictionary_key_helper(mapping_dict, "label")
    vanadium_run_numbers = common.cal_map_dictionary_key_helper(mapping_dict, "vanadium_run_numbers")

    # Use generate out name to provide the unique fingerprint for this file
    splined_vanadium_name = common.generate_splined_name(
        generate_out_name(run_number_string=vanadium_run_numbers,
                          absorb_on=inst_settings.absorb_corrections,
                          long_mode_on=inst_settings.long_mode, tt_mode=inst_settings.tt_mode))

    calibration_dir = inst_settings.calibration_dir
    cycle_calibration_dir = os.path.join(calibration_dir, label)

    absorption_file_path = os.path.join(calibration_dir, inst_settings.van_absorb_file)
    calibration_file_path = os.path.join(cycle_calibration_dir, calibration_file_name)
    tt_grouping_key = str(inst_settings.tt_mode).lower() + '_grouping'
    try:
        grouping_file_path = os.path.join(calibration_dir, getattr(inst_settings, tt_grouping_key))
    except AttributeError:
        raise ValueError("The tt_mode: " + str(inst_settings.tt_mode).lower() + " is unknown")

    splined_vanadium_path = os.path.join(cycle_calibration_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=first_run_number)
    run_details.user_input_run_number = run_number_string
    run_details.offset_file_path = calibration_file_path
    run_details.grouping_file_path = grouping_file_path
    run_details.empty_runs = empty_run_numbers
    run_details.label = label
    run_details.splined_vanadium_file_path = splined_vanadium_path
    run_details.vanadium_absorption_path = absorption_file_path
    run_details.vanadium_run_numbers = vanadium_run_numbers

    return run_details


def normalise_ws_current(ws_to_correct, monitor_ws, spline_coeff, lambda_values, integration_range):
    processed_monitor_ws = mantid.ConvertUnits(InputWorkspace=monitor_ws, Target="Wavelength")
    processed_monitor_ws = mantid.CropWorkspace(InputWorkspace=processed_monitor_ws,
                                                XMin=lambda_values[0], XMax=lambda_values[-1])
    # TODO move these masks to the adv. config file
    ex_regions = numpy.zeros((2, 4))
    ex_regions[:, 0] = [3.45, 3.7]
    ex_regions[:, 1] = [2.96, 3.2]
    ex_regions[:, 2] = [2.1, 2.26]
    ex_regions[:, 3] = [1.73, 1.98]

    for reg in range(0, 4):
        processed_monitor_ws = mantid.MaskBins(InputWorkspace=processed_monitor_ws, XMin=ex_regions[0, reg],
                                               XMax=ex_regions[1, reg])

    splined_monitor_ws = mantid.SplineBackground(InputWorkspace=processed_monitor_ws,
                                                 WorkspaceIndex=0, NCoeff=spline_coeff)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target="Wavelength", OutputWorkspace=ws_to_correct)
    normalised_ws = mantid.NormaliseToMonitor(InputWorkspace=normalised_ws, MonitorWorkspace=splined_monitor_ws,
                                              IntegrationRangeMin=integration_range[0],
                                              IntegrationRangeMax=integration_range[-1],
                                              OutputWorkspace=normalised_ws)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=normalised_ws, Target="TOF", OutputWorkspace=normalised_ws)

    common.remove_intermediate_workspace(processed_monitor_ws)
    common.remove_intermediate_workspace(splined_monitor_ws)

    return normalised_ws


def strip_bragg_peaks(ws_list_to_correct):
    # TODO move hardcoded spline values into adv. config

    # Strip peaks on banks 1-12 (or 0-11 using 0 based index)
    for index, ws in enumerate(ws_list_to_correct[:12]):
        ws_list_to_correct[index] = mantid.StripPeaks(InputWorkspace=ws, OutputWorkspace=ws, FWHM=15, Tolerance=8)

    # Banks 13 / 14 have broad peaks which are missed so compensate for that and run twice as peaks are very broad
    for _ in range(2):
        ws_list_to_correct[12] = mantid.StripPeaks(InputWorkspace=ws_list_to_correct[12],
                                                   OutputWorkspace=ws_list_to_correct[12], FWHM=100, Tolerance=10)

        ws_list_to_correct[13] = mantid.StripPeaks(InputWorkspace=ws_list_to_correct[13],
                                                   OutputWorkspace=ws_list_to_correct[13], FWHM=60, Tolerance=10)

    return ws_list_to_correct
