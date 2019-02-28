# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.api import WorkspaceGroup
import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import INPUT_BATCHING
import numpy
import os


def focus(run_number_string, instrument, perform_vanadium_norm, absorb, sample_details=None):
    input_batching = instrument._get_input_batching_mode()
    if input_batching == INPUT_BATCHING.Individual:
        return _individual_run_focusing(instrument=instrument, perform_vanadium_norm=perform_vanadium_norm,
                                        run_number=run_number_string, absorb=absorb, sample_details=sample_details)
    elif input_batching == INPUT_BATCHING.Summed:
        return _batched_run_focusing(instrument, perform_vanadium_norm, run_number_string, absorb=absorb,
                                     sample_details=sample_details)
    else:
        raise ValueError("Input batching not passed through. Please contact development team.")


def _focus_one_ws(input_workspace, run_number, instrument, perform_vanadium_norm, absorb, sample_details,
                  vanadium_path):
    run_details = instrument._get_run_details(run_number_string=run_number)
    if perform_vanadium_norm:
        _test_splined_vanadium_exists(instrument, run_details)

    # Subtract empty instrument runs, as long as this run isn't an empty and user hasn't turned empty subtraction off
    if not common.runs_overlap(run_number, run_details.empty_runs) and instrument.should_subtract_empty_inst():
        input_workspace = common.subtract_summed_runs(ws_to_correct=input_workspace, instrument=instrument,
                                                      empty_sample_ws_string=run_details.empty_runs)

    # Subtract a sample empty if specified
    if run_details.sample_empty:
        input_workspace = common.subtract_summed_runs(ws_to_correct=input_workspace, instrument=instrument,
                                                      empty_sample_ws_string=run_details.sample_empty,
                                                      scale_factor=instrument._inst_settings.sample_empty_scale)

    # Crop to largest acceptable TOF range
    input_workspace = instrument._crop_raw_to_expected_tof_range(ws_to_crop=input_workspace)

    # Correct for absorption / multiple scattering if required
    if absorb:
        input_workspace = instrument._apply_absorb_corrections(run_details=run_details, ws_to_correct=input_workspace)
    else:
        # Set sample material if specified by the user
        if sample_details is not None:
            mantid.SetSample(InputWorkspace=input_workspace,
                             Geometry=common.generate_sample_geometry(sample_details),
                             Material=common.generate_sample_material(sample_details))
    # Align
    aligned_ws = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                       CalibrationFile=run_details.offset_file_path)

    solid_angle = instrument.get_solid_angle_corrections(run_details.vanadium_run_numbers, run_details)
    if solid_angle:
        aligned_ws = mantid.Divide(LHSWorkspace=aligned_ws, RHSWorkspace=solid_angle)

    # Focus the spectra into banks
    focused_ws = mantid.DiffractionFocussing(InputWorkspace=aligned_ws,
                                             GroupingFileName=run_details.grouping_file_path)

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
        processed_spectra = _divide_by_vanadium_splines(spectra_list=split_data_spectra,
                                                        vanadium_splines=vanadium_splines,
                                                        instrument=instrument)
    else:
        processed_spectra = split_data_spectra

    return processed_spectra


def _batched_run_focusing(instrument, perform_vanadium_norm, run_number_string, absorb, sample_details):
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
                               sample_details=sample_details, vanadium_path=vanadium_splines)
    if instrument.get_instrument_prefix() == "PEARL" and vanadium_splines is not None :
        mantid.DeleteWorkspace(vanadium_splines.OutputWorkspace)
    return output


def _divide_one_spectrum_by_spline(spectrum, spline, instrument):
    rebinned_spline = mantid.RebinToWorkspace(WorkspaceToRebin=spline, WorkspaceToMatch=spectrum, StoreInADS=False)
    if instrument.get_instrument_prefix() == "GEM":
        divided = mantid.Divide(LHSWorkspace=spectrum, RHSWorkspace=rebinned_spline, OutputWorkspace=spectrum,
                                StoreInADS=False)
        complete = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, StoreInADS=False)
        # crop based off max between 1000 and 2000 tof as the vanadium peak on Gem will always occur here
        return _crop_spline_to_percent_of_max(rebinned_spline, complete, spectrum, 1000, 2000)

    divided = mantid.Divide(LHSWorkspace=spectrum, RHSWorkspace=rebinned_spline,
                            StoreInADS=False)
    complete = mantid.ReplaceSpecialValues(InputWorkspace=divided, NaNValue=0, OutputWorkspace=spectrum)

    return complete


def _divide_by_vanadium_splines(spectra_list, vanadium_splines, instrument):
    if hasattr(vanadium_splines, "OutputWorkspace"):
        vanadium_splines = vanadium_splines.OutputWorkspace
    if type(vanadium_splines) is WorkspaceGroup:  # vanadium splines is a workspacegroup
        num_splines = len(vanadium_splines)
        num_spectra = len(spectra_list)
        if num_splines != num_spectra:
            raise RuntimeError("Mismatch between number of banks in vanadium and number of banks in workspace to focus"
                               "\nThere are {} banks for vanadium but {} for the run".format(num_splines, num_spectra))
        output_list = [_divide_one_spectrum_by_spline(data_ws, van_ws, instrument)
                       for data_ws, van_ws in zip(spectra_list, vanadium_splines)]
        return output_list
    output_list = [_divide_one_spectrum_by_spline(spectra_list[0], vanadium_splines, instrument)]
    common.remove_intermediate_workspace(vanadium_splines)
    return output_list


def _individual_run_focusing(instrument, perform_vanadium_norm, run_number, absorb, sample_details):
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
                               vanadium_path=vanadium_splines)
    return output


def _test_splined_vanadium_exists(instrument, run_details):
    # Check the necessary splined vanadium file has been created
    if not os.path.isfile(run_details.splined_vanadium_file_path):
        raise ValueError("Processed vanadium runs not found at this path: "
                         + str(run_details.splined_vanadium_file_path) +
                         " \nHave you run the method to create a Vanadium spline with these settings yet?\n")


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
