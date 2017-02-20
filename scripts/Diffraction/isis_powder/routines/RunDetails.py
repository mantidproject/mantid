from __future__ import (absolute_import, division, print_function)


class RunDetails(object):
    """
    This class holds the full file paths associated with each run and various other useful attributes
    """

    def __init__(self, calibration_path, grouping_path, vanadium_runs, run_number):
        # Essential attributes
        self.run_number = run_number
        self.calibration = calibration_path
        self.grouping = grouping_path
        self.vanadium = vanadium_runs

        # Optional Attributes
        self.instrument_version = None
        self.vanadium_absorption = None
        self.splined_vanadium = None
        self.sample_empty = None

        self.solid_angle_corr = None

        self.label = None
