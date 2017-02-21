from __future__ import (absolute_import, division, print_function)

import os
import mantid.simpleapi as mantid

from isis_powder.routines import common, RunDetails, yaml_parser
from isis_powder.gem_routines import gem_advanced_config


def calculate_absorb_corrections(ws_to_correct, multiple_scattering):
    # First 100 detectors are monitors or not connected to DAE
    mantid.MaskDetectors(ws_to_correct, SpectraList=list(range(0, 101)))

    absorb_dict = gem_advanced_config.absorption_correction_params

    height_key = "cylinder_sample_height"
    radius_key = "cylinder_sample_radius"
    pos_key = "cylinder_position"
    formula_key = "chemical_formula"

    e_msg = "The following key was not found in the advanced configuration for sample correction:\n"

    height = common.dictionary_key_helper(dictionary=absorb_dict, key=height_key, exception_msg=e_msg + height_key)
    radius = common.dictionary_key_helper(dictionary=absorb_dict, key=radius_key, exception_msg=e_msg + radius_key)
    pos = common.dictionary_key_helper(dictionary=absorb_dict, key=pos_key, exception_msg=e_msg + pos_key)

    formula = common.dictionary_key_helper(dictionary=absorb_dict, key=formula_key, exception_msg=e_msg + formula_key)

    ws_to_correct = common.calculate__cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering,
        c_height=height, c_radius=radius, c_pos=pos, chemical_formula=formula)

    return ws_to_correct


def get_run_details(run_number_string, inst_settings):
    run_number = common.get_first_run_number(run_number_string=run_number_string)

    # Get calibration mapping file
    cycle_map = yaml_parser.get_run_dictionary(run_number_string=run_number_string,
                                               file_path=inst_settings.cal_mapping_path)

    label = common.cal_map_dictionary_key_helper(cycle_map, "label")
    offset_file_name = common.cal_map_dictionary_key_helper(cycle_map, "offset_file_name")
    empty_runs = common.cal_map_dictionary_key_helper(cycle_map, "empty_run_numbers")
    vanadium_runs = common.cal_map_dictionary_key_helper(cycle_map, "vanadium_run_numbers")

    # For GEM the grouping and offset file are identical
    calibration_folder = os.path.normpath(os.path.expanduser(inst_settings.calibration_dir))
    label_calibration_folder = os.path.join(calibration_folder, label)

    splined_vanadium_name = common.generate_splined_name(vanadium_runs, offset_file_name)

    offset_file_path = os.path.join(label_calibration_folder, offset_file_name)
    splined_file_path = os.path.join(label_calibration_folder, splined_vanadium_name)

    # TODO generate splined vanadium name from common

    run_details = RunDetails.RunDetails(run_number=run_number)
    run_details.empty_runs = empty_runs
    run_details.user_input_run_number = run_number_string
    run_details.label = label
    run_details.vanadium_run_numbers = vanadium_runs

    run_details.grouping_file_path = offset_file_path
    run_details.offset_file_path = offset_file_path

    run_details.splined_vanadium_file_path = splined_file_path

    return run_details