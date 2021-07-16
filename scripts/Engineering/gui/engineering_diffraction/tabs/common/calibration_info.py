# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import Engineering.EnggUtils as EnggUtils


class CalibrationInfo(EnggUtils.GroupingInfo):
    """
    Keeps track of the parameters that went into a calibration created by the engineering diffraction GUI.
    """
    def __init__(self, vanadium_path=None, sample_path=None, instrument=None):
        super().__init__()
        self.vanadium_path = vanadium_path
        self.sample_path = sample_path
        self.instrument = instrument
        self._prm_templates = {EnggUtils.GROUP.NORTH: "template_ENGINX_241391_236516_North_bank.prm",
                               EnggUtils.GROUP.SOUTH: "template_ENGINX_241391_236516_South_bank.prm",
                               EnggUtils.GROUP.BOTH: "template_ENGINX_241391_236516_South_bank.prm"}

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
        inst, van, ceria = self.set_group_from_prm_fname(file_path)
        self.set_calibration_paths(van, ceria, inst)

    def generate_output_file_name(self):
        return super().generate_output_file_name(self.get_vanadium(),self.get_sample(),self.get_instrument())

    # getters
    def get_prm_template_file(self):
        return self._prm_templates[self.group]

    def get_vanadium(self):
        return self.vanadium_path

    def get_sample(self):
        return self.sample_path

    def get_instrument(self):
        return self.instrument

    def clear(self):
        super().clear()
        self.vanadium_path = None
        self.sample_path = None
        self.instrument = None

    def is_valid(self):
        return True if self.vanadium_path and self.sample_path and self.instrument else False
