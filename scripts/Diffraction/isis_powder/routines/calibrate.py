from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import InputBatchingEnum


def create_van(instrument, van, empty, absorb, gen_absorb):
    run_details = instrument.get_run_details(run_number=van)
    # Always sum a range of inputs as its a vanadium run over multiple captures
    input_van_ws_list = common.load_current_normalised_ws_list(run_number_string=van, instrument=instrument,
                                                               input_batching=InputBatchingEnum.Summed)
    input_van_ws = input_van_ws_list[0]  # As we asked for a summed ws there should only be one returned
    corrected_van_ws = common.subtract_sample_empty(ws_to_correct=input_van_ws, empty_sample_ws_string=empty,
                                                    instrument=instrument)

    # Crop the tail end of the data on PEARL if they are not capturing slow neutrons
    corrected_van_ws = instrument.crop_data_tail(ws_to_crop=corrected_van_ws)

    corrected_van_ws = mantid.AlignDetectors(InputWorkspace=corrected_van_ws,
                                             CalibrationFile=run_details.calibration_file_path)

    corrected_van_ws = instrument.apply_solid_angle_efficiency_corr(ws_to_correct=corrected_van_ws,
                                                                    run_details=run_details)
    if absorb:
        corrected_van_ws = _apply_absorb_corrections(instrument=instrument,
                                                     run_details=run_details,
                                                     corrected_van_ws=corrected_van_ws, gen_absorb=gen_absorb)

    focused_vanadium = mantid.DiffractionFocussing(InputWorkspace=corrected_van_ws,
                                                   GroupingFileName=run_details.grouping_file_path)

    focused_cropped_spectra = instrument.extract_and_crop_spectra(focused_ws=focused_vanadium)
    d_spacing_group = _save_focused_vanadium(instrument=instrument, run_details=run_details,
                                             cropped_spectra=focused_cropped_spectra)
    _create_vanadium_splines(focused_cropped_spectra, instrument, run_details)

    common.remove_intermediate_workspace(corrected_van_ws)
    common.remove_intermediate_workspace(focused_vanadium)
    common.remove_intermediate_workspace(focused_cropped_spectra)

    return d_spacing_group


def _create_vanadium_splines(focused_spectra, instrument, run_details):
    splined_ws_list = instrument.spline_vanadium_ws(focused_spectra)
    out_spline_van_file_path = run_details.splined_vanadium_file_path
    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=out_spline_van_file_path, InputWorkspace=ws, Append=append)
        append = True
    mantid.GroupWorkspaces(InputWorkspaces=splined_ws_list, OutputWorkspace="Van_spline_data")


def _apply_absorb_corrections(instrument, run_details, corrected_van_ws, gen_absorb):
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="Wavelength")

    if gen_absorb or not run_details.vanadium_absorption_path:
        absorb_ws = instrument.generate_vanadium_absorb_corrections(run_details, corrected_van_ws)
    else:
        absorb_ws = mantid.LoadNexus(Filename=run_details.vanadium_absorption_path)

    # PEARL rebins whilst POLARIS does not as some of the older absorption files have different number of bins
    corrected_van_ws = instrument.pearl_rebin_to_workspace(ws_to_rebin=corrected_van_ws, ws_to_match=absorb_ws)
    corrected_van_ws = mantid.Divide(LHSWorkspace=corrected_van_ws, RHSWorkspace=absorb_ws)

    #  corrected_van_ws = instrument.vanadium_calibration_rebinning(vanadium_ws=corrected_van_ws)

    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="dSpacing")
    common.remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws


def _save_focused_vanadium(instrument, run_details, cropped_spectra):
    d_spacing_group = instrument.output_focused_ws(processed_spectra=cropped_spectra,
                                                   run_details=run_details, output_mode="all")
    return d_spacing_group
