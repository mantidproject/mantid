from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import InputBatchingEnum


def create_van(instrument, van, empty, output_van_file_name, absorb, gen_absorb):
    run_details = instrument.get_run_details(run_number=van)
    # Always sum a range of inputs as its a vanadium run over multiple captures
    input_van_ws_list = common.load_current_normalised_ws_list(run_number_string=van, instrument=instrument,
                                                               input_batching=InputBatchingEnum.Summed)
    input_van_ws = input_van_ws_list[0]  # As we asked for a summed ws there should only be one returned
    corrected_van_ws = common.subtract_sample_empty(ws_to_correct=input_van_ws, empty_sample_ws_string=empty,
                                                    instrument=instrument)

    # TODO get POLARIS to crop the tail end of data here
    corrected_van_ws = instrument.crop_data_tail(ws_to_crop=corrected_van_ws)

    corrected_van_ws = mantid.AlignDetectors(InputWorkspace=corrected_van_ws,
                                             CalibrationFile=run_details.calibration)

    corrected_van_ws = instrument.apply_solid_angle_efficiency_corr(ws_to_correct=corrected_van_ws,
                                                                    run_details=run_details)
    if absorb:
        corrected_van_ws = _apply_absorb_corrections(instrument=instrument,
                                                     run_details=run_details,
                                                     corrected_van_ws=corrected_van_ws, gen_absorb=gen_absorb)

    focused_van_file = mantid.DiffractionFocussing(InputWorkspace=corrected_van_ws,
                                                   GroupingFileName=run_details.grouping)
    # TODO get POLARIS to apply rebinning here too
    focused_van_file = instrument. vanadium_calibration_rebinning(vanadium_ws=focused_van_file)

    focused_output = _save_focused_vanadium(instrument=instrument, run_details=run_details,
                                            focused_vanadium_ws=focused_van_file)
    common.remove_intermediate_workspace(corrected_van_ws)

    splined_ws_list = instrument.spline_vanadium_ws(focused_van_file, run_details.instrument_version)
    if output_van_file_name:
        # The user has manually specified the output file
        out_spline_van_file_path = os.path.join(instrument.calibration_dir, output_van_file_name)
    elif run_details.splined_vanadium:
        out_spline_van_file_path = run_details.splined_vanadium
    else:
        raise ValueError("The output name must be manually specified for this instrument/run")

    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=out_spline_van_file_path, InputWorkspace=ws, Append=append)
        common.remove_intermediate_workspace(ws)
        append = True

    output_ws = mantid.LoadNexus(Filename=out_spline_van_file_path, OutputWorkspace="Van_spline_data")
    return output_ws


def _apply_absorb_corrections(instrument, run_details, corrected_van_ws, gen_absorb):
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="Wavelength")

    if gen_absorb or not run_details.vanadium_absorption:
        absorb_ws = instrument.generate_vanadium_absorb_corrections(run_details, corrected_van_ws)
    else:
        absorb_ws = mantid.LoadNexus(Filename=run_details.vanadium_absorption)

    # PEARL rebins whilst POLARIS does not as some of the older absorption files have different number of bins
    corrected_van_ws = instrument.pearl_rebin_to_workspace(ws_to_rebin=corrected_van_ws, ws_to_match=absorb_ws)
    corrected_van_ws = mantid.Divide(LHSWorkspace=corrected_van_ws, RHSWorkspace=absorb_ws)

    #  corrected_van_ws = instrument.vanadium_calibration_rebinning(vanadium_ws=corrected_van_ws)

    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="dSpacing")
    common.remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws


def _save_focused_vanadium(instrument, run_details, focused_vanadium_ws):
    number_of_spectra = focused_vanadium_ws.getNumberHistograms()
    ws_spectra_list = []
    for i in range(0, number_of_spectra):
        out_spectra_name = focused_vanadium_ws.name() + "_bank-" + str(i + 1)
        ws_spectra_list.append(mantid.ExtractSingleSpectrum(InputWorkspace=focused_vanadium_ws, WorkspaceIndex=i,
                                                            OutputWorkspace=out_spectra_name))
    output_list = instrument.output_focused_ws(processed_spectra=ws_spectra_list, run_details=run_details,
                                               output_mode="all")
    common.remove_intermediate_workspace(ws_spectra_list)

    group_name = instrument.generate_inst_file_name(run_details.run_number) + "_focused"
    return mantid.GroupWorkspaces(InputWorkspaces=output_list, OutputWorkspace=group_name)
