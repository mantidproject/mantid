# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from qtpy.QtWidgets import QMessageBox

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

        # Variables from other GUI tabs.
        self.current_calibration = {"vanadium_path": None, "ceria_path": None}
        self.instrument = "ENGINX"
        self.rb_num = None

    def on_focus_clicked(self):
        banks = self.get_banks()
        if not self.validate(banks):
            return
        focus_path = self.view.get_focus_filename()
        self.start_focus_worker(focus_path, banks, self.view.get_plot_output(), self.rb_num)

    def start_focus_worker(self, focus_path, banks, plot_output, rb_num):
        self.worker = AsyncTask(self.model.focus_run,
                                (focus_path, banks, plot_output, self.instrument, rb_num, self.current_calibration),
                                error_cb=self._on_worker_error,
                                finished_cb=self.enable_focus_controls)
        self.disable_focus_controls()
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

    def validate(self, banks):
        if not self.view.get_focus_valid():
            return False
        if self.current_calibration["vanadium_path"] is None:
            self.create_error_message("Load a calibration from the Calibration tab before focusing.")
            return False
        if self.view.is_searching():
            return False
        if len(banks) == 0:
            self.create_error_message("Please select at least one bank.")
            return False
        return True

    def create_error_message(self, message):
        QMessageBox.warning(self.view, "Engineering Diffraction - Error", str(message))

    def _on_worker_error(self, failure_info):
        logger.warning(str(failure_info))
        self.enable_focus_controls()

    def enable_focus_controls(self):
        self.view.set_focus_button_enabled(True)
        self.view.set_plot_output_enabled(True)

    def disable_focus_controls(self):
        self.view.set_focus_button_enabled(False)
        self.view.set_plot_output_enabled(False)

    def get_banks(self):
        banks = []
        if self.view.get_north_bank():
            banks.append("North")
        if self.view.get_south_bank():
            banks.append("South")
        return banks

    def update_calibration(self, calibration):
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
