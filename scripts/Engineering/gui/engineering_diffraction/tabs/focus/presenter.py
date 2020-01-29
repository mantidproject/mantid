# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT, create_error_message
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from Engineering.gui.engineering_diffraction.tabs.common.vanadium_corrections import check_workspaces_exist
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_widget import CroppingWidget
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import Observer


class FocusPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_observer = self.CalibrationObserver(self)

        # Connect view signals to local methods.
        self.view.set_on_focus_clicked(self.on_focus_clicked)
        self.view.set_enable_controls_connection(self.set_focus_controls_enabled)
        self.view.set_on_check_cropping_state_changed(self.show_cropping)

        # Variables from other GUI tabs.
        self.current_calibration = CalibrationInfo()
        self.instrument = "ENGINX"
        self.rb_num = None

        # Cropping Options
        self.cropping_widget = CroppingWidget(self.view, view=self.view.get_cropping_widget())
        self.show_cropping(False)

    def on_focus_clicked(self):
        if not self._validate():
            return
        banks, spectrum_numbers = self._get_banks()
        focus_path = self.view.get_focus_filename()
        self.start_focus_worker(focus_path, banks, self.view.get_plot_output(), self.rb_num, spectrum_numbers)

    def start_focus_worker(self, focus_path, banks, plot_output, rb_num, spectrum_numbers=None):
        """
        Focus data in a separate thread to stop the main GUI from hanging.
        :param focus_path: The path to the file containing the data to focus.
        :param banks: A list of banks that are to be focused.
        :param plot_output: True if the output should be plotted.
        :param rb_num: The RB Number from the main window (often an experiment id)
        :param spectrum_numbers: Optional parameter to crop to a specific list of spectrum numbers.
        """
        self.worker = AsyncTask(self.model.focus_run,
                                (focus_path, banks, plot_output, self.instrument, rb_num, spectrum_numbers),
                                error_cb=self._on_worker_error,
                                finished_cb=self.emit_enable_button_signal)
        self.set_focus_controls_enabled(False)
        self.worker.start()

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def set_rb_num(self, rb_num):
        self.rb_num = rb_num

    def _validate(self):
        """
        Ensure that the worker is ready to be started.
        :return: True if the worker can be started safely.
        """
        if self.view.is_searching():
            create_error_message(self.view, "Mantid is searching for data files. Please wait.")
            return False
        if not self.view.get_focus_valid():
            create_error_message(self.view, "Check run numbers/path is valid.")
            return False
        if not check_workspaces_exist() or not self.current_calibration.is_valid():
            create_error_message(
                self.view, "Create or Load a calibration via the Calibration tab before focusing.")
            return False
        if self.current_calibration.get_instrument() != self.instrument:
            create_error_message(
                self.view,
                "Please make sure the selected instrument matches instrument for the current calibration.\n"
                "The instrument for the current calibration is: " +
                self.current_calibration.get_instrument())
            return False
        if self.view.get_crop_checked() and not self.cropping_widget.is_valid():
            create_error_message(self.view, "Check cropping values are valid.")
            return False
        return True

    def _on_worker_error(self, _):
        self.emit_enable_button_signal()

    def set_focus_controls_enabled(self, enabled):
        self.view.set_focus_button_enabled(enabled)
        self.view.set_plot_output_enabled(enabled)

    def _get_banks(self):
        if self.view.get_crop_checked():
            if self.cropping_widget.is_custom():
                return None, self.cropping_widget.get_custom_spectra()
            else:
                return [self.cropping_widget.get_bank()], None
        else:
            return ["1", "2"], None

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def update_calibration(self, calibration):
        """
        Update the current calibration following an call from a CalibrationNotifier
        :param calibration: The new current calibration.
        """
        self.current_calibration = calibration

    def show_cropping(self, visible):
        self.view.set_cropping_widget_visibility(visible)

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationObserver(Observer):
        def __init__(self, outer):
            Observer.__init__(self)
            self.outer = outer

        def update(self, observable, calibration):
            self.outer.update_calibration(calibration)
