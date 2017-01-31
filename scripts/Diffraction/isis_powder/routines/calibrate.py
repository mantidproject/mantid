from __future__ import (absolute_import, division, print_function)

import os

import mantid.simpleapi as mantid

import isis_powder.routines.common as common


def create_van(instrument, van, empty, output_van_file_name, num_of_splines, absorb, gen_absorb):

    input_van_ws = common.load_current_normalised_ws(run_number_string=van, instrument=instrument)
    corrected_van_ws = common.subtract_sample_empty(ws_to_correct=input_van_ws, empty_sample_ws_string=empty,
                                                    instrument=instrument)

    common.remove_intermediate_workspace(input_van_ws)

    run_details = instrument._get_run_details(run_number=van)

    corrected_van_ws = instrument. _apply_van_calibration_tof_rebinning(vanadium_ws=corrected_van_ws,
                                                                        tof_rebin_pass=1, return_units="TOF")

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
    focused_van_file = instrument. _apply_van_calibration_tof_rebinning(vanadium_ws=focused_van_file,
                                                                        tof_rebin_pass=2, return_units="dSpacing")

    common.remove_intermediate_workspace(corrected_van_ws)

    cycle_information = instrument._get_run_details(run_number=van)
    splined_ws_list = instrument._spline_background(focused_van_file, num_of_splines,
                                                    cycle_information.instrument_version)
    # Figure out who will provide the path name
    if instrument._PEARL_filename_is_full_path():
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
        absorb_ws = instrument._generate_vanadium_absorb_corrections(run_details, corrected_van_ws)
    else:
        absorb_ws = mantid.LoadNexus(Filename=run_details.vanadium_absorption)

    # PEARL rebins whilst POLARIS does not as some of the older absorption files have different number of bins
    corrected_van_ws = instrument._calibration_rebin_to_workspace(ws_to_rebin=corrected_van_ws, ws_to_match=absorb_ws)
    corrected_van_ws = mantid.Divide(LHSWorkspace=corrected_van_ws, RHSWorkspace=absorb_ws)
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="dSpacing")
    common.remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws
