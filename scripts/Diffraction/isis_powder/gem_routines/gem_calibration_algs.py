# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import mantid.simpleapi as mantid
import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING, WORKSPACE_UNITS


def create_calibration(
    calibration_runs,
    instrument,
    offset_file_name,
    grouping_file_name,
    calibration_dir,
    rebin_1_params,
    rebin_2_params,
    cross_correlate_params,
    get_det_offset_params,
):
    """
    Create a calibration file from (usually) a ceria run
    :param calibration_runs: Run number(s) for this run
    :param instrument: The GEM instrument object
    :param offset_file_name: Name of the file to write detector offset information to
    :param grouping_file_name: Name of grouping calibration file
    :param calibration_dir: Path to directory containing calibration information
    :param rebin_1_params: Parameters for the first rebin step (as a string in the usual format)
    :param rebin_2_params: Parameters for the second rebin step (as a string in the usual format)
    :param cross_correlate_params: Parameters for CrossCorrelate (as a dictionary PropertyName: PropertyValue)
    :param get_det_offset_params: Parameters for GetDetectorOffsets (as a dictionary PropertyName: PropertyValue)
    """
    input_ws_list = common.load_current_normalised_ws_list(
        run_number_string=calibration_runs, instrument=instrument, input_batching=INPUT_BATCHING.Summed
    )

    input_ws = input_ws_list[0]
    focused = _calibration_processing(
        calibration_dir,
        calibration_runs,
        cross_correlate_params,
        get_det_offset_params,
        grouping_file_name,
        input_ws,
        instrument,
        offset_file_name,
        rebin_1_params,
        rebin_2_params,
    )
    return focused


def adjust_calibration(
    calibration_runs,
    instrument,
    offset_file_name,
    grouping_file_name,
    calibration_dir,
    rebin_1_params,
    rebin_2_params,
    cross_correlate_params,
    get_det_offset_params,
    original_cal,
):
    """
    Create a calibration file from (usually) a ceria run
    :param calibration_runs: Run number(s) for this run
    :param instrument: The GEM instrument object
    :param offset_file_name: Name of the file to write detector offset information to
    :param grouping_file_name: Name of grouping calibration file
    :param calibration_dir: Path to directory containing calibration information
    :param rebin_1_params: Parameters for the first rebin step (as a string in the usual format)
    :param rebin_2_params: Parameters for the second rebin step (as a string in the usual format)
    :param cross_correlate_params: Parameters for CrossCorrelate (as a dictionary PropertyName: PropertyValue)
    :param get_det_offset_params: Parameters for GetDetectorOffsets (as a dictionary PropertyName: PropertyValue)
    :param original_cal: path to calibration file to adjust
    """
    input_ws_list = common.load_current_normalised_ws_list(
        run_number_string=calibration_runs, instrument=instrument, input_batching=INPUT_BATCHING.Summed
    )

    input_ws = input_ws_list[0]
    mantid.ApplyDiffCal(InstrumentWorkspace=input_ws, CalibrationFile=original_cal)
    input_ws = mantid.ConvertUnits(InputWorkspace=input_ws, Target="dSpacing")
    mantid.ApplyDiffCal(InstrumentWorkspace=input_ws, ClearCalibration=True)
    offset_file = os.path.join(calibration_dir, offset_file_name)
    focused = _calibration_processing(
        calibration_dir,
        calibration_runs,
        cross_correlate_params,
        get_det_offset_params,
        grouping_file_name,
        input_ws,
        instrument,
        offset_file,
        rebin_1_params,
        rebin_2_params,
    )
    _adjust_cal_file(original_cal, offset_file)
    return focused


def _calibration_processing(
    calibration_dir,
    calibration_runs,
    cross_correlate_params,
    get_det_offset_params,
    grouping_file_name,
    input_ws,
    instrument,
    offset_file,
    rebin_1_params,
    rebin_2_params,
):
    calibration_ws = input_ws
    if calibration_ws.getAxis(0).getUnit().unitID() != WORKSPACE_UNITS.d_spacing:
        calibration_ws = mantid.Rebin(InputWorkspace=input_ws, Params=rebin_1_params)
        calibration_ws = mantid.ConvertUnits(InputWorkspace=calibration_ws, Target="dSpacing")
    spectrum_list = []
    for i in range(0, calibration_ws.getNumberHistograms()):
        try:
            calibration_ws.getDetector(i)

        except RuntimeError:
            pass
        else:
            spectrum_list.append(i)
    calibration_ws = mantid.ExtractSpectra(InputWorkspace=calibration_ws, WorkspaceIndexList=spectrum_list)
    rebinned = mantid.Rebin(InputWorkspace=calibration_ws, Params=rebin_2_params)
    cross_correlated = mantid.CrossCorrelate(InputWorkspace=rebinned, **cross_correlate_params)

    # Offsets workspace must be referenced as string so it can be deleted, as simpleapi doesn't recognise it as a ws
    offsets_ws_name = "offsets"
    mantid.GetDetectorOffsets(
        InputWorkspace=cross_correlated, GroupingFileName=offset_file, OutputWorkspace=offsets_ws_name, **get_det_offset_params
    )
    rebinned_tof = mantid.ConvertUnits(InputWorkspace=rebinned, Target="TOF")
    applyDiffCal = mantid.ApplyDiffCal(InputWorkspace=rebinned_tof, CalibrationFile=offset_file)
    aligned = mantid.ConvertUnits(InputWorkspace=applyDiffCal, Target="dSpacing")
    grouping_file = os.path.join(calibration_dir, grouping_file_name)
    focused = mantid.DiffractionFocussing(
        InputWorkspace=aligned,
        GroupingFileName=grouping_file,
        OutputWorkspace=instrument._generate_output_file_name(calibration_runs) + "_grouped",
    )
    print("Saved cal file to " + offset_file)
    common.remove_intermediate_workspace([calibration_ws, rebinned, cross_correlated, rebinned_tof, offsets_ws_name])
    return focused


def _adjust_cal_file(original_cal, generated_cal):
    origin_ws = "origin{}"
    gen_ws = "newCal{}"
    out_ws = "adjusted_cal"
    mantid.LoadCalFile(
        InstrumentName="Gem",
        MakeGroupingWorkspace=False,
        MakeMaskWorkspace=False,
        MakeOffsetsWorkspace=True,
        WorkspaceName=origin_ws.format(""),
        CalFilename=original_cal,
    )
    mantid.LoadCalFile(
        InstrumentName="Gem",
        MakeGroupingWorkspace=False,
        MakeMaskWorkspace=False,
        MakeOffsetsWorkspace=True,
        WorkspaceName=gen_ws.format(""),
        CalFilename=generated_cal,
    )
    mantid.Plus(LHSWorkspace=origin_ws.format("_offsets"), RHSWorkspace=gen_ws.format("_offsets"), OutputWorkspace=out_ws)
    mantid.SaveCalFile(OffsetsWorkspace=out_ws, Filename=generated_cal)
    common.remove_intermediate_workspace(
        [origin_ws.format("_offsets"), gen_ws.format("_offsets"), origin_ws.format("_cal"), gen_ws.format("_cal")]
    )
