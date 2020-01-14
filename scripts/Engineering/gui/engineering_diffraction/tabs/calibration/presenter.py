# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from copy import deepcopy

from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT, create_error_message
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_widget import CroppingWidget

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

        self.connect_view_signals()

        # Main Window State Variables
        self.instrument = "ENGINX"
        self.rb_num = None

        # Cropping Options
        self.cropping_widget = CroppingWidget(self.view, view=self.view.get_cropping_widget())
        self.view.set_cropping_widget_hidden()

    def connect_view_signals(self):
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.view.set_enable_controls_connection(self.set_calibrate_controls_enabled)
        self.view.set_update_fields_connection(self.set_field_values)
        self.view.set_on_radio_new_toggled(self.set_create_new_enabled)
        self.view.set_on_radio_existing_toggled(self.set_load_existing_enabled)
        self.view.set_on_check_cropping_state_changed(self.show_cropping)

    def on_calibrate_clicked(self):
        plot_output = self.view.get_plot_output()
        if self.view.get_new_checked() and self._validate():
            vanadium_file = self.view.get_vanadium_filename()
            sample_file = self.view.get_sample_filename()
            if self.view.get_crop_checked():
                self.start_cropped_calibration_worker(vanadium_file, sample_file, plot_output, self.rb_num)
            else:
                self.start_calibration_worker(vanadium_file, sample_file, plot_output, self.rb_num)
        elif self.view.get_load_checked():
            if not self.validate_path():
                return
            filename = self.view.get_path_filename()
            instrument, vanadium_file, sample_file = self.model.load_existing_gsas_parameters(
                filename)
            self.pending_calibration.set_calibration(vanadium_file, sample_file, instrument)
            self.set_current_calibration()

    def start_calibration_worker(self, vanadium_path, sample_path, plot_output, rb_num, bank=None,
                                 spectrum_numbers=None):
        """
        Calibrate the data in a separate thread so as to not freeze the GUI.
        :param vanadium_path: Path to vanadium data file.
        :param sample_path: Path to sample data file.
        :param plot_output: Whether to plot the output.
        :param rb_num: The current RB number set in the GUI.
        :param bank: Optional parameter to crop by bank.
        :param spectrum_numbers: Optional parameter to crop by spectrum number.
        """
        self.worker = AsyncTask(self.model.create_new_calibration, (vanadium_path, sample_path), {
            "plot_output": plot_output,
            "instrument": self.instrument,
            "rb_num": rb_num,
            "bank": bank,
            "spectrum_numbers": spectrum_numbers
        },
                                error_cb=self._on_error,
                                success_cb=self._on_success)
        self.pending_calibration.set_calibration(vanadium_path, sample_path, self.instrument)
        self.set_calibrate_controls_enabled(False)
        self.worker.start()

    def start_cropped_calibration_worker(self, vanadium_path, sample_path, plot_output, rb_num):
        if self.cropping_widget.is_custom():
            spec_nums = self.cropping_widget.get_custom_spectra()
            self.start_calibration_worker(vanadium_path, sample_path, plot_output, rb_num, spectrum_numbers=spec_nums)
        else:
            bank = self.cropping_widget.get_bank()
            self.start_calibration_worker(vanadium_path, sample_path, plot_output, rb_num, bank=bank)

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

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def _validate(self):
        # Do nothing if run numbers are invalid or view is searching.
        if self.view.is_searching():
            create_error_message(self.view, "Mantid is searching for data files. Please wait.")
            return False
        if not self.validate_run_numbers():
            create_error_message(self.view, "Check run numbers/path is valid.")
            return False
        if not self.cropping_widget.is_valid():
            create_error_message(self.view, "Check cropping values are valid.")
        return True

    def validate_run_numbers(self):
        return self.view.get_sample_valid() and self.view.get_vanadium_valid()

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
            self.view.set_check_cropping_enabled(True)
            self.find_files()

    def set_load_existing_enabled(self, enabled):
        self.view.set_path_enabled(enabled)
        if enabled:
            self.set_calibrate_button_text("Load")
            self.view.set_check_plot_output_enabled(False)
            self.show_cropping(False)
            self.view.set_check_cropping_enabled(False)
            self.view.set_check_cropping_state(0)

    def set_calibrate_button_text(self, text):
        self.view.set_calibrate_button_text(text)

    def find_files(self):
        self.view.find_sample_files()
        self.view.find_vanadium_files()

    def show_cropping(self, show):
        if show:
            self.view.set_cropping_widget_visible()
        else:
            self.view.set_cropping_widget_hidden()

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
