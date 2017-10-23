from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid
import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING, WORKSPACE_UNITS


def create_calibration(calibration_runs, instrument, offset_file_name, grouping_file_name, calibration_dir,
                       rebin_1_params, rebin_2_params, cross_correlate_params, get_det_offset_params):
    """
    Create a calibration file from (usually) a ceria run
    :param calibration_runs: Run number(s) for this run
    :param instrument: The PEARL instrument object
    :param offset_file_name: Name of the file to write detector offset information to
    :param grouping_file_name: Name of grouping calibration file
    :param calibration_dir: Path to directory containing calibration information
    :param rebin_1_params: Parameters for the first rebin step (as a string in the usual format)
    :param rebin_2_params: Parameters for the second rebin step (as a string in the usual format)
    :param cross_correlate_params: Parameters for CrossCorrelate (as a dictionary PropertyName: PropertyValue)
    :param get_det_offset_params: Parameters for GetDetectorOffsets (as a dictionary PropertyName: PropertyValue)
    :return:
    """
    input_ws_list = common.load_current_normalised_ws_list(run_number_string=calibration_runs, instrument=instrument,
                                                           input_batching=INPUT_BATCHING.Summed)
    input_ws = input_ws_list[0]
    if input_ws.getRunNumber() < 74758:
        raise Warning("Creating a calibration for run earlier than cycle 12_1. Default params are for cycles 12_1 "
                      "onwards - make sure you have set the calibration parameters accordingly")

    calibration_ws = mantid.Rebin(InputWorkspace=input_ws, Params=rebin_1_params)

    if calibration_ws.getAxis(0).getUnit().unitID() != WORKSPACE_UNITS.d_spacing:
        calibration_ws = mantid.ConvertUnits(InputWorkspace=calibration_ws, Target="dSpacing")

    rebinned = mantid.Rebin(InputWorkspace=calibration_ws, Params=rebin_2_params)
    cross_correlated = mantid.CrossCorrelate(InputWorkspace=rebinned, **cross_correlate_params)

    offset_file = os.path.join(calibration_dir, offset_file_name)
    mantid.GetDetectorOffsets(InputWorkspace=cross_correlated, GroupingFileName=offset_file, OutputWorkspace="offsets",
                              **get_det_offset_params)

    rebinned_tof = mantid.ConvertUnits(InputWorkspace=rebinned, Target="TOF")
    aligned = mantid.AlignDetectors(InputWorkspace=rebinned_tof, CalibrationFile=offset_file)

    grouping_file = os.path.join(calibration_dir, grouping_file_name)
    focused = mantid.DiffractionFocussing(InputWorkspace=aligned, GroupingFileName=grouping_file,
                                          OutputWorkspace=instrument._generate_output_file_name(calibration_runs)
                                          + "_grouped")

    # Note: offsets must be deleted as a string, as the simpleapi doesn't recognise it as a workspace
    common.remove_intermediate_workspace([calibration_ws, rebinned, cross_correlated,
                                          "offsets", rebinned_tof, aligned])

    return focused
