# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from qtpy.QtWidgets import QMessageBox

from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import Observer
from mantid.simpleapi import logger


class FocusPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_observer = self.CalibrationObserver(self)

        # Connect view signals to local methods.
        self.view.set_on_focus_clicked(self.on_focus_clicked)
        self.view.set_enable_controls_connection(self.set_focus_controls_enabled)

        # Variables from other GUI tabs.
        self.current_calibration = {"vanadium_path": None, "ceria_path": None}
        self.instrument = "ENGINX"
        self.rb_num = None

    def on_focus_clicked(self):
        banks = self._get_banks()
        if not self._validate(banks):
            return
        focus_path = self.view.get_focus_filename()
        self.start_focus_worker(focus_path, banks, self.view.get_plot_output(), self.rb_num)

    def start_focus_worker(self, focus_path, banks, plot_output, rb_num):
        """
        Focus data in a separate thread to stop the main GUI from hanging.
        :param focus_path: The path to the file containing the data to focus.
        :param banks: A list of banks that are to be focused.
        :param plot_output: True if the output should be plotted.
        :param rb_num: The rb_number from the main window (often an experiment id)
        """
        self.worker = AsyncTask(
            self.model.focus_run,
            (focus_path, banks, plot_output, self.instrument, rb_num, self.current_calibration),
            error_cb=self._on_worker_error,
            finished_cb=self.emit_enable_button_signal)
        self.set_focus_controls_enabled(False)
        self.worker.start()

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def set_rb_number(self, rb_number):
        self.rb_num = rb_number

    def _validate(self, banks):
        """
        Ensure that the worker is ready to be started.
        :param banks: A list of banks to focus.
        :return: True if the worker can be started safely.
        """
        if not self.view.get_focus_valid():
            return False
        if self.current_calibration["vanadium_path"] is None:
            self._create_error_message(
                "Load a calibration from the Calibration tab before focusing.")
            return False
        if self.view.is_searching():
            return False
        if len(banks) == 0:
            self._create_error_message("Please select at least one bank.")
            return False
        return True

    def _create_error_message(self, message):
        QMessageBox.warning(self.view, "Engineering Diffraction - Error", str(message))

    def _on_worker_error(self, failure_info):
        logger.warning(str(failure_info))
        self.emit_enable_button_signal()

    def set_focus_controls_enabled(self, enabled):
        self.view.set_focus_button_enabled(enabled)
        self.view.set_plot_output_enabled(enabled)

    def _get_banks(self):
        banks = []
        if self.view.get_north_bank():
            banks.append("North")
        if self.view.get_south_bank():
            banks.append("South")
        return banks

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def update_calibration(self, calibration):
        """
        Update the current calibration following an call from a CalibrationNotifier
        :param calibration: The new current calibration.
        """
        self.current_calibration = calibration

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, calibration):
            self.outer.update_calibration(calibration)
