from __future__ import (absolute_import, division, print_function)
import os
import numpy as numpy
import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines import yaml_parser
from isis_powder.routines.RunDetails import RunDetails
from isis_powder.pearl_routines import pearl_advanced_config


def attenuate_workspace(attenuation_file_path, ws_to_correct):
    wc_attenuated = mantid.PearlMCAbsorption(attenuation_file_path)
    wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
    wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=ws_to_correct,
                                            OutputWorkspace=wc_attenuated)
    pearl_attenuated_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=wc_attenuated)
    common.remove_intermediate_workspace(workspaces=wc_attenuated)
    return pearl_attenuated_ws


def generate_vanadium_absorb_corrections(van_ws):
    raise NotImplementedError("Generating absorption corrections needs to be implemented correctly")

    # TODO are these values applicable to all instruments
    shape_ws = mantid.CloneWorkspace(InputWorkspace=van_ws)
    mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                      <radius val="0.005" /> </sphere>')

    absorb_ws = \
        mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                    ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                    NumberOfWavelengthPoints="25", ElementSize="0.05")
    mantid.SaveNexus(Filename=calibration_full_paths["vanadium_absorption"],
                     InputWorkspace=absorb_ws, Append=False)
    common.remove_intermediate_workspace(shape_ws)
    return absorb_ws


def get_run_details(absorb_on, long_mode_on, run_number_string, calibration_dir, mapping_file):
    mapping_dict = yaml_parser.get_run_dictionary(run_number=run_number_string, file_path=mapping_file)

    calibration_file = mapping_dict["calibration_file"]
    empty_run_numbers = mapping_dict["empty_run_numbers"]
    label = mapping_dict["label"]
    splined_vanadium_name = _generate_splined_van_name(absorb_on=absorb_on, long_mode=long_mode_on,
                                                       vanadium_run_string=run_number_string)
    vanadium_run_numbers = mapping_dict["vanadium_run_numbers"]

    cycle_calibration_dir = os.path.join(calibration_dir, label)
    calibration_file_path = os.path.join(cycle_calibration_dir, calibration_file)
    splined_vanadium_path = os.path.join(cycle_calibration_dir, splined_vanadium_name)

    run_details = RunDetails(run_number=run_number_string)
    run_details.calibration_file_path = calibration_file_path
    run_details.empty_runs = empty_run_numbers
    run_details.label = label
    run_details.splined_vanadium_file_path = splined_vanadium_path
    run_details.vanadium_run_numbers = vanadium_run_numbers

    return run_details


def normalise_ws_current(ws_to_correct, monitor_ws, spline_coeff):
    lambda_lower = 0.03  # TODO move these into config
    lambda_upper = 6.00

    processed_monitor_ws = mantid.ConvertUnits(InputWorkspace=monitor_ws, Target="Wavelength")
    processed_monitor_ws = mantid.CropWorkspace(InputWorkspace=processed_monitor_ws,
                                                XMin=lambda_lower, XMax=lambda_upper)
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
                                              IntegrationRangeMin=0.6, IntegrationRangeMax=5.0,
                                              OutputWorkspace=normalised_ws)
    normalised_ws = mantid.ConvertUnits(InputWorkspace=normalised_ws, Target="TOF", OutputWorkspace=normalised_ws)

    common.remove_intermediate_workspace(processed_monitor_ws)
    common.remove_intermediate_workspace(splined_monitor_ws)

    return normalised_ws


def set_advanced_run_details(run_details, calibration_dir, tt_mode):
    advanced_dictionary = pearl_advanced_config.file_names

    grouping_file_path = os.path.join(calibration_dir, advanced_dictionary[tt_mode.lower() + "_grouping"])
    run_details.grouping_file_path = grouping_file_path

    absorption_file_path = os.path.join(calibration_dir, advanced_dictionary["vanadium_absorption"])
    run_details.vanadium_absorption_path = absorption_file_path
    return run_details


def _generate_splined_van_name(absorb_on, long_mode, vanadium_run_string):
    output_string = "SVan_" + str(vanadium_run_string)
    if absorb_on:
        output_string += "_absorb"
    if long_mode:
        output_string += "_long"

    output_string += ".nxs"
    return output_string
