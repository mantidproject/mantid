# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Sequence

from mantid.simpleapi import Load, logger
from Engineering.EnggUtils import focus_run, create_new_calibration
from Engineering.common.calibration_info import CalibrationInfo
from Engineering.common.instrument_config import get_instr_config, INSTRUMENT_GROUP


class BaseEngInstrument:
    def __init__(
        self,
        vanadium_run: str,
        focus_runs: Sequence[str],
        save_dir: str,
        full_inst_calib_path: str,
        instrument: str,
        prm_path: str | None = None,
        ceria_run: str | None = None,
        group: INSTRUMENT_GROUP | None = None,
        groupingfile_path: str | None = None,
        spectrum_num: str | None = None,
    ) -> None:
        # init attributes
        self.calibration = CalibrationInfo()
        self.focus_runs = focus_runs
        self.save_dir = save_dir
        self.copy_peak_params_in_calib = True
        self.instrument = instrument

        self.config = get_instr_config(self.instrument)
        self.GROUP = self.config.group

        # Load custom full inst calib if supplied (needs to be in ADS)
        try:
            self.full_calib_ws = Load(full_inst_calib_path, OutputWorkspace=f"full_inst_calib_{self.instrument}")
        except ValueError as e:
            logger.error("Unable to load calibration file " + full_inst_calib_path + ". Error: " + str(e))

        # setup CalibrationInfo object
        if prm_path:
            self.calibration.set_calibration_from_prm_fname(prm_path, self.instrument)  # to load existing calibration
        elif ceria_run and group:
            # make new calibration
            self.calibration.set_group(group)
            self.calibration.set_calibration_paths(self.instrument, ceria_run, vanadium_run)
            self.setup_group(group, groupingfile_path, spectrum_num)

    # this can be overridden by individual instruments if they have specific group behaviour
    def setup_group(self, group: INSTRUMENT_GROUP, groupingfile_path: str, spectrum_num: str) -> None:
        if group == self.GROUP.CUSTOM and groupingfile_path:
            self.calibration.set_grouping_file(groupingfile_path)
        elif group == self.GROUP.CROPPED and spectrum_num:
            self.calibration.set_spectra_list(spectrum_num)

    def set_calibration_to_copy_starting_parameters(self, val: bool):
        """In the calibration step, currently the algorithm uses PDCalibration
        this has the option to init peak parameters from the results of previous fits
        in practise this is very useful, but for testing, where fitting can be OS dependent
        it is more stable to start from the same starting point"""
        self.copy_peak_params_in_calib = val

    def calibrate(self, plot_output: bool) -> None:
        if self.calibration.get_prm_filepath():
            self.calibration.load_relevant_calibration_files()  # loading existing calibration files
        else:
            create_new_calibration(
                self.calibration,
                rb_num=None,
                plot_output=plot_output,
                save_dir=self.save_dir,
                full_calib=self.full_calib_ws,
                copy_params_in_calib=self.copy_peak_params_in_calib,
            )

    def focus(self, plot_output: bool) -> None:
        if self.calibration.is_valid() and self.calibration.get_vanadium_path():
            focus_run(
                self.focus_runs,
                plot_output,
                rb_num=None,
                calibration=self.calibration,
                save_dir=self.save_dir,
                full_calib=self.full_calib_ws,
            )

    def main(self, plot_cal: bool = False, plot_foc: bool = False):
        self.calibrate(plot_cal)
        self.focus(plot_foc)
