from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import isis_powder.common as common


def focus(number, instrument, attenuate=True, van_norm=True):
    return _run_focus(run_number=number, perform_attenuation=attenuate,
                      perform_vanadium_norm=van_norm, instrument=instrument)


def _run_focus(instrument, run_number, perform_attenuation, perform_vanadium_norm):
    # Read
    read_ws = common._load_current_normalised_ws(run_number=run_number, instrument=instrument)
    input_workspace = instrument._do_tof_rebinning_focus(read_ws)  # Rebins for PEARL

    calibration_file_paths = instrument._get_run_details(run_number_input=run_number)

    # Compensate for empty sample if specified
    input_workspace = instrument._subtract_sample_empty(input_workspace)

    # Align / Focus
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                            CalibrationFile=calibration_file_paths.calibration)

    input_workspace = instrument._apply_solid_angle_efficiency_corr(ws_to_correct=input_workspace,
                                                                    run_details=calibration_file_paths)

    focused_ws = mantid.DiffractionFocussing(InputWorkspace=input_workspace,
                                             GroupingFileName=calibration_file_paths.grouping)

    # Process
    rebinning_params = instrument.calculate_focus_binning_params(sample=focused_ws)

    calibrated_spectra = _divide_sample_by_vanadium(instrument=instrument, run_number=run_number,
                                                    input_workspace=focused_ws,
                                                    perform_vanadium_norm=perform_vanadium_norm)

    _apply_binning_to_spectra(spectra_list=calibrated_spectra, binning_list=rebinning_params)

    # Output
    processed_nexus_files = instrument._process_focus_output(calibrated_spectra, run_number, attenuate=perform_attenuation)

    # Tidy
    common.remove_intermediate_workspace(read_ws)
    common.remove_intermediate_workspace(input_workspace)
    common.remove_intermediate_workspace(focused_ws)
    for ws in calibrated_spectra:
        common.remove_intermediate_workspace(ws)
        pass

    return processed_nexus_files


def _divide_sample_by_vanadium(instrument, run_number, input_workspace, perform_vanadium_norm):
    processed_spectra = []

    run_details = instrument._get_run_details(run_number_input=run_number)

    alg_range, save_range = instrument._get_instrument_alg_save_ranges(run_details.instrument_version)

    for index in range(0, alg_range):
        if perform_vanadium_norm:
            vanadium_ws = mantid.LoadNexus(Filename=run_details.splined_vanadium, EntryNumber=index + 1)

            processed_spectra.append(
                instrument.correct_sample_vanadium(focused_ws=input_workspace, index=index, vanadium_ws=vanadium_ws))

            common.remove_intermediate_workspace(vanadium_ws)
        else:
            processed_spectra.append(
                instrument.correct_sample_vanadium(focused_ws=input_workspace, index=index))

    return processed_spectra


def _apply_binning_to_spectra(spectra_list, binning_list):
    if not binning_list:
        return

    for ws, bin_params in zip(spectra_list, binning_list):
        # Starting bin edge / bin width / last bin edge
        rebin_string = bin_params[0] + ',' + bin_params[1] + ',' + bin_params[2]
        mantid.Rebin(InputWorkspace=ws, OutputWorkspace=ws, Params=rebin_string)