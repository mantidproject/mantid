# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT
from mantidqt.utils.asynchronous import AsyncTask
from mantid.simpleapi import logger
from mantidqt.utils.observer_pattern import Observable


class CalibrationPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_notifier = self.CalibrationNotifier(self)

        self.current_calibration = {"vanadium_path": None, "ceria_path": None}
        self.pending_calibration = {"vanadium_path": None, "ceria_path": None}

        # Connect view signals to local functions.
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.view.set_enable_controls_connection(self.set_calibrate_controls_enabled)

        # Main Window State Variables
        self.instrument = "ENGINX"
        self.rb_num = None

    def on_calibrate_clicked(self):
        # Do nothing if run numbers are invalid or view is searching.
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
        self.worker = AsyncTask(self.model.create_new_calibration, (vanadium_path, calib_path), {
            "plot_output": plot_output,
            "instrument": self.instrument,
            "rb_num": rb_num
        },
                                error_cb=self._on_error,
                                success_cb=self.set_current_calibration)
        self.pending_calibration["vanadium_path"] = vanadium_path
        self.pending_calibration["ceria_path"] = calib_path
        self.set_calibrate_controls_enabled(False)
        self.worker.start()

    def set_current_calibration(self, success_info):
        logger.information("Thread executed in " + str(success_info.elapsed_time) + " seconds.")
        self.current_calibration = self.pending_calibration
        self.calibration_notifier.notify_subscribers(self.current_calibration)
        self.pending_calibration = {"vanadium_path": None, "ceria_path": None}
        self.emit_enable_button_signal()

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def set_rb_number(self, rb_number):
        self.rb_num = rb_number

    def validate_run_numbers(self):
        if self.view.get_calib_valid() and self.view.get_vanadium_valid():
            return True
        else:
            return False

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def set_calibrate_controls_enabled(self, enabled):
        self.view.set_calibrate_button_enabled(enabled)
        self.view.set_check_plot_output_enabled(enabled)

    def _on_error(self, failure_info):
        logger.warning(str(failure_info))
        self.emit_enable_button_signal()

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
