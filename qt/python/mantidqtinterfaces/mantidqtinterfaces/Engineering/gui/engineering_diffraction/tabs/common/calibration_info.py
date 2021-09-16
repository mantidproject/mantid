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
    def __init__(self, sample_path=None, instrument=None, grouping_ws=None, roi_text: str = ""):
        self.sample_path = sample_path
        self.instrument = instrument
        self.grouping_ws_name = grouping_ws
        self.roi_text = roi_text
        self.bank = None

    def set_calibration(self, sample_path, instrument):
        """
        Set the values of the calibration
        :param sample_path: Path to the sample data file used.
        :param instrument: String defining the instrument the data came from.
        """
        self.sample_path = sample_path
        self.instrument = instrument

    def set_roi_info_load(self, banks: list, grp_ws: str, roi_text: str) -> None:
        """
        Set the region of interest fields, used in the event that a calibration is being loaded rather than created
        :param banks: list of banks defining chosen roi, None if Custom or Cropped region
        :param grp_ws: Name of the grouping workspace
        :param roi_text: Text to signify this region of interest to be displayed on the Focus tab
        """
        self.grouping_ws_name = grp_ws
        self.roi_text = roi_text
        self.bank = banks

    def set_roi_info(self, bank: str = None, calfile: str = None, spec_nos=None) -> None:
        """
        Set the region of interest fields using the inputs to the calibration that has just been run
        :param bank: Single string bank to identify North (1) or South (2) bank. If None & all other params are None,
        signifies that both banks are to be treated as a region of interest
        :param calfile: Custom calfile that can be used to define a region of interest. Can be None
        :param spec_nos: Custom spectrum number list that can be used to define a region of interest. Can be None
        """
        if bank == '1':
            self.grouping_ws_name = "NorthBank_grouping"
            self.bank = ['1']
            self.roi_text = "North Bank"
        elif bank == '2':
            self.grouping_ws_name = "SouthBank_grouping"
            self.roi_text = "South Bank"
            self.bank = ['2']
        elif calfile:
            self.grouping_ws_name = "Custom_calfile_grouping"
            self.roi_text = "Custom CalFile"
            self.bank = None
        elif spec_nos:
            self.grouping_ws_name = "Custom_spectra_grouping"
            self.roi_text = "Custom spectra cropping"
            self.bank = None
        elif bank is None:
            self.grouping_ws_name = None
            self.roi_text = "North and South Banks"
            self.bank = ['1', '2']

    def create_focus_roi_dictionary(self) -> dict:
        """
        With the stored region of interest data, create a dictionary for use in the focussing workflow to define the
        regions to focus and their corresponding grouping workspace
        :return: dict mapping region_name -> grp_ws_name
        """
        regions = dict()
        if self.bank:
            # focus over one or both banks
            for bank in self.bank:
                if bank == '1':
                    roi = "bank_1"
                    grp_ws = "NorthBank_grouping"
                elif bank == '2':
                    roi = "bank_2"
                    grp_ws = "SouthBank_grouping"
                else:
                    raise ValueError
                regions[roi] = grp_ws
        elif self.grouping_ws_name:
            if "calfile" in self.grouping_ws_name:
                regions["Custom"] = self.grouping_ws_name
            elif "spectra" in self.grouping_ws_name:
                regions["Cropped"] = self.grouping_ws_name
        else:
            raise ValueError("CalibrationInfo object contains no region-of-interest data")
        return regions

    def get_roi_text(self):
        return self.roi_text

    def get_sample(self):
        return self.sample_path

    def get_instrument(self):
        return self.instrument

    def clear(self):
        self.sample_path = None
        self.instrument = None

    def is_valid(self):
        return True if self.sample_path and self.instrument else False
