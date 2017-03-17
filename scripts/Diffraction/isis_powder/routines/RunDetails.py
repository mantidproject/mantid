from __future__ import (absolute_import, division, print_function)

from isis_powder.routines import common, yaml_parser


def create_run_details_object(run_number_string, inst_settings):
    pass


def get_cal_mapping(run_number_string, inst_settings):
    # Get the python dictionary from the YAML mapping
    run_number = common.get_first_run_number(run_number_string=run_number_string)
    cal_mapping_dict = yaml_parser.get_run_dictionary(run_number_string=run_number,
                                                      file_path=inst_settings.cal_mapping_file)

    return cal_mapping_dict


def cal_map_dictionary_key_helper_wrapper(*args, **kwargs):
    forwarded_value = kwargs.pop("forwarded_value")
    return common.cal_map_dictionary_key_helper(forwarded_value, *args, **kwargs)


class RunDetailsFuncWrapper(object):
    # Holds a callable method, associated args and return value so we can pass it in
    # as a single method

    def __init__(self, function=None, func_args=None, func_kwargs=None):
        self.function = function
        self.function_args = func_args
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
            self._returned_value = self.function(*self.function_args, **self.function_kwargs)
        else:
            self._returned_value = self.function(*self.function_args, **self.function_kwargs)

        self._function_is_executed = True

    def _set_previous_callable(self, previous_callable):
        if not previous_callable:
            return None
        elif not isinstance(previous_callable, RunDetailsFuncWrapper):
            raise ValueError("previous callable is not a RunDetailsFuncWrapper type")

        self._previous_callable = previous_callable

    def add_to_func_chain(self, function, *args, **kwargs):
        # Construct a new object that will be the next in line
        next_in_chain = RunDetailsFuncWrapper(function=function, func_args=args, func_kwargs=kwargs)
        next_in_chain._set_previous_callable(self)
        return next_in_chain

    def get_result(self):
        if not self._function_is_executed:
            self._exec_func()
        return self._returned_value


class RunDetails(object):
    """
    This class holds the full file paths associated with each run and various other useful attributes
    """

    def __init__(self, empty_run_number, run_number, output_run_string, label,
                 offset_file_path, grouping_file_path, splined_vanadium_path, vanadium_run_number,
                 sample_empty=None, vanadium_abs_path=None):
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
