from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

import isis_powder.common as common


def create_van(instrument, van, empty, output_van_file_name, num_of_splines, absorb, gen_absorb):
    cycle_information = instrument._get_cycle_information(van)

    input_van_ws = common._load_current_normalised_ws(number=van, instrument=instrument)
    input_empty_ws = common._load_current_normalised_ws(number=empty, instrument=instrument)

    corrected_van_ws = mantid.Minus(LHSWorkspace=input_van_ws, RHSWorkspace=input_empty_ws)

    common.remove_intermediate_workspace(input_empty_ws)
    common.remove_intermediate_workspace(input_van_ws)

    calibration_full_paths = instrument._get_calibration_full_paths(cycle=cycle_information["cycle"])

    # Absorb was here

    corrected_van_ws = instrument. _apply_van_calibration_tof_rebinning(vanadium_ws=corrected_van_ws,
                                                                        tof_rebin_pass=1, return_units="TOF")

    corrected_van_ws = mantid.AlignDetectors(InputWorkspace=corrected_van_ws,
                                             CalibrationFile=calibration_full_paths["calibration"])

    corrected_van_ws = instrument._apply_solid_angle_efficiency_corr(ws_to_correct=corrected_van_ws,
                                                                     vanadium_number=van)
    if absorb:
        corrected_van_ws = _apply_absorb_corrections(instrument=instrument,
                                                     calibration_full_paths=calibration_full_paths,
                                                     corrected_van_ws=corrected_van_ws, gen_absorb=gen_absorb)

    focused_van_file = mantid.DiffractionFocussing(InputWorkspace=corrected_van_ws,
                                                   GroupingFileName=calibration_full_paths["grouping"])

    # Optional
    focused_van_file = instrument. _apply_van_calibration_tof_rebinning(vanadium_ws=focused_van_file,
                                                                        tof_rebin_pass=2, return_units="dSpacing")

    common.remove_intermediate_workspace(corrected_van_ws)

    splined_ws_list = instrument._spline_background(focused_van_file, num_of_splines,
                                                    cycle_information["instrument_version"])

    if instrument._PEARL_filename_is_full_path():
        out_van_file_path = output_van_file_name
    else:
        out_van_file_path = instrument.calibration_dir + output_van_file_name

    append = False
    for ws in splined_ws_list:
        mantid.SaveNexus(Filename=out_van_file_path, InputWorkspace=ws, Append=append)
        common.remove_intermediate_workspace(ws)
        append = True

    output_ws = mantid.LoadNexus(Filename=out_van_file_path, OutputWorkspace="Van_data")
    return output_ws


def _apply_absorb_corrections(instrument, calibration_full_paths, corrected_van_ws, gen_absorb):
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="Wavelength")

    if gen_absorb:
        absorb_ws = instrument._generate_vanadium_absorb_corrections(calibration_full_paths, corrected_van_ws)
    else:
        absorb_ws = mantid.LoadNexus(Filename=calibration_full_paths["vanadium_absorption"])

    # PEARL rebins whilst POLARIS does not as some of the older absorption files have different number of bins
    corrected_van_ws = instrument._calibration_rebin_to_workspace(ws_to_rebin=corrected_van_ws, ws_to_match=absorb_ws)
    corrected_van_ws = mantid.Divide(LHSWorkspace=corrected_van_ws, RHSWorkspace=absorb_ws)
    corrected_van_ws = mantid.ConvertUnits(InputWorkspace=corrected_van_ws, Target="dSpacing")
    common.remove_intermediate_workspace(absorb_ws)
    return corrected_van_ws
