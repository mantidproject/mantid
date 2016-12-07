from __future__ import (absolute_import, division, print_function)


class RunDetails(object):
    """
    This class holds the full file paths associated with each run and various other useful attributes
    """

    def __init__(self, run_number):
        # Essential attributes
        self.run_number = run_number

        self.empty_runs = None
        self.label = None

        self.calibration_file_path = None
        self.grouping_file_path = None

        self.splined_vanadium_file_path = None
        self.vanadium_absorption_path = None
        self.vanadium_run_numbers = None

        self.solid_angle_corr = None  # TODO move back into POLARIS
