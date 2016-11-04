from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
import isis_powder.common as common


def focus(number, instrument, attenuate=True, van_norm=True):
    return _run_focus(run_number=number, perform_attenuation=attenuate,
                      perform_vanadium_norm=van_norm, instrument=instrument)


def _run_focus(instrument, run_number, perform_attenuation, perform_vanadium_norm):
    # Read
    read_ws = common._load_current_normalised_ws(number=run_number, instrument=instrument)
    input_workspace = instrument._do_tof_rebinning_focus(read_ws)  # Rebins for PEARL

    # Align / Focus
    cycle_information = instrument._get_cycle_information(run_number=run_number)
    calibration_file_paths = instrument._get_calibration_full_paths(cycle=cycle_information["cycle"])
    # Compensate for empty sample if specified
    input_workspace = instrument._subtract_sample_empty(input_workspace)

    input_workspace = mantid.AlignDetectors(InputWorkspace=input_workspace,
                                            CalibrationFile=calibration_file_paths["calibration"])

    input_workspace = instrument._apply_solid_angle_efficiency_corr(ws_to_correct=input_workspace,
                                                                    vanadium_ws_path=calibration_file_paths["vanadium"])

    input_workspace = mantid.DiffractionFocussing(InputWorkspace=input_workspace,
                                                  GroupingFileName=calibration_file_paths["grouping"])

    # Process
    calibrated_spectra = instrument._focus_load(run_number, input_workspace, perform_vanadium_norm)

    common.remove_intermediate_workspace(read_ws)
    common.remove_intermediate_workspace(input_workspace)

    # Output
    processed_nexus_files = instrument._process_output(calibrated_spectra, run_number, attenuate=perform_attenuation)

    for ws in calibrated_spectra:
        common.remove_intermediate_workspace(ws)

    return processed_nexus_files
