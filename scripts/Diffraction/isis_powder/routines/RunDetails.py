from __future__ import (absolute_import, division, print_function)


class RunDetails(object):
    """
    This class holds the full file paths associated with each run and various other useful attributes
    """

    def __init__(self, run_number):
        # Essential attribute
        self.run_number = run_number
        self.user_input_run_number = None

        self.empty_runs = None
        self.sample_empty = None
        self.label = None

        self.offset_file_path = None
        self.grouping_file_path = None

        self.splined_vanadium_file_path = None
        self.vanadium_absorption_path = None
        self.vanadium_run_numbers = None
