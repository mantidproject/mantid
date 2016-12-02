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

    corrected_van_ws = instrument. pearl_van_calibration_tof_rebinning(vanadium_ws=corrected_van_ws, return_units="TOF")

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

    # Optional
    focused_van_file = instrument. pearl_van_calibration_tof_rebinning(vanadium_ws=focused_van_file, return_units="dSpacing")

    common.remove_intermediate_workspace(corrected_van_ws)

    splined_ws_list = instrument.spline_vanadium_ws(focused_van_file, run_details.instrument_version)
    # Figure out who will provide the path name
    if instrument._old_api_pearl_filename_is_full_path():
        out_van_file_path = output_van_file_name
    elif output_van_file_name:
        # The user has manually specified the output file
        out_van_file_path = os.path.join(instrument.calibration_dir, output_van_file_name)
    elif run_details.splined_vanadium:
        out_van_file_path = run_details.splined_vanadium
    else:
        raise ValueError("The output name must be manually specified for this instrument/run")

    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=out_van_file_path, InputWorkspace=ws, Append=append)
        common.remove_intermediate_workspace(ws)
        append = True

    output_ws = mantid.LoadNexus(Filename=out_van_file_path, OutputWorkspace="Van_data")
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
    corrected_van_ws = instrument.pearl_van_calibration_tof_rebinning(vanadium_ws=corrected_van_ws,
                                                                      return_units="Wavelength")
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="dSpacing")
    common.remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws
