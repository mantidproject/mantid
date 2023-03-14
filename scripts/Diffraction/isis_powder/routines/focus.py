# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import WorkspaceGroup
import mantid.simpleapi as mantid
from mantid.kernel import logger

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

    summed_empty = _load_summed_empty(
        instrument,
        run_number,
        run_details,
        empty_can_subtraction_method,
    )

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

    solid_angle = instrument.get_solid_angle_corrections(run_details.vanadium_run_numbers, run_details)

    focused_ws = _calibrate_and_focus_workspace(
        input_workspace, run_details.offset_file_path, run_details.grouping_file_path, solid_angle=solid_angle
    )

    if instrument.get_instrument_prefix() != "OSIRIS":
        instrument.apply_calibration_to_focused_data(focused_ws)
        focused_ws = mantid.ConvertUnits(InputWorkspace=focused_ws, OutputWorkspace=focused_ws, Target="TOF")
    else:
        focused_ws = instrument.apply_drange_cropping(run_number, focused_ws)

    calibrated_spectra = common.extract_ws_spectra(focused_ws)
    if perform_vanadium_norm:
        calibrated_spectra = _apply_vanadium_corrections(
            instrument=instrument,
            input_spectra=calibrated_spectra,
            run_number=run_number,
            vanadium_ws=vanadium_ws,
        )

    output_spectra = instrument._crop_banks_to_user_tof(calibrated_spectra)

    bin_widths = instrument._get_instrument_bin_widths()
    if bin_widths:
        # Reduce the bin width if required on this instrument
        output_spectra = common.rebin_workspace_list(workspace_list=output_spectra, bin_width_list=bin_widths)

    # Tidy workspaces from Mantid
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(focused_ws)

    return output_spectra


def _load_summed_empty(
    instrument,
    run_number,
    run_details,
    empty_can_subtraction_method,
):
    # Subtract empty instrument runs, as long as this run isn't an empty, user hasn't turned empty subtraction off, or
    # The user has not supplied a sample empty
    is_run_empty = common.runs_overlap(run_number, run_details.empty_inst_runs)

    if not is_run_empty and instrument.should_subtract_empty_inst() and not run_details.sample_empty:
        if os.path.isfile(run_details.summed_empty_inst_file_path):
            logger.warning("Pre-summed empty instrument workspace found at " + run_details.summed_empty_inst_file_path)
            return mantid.LoadNexus(Filename=run_details.summed_empty_inst_file_path)
        else:
            return common.generate_summed_runs(empty_ws_string=run_details.empty_inst_runs, instrument=instrument)
    elif run_details.sample_empty:
        is_osiris = instrument.get_instrument_prefix() == "OSIRIS"

        scale_factor = 1.0
        if empty_can_subtraction_method != "PaalmanPings" and not is_osiris:
            scale_factor = instrument._inst_settings.sample_empty_scale

        # Subtract a sample empty if specified ie empty can
        return common.generate_summed_runs(
            empty_ws_string=run_details.sample_empty,
            instrument=instrument,
            scale_factor=scale_factor,
            normalised=not is_osiris,
        )

    return None


def _calibrate_and_focus_workspace(
    input_workspace,
    calibration_file,
    grouping_file_name,
    solid_angle=None,
):
    mantid.ApplyDiffCal(InstrumentWorkspace=input_workspace, CalibrationFile=calibration_file)
    aligned_ws = mantid.ConvertUnits(InputWorkspace=input_workspace, Target="dSpacing")

    if solid_angle:
        aligned_ws = mantid.Divide(LHSWorkspace=aligned_ws, RHSWorkspace=solid_angle)
        mantid.DeleteWorkspace(solid_angle)

    focused_ws = mantid.DiffractionFocussing(
        InputWorkspace=aligned_ws, GroupingFileName=grouping_file_name, OutputWorkspace=f"{input_workspace}_focused"
    )

    common.remove_intermediate_workspace(aligned_ws)

    return focused_ws


def _apply_vanadium_corrections(instrument, input_spectra, run_number, vanadium_ws):
    if instrument.get_instrument_prefix() == "OSIRIS":
        vanadium_ws = instrument.apply_drange_cropping(run_number, vanadium_ws.OutputWorkspace)

    processed_spectra = _normalize_spectra(spectra_list=input_spectra, vanadium_ws=vanadium_ws, instrument=instrument)

    return processed_spectra


def _get_vanadium_ws(instrument, run_details):
    van = "van_{}".format(run_details.vanadium_run_numbers)
    if van in mantid.mtd:
        return mantid.mtd[van]

    if instrument.get_instrument_prefix() != "OSIRIS":
        van_path = run_details.splined_vanadium_file_path
    else:
        van_path = run_details.unsplined_vanadium_file_path

    if not os.path.isfile(van_path):
        raise ValueError(
            "Processed vanadium runs not found at this path: "
            + str(van_path)
            + " \nHave you run the method to create a Vanadium spline with these settings yet?\n"
        )

    return mantid.LoadNexus(Filename=van_path, OutputWorkspace=van)


def _batched_run_focusing(
    instrument,
    perform_vanadium_norm,
    run_number_string,
    absorb,
    sample_details,
    empty_can_subtraction_method,
    paalman_pings_events_per_point=None,
):
    read_ws_list = common.load_current_normalised_ws_list(run_number_string=run_number_string, instrument=instrument)

    vanadium_ws = None
    if perform_vanadium_norm:
        run_details = instrument._get_run_details(run_number_string=run_number_string)
        vanadium_ws = _get_vanadium_ws(instrument, run_details)

    output = []
    for ws in read_ws_list:
        focused_ws = _focus_one_ws(
            input_workspace=ws,
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


def _normalize_one_spectrum(single_spectrum_ws, vanadium_ws, instrument):
    rebinned_vanadium = mantid.RebinToWorkspace(WorkspaceToRebin=vanadium_ws, WorkspaceToMatch=single_spectrum_ws, StoreInADS=False)
    divided = mantid.Divide(LHSWorkspace=single_spectrum_ws, RHSWorkspace=rebinned_vanadium, StoreInADS=False)
    if instrument.get_instrument_prefix() == "GEM":
        values_replaced = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, StoreInADS=False)
        # crop based off max between 1000 and 2000 tof as the vanadium peak on Gem will always occur here
        complete = _crop_vanadium_to_percent_of_max(rebinned_vanadium, values_replaced, single_spectrum_ws, 1000, 2000)
    else:
        complete = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, OutputWorkspace=single_spectrum_ws)

    if instrument.perform_abs_vanadium_norm():
        vanadium_material = vanadium_ws.sample().getMaterial()
        v_number_density = vanadium_material.numberDensityEffective
        v_cross_section = vanadium_material.totalScatterXSection()
        vanadium_shape = vanadium_ws.sample().getShape()
        # number density in Angstroms-3, volume in m3. Don't bother with 1E30 factor because will cancel
        num_v_atoms = vanadium_shape.volume() * v_number_density

        sample_material = single_spectrum_ws.sample().getMaterial()
        sample_number_density = sample_material.numberDensityEffective
        sample_shape = vanadium_ws.sample().getShape()
        num_sample_atoms = sample_shape.volume() * sample_number_density

        abs_norm_factor = v_cross_section * num_v_atoms / (num_sample_atoms * 4 * math.pi)
        logger.notice("Performing absolute normalisation, multiplying by factor=" + str(abs_norm_factor))
        # avoid "Variable invalidated, data has been deleted" error when debugging
        output_ws_name = single_spectrum_ws.name()
        abs_norm_factor_ws = mantid.CreateSingleValuedWorkspace(DataValue=abs_norm_factor, OutputWorkspace="__abs_norm_factor_ws")
        complete = mantid.Multiply(LHSWorkspace=complete, RHSWorkspace=abs_norm_factor_ws, OutputWorkspace=output_ws_name)

    return complete


def _normalize_spectra(spectra_list, vanadium_ws, instrument):
    if hasattr(vanadium_ws, "OutputWorkspace"):
        vanadium_ws = vanadium_ws.OutputWorkspace
    if type(vanadium_ws) is WorkspaceGroup:  # vanadium splines is a workspacegroup
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
