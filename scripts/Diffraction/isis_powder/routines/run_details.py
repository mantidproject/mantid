# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from isis_powder.routines import common, yaml_parser
import os


def create_run_details_object(run_number_string, inst_settings, is_vanadium_run, empty_run_number,
                              grouping_file_name, vanadium_string, splined_name_list=None, van_abs_file_name=None):
    """
    Creates and returns a run details object which holds various
    properties about the current run.
    :param run_number_string: The user string for the current run
    :param inst_settings: The current instrument object
    :param is_vanadium_run: Boolean of if the current run is a vanadium run
    :param empty_run_number: Empty run number(s) from mapping file
    :param grouping_file_name: Filename of the grouping file found in the calibration folder
    :param vanadium_string: Vanadium run number(s) from mapping file
    :param splined_name_list: (Optional) List of unique properties to generate a splined vanadium name from
    :param van_abs_file_name: (Optional) The name of the vanadium absorption file
    :return: RunDetails object with attributes set to applicable values
    """
    cal_map_dict = get_cal_mapping_dict(run_number_string=run_number_string,
                                        cal_mapping_path=inst_settings.cal_mapping_path)

    run_number = common.get_first_run_number(run_number_string=run_number_string)

    # Get names of files we will be using
    calibration_dir = os.path.normpath(os.path.expanduser(inst_settings.calibration_dir))
    label = common.cal_map_dictionary_key_helper(dictionary=cal_map_dict, key="label")
    offset_file_name = common.cal_map_dictionary_key_helper(dictionary=cal_map_dict, key="offset_file_name")

    # Prepend the properties used for creating a van spline so we can fingerprint the file
    new_splined_list = splined_name_list if splined_name_list else []
    new_splined_list.append(os.path.basename(offset_file_name))

    splined_van_name = common.generate_splined_name(vanadium_string, new_splined_list)
    unsplined_van_name = common.generate_unsplined_name(vanadium_string, new_splined_list)

    if is_vanadium_run:
        # The run number should be the vanadium number in this case
        run_number = vanadium_string

    output_run_string = vanadium_string if is_vanadium_run else run_number_string

    # Get the file extension if set
    file_extension = getattr(inst_settings, "file_extension")
    if file_extension:
        # Prefix dot if user has forgotten to
        file_extension = file_extension if file_extension.startswith('.') else '.' + file_extension

    # Get the output name suffix if set
    suffix = getattr(inst_settings, "suffix", None)

    # Sample empty if there is one as this is instrument specific
    sample_empty = getattr(inst_settings, "sample_empty", None)

    # By default, offset file sits in the calibration folder, but it can also be given as an absolute path
    if os.path.exists(offset_file_name):
        offset_file_path = offset_file_name
    else:
        offset_file_path = os.path.join(calibration_dir, label, offset_file_name)

    # Generate the paths
    grouping_file_path = os.path.join(calibration_dir,  grouping_file_name)
    van_paths = os.path.join(calibration_dir, label)
    splined_van_path = os.path.join(van_paths, splined_van_name)
    unsplined_van_path = os.path.join(van_paths, unsplined_van_name)
    van_absorb_path = os.path.join(calibration_dir, van_abs_file_name) if van_abs_file_name else None

    return _RunDetails(empty_run_number=empty_run_number, file_extension=file_extension,
                       run_number=run_number, output_run_string=output_run_string, label=label,
                       offset_file_path=offset_file_path, grouping_file_path=grouping_file_path,
                       splined_vanadium_path=splined_van_path, vanadium_run_number=vanadium_string,
                       sample_empty=sample_empty, vanadium_abs_path=van_absorb_path,
                       unsplined_vanadium_path=unsplined_van_path, output_suffix=suffix,van_paths=van_paths)


def get_cal_mapping_dict(run_number_string, cal_mapping_path):
    # Get the python dictionary from the YAML mapping
    run_number = common.get_first_run_number(run_number_string=run_number_string)
    cal_mapping_dict = yaml_parser.get_run_dictionary(run_number_string=run_number,
                                                      file_path=cal_mapping_path)
    return cal_mapping_dict


class _RunDetails(object):
    """
    This class holds the full file paths associated with each run and various other useful attributes
    """

    def __init__(self, empty_run_number, file_extension, run_number, output_run_string, label,
                 offset_file_path, grouping_file_path, splined_vanadium_path, vanadium_run_number,
                 sample_empty, vanadium_abs_path, unsplined_vanadium_path, output_suffix,van_paths):

        # Essential attribute
        self.empty_runs = empty_run_number
        self.run_number = run_number
        self.output_run_string = output_run_string

        self.label = label

        self.offset_file_path = offset_file_path
        self.grouping_file_path = grouping_file_path

        self.splined_vanadium_file_path = splined_vanadium_path
        self.unsplined_vanadium_file_path = unsplined_vanadium_path
        self.vanadium_run_numbers = vanadium_run_number

        # Optional
        self.file_extension = str(file_extension) if file_extension else None
        self.sample_empty = sample_empty
        self.vanadium_absorption_path = vanadium_abs_path
        self.output_suffix = output_suffix
        self.van_paths = van_paths
