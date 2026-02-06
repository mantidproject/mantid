# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import (
    INSTRUMENT_DICT,
    create_error_message,
    CalibrationObserver,
)
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.common.calibration_info import CalibrationInfo
from mantidqt.utils.asynchronous import AsyncTask
from mantidqt.utils.observer_pattern import GenericObservable, GenericObserverWithArgPassing
from mantid.kernel import logger

from qtpy.QtWidgets import QMessageBox


class FocusPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_observer = CalibrationObserver(self)
        self.correction_observer = GenericObserverWithArgPassing(self.set_default_files)

        # Observable Setup
        self.focus_run_notifier = GenericObservable()
        self.focus_run_notifier_gsas2 = GenericObservable()
        self.focus_run_notifier_texture = GenericObservable()

        # Connect view signals to local methods.
        self.view.set_on_focus_clicked(self.on_focus_clicked)
        self.view.set_enable_controls_connection(self.set_focus_controls_enabled)

        # Variables from other GUI tabs.
        self.current_calibration = CalibrationInfo()
        self.instrument = "ENGINX"
        self.rb_num = None

        self.set_default_directories()
        self.view.set_focus_button_enabled(False)

    def set_default_files(self, filepaths):
        directory = self.model.get_last_directory(filepaths)
        self.view.set_default_files(filepaths, directory)

    def add_focus_subscriber(self, obs):
        self.focus_run_notifier.add_subscriber(obs)

    def add_focus_gsas2_subscriber(self, obs):
        self.focus_run_notifier_gsas2.add_subscriber(obs)

    def add_focus_texture_subscriber(self, obs):
        self.focus_run_notifier_texture.add_subscriber(obs)

    def on_focus_clicked(self):
        if not self.current_calibration.get_vanadium_path():
            van_file = self.view.get_vanadium_filename()
            self.current_calibration.vanadium_path = van_file if van_file else None
        if not self._validate():
            return
        focus_paths = self.view.get_focus_filenames()
        if self._number_of_files_warning(focus_paths):
            self.start_focus_worker(focus_paths, self.view.get_plot_output(), self.rb_num, self.current_calibration)

    def start_focus_worker(self, focus_paths: list, plot_output: bool, rb_num: str, calibration: CalibrationInfo) -> None:
        """
        Focus data in a separate thread to stop the main GUI from hanging.
        :param focus_paths: List of paths to the files containing the data to focus.
        :param plot_output: True if the output should be plotted.
        :param rb_num: The RB Number from the main window (often an experiment id)
        :param regions_dict: Dictionary containing the regions to focus over, mapping region_name -> grouping_ws_name
        """
        self.worker = AsyncTask(
            self.model.focus_run,
            (focus_paths, plot_output, rb_num, calibration),
            error_cb=self._on_worker_error,
            finished_cb=self._on_worker_success,
        )
        self.set_focus_controls_enabled(False)
        self.worker.start()

    def _on_worker_success(self):
        self.emit_enable_button_signal()
        self.focus_run_notifier.notify_subscribers(self.model.get_last_focused_files())
        self.focus_run_notifier_gsas2.notify_subscribers(self.model.get_last_focused_files_gsas2())
        self.focus_run_notifier_texture.notify_subscribers(self.model.get_last_focused_files_texture())

    def set_instrument_override(self, instrument):
        instrument = INSTRUMENT_DICT[instrument]
        self.view.set_instrument_override(instrument)
        self.instrument = instrument

    def set_default_directories(self):
        self.view.set_finder_last_directory(output_settings.get_output_path())

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
        if not self.current_calibration.is_valid():
            create_error_message(
                self.view, "Create or Load a calibration via the Calibration tab before focusing. Ensure that a Vanadium run has been set."
            )
            return False
        if self.current_calibration.get_instrument() != self.instrument:
            create_error_message(
                self.view,
                "Please make sure the selected instrument matches instrument for the current calibration.\n"
                "The instrument for the current calibration is: " + self.current_calibration.get_instrument(),
            )
            return False
        return True

    def _number_of_files_warning(self, paths):
        if len(paths) > 10:  # Just a guess on the warning for now. May change in future.
            response = QMessageBox.warning(
                self.view,
                "Engineering Diffraction - Warning",
                "You are attempting to focus {} workspaces. This may take some time.\n\n Would you like to continue?".format(len(paths)),
                QMessageBox.Ok | QMessageBox.Cancel,
            )
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
        Update the current calibration following a call from a CalibrationNotifier
        :param calibration: The new current calibration.
        """
        self.current_calibration = calibration
        if calibration:
            region_text = calibration.get_group_description()
            self.view.set_region_display_text(region_text)
            self.view.set_focus_button_enabled(True)
        else:
            self.view.set_region_display_text("Calibration not loaded")
            self.view.set_focus_button_enabled(False)
