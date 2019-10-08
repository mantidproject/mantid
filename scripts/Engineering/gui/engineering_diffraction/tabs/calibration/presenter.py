# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from mantidqt.utils.asynchronous import AsyncTask
from mantid.simpleapi import logger


class CalibrationPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.worker = None
        self.instrument = "ENGINX"
        self.rb_num = None

    def on_calibrate_clicked(self):
        if not self.validate_run_numbers():
            return
        if self.view.is_searching():
            return
        vanadium_no = self.view.get_vanadium_filename()
        calib_no = self.view.get_calib_filename()
        plot_output = self.view.get_plot_output()
        self.start_calibration_worker(vanadium_no, calib_no, plot_output, self.rb_num)

    def start_calibration_worker(self, vanadium_path, calib_path, plot_output, rb_num):
        """
        Calibrate the data in a separate thread so as to not freeze the GUI.
        :param vanadium_path: Path to vanadium data file.
        :param calib_path: Path to calibration data file.
        :param plot_output: Whether to plot the output.
        :param rb_num: The current RB number set in the GUI.
        """
        self.worker = AsyncTask(self.model.create_new_calibration, (vanadium_path, calib_path),
                                {"plot_output": plot_output, "instrument": self.instrument, "rb_num": rb_num},
                                error_cb=self._on_error, finished_cb=self.enable_calibrate_buttons)
        self.disable_calibrate_buttons()
        self.worker.start()

    def set_instrument_override(self, instrument):
        if instrument == 0:
            instrument = "ENGINX"
        elif instrument == 1:
            instrument = "IMAT"
        else:
            raise ValueError("Invalid instrument index")
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def set_rb_number(self, rb_number):
        self.rb_num = rb_number

    def validate_run_numbers(self):
        if self.view.get_calib_valid() and self.view.get_vanadium_valid():
            return True
        else:
            return False

    def disable_calibrate_buttons(self):
        self.view.set_calibrate_button_enabled(False)
        self.view.set_check_plot_output_enabled(False)

    def enable_calibrate_buttons(self):
        self.view.set_calibrate_button_enabled(True)
        self.view.set_check_plot_output_enabled(True)

    def _on_error(self, failure_info):
        logger.warning(str(failure_info))
        self.enable_calibrate_buttons()
