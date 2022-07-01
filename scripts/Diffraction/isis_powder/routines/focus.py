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


def focus(run_number_string, instrument, perform_vanadium_norm, absorb, sample_details=None,
          empty_can_subtraction_method="Simple", paalman_pings_events_per_point=None):
    input_batching = instrument._get_input_batching_mode()
    if input_batching == INPUT_BATCHING.Individual:
        return _individual_run_focusing(instrument=instrument, perform_vanadium_norm=perform_vanadium_norm,
                                        run_number=run_number_string, absorb=absorb,
                                        sample_details=sample_details, empty_can_subtraction_method=empty_can_subtraction_method,
                                        paalman_pings_events_per_point=paalman_pings_events_per_point)
    elif input_batching == INPUT_BATCHING.Summed:
        return _batched_run_focusing(instrument, perform_vanadium_norm, run_number_string, absorb=absorb,
                                     sample_details=sample_details, empty_can_subtraction_method=empty_can_subtraction_method,
                                     paalman_pings_events_per_point=paalman_pings_events_per_point)
    else:
        raise ValueError("Input batching not passed through. Please contact development team.")


def _focus_one_ws(input_workspace, run_number, instrument, perform_vanadium_norm, absorb,
                  sample_details, vanadium_path, empty_can_subtraction_method, paalman_pings_events_per_point=None):
    run_details = instrument._get_run_details(run_number_string=run_number)
    if perform_vanadium_norm:
        _test_splined_vanadium_exists(instrument, run_details)

    # Subtract empty instrument runs, as long as this run isn't an empty, user hasn't turned empty subtraction off, or
    # The user has not supplied a sample empty
    is_run_empty = common.runs_overlap(run_number, run_details.empty_inst_runs)
    summed_empty = None
    if not is_run_empty and instrument.should_subtract_empty_inst() and not run_details.sample_empty:
        if os.path.isfile(run_details.summed_empty_inst_file_path):
            logger.warning('Pre-summed empty instrument workspace found at ' + run_details.summed_empty_inst_file_path)
            summed_empty = mantid.LoadNexus(Filename=run_details.summed_empty_inst_file_path)
        else:
            summed_empty = common.generate_summed_runs(empty_sample_ws_string=run_details.empty_inst_runs,
                                                       instrument=instrument)
    elif run_details.sample_empty:
        scale_factor = 1.0
        if empty_can_subtraction_method != 'PaalmanPings':
            scale_factor = instrument._inst_settings.sample_empty_scale
        # Subtract a sample empty if specified ie empty can
        summed_empty = common.generate_summed_runs(empty_sample_ws_string=run_details.sample_empty,
                                                   instrument=instrument,
                                                   scale_factor=scale_factor)

    if absorb and empty_can_subtraction_method == 'PaalmanPings':
        if run_details.sample_empty: # need summed_empty including container
            input_workspace = instrument._apply_paalmanpings_absorb_and_subtract_empty(
                                                        workspace=input_workspace,
                                                        summed_empty=summed_empty,
                                                        sample_details=sample_details,
                                                        paalman_pings_events_per_point=paalman_pings_events_per_point)
            # Crop to largest acceptable TOF range
            input_workspace = instrument._crop_raw_to_expected_tof_range(ws_to_crop=input_workspace)
        else:
            raise TypeError("The PaalmanPings absorption method requires 'sample_empty' to be supplied.")
    else:
        if summed_empty:
            input_workspace = common.subtract_summed_runs(ws_to_correct=input_workspace, empty_sample=summed_empty)
        # Crop to largest acceptable TOF range
        input_workspace = instrument._crop_raw_to_expected_tof_range(ws_to_crop=input_workspace)

        if absorb:
            input_workspace = instrument._apply_absorb_corrections(run_details=run_details,
                                                                   ws_to_correct=input_workspace)
        else:
            # Set sample material if specified by the user
            if sample_details is not None:
                mantid.SetSample(InputWorkspace=input_workspace,
                                 Geometry=sample_details.generate_sample_geometry(),
                                 Material=sample_details.generate_sample_material())

    # Align
    mantid.ApplyDiffCal(InstrumentWorkspace=input_workspace,
                        CalibrationFile=run_details.offset_file_path)
    aligned_ws = mantid.ConvertUnits(InputWorkspace=input_workspace, Target="dSpacing")

    solid_angle = instrument.get_solid_angle_corrections(run_details.vanadium_run_numbers, run_details)
    if solid_angle:
        aligned_ws = mantid.Divide(LHSWorkspace=aligned_ws, RHSWorkspace=solid_angle)
        mantid.DeleteWorkspace(solid_angle)

    # Focus the spectra into banks
    focused_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws,
                                             GroupingFileName=run_details.grouping_file_path)

    instrument.apply_calibration_to_focused_data(focused_ws)

    calibrated_spectra = _apply_vanadium_corrections(instrument=instrument,
                                                     input_workspace=focused_ws,
                                                     perform_vanadium_norm=perform_vanadium_norm,
                                                     vanadium_splines=vanadium_path)

    output_spectra = instrument._crop_banks_to_user_tof(calibrated_spectra)

    bin_widths = instrument._get_instrument_bin_widths()
    if bin_widths:
        # Reduce the bin width if required on this instrument
        output_spectra = common.rebin_workspace_list(workspace_list=output_spectra,
                                                     bin_width_list=bin_widths)

    # Output
    d_spacing_group, tof_group = instrument._output_focused_ws(output_spectra, run_details=run_details)

    common.keep_single_ws_unit(d_spacing_group=d_spacing_group, tof_group=tof_group,
                               unit_to_keep=instrument._get_unit_to_keep())

    # Tidy workspaces from Mantid
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(aligned_ws)
    common.remove_intermediate_workspace(focused_ws)
    common.remove_intermediate_workspace(output_spectra)

    return d_spacing_group


def _apply_vanadium_corrections(instrument, input_workspace, perform_vanadium_norm, vanadium_splines):
    input_workspace = mantid.ConvertUnits(InputWorkspace=input_workspace, OutputWorkspace=input_workspace, Target="TOF")
    split_data_spectra = common.extract_ws_spectra(input_workspace)

    if perform_vanadium_norm:
        processed_spectra = _normalize_spectra(spectra_list=split_data_spectra,
                                               vanadium_splines=vanadium_splines,
                                               instrument=instrument)
    else:
        processed_spectra = split_data_spectra

    return processed_spectra


def _batched_run_focusing(instrument, perform_vanadium_norm, run_number_string, absorb, sample_details,
                          empty_can_subtraction_method, paalman_pings_events_per_point=None):
    read_ws_list = common.load_current_normalised_ws_list(run_number_string=run_number_string,
                                                          instrument=instrument)
    run_details = instrument._get_run_details(run_number_string=run_number_string)
    vanadium_splines = None
    van = "van_{}".format(run_details.vanadium_run_numbers)
    if perform_vanadium_norm:
        if van not in mantid.mtd:
            vanadium_splines = mantid.LoadNexus(Filename=run_details.splined_vanadium_file_path,
                                                OutputWorkspace=van)
        else:
            vanadium_splines = mantid.mtd[van]
    output = None
    for ws in read_ws_list:
        output = _focus_one_ws(input_workspace=ws, run_number=run_number_string, instrument=instrument,
                               perform_vanadium_norm=perform_vanadium_norm, absorb=absorb,
                               sample_details=sample_details, vanadium_path=vanadium_splines,
                               empty_can_subtraction_method=empty_can_subtraction_method,
                               paalman_pings_events_per_point=paalman_pings_events_per_point)
    if instrument.get_instrument_prefix() == "PEARL" and vanadium_splines is not None :
        if hasattr(vanadium_splines, "OutputWorkspace"):
            vanadium_splines = vanadium_splines.OutputWorkspace
        mantid.DeleteWorkspace(vanadium_splines)
    return output


def _normalize_one_spectrum(single_spectrum_ws, spline, instrument):
    rebinned_spline = mantid.RebinToWorkspace(WorkspaceToRebin=spline, WorkspaceToMatch=single_spectrum_ws, StoreInADS=False)
    divided = mantid.Divide(LHSWorkspace=single_spectrum_ws, RHSWorkspace=rebinned_spline, StoreInADS=False)
    if instrument.get_instrument_prefix() == "GEM":
        values_replaced = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, StoreInADS=False)
        # crop based off max between 1000 and 2000 tof as the vanadium peak on Gem will always occur here
        complete = _crop_spline_to_percent_of_max(rebinned_spline, values_replaced, single_spectrum_ws, 1000, 2000)
    else:
        complete = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, OutputWorkspace=single_spectrum_ws)

    if instrument.perform_abs_vanadium_norm():
        vanadium_material = spline.sample().getMaterial()
        v_number_density = vanadium_material.numberDensityEffective
        v_cross_section = vanadium_material.totalScatterXSection()
        vanadium_shape = spline.sample().getShape()
        # number density in Angstroms-3, volume in m3. Don't bother with 1E30 factor because will cancel
        num_v_atoms = vanadium_shape.volume() * v_number_density

        sample_material = single_spectrum_ws.sample().getMaterial()
        sample_number_density = sample_material.numberDensityEffective
        sample_shape = spline.sample().getShape()
        num_sample_atoms = sample_shape.volume() * sample_number_density

        abs_norm_factor = v_number_density * v_cross_section * num_v_atoms / \
                          (sample_number_density * num_sample_atoms * 4 * math.pi)
        logger.debug("Performing absolute normalisation, multiplying by factor=" + str(abs_norm_factor))
        abs_norm_factor_ws = mantid.CreateSingleValuedWorkspace(DataValue=abs_norm_factor)
        complete = mantid.Multiply(LHSWorkspace=complete, RHSWorkspace=abs_norm_factor_ws,
                                   OutputWorkspace=single_spectrum_ws)

    return complete


def _normalize_spectra(spectra_list, vanadium_splines, instrument):
    if hasattr(vanadium_splines, "OutputWorkspace"):
        vanadium_splines = vanadium_splines.OutputWorkspace
    if type(vanadium_splines) is WorkspaceGroup:  # vanadium splines is a workspacegroup
        num_splines = len(vanadium_splines)
        num_spectra = len(spectra_list)
        if num_splines != num_spectra:
            raise RuntimeError("Mismatch between number of banks in vanadium and number of banks in workspace to focus"
                               "\nThere are {} banks for vanadium but {} for the run".format(num_splines, num_spectra))
        output_list = [_normalize_one_spectrum(data_ws, van_ws, instrument)
                       for data_ws, van_ws in zip(spectra_list, vanadium_splines)]
        return output_list
    output_list = [_normalize_one_spectrum(spectra_list[0], vanadium_splines, instrument)]
    return output_list


def _individual_run_focusing(instrument, perform_vanadium_norm, run_number, absorb, sample_details,
                             empty_can_subtraction_method, paalman_pings_events_per_point=None):
    # Load and process one by one
    run_numbers = common.generate_run_numbers(run_number_string=run_number)
    run_details = instrument._get_run_details(run_number_string=run_number)
    vanadium_splines = None
    van = "van_{}".format(run_details.vanadium_run_numbers)
    if perform_vanadium_norm:
        if van not in mantid.mtd:
            vanadium_splines = mantid.LoadNexus(Filename=run_details.splined_vanadium_file_path,
                                                OutputWorkspace=van)
        else:
            vanadium_splines = mantid.mtd[van]

    output = None
    for run in run_numbers:
        ws = common.load_current_normalised_ws_list(run_number_string=run, instrument=instrument)
        output = _focus_one_ws(input_workspace=ws[0], run_number=run, instrument=instrument, absorb=absorb,
                               perform_vanadium_norm=perform_vanadium_norm, sample_details=sample_details,
                               vanadium_path=vanadium_splines, empty_can_subtraction_method=empty_can_subtraction_method,
                               paalman_pings_events_per_point=paalman_pings_events_per_point)
    return output


def _test_splined_vanadium_exists(instrument, run_details):
    # Check the necessary splined vanadium file has been created
    if not os.path.isfile(run_details.splined_vanadium_file_path):
        raise ValueError("Processed vanadium runs not found at this path: "
                         + str(run_details.splined_vanadium_file_path)
                         + " \nHave you run the method to create a Vanadium spline with these settings yet?\n")


def _crop_spline_to_percent_of_max(spline, input_ws, output_workspace, min_value, max_value):
    spline_spectrum = spline.readY(0)
    if not spline_spectrum.any():
        return mantid.CloneWorkspace(inputWorkspace=input_ws, OutputWorkspace=output_workspace)

    x_list = input_ws.readX(0)
    min_index = x_list.searchsorted(min_value)
    max_index = x_list.searchsorted(max_value)
    sliced_spline_spectrum = spline_spectrum[min_index:max_index:1]
    y_val = numpy.amax(sliced_spline_spectrum)
    y_val = y_val / 100
    small_spline_indecies = numpy.nonzero(spline_spectrum > y_val)[0]
    x_max = x_list[small_spline_indecies[-1]]
    x_min = x_list[small_spline_indecies[0]]
    output = mantid.CropWorkspace(inputWorkspace=input_ws, XMin=x_min, XMax=x_max, OutputWorkspace=output_workspace)
    return output
