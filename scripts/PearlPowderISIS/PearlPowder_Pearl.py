from __future__ import (absolute_import, division, print_function)


class Pearl:

    def __init__(self, user_name=None, calibration_dir=None, raw_data_dir=None, output_dir=None):
        if user_name is None:
            raise ValueError("A username must be provided in the startup script")

        self.user_name = user_name
        self.calibration_dir = calibration_dir
        self.raw_data_dir = raw_data_dir
        self.output_dir = output_dir

        # This advanced option disables appending the current cycle to the
        # path given for raw files.
        self.disable_appending_cycle_to_raw_dir = False

        # live_data_directory = None  # TODO deal with this

        # File names # TODO remove this
        pearl_MC_absorption_file_name = "PRL112_DC25_10MM_FF.OUT"
        cal_file_name = "pearl_offset_11_2.cal"
        group_file_name = "pearl_group_11_2_TT88.cal"
        van_absorb_file_name = "van_spline_all_cycle_11_1.nxs"
        van_file_name = "van_spline_all_cycle_11_1.nxs"

        # Instrument defaults settings
        self.tof_binning = "1500,-0.0006,19900"

        self.attenuation_full_path = calibration_dir + pearl_MC_absorption_file_name

# TODO create static wrapper called start