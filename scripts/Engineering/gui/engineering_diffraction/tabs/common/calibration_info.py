# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class CalibrationInfo(object):
    """
    Keeps track of the parameters that went into a calibration created by the engineering diffraction GUI.
    """
    def __init__(self, vanadium_path=None, sample_path=None, instrument=None, crop_info=None):
        self.vanadium_path = vanadium_path
        self.sample_path = sample_path
        self.instrument = instrument
        self.crop_info = crop_info

    def set_calibration(self, vanadium_path, sample_path, instrument):
        """
        Set the values of the calibration. requires a complete set of calibration info to be supplied.
        :param vanadium_path: Path to the vanadium data file used in the calibration.
        :param sample_path: Path to the sample data file used.
        :param instrument: String defining the instrument the data came from.
        """
        self.vanadium_path = vanadium_path
        self.sample_path = sample_path
        self.instrument = instrument

    def set_crop_info(self, bank, spec_nums, calfile) -> None:
        """
        Set the three components that form the possible crop info - banks, custom spectrum numbers, or a custom calfile.
        All may be None - denoting no crop (calibrate/focus over both banks)
        :param bank: '1' or '2' representing the North or South bank respectively
        :param spec_nums: A string representation of range(s) of spectrum numbers
        :param calfile: A custom calfile
        """
        if calfile is not None:
            self.crop_info = None, None, calfile
        elif spec_nums is not None:
            self.crop_info = None, spec_nums, None
        elif bank is None:
            self.crop_info = ['1', '2'], None, None
        else:
            self.crop_info = [bank], None, None

    def get_crop_info(self):
        return self.crop_info

    def get_vanadium(self):
        return self.vanadium_path

    def get_sample(self):
        return self.sample_path

    def get_instrument(self):
        return self.instrument

    def clear(self):
        self.vanadium_path = None
        self.sample_path = None
        self.instrument = None

    def is_valid(self):
        return True if self.vanadium_path and self.sample_path and self.instrument else False
