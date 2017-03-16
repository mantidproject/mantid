from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.routines.common as common
from isis_powder.routines.common_enums import InputBatchingEnum


def create_van(instrument, van, empty, absorb, gen_absorb):
    run_details = instrument._get_run_details(run_number_string=van)
    # Always sum a range of inputs as its a vanadium run over multiple captures
    input_van_ws_list = common.load_current_normalised_ws_list(run_number_string=van, instrument=instrument,
                                                               input_batching=InputBatchingEnum.Summed)
    input_van_ws = input_van_ws_list[0]  # As we asked for a summed ws there should only be one returned
    corrected_van_ws = common.subtract_sample_empty(ws_to_correct=input_van_ws, empty_sample_ws_string=empty,
                                                    instrument=instrument)

    # Crop the tail end of the data on PEARL if they are not capturing slow neutrons
    corrected_van_ws = instrument._crop_raw_to_expected_tof_range(ws_to_crop=corrected_van_ws)

    aligned_ws = mantid.AlignDetectors(InputWorkspace=corrected_van_ws,
                                       CalibrationFile=run_details.calibration_file_path)

    if absorb:
        aligned_ws = _apply_absorb_corrections(instrument=instrument, run_details=run_details,
                                               van_ws=aligned_ws, gen_absorb=gen_absorb)

    focused_vanadium = mantid.DiffractionFocussing(InputWorkspace=aligned_ws,
                                                   GroupingFileName=run_details.grouping_file_path)

    focused_spectra = common.extract_ws_spectra(focused_vanadium)
    focused_spectra = instrument._crop_van_to_expected_tof_range(focused_spectra)

    d_spacing_group = _save_focused_vanadium(instrument=instrument, run_details=run_details,
                                             van_spectra=focused_spectra)

    _create_vanadium_splines(focused_spectra, instrument, run_details)

    common.remove_intermediate_workspace(corrected_van_ws)
    common.remove_intermediate_workspace(aligned_ws)
    common.remove_intermediate_workspace(focused_vanadium)
    common.remove_intermediate_workspace(focused_spectra)

    return d_spacing_group


def _create_vanadium_splines(focused_spectra, instrument, run_details):
    splined_ws_list = instrument._spline_vanadium_ws(focused_spectra)
    out_spline_van_file_path = run_details.splined_vanadium_file_path
    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=out_spline_van_file_path, InputWorkspace=ws, Append=append)
        append = True
    mantid.GroupWorkspaces(InputWorkspaces=splined_ws_list, OutputWorkspace="Van_spline_data")


def _apply_absorb_corrections(instrument, run_details, van_ws, gen_absorb):
    absorb_ws = instrument._apply_absorb_corrections(run_details, van_ws, gen_absorb=gen_absorb)
    return absorb_ws


def _save_focused_vanadium(instrument, run_details, van_spectra):
    d_spacing_group = instrument._output_focused_ws(processed_spectra=van_spectra,
                                                    run_details=run_details, output_mode="all")
    return d_spacing_group
