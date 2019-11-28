# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from copy import deepcopy
from qtpy.QtWidgets import QMessageBox

from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from mantidqt.utils.asynchronous import AsyncTask
from mantid.simpleapi import logger
from mantidqt.utils.observer_pattern import Observable


class CalibrationPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_notifier = self.CalibrationNotifier(self)

        self.current_calibration = CalibrationInfo()
        self.pending_calibration = CalibrationInfo()

        # Connect view signals to local functions.
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.view.set_enable_controls_connection(self.set_calibrate_controls_enabled)
        self.view.set_update_fields_connection(self.set_field_values)
        self.view.set_on_radio_new_toggled(self.set_create_new_enabled)
        self.view.set_on_radio_existing_toggled(self.set_load_existing_enabled)

        # Main Window State Variables
        self.instrument = "ENGINX"
        self.rb_num = None

    def on_calibrate_clicked(self):
        plot_output = self.view.get_plot_output()
        if self.view.get_new_checked():
            # Do nothing if run numbers are invalid or view is searching.
            if not self.validate_run_numbers():
                self._create_error_message("Check run numbers/path is valid.")
                return
            if self.view.is_searching():
                self._create_error_message("Mantid is searching for the file. Please wait.")
                return
            vanadium_file = self.view.get_vanadium_filename()
            sample_file = self.view.get_sample_filename()
            self.start_calibration_worker(vanadium_file, sample_file, plot_output, self.rb_num)
        elif self.view.get_load_checked():
            if not self.validate_path():
                return
            filename = self.view.get_path_filename()
            instrument, vanadium_file, sample_file = self.model.load_existing_gsas_parameters(filename)
            self.pending_calibration.set_calibration(vanadium_file, sample_file, instrument)
            self.set_current_calibration()

    def start_calibration_worker(self, vanadium_path, sample_path, plot_output, rb_num):
        """
        Calibrate the data in a separate thread so as to not freeze the GUI.
        :param vanadium_path: Path to vanadium data file.
        :param sample_path: Path to sample data file.
        :param plot_output: Whether to plot the output.
        :param rb_num: The current RB number set in the GUI.
        """
        self.worker = AsyncTask(self.model.create_new_calibration, (vanadium_path, sample_path), {
            "plot_output": plot_output,
            "instrument": self.instrument,
            "rb_num": rb_num
        },
                                error_cb=self._on_error,
                                success_cb=self._on_success)
        self.pending_calibration.set_calibration(vanadium_path, sample_path, self.instrument)
        self.set_calibrate_controls_enabled(False)
        self.worker.start()

    def _create_error_message(self, message):
        QMessageBox.warning(self.view, "Engineering Diffraction - Error", str(message))

    def set_current_calibration(self, success_info=None):
        if success_info:
            logger.information("Thread executed in " + str(success_info.elapsed_time) + " seconds.")
        self.current_calibration = deepcopy(self.pending_calibration)
        self.calibration_notifier.notify_subscribers(self.current_calibration)
        self.emit_update_fields_signal()
        self.pending_calibration.clear()

    def set_field_values(self):
        self.view.set_sample_text(self.current_calibration.get_sample())
        self.view.set_vanadium_text(self.current_calibration.get_vanadium())

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def set_rb_number(self, rb_number):
        self.rb_num = rb_number

    def validate_run_numbers(self):
        if self.view.get_sample_valid() and self.view.get_vanadium_valid():
            return True
        else:
            return False

    def validate_path(self):
        return self.view.get_path_valid()

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def emit_update_fields_signal(self):
        self.view.sig_update_fields.emit()

    def set_calibrate_controls_enabled(self, enabled):
        self.view.set_calibrate_button_enabled(enabled)
        self.view.set_check_plot_output_enabled(enabled)

    def _on_error(self, failure_info):
        logger.warning(str(failure_info))
        self.emit_enable_button_signal()

    def _on_success(self, success_info):
        self.set_current_calibration(success_info)
        self.emit_enable_button_signal()

    def set_create_new_enabled(self, enabled):
        self.view.set_vanadium_enabled(enabled)
        self.view.set_sample_enabled(enabled)
        if enabled:
            self.set_calibrate_button_text("Calibrate")
            self.view.set_check_plot_output_enabled(True)
            self.find_files()

    def set_load_existing_enabled(self, enabled):
        self.view.set_path_enabled(enabled)
        if enabled:
            self.set_calibrate_button_text("Load")
            self.view.set_check_plot_output_enabled(False)

    def set_calibrate_button_text(self, text):
        self.view.set_calibrate_button_text(text)

    def find_files(self):
        self.view.find_sample_files()
        self.view.find_vanadium_files()

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
