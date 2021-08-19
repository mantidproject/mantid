# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT, create_error_message, \
    CalibrationObserver, output_settings
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.kernel import logger

from qtpy.QtWidgets import QMessageBox


class FocusPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_observer = CalibrationObserver(self)

        # Observable Setup
        self.focus_run_notifier = GenericObservable()

        # Connect view signals to local methods.
        self.view.set_on_focus_clicked(self.on_focus_clicked)
        self.view.set_enable_controls_connection(self.set_focus_controls_enabled)

        # Variables from other GUI tabs.
        self.current_calibration = CalibrationInfo()
        self.instrument = "ENGINX"
        self.rb_num = None

        last_van_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                                    "last_vanadium_run")
        if last_van_path:
            self.view.set_van_file_text_with_search(last_van_path)

    def add_focus_subscriber(self, obs):
        self.focus_run_notifier.add_subscriber(obs)

    def on_focus_clicked(self):
        if not self._validate():
            return
        regions_dict = self.current_calibration.create_focus_roi_dictionary()
        focus_paths = self.view.get_focus_filenames()
        van_path = self.view.get_vanadium_filename()
        if self._number_of_files_warning(focus_paths):
            self.start_focus_worker(focus_paths, van_path, self.view.get_plot_output(), self.rb_num, regions_dict)
        van_run = self.view.get_vanadium_run()
        set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                    "last_vanadium_run", van_run)

    def start_focus_worker(self, focus_paths: list, van_path: str, plot_output: bool, rb_num: str, regions_dict: dict) -> None:
        """
        Focus data in a separate thread to stop the main GUI from hanging.
        :param focus_paths: List of paths to the files containing the data to focus.
        :param plot_output: True if the output should be plotted.
        :param rb_num: The RB Number from the main window (often an experiment id)
        :param regions_dict: Dictionary containing the regions to focus over, mapping region_name -> grouping_ws_name
        """
        self.worker = AsyncTask(self.model.focus_run,
                                (focus_paths, van_path, plot_output, self.instrument, rb_num, regions_dict),
                                error_cb=self._on_worker_error,
                                finished_cb=self._on_worker_success)
        self.set_focus_controls_enabled(False)
        self.worker.start()

    def _on_worker_success(self):
        self.emit_enable_button_signal()
        self.focus_run_notifier.notify_subscribers(self.model.get_last_focused_files())

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
        if not self.view.get_vanadium_valid():
            create_error_message(self.view, "Check vanadium run number/path is valid.")
            return False
        if not self.current_calibration.is_valid():
            create_error_message(
                self.view, "Create or Load a calibration via the Calibration tab before focusing.")
            return False
        if self.current_calibration.get_instrument() != self.instrument:
            create_error_message(
                self.view,
                "Please make sure the selected instrument matches instrument for the current calibration.\n"
                "The instrument for the current calibration is: " + self.current_calibration.get_instrument())
            return False
        return True

    def _number_of_files_warning(self, paths):
        if len(paths) > 10:  # Just a guess on the warning for now. May change in future.
            response = QMessageBox.warning(
                self.view, 'Engineering Diffraction - Warning',
                'You are attempting to focus {} workspaces. This may take some time.\n\n Would you like to continue?'
                .format(len(paths)), QMessageBox.Ok | QMessageBox.Cancel)
            return response == QMessageBox.Ok
        else:
            return True

    def _on_worker_error(self, error_info):
        logger.error(str(error_info))
        self.emit_enable_button_signal()

    def set_focus_controls_enabled(self, enabled):
        self.view.set_focus_button_enabled(enabled)
        self.view.set_plot_output_enabled(enabled)

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def update_calibration(self, calibration):
        """
        Update the current calibration following an call from a CalibrationNotifier
        :param calibration: The new current calibration.
        """
        self.current_calibration = calibration
        region_text = calibration.get_roi_text()
        self.view.set_region_display_text(region_text)
