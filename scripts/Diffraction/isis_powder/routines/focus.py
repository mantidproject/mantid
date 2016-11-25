from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
import os


def focus(run_number, instrument, perform_attenuation=True, perform_vanadium_norm=True):
    # Read

    read_ws = common.load_current_normalised_ws(run_number_string=run_number, instrument=instrument)
    input_workspace = instrument.pearl_focus_tof_rebinning(read_ws)  # Rebins for PEARL
    run_details = instrument.get_run_details(run_number=run_number)

    # Check the necessary splined vanadium file has been created
    if not os.path.isfile(run_details.splined_vanadium):
        raise ValueError("Processed vanadium runs not found at this path: "
                         + str(run_details.splined_vanadium) +
                         " \n\nHave you created a vanadium calibration with these settings yet?")

    # Compensate for empty sample if specified
    input_workspace = common.subtract_sample_empty(ws_to_correct=input_workspace, instrument=instrument,
                                                   empty_sample_ws_string=run_details.sample_empty)

    # Align / Focus
    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                            CalibrationFile=run_details.calibration)

    input_workspace = instrument.apply_solid_angle_efficiency_corr(ws_to_correct=input_workspace,
                                                                   run_details=run_details)

    focused_ws = mantid.DiffractionFocussing(InputWorkspace=input_workspace,
                                             GroupingFileName=run_details.grouping)

    # Process
    rebinning_params = instrument.calculate_focus_binning_params(sample=focused_ws)

    calibrated_spectra = _divide_sample_by_vanadium(instrument=instrument, run_number=run_number,
                                                    input_workspace=focused_ws,
                                                    perform_vanadium_norm=perform_vanadium_norm)

    _apply_binning_to_spectra(spectra_list=calibrated_spectra, binning_list=rebinning_params)

    # Output
    processed_nexus_files = instrument.output_focused_ws(calibrated_spectra, run_details=run_details,
                                                         attenuate=perform_attenuation)

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
    run_details = instrument.get_run_details(run_number=run_number)
    num_of_banks = instrument.get_num_of_banks(run_details.instrument_version)

    for index in range(0, num_of_banks):
        if perform_vanadium_norm:
            vanadium_ws = mantid.LoadNexus(Filename=run_details.splined_vanadium, EntryNumber=index + 1)

            processed_spectra.append(
                instrument.correct_sample_vanadium(focused_ws=input_workspace, index=index, vanadium_ws=vanadium_ws))

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
