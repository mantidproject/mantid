# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Sequence, Optional

from mantid.simpleapi import Load
from Engineering.EnggUtils import GROUP
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model import CalibrationModel
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus.model import FocusModel
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo


class EnginX:
    def __init__(self,
                 vanadium_run: str,
                 focus_runs: Sequence[str],
                 save_dir: str,
                 full_inst_calib_path: Optional[str] = None,
                 prm_path: Optional[str] = None,
                 ceria_run: Optional[str] = None,
                 group: Optional[GROUP] = None,
                 calfile_path: Optional[str] = None,
                 spectrum_num: Optional[str] = None) -> None:

        # init attributes
        self.calibration = CalibrationInfo()
        self.calib_model = CalibrationModel()
        self.focus_model = FocusModel()
        self.van_run = vanadium_run
        self.focus_runs = focus_runs
        self.save_dir = save_dir

        # setup CalibrationInfo object
        if prm_path:
            self.calibration.set_calibration_from_prm_fname(prm_path)  # to load existing calibration
        elif ceria_run and group:
            # make new calibration
            self.calibration.set_group(group)
            self.calibration.set_calibration_paths("ENGINX", ceria_run)
            if group == GROUP.CUSTOM and calfile_path:
                self.calibration.set_cal_file(calfile_path)
            elif group == GROUP.CROPPED and spectrum_num:
                self.calibration.set_spectra_list(spectrum_num)

        if full_inst_calib_path:
            # Load custom full inst calib if supplied (needs to be in ADS otherwise default used)
            Load(full_inst_calib_path, OutputWorkspace="full_inst_calib")

    def calibrate(self, plot_output: bool) -> None:
        if self.calibration.get_prm_filepath():
            self.calibration.load_relevant_calibration_files()  # loading existing calibration files
        else:
            self.calib_model.create_new_calibration(self.calibration, rb_num=None, plot_output=plot_output,
                                                    save_dir=self.save_dir)

    def focus(self, plot_output: bool) -> None:
        if self.calibration.is_valid() and self.van_run:
            self.focus_model.focus_run(self.focus_runs, self.van_run, plot_output, rb_num=None,
                                       calibration=self.calibration, save_dir=self.save_dir)

    def main(self, plot_cal: bool = False, plot_foc: bool = False):
        self.calibrate(plot_cal)
        self.focus(plot_foc)
