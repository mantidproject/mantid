from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid
import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING, WORKSPACE_UNITS


def create_calibration(calibration_runs, instrument, offset_file_name, grouping_file_name, calibration_dir,
                       rebin_1_params, rebin_2_params, cross_correlate_params, get_det_offset_params):
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
    offsets = mantid.GetDetectorOffsets(InputWorkspace=cross_correlated, GroupingFileName=offset_file,
                                        **get_det_offset_params)

    rebinned_tof = mantid.ConvertUnits(InputWorkspace=rebinned, Target="TOF")
    aligned = mantid.AlignDetectors(InputWorkspace=rebinned_tof, CalibrationFile=offset_file)

    grouping_file = os.path.join(calibration_dir, grouping_file_name)
    focused = mantid.DiffractionFocussing(InputWorkspace=aligned, GroupingFileName=grouping_file,
                                          OutputWorkspace=instrument._generate_output_file_name(calibration_runs)
                                          + "_cal")

    #common.remove_intermediate_workspace(calibration_ws)
    #common.remove_intermediate_workspace(rebinned)
    #common.remove_intermediate_workspace(cross_correlated)
    #common.remove_intermediate_workspace(rebinned_tof)
    #common.remove_intermediate_workspace(aligned)

    return focused
