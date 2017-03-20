from __future__ import (absolute_import, division, print_function)

from isis_powder.routines import common, yaml_parser
import os


def create_run_details_object(run_number_string, inst_settings, is_vanadium_run, empty_run_call=None,
                              grouping_file_name_call=None, vanadium_run_call=None,
                              splined_name_list=None, van_abs_file_name=None):
    cal_map_dict = WrappedFunctionsRunDetails.get_cal_mapping_dict(
        run_number_string=run_number_string, inst_settings=inst_settings)
    run_number = common.get_first_run_number(run_number_string=run_number_string)

    # Get names of files we will be using
    calibration_dir = os.path.normpath(os.path.expanduser(inst_settings.calibration_dir))
    label = common.cal_map_dictionary_key_helper(dictionary=cal_map_dict, key="label")
    offset_file_name = common.cal_map_dictionary_key_helper(dictionary=cal_map_dict, key="offset_file_name")

    # Always make sure the offset file name is included
    if splined_name_list:
        # Force Python to make a copy so we don't modify original
        new_splined_list = list(splined_name_list)
        new_splined_list.append(offset_file_name)
    else:
        new_splined_list = [offset_file_name]

    # These can either be generic or custom so defer to another method
    results_dict = _get_customisable_attributes(
        cal_dict=cal_map_dict, inst_settings=inst_settings, empty_run_call=empty_run_call,
        grouping_name_call=grouping_file_name_call, vanadium_run_call=vanadium_run_call,
        splined_name_list=new_splined_list)

    vanadium_run_string = results_dict["vanadium_runs"]

    if is_vanadium_run:
        # The run number should be the vanadium number in this case
        run_number = vanadium_run_string
        output_run_string = vanadium_run_string
    else:
        output_run_string = run_number_string

    # Sample empty if there is one
    sample_empty = inst_settings.sample_empty if hasattr(inst_settings, "sample_empty") else None

    # Generate the paths
    grouping_file_path = os.path.join(calibration_dir, results_dict["grouping_file_name"])
    # Offset  and splined vanadium is within the correct label folder
    offset_file_path = os.path.join(calibration_dir, label, offset_file_name)
    splined_van_path = os.path.join(calibration_dir, label, results_dict["splined_van_name"])
    van_absorb_path = os.path.join(calibration_dir, van_abs_file_name) if van_abs_file_name else None

    return _RunDetails(empty_run_number=results_dict["empty_runs"], run_number=run_number,
                       output_run_string=output_run_string, label=label, offset_file_path=offset_file_path,
                       grouping_file_path=grouping_file_path, splined_vanadium_path=splined_van_path,
                       vanadium_run_number=vanadium_run_string, sample_empty=sample_empty,
                       vanadium_abs_path=van_absorb_path)


def _get_customisable_attributes(cal_dict, inst_settings, empty_run_call, grouping_name_call, vanadium_run_call,
                                 splined_name_list):
    dict_to_return = {}
    if empty_run_call:
        empty_runs = empty_run_call.get_result()
    else:
        empty_runs = common.cal_map_dictionary_key_helper(dictionary=cal_dict, key="empty_run_numbers")
    dict_to_return["empty_runs"] = empty_runs

    if vanadium_run_call:
        vanadium_runs = vanadium_run_call.get_result()
    else:
        vanadium_runs = common.cal_map_dictionary_key_helper(dictionary=cal_dict, key="vanadium_run_numbers")
    dict_to_return["vanadium_runs"] = vanadium_runs

    if grouping_name_call:
        grouping_name = grouping_name_call.get_result()
    else:
        grouping_name = inst_settings.grouping_file_name
    dict_to_return["grouping_file_name"] = grouping_name

    dict_to_return["splined_van_name"] = common.generate_splined_name(vanadium_runs, splined_name_list)

    return dict_to_return


class WrappedFunctionsRunDetails(object):
    def __init__(self):
        pass

    @staticmethod
    def get_cal_mapping_dict(run_number_string, inst_settings):
        # Get the python dictionary from the YAML mapping
        run_number = common.get_first_run_number(run_number_string=run_number_string)
        cal_mapping_dict = yaml_parser.get_run_dictionary(run_number_string=run_number,
                                                          file_path=inst_settings.cal_mapping_path)
        return cal_mapping_dict

    @staticmethod
    def cal_dictionary_key_helper(key, append_to_error_message=None, **kwargs):
        forwarded_value = kwargs.pop("forwarded_value")
        return common.cal_map_dictionary_key_helper(dictionary=forwarded_value, key=key,
                                                    append_to_error_message=append_to_error_message)


class RunDetailsFuncWrapper(object):
    # Holds a callable method, associated args and return value so we can pass it in
    # as a single method

    def __init__(self, function=None, *args, **func_kwargs):
        if args:
            # If we allow args with position Python gets confused as the forwarded value can be in the first place too
            raise RuntimeError("Cannot use un-named arguments with callable methods")

        self.function = function
        self.function_kwargs = func_kwargs

        self._previous_callable = None
        self._returned_value = None
        self._function_is_executed = False

    def _exec_func(self):
        forwarded_value = self._previous_callable.get_result() if self._previous_callable else None

        if not self.function:
            # We maybe are the 0th case just return any values we hold
            return forwarded_value

        if forwarded_value:
            self.function_kwargs["forwarded_value"] = forwarded_value
            self._returned_value = self.function(**self.function_kwargs)
        else:
            self._returned_value = self.function(**self.function_kwargs)

        self._function_is_executed = True

    def _set_previous_callable(self, previous_callable):
        if not previous_callable:
            return None
        elif not isinstance(previous_callable, RunDetailsFuncWrapper):
            raise ValueError("Previous callable is not a RunDetailsFuncWrapper type")

        self._previous_callable = previous_callable

    def add_to_func_chain(self, function, *args, **func_kwargs):
        # Construct a new object that will be the next in line
        next_in_chain = RunDetailsFuncWrapper(function=function, *args, **func_kwargs)
        next_in_chain._set_previous_callable(self)
        return next_in_chain

    def get_result(self):
        if not self._function_is_executed:
            self._exec_func()
        return self._returned_value


class _RunDetails(object):
    """
    This class holds the full file paths associated with each run and various other useful attributes
    """

    def __init__(self, empty_run_number, run_number, output_run_string, label,
                 offset_file_path, grouping_file_path, splined_vanadium_path, vanadium_run_number,
                 sample_empty, vanadium_abs_path):

        # Essential attribute
        self.empty_runs = empty_run_number
        self.run_number = run_number
        self.output_run_string = output_run_string

        self.label = label

        self.offset_file_path = offset_file_path
        self.grouping_file_path = grouping_file_path

        self.splined_vanadium_file_path = splined_vanadium_path
        self.vanadium_run_numbers = vanadium_run_number

        # Optional
        self.sample_empty = sample_empty
        self.vanadium_absorption_path = vanadium_abs_path
