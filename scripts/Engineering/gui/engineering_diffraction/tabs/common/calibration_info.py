# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import Engineering.EnggUtils as EnggUtils
from Engineering.gui.engineering_diffraction.tabs.common import path_handling


class CalibrationInfo(EnggUtils.GroupingInfo):
    """
    Keeps track of the parameters that went into a calibration created by the engineering diffraction GUI.
    """
    def __init__(self, group=None):
        super().__init__(group)
        self.vanadium_path = None
        self.sample_path = None
        self.instrument = None
        self.calibration_table = None

    def set_calibration_table(self, cal_table):
        self.calibration_table = cal_table

    def set_calibration_paths(self, vanadium_path, sample_path, instrument):
        """
        Set the values of the calibration. requires a complete set of calibration info to be supplied.
        :param vanadium_path: Path to the vanadium data file used in the calibration.
        :param sample_path: Path to the sample data file used.
        :param instrument: String defining the instrument the data came from.
        """
        self.vanadium_path = vanadium_path
        self.sample_path = sample_path
        self.instrument = instrument

    def set_calibration_from_prm_fname(self, file_path):
        inst, van, ceria = self.set_group_from_prm_fname(file_path)  # ceria and van are run numbers
        self.set_calibration_paths(van, ceria, inst)

    def generate_output_file_name(self, ext='.prm'):
        return super().generate_output_file_name(self.get_vanadium_path(), self.get_sample_path(),
                                                 self.get_instrument(), ext)

    def save_grouping_workspace(self, directory):
        return super().save_grouping_workspace(directory, self.get_vanadium_path(), self.get_sample_path(),
                                               self.get_instrument())

    # getters
    def get_calibration_table(self):
        return self.calibration_table

    def get_vanadium_path(self):
        return self.vanadium_path

    def get_vanadium_runno(self):
        if self.vanadium_path and self.instrument:
            return path_handling.get_run_number_from_path(self.vanadium_path, self.instrument)

    def get_sample_path(self):
        return self.sample_path

    def get_sample_runno(self):
        if self.sample_path and self.instrument:
            return path_handling.get_run_number_from_path(self.sample_path, self.instrument)

    def get_instrument(self):
        return self.instrument

    def clear(self):
        super().clear()
        self.vanadium_path = None
        self.sample_path = None
        self.instrument = None

    def is_valid(self):
        return True if self.vanadium_path and self.sample_path and self.instrument else False
