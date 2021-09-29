# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import Engineering.EnggUtils as EnggUtils
from Engineering.common import path_handling
from mantid.api import AnalysisDataService as ADS


class CalibrationInfo(EnggUtils.GroupingInfo):
    """
    Keeps track of the parameters that went into a calibration created by the engineering diffraction GUI.
    """

    def __init__(self, group=None):
        super().__init__(group)
        self.sample_path = None
        self.instrument = None
        self.calibration_table = None

    def set_calibration_table(self, cal_table):
        self.calibration_table = cal_table

    def set_calibration_paths(self, sample_path, instrument):
        """
        Set the values of the calibration
        :param sample_path: Path to the sample data file used.
        :param instrument: String defining the instrument the data came from.
        """
        self.sample_path = sample_path
        self.instrument = instrument

    def set_calibration_from_prm_fname(self, file_path):
        inst, ceria = self.set_group_from_prm_fname(file_path)  # ceria and van are run numbers
        self.set_calibration_paths(ceria, inst)

    def generate_output_file_name(self, ext='.prm'):
        return super().generate_output_file_name(self.get_sample_path(),
                                                 self.get_instrument(), ext)

    def save_grouping_workspace(self, directory):
        filename = self.generate_output_file_name(ext='.xml')
        super().save_grouping_workspace(directory, filename)

    def load_relevant_calibration_files(self, output_prefix="engggui"):
        self.calibration_table = super().load_relevant_calibration_files(output_prefix).name()

    # getters
    def get_group_ws(self):
        return super().get_group_ws(self.get_instrument())

    def get_calibration_table(self):
        return self.calibration_table

    def get_sample_path(self):
        return self.sample_path

    def get_sample_runno(self):
        if self.sample_path and self.instrument:
            return path_handling.get_run_number_from_path(self.sample_path, self.instrument)

    def get_instrument(self):
        return self.instrument

    def clear(self):
        super().clear()
        self.sample_path = None
        self.instrument = None
        self.calibration_table = None

    def is_valid(self):
        return self.sample_path is not None and self.instrument is not None and self.calibration_table is not None \
               and self.group_ws is not None and self.calibration_table in ADS and self.group_ws in ADS
