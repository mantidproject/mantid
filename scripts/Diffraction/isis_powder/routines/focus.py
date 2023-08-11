# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import WorkspaceGroup
import mantid.simpleapi as mantid
from mantid.kernel import logger
from mantid.dataobjects import Workspace2D

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING
import math
import numpy
import os


def focus(
    run_number_string,
    instrument,
    perform_vanadium_norm,
    absorb,
    sample_details=None,
    empty_can_subtraction_method="Simple",
    paalman_pings_events_per_point=None,
):
    input_batching = instrument._get_input_batching_mode()
    if input_batching == INPUT_BATCHING.Individual:
        return _individual_run_focusing(
            instrument=instrument,
            perform_vanadium_norm=perform_vanadium_norm,
            run_number_string=run_number_string,
            absorb=absorb,
            sample_details=sample_details,
            empty_can_subtraction_method=empty_can_subtraction_method,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )
    elif input_batching == INPUT_BATCHING.Summed:
        return _batched_run_focusing(
            instrument,
            perform_vanadium_norm,
            run_number_string,
            absorb=absorb,
            sample_details=sample_details,
            empty_can_subtraction_method=empty_can_subtraction_method,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )
    else:
        raise ValueError("Input batching not passed through. Please contact development team.")


def _focus_one_ws(
    input_workspace,
    run_number,
    instrument,
    perform_vanadium_norm,
    absorb,
    sample_details,
    vanadium_ws,
    empty_can_subtraction_method,
    paalman_pings_events_per_point=None,
):
    run_details = instrument._get_run_details(run_number_string=run_number)

    # Subtract empty instrument runs, as long as this run isn't an empty, user hasn't turned empty subtraction off, or
    # The user has not supplied a sample empty
    is_run_empty = common.runs_overlap(run_number, run_details.empty_inst_runs)
    summed_empty = None
    if not is_run_empty and instrument.should_subtract_empty_inst() and not run_details.sample_empty:
        if os.path.isfile(run_details.summed_empty_inst_file_path):
            logger.warning("Pre-summed empty instrument workspace found at " + run_details.summed_empty_inst_file_path)
            summed_empty = mantid.LoadNexus(Filename=run_details.summed_empty_inst_file_path)
        else:
            summed_empty = common.generate_summed_runs(empty_ws_string=run_details.empty_inst_runs, instrument=instrument)
    elif run_details.sample_empty:
        scale_factor = 1.0
        if empty_can_subtraction_method != "PaalmanPings":
            scale_factor = instrument._inst_settings.sample_empty_scale
        # Subtract a sample empty if specified ie empty can
        summed_empty = common.generate_summed_runs(
            empty_ws_string=run_details.sample_empty, instrument=instrument, scale_factor=scale_factor
        )

    input_workspace = _absorb_and_empty_corrections(
        input_workspace,
        instrument,
        run_details,
        sample_details,
        absorb,
        summed_empty,
        empty_can_subtraction_method,
        paalman_pings_events_per_point,
    )
    if instrument._inst_settings.per_detector_vanadium:
        # per detector routine
        input_workspace = apply_per_detector_corrections(
            input_workspace,
            instrument,
            perform_vanadium_norm,
            vanadium_ws,
            sample_details,
            run_details,
        )

    # Align
    mantid.ApplyDiffCal(InstrumentWorkspace=input_workspace, CalibrationFile=run_details.offset_file_path)
    aligned_ws = mantid.ConvertUnits(InputWorkspace=input_workspace, Target="dSpacing")

    solid_angle = instrument.get_solid_angle_corrections(run_details.vanadium_run_numbers, run_details)
    if solid_angle:
        aligned_ws = mantid.Divide(LHSWorkspace=aligned_ws, RHSWorkspace=solid_angle)
        mantid.DeleteWorkspace(solid_angle)

    # must convert to point data before focussing
    if aligned_ws.isDistribution():
        aligned_ws = convert_between_distribution(aligned_ws, "From")
    # Focus the spectra into banks
    focused_ws = mantid.DiffractionFocussing(
        InputWorkspace=aligned_ws,
        GroupingFileName=run_details.grouping_file_path,
        OutputWorkspace=f"{run_number}_focused",
    )
    instrument.apply_calibration_to_focused_data(focused_ws)

    focused_ws = mantid.ConvertUnits(InputWorkspace=focused_ws, OutputWorkspace=focused_ws, Target="TOF")
    if perform_vanadium_norm:
        if instrument._inst_settings.per_detector_vanadium:
            focused_workspace = divide_by_number_of_detectors_in_bank(focused_ws, run_details.grouping_file_path)
            focused_workspace = convert_between_distribution(focused_workspace, "To")
            calibrated_spectra_list = common.extract_ws_spectra(focused_workspace)
        else:
            # per bank routine
            calibrated_spectra_list = _apply_vanadium_corrections(
                instrument=instrument, input_workspace=focused_ws, vanadium_ws=vanadium_ws
            )
    else:
        calibrated_spectra_list = common.extract_ws_spectra(focused_ws)

    output_spectra = instrument._crop_banks_to_user_tof(calibrated_spectra_list)

    bin_widths = instrument._get_instrument_bin_widths()
    if bin_widths:
        # Reduce the bin width if required on this instrument
        output_spectra = common.rebin_workspace_list(workspace_list=output_spectra, bin_width_list=bin_widths)

    # Tidy workspaces from Mantid
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(aligned_ws)
    common.remove_intermediate_workspace(focused_ws)

    return output_spectra


def _absorb_and_empty_corrections(
    input_workspace,
    instrument,
    run_details,
    sample_details,
    absorb,
    summed_empty,
    empty_can_subtraction_method,
    paalman_pings_events_per_point,
):
    if absorb and empty_can_subtraction_method == "PaalmanPings":
        if run_details.sample_empty:  # need summed_empty including container
            input_workspace = instrument._apply_paalmanpings_absorb_and_subtract_empty(
                workspace=input_workspace,
                summed_empty=summed_empty,
                sample_details=sample_details,
                paalman_pings_events_per_point=paalman_pings_events_per_point,
            )
            # Crop to largest acceptable TOF range
            input_workspace = instrument._crop_raw_to_expected_tof_range(ws_to_crop=input_workspace)
        else:
            raise TypeError("The PaalmanPings absorption method requires 'sample_empty' to be supplied.")
    else:
        if summed_empty:
            input_workspace = common.subtract_summed_runs(ws_to_correct=input_workspace, empty_ws=summed_empty)
        # Crop to largest acceptable TOF range
        input_workspace = instrument._crop_raw_to_expected_tof_range(ws_to_crop=input_workspace)
        if absorb:
            input_workspace = instrument._apply_absorb_corrections(run_details=run_details, ws_to_correct=input_workspace)
        else:
            # Set sample material if specified by the user
            if sample_details is not None:
                mantid.SetSample(
                    InputWorkspace=input_workspace,
                    Geometry=sample_details.generate_sample_geometry(),
                    Material=sample_details.generate_sample_material(),
                )
    return input_workspace


def apply_per_detector_corrections(input_workspace, instrument, perform_vanadium_norm, vanadium_ws, sample_details, run_details):
    # apply per detector vanadium correction on uncalibrated data
    input_workspace = _apply_vanadium_corrections_per_detector(
        instrument=instrument,
        input_workspace=input_workspace,
        perform_vanadium_norm=perform_vanadium_norm,
        vanadium_splines=vanadium_ws,
        run_details=run_details,
    )
    input_workspace = instrument.apply_additional_per_detector_corrections(input_workspace, sample_details, run_details)
    return input_workspace


def _apply_vanadium_corrections(instrument, input_workspace, vanadium_ws):
    split_data_spectra = common.extract_ws_spectra(input_workspace)

    processed_spectra = _normalize_spectra(spectra_list=split_data_spectra, vanadium_ws=vanadium_ws, instrument=instrument)

    return processed_spectra


def _get_vanadium_ws(instrument, run_details):
    van = "van_{}".format(run_details.vanadium_run_numbers)
    if van in mantid.mtd:
        return mantid.mtd[van]

    van_path = instrument.get_vanadium_path(run_details)

    if not os.path.isfile(van_path):
        raise ValueError(
            "Processed vanadium runs not found at this path: "
            + str(van_path)
            + " \nHave you run the method to create a Vanadium with these settings yet?\n"
        )

    return mantid.LoadNexus(Filename=van_path, OutputWorkspace=van)


def _apply_vanadium_corrections_per_detector(
    instrument, input_workspace: Workspace2D, perform_vanadium_norm, vanadium_splines: Workspace2D, run_details
):
    input_workspace = mantid.ConvertUnits(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, Target="TOF")
    if perform_vanadium_norm:
        input_workspace, vanadium_splines = common._remove_masked_and_monitor_spectra(
            data_workspace=input_workspace, correction_workspace=vanadium_splines, run_details=run_details
        )
        processed_workspace = _normalize_per_detector_workspace(input_workspace, vanadium_splines, instrument)
        processed_workspace = mantid.ReplaceSpecialValues(
            InputWorkspace=processed_workspace, OutputWorkspace=processed_workspace, NaNValue=0, InfinityValue=0
        )
    else:
        processed_workspace = input_workspace
    return processed_workspace


def divide_by_number_of_detectors_in_bank(focussed_data, cal_filepath):
    # Divide each spectrum by number of detectors in their bank
    cal_workspace = mantid.LoadCalFile(
        InputWorkspace=focussed_data,
        CalFileName=cal_filepath,
        WorkspaceName="cal_workspace",
        MakeOffsetsWorkspace=False,
        MakeMaskWorkspace=False,
        MakeGroupingWorkspace=True,
    )
    n_pixel = numpy.zeros(focussed_data.getNumberHistograms())
    for ws_index in range(cal_workspace.getNumberHistograms()):
        grouping = cal_workspace.dataY(ws_index)
        if grouping[0] > 0:
            n_pixel[int(grouping[0] - 1)] += 1
    number_detectors_in_bank_ws = mantid.CreateWorkspace(DataY=n_pixel, DataX=[0, 1], NSpec=focussed_data.getNumberHistograms())
    focussed_data = mantid.Divide(LHSWorkspace=focussed_data, RHSWorkspace=number_detectors_in_bank_ws, OutputWorkspace=focussed_data)
    common.remove_intermediate_workspace(number_detectors_in_bank_ws)
    common.remove_intermediate_workspace(cal_workspace)
    return focussed_data


def _batched_run_focusing(
    instrument,
    perform_vanadium_norm,
    run_number_string,
    absorb,
    sample_details,
    empty_can_subtraction_method,
    paalman_pings_events_per_point=None,
):
    # The mode will always be summed so this will return one workspace
    read_ws_list = common.load_current_normalised_ws_list(run_number_string=run_number_string, instrument=instrument)
    summed_sample_ws = read_ws_list[0]

    vanadium_ws = None
    if perform_vanadium_norm:
        run_details = instrument._get_run_details(run_number_string=run_number_string)
        vanadium_ws = _get_vanadium_ws(instrument, run_details)

    output = []
    focused_ws = _focus_one_ws(
        input_workspace=summed_sample_ws,
        run_number=run_number_string,
        instrument=instrument,
        perform_vanadium_norm=perform_vanadium_norm,
        absorb=absorb,
        sample_details=sample_details,
        vanadium_ws=vanadium_ws,
        empty_can_subtraction_method=empty_can_subtraction_method,
        paalman_pings_events_per_point=paalman_pings_events_per_point,
    )
    output.append(focused_ws)

    # clean up ws in this special case
    if instrument.get_instrument_prefix() == "PEARL" and vanadium_ws is not None:
        if hasattr(vanadium_ws, "OutputWorkspace"):
            vanadium_ws = vanadium_ws.OutputWorkspace
        mantid.DeleteWorkspace(vanadium_ws)
    return output


def _perform_absolute_normalization(spline, ws):
    vanadium_material = spline.sample().getMaterial()
    v_number_density = vanadium_material.numberDensityEffective
    v_cross_section = vanadium_material.totalScatterXSection()
    vanadium_shape = spline.sample().getShape()
    try:
        # number density in Angstroms-3, volume in m3. Don't bother with 1E30 factor because will cancel
        num_v_atoms = vanadium_shape.volume() * v_number_density
    except RuntimeError as exception:
        raise RuntimeError("Please set a valid shape on the vanadium workspace.") from exception

    sample_material = ws.sample().getMaterial()
    sample_number_density = sample_material.numberDensityEffective
    sample_shape = ws.sample().getShape()

    try:
        # number density in Angstroms-3, volume in m3. Don't bother with 1E30 factor because will cancel
        num_sample_atoms = sample_shape.volume() * sample_number_density
    except RuntimeError as exception:
        raise RuntimeError("Absolute unit normalization cannot be done without a sample material and shape/volume defined.") from exception

    abs_norm_factor = v_cross_section * num_v_atoms / (num_sample_atoms * 4 * math.pi)
    logger.notice("Performing absolute normalisation, multiplying by factor=" + str(abs_norm_factor))
    abs_norm_factor_ws = mantid.CreateSingleValuedWorkspace(DataValue=abs_norm_factor, OutputWorkspace="__abs_norm_factor_ws")
    ws = mantid.Multiply(LHSWorkspace=ws, RHSWorkspace=abs_norm_factor_ws, OutputWorkspace=ws)
    return ws


def _normalize_one_spectrum(single_spectrum_ws, vanadium_ws, instrument):
    rebinned_vanadium = mantid.RebinToWorkspace(WorkspaceToRebin=vanadium_ws, WorkspaceToMatch=single_spectrum_ws, StoreInADS=False)
    divided = mantid.Divide(LHSWorkspace=single_spectrum_ws, RHSWorkspace=rebinned_vanadium, StoreInADS=False)
    if instrument.get_instrument_prefix() == "GEM":
        values_replaced = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, StoreInADS=False)
        # crop based off max between 1000 and 2000 tof as the vanadium peak on Gem will always occur here
        complete = _crop_vanadium_to_percent_of_max(rebinned_vanadium, values_replaced, single_spectrum_ws, 1000, 2000)
    else:
        complete = mantid.ReplaceSpecialValues(
            InputWorkspace=divided,
            NaNValue=0,
            InfinityValue=0.0,
            BigNumberThreshold=1e13,
            SmallNumberThreshold=-1e13,
            OutputWorkspace=single_spectrum_ws,
        )

    if instrument.perform_abs_vanadium_norm():
        complete = _perform_absolute_normalization(rebinned_vanadium, complete)

    return complete


def _normalize_spectra(spectra_list, vanadium_ws, instrument):
    if hasattr(vanadium_ws, "OutputWorkspace"):
        vanadium_ws = vanadium_ws.OutputWorkspace
    if type(vanadium_ws) is WorkspaceGroup:  # vanadium is a workspacegroup
        num_vanadium_ws = len(vanadium_ws)
        num_spectra = len(spectra_list)
        if num_vanadium_ws != num_spectra:
            raise RuntimeError(
                "Mismatch between number of banks in vanadium and number of banks in workspace to focus"
                "\nThere are {} banks for vanadium but {} for the run".format(num_vanadium_ws, num_spectra)
            )
        output_list = [_normalize_one_spectrum(data_ws, van_ws, instrument) for data_ws, van_ws in zip(spectra_list, vanadium_ws)]
        return output_list
    output_list = [_normalize_one_spectrum(spectra_list[0], vanadium_ws, instrument)]
    return output_list


def _normalize_per_detector_workspace(multi_spectrum_ws, spline_ws, instrument):
    rebinned_spline = mantid.RebinToWorkspace(WorkspaceToRebin=spline_ws, WorkspaceToMatch=multi_spectrum_ws, StoreInADS=False)
    complete = mantid.Divide(
        LHSWorkspace=multi_spectrum_ws, RHSWorkspace=rebinned_spline, AllowDifferentNumberSpectra=True, OutputWorkspace=multi_spectrum_ws
    )

    if instrument.perform_abs_vanadium_norm():
        complete = _perform_absolute_normalization(spline_ws, complete)

    return complete


def _individual_run_focusing(
    instrument,
    perform_vanadium_norm,
    run_number_string,
    absorb,
    sample_details,
    empty_can_subtraction_method,
    paalman_pings_events_per_point=None,
):
    run_numbers = common.generate_run_numbers(run_number_string=run_number_string)
    run_details = instrument._get_run_details(run_number_string=run_number_string)

    vanadium_ws = None
    if perform_vanadium_norm:
        run_details = instrument._get_run_details(run_number_string=run_number_string)
        vanadium_ws = _get_vanadium_ws(instrument, run_details)

    output = []
    for run in run_numbers:
        ws = common.load_current_normalised_ws_list(run_number_string=run, instrument=instrument)
        focused_ws = _focus_one_ws(
            input_workspace=ws[0],
            run_number=run,
            instrument=instrument,
            absorb=absorb,
            perform_vanadium_norm=perform_vanadium_norm,
            sample_details=sample_details,
            vanadium_ws=vanadium_ws,
            empty_can_subtraction_method=empty_can_subtraction_method,
            paalman_pings_events_per_point=paalman_pings_events_per_point,
        )
        output.append(focused_ws)

    return output


def _crop_vanadium_to_percent_of_max(vanadium_ws, input_ws, output_workspace, min_value, max_value):
    vanadium_spectrum = vanadium_ws.readY(0)
    if not vanadium_spectrum.any():
        return mantid.CloneWorkspace(inputWorkspace=input_ws, OutputWorkspace=output_workspace)

    x_list = input_ws.readX(0)
    min_index = x_list.searchsorted(min_value)
    max_index = x_list.searchsorted(max_value)
    sliced_vanadium_spectrum = vanadium_spectrum[min_index:max_index:1]
    y_val = numpy.amax(sliced_vanadium_spectrum)
    y_val = y_val / 100
    small_vanadium_indecies = numpy.nonzero(vanadium_spectrum > y_val)[0]
    x_max = x_list[small_vanadium_indecies[-1]]
    x_min = x_list[small_vanadium_indecies[0]]
    output = mantid.CropWorkspace(inputWorkspace=input_ws, XMin=x_min, XMax=x_max, OutputWorkspace=output_workspace)
    return output


def convert_between_distribution(workspace, direction):
    # ensure conversion from and to distribution is always done in the same unit - arbitrarily use dSpacing here
    if direction != "To" and direction != "From":
        return workspace
    original_unit = workspace.getAxis(0).getUnit().unitID()
    workspace = mantid.ConvertUnits(InputWorkspace=workspace, OutputWorkspace=workspace, Target="dSpacing", EMode="Elastic")
    if direction == "To":
        mantid.ConvertToDistribution(workspace)
    if direction == "From":
        mantid.ConvertFromDistribution(workspace)
    workspace = mantid.ConvertUnits(InputWorkspace=workspace, OutputWorkspace=workspace, Target=original_unit, EMode="Elastic")
    return workspace
