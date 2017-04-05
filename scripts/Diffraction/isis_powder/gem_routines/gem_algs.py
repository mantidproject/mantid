from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid

from isis_powder.routines import absorb_corrections, common, RunDetails, yaml_parser
from isis_powder.gem_routines import gem_advanced_config


def calculate_absorb_corrections(ws_to_correct, multiple_scattering):
    # First 100 detectors are monitors or not connected to DAE
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(0, 101)))

    absorb_dict = gem_advanced_config.absorption_correction_params
    ws_to_correct = absorb_corrections.run_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering, config_dict=absorb_dict)
    return ws_to_correct


def get_run_details(run_number_string, inst_settings):
    run_number = common.get_first_run_number(run_number_string=run_number_string)

    # Get calibration mapping file
    cycle_map = yaml_parser.get_run_dictionary(run_number_string=run_number_string,
                                               file_path=inst_settings.cal_mapping_path)

    label = common.cal_map_dictionary_key_helper(cycle_map, "label")
    offset_file_name = common.cal_map_dictionary_key_helper(cycle_map, "offset_file_name")

    chopper_dict = common.cal_map_dictionary_key_helper(cycle_map, inst_settings.mode)

    empty_runs = common.cal_map_dictionary_key_helper(chopper_dict, "empty_run_numbers")
    vanadium_runs = common.cal_map_dictionary_key_helper(chopper_dict, "vanadium_run_numbers")

    # For GEM the grouping and offset file are identical
    calibration_folder = os.path.normpath(os.path.expanduser(inst_settings.calibration_dir))
    grouping_file_path = os.path.join(calibration_folder, inst_settings.grouping_file_name)

    splined_vanadium_name = common.generate_splined_name(vanadium_runs, offset_file_name)

    label_calibration_folder = os.path.join(calibration_folder, label)
    offset_file_path = os.path.join(label_calibration_folder, offset_file_name)
    splined_file_path = os.path.join(label_calibration_folder, splined_vanadium_name)

    run_details = RunDetails.RunDetails(run_number=run_number)
    run_details.empty_runs = empty_runs
    # Rarely used attribute so we will leave it as optional
    run_details.sample_empty = inst_settings.sample_empty if hasattr(inst_settings, "sample_empty") else None
    run_details.user_input_run_number = run_number_string
    run_details.label = label
    run_details.vanadium_run_numbers = vanadium_runs

    run_details.grouping_file_path = grouping_file_path
    run_details.offset_file_path = offset_file_path

    run_details.splined_vanadium_file_path = splined_file_path

    return run_details
