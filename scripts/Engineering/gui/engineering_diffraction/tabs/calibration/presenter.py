# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from copy import deepcopy

from Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT, create_error_message, output_settings
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_presenter import CroppingPresenter
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting

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
        self.last_calibration_successful = False

        # Cropping Options
        self.cropping_widget = CroppingPresenter(parent=self.view, view=self.view.get_cropping_widget())
        self.show_cropping(False)

    def connect_view_signals(self):
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.view.set_enable_controls_connection(self.set_calibrate_controls_enabled)
        self.view.set_update_fields_connection(self.set_field_values)
        self.view.set_on_radio_new_toggled(self.set_create_new_enabled)
        self.view.set_on_radio_existing_toggled(self.set_load_existing_enabled)
        self.view.set_on_check_cropping_state_changed(self.show_cropping)
        self.view.set_on_check_update_vanadium_state_changed(self.disable_sample_and_crop)
        self.view.set_enable_update_vanadium(self.try_enable_update_vanadium)

    def on_calibrate_clicked(self):
        plot_output = self.view.get_plot_output()
        if self.view.get_new_checked() and self._validate():
            vanadium_file = self.view.get_vanadium_filename()
            sample_file = self.view.get_sample_filename()
            only_update_vanadium = self.view.get_update_vanadium_checked()
            if self.view.get_crop_checked():
                self.start_cropped_calibration_worker(vanadium_file, sample_file,
                                                      plot_output, self.rb_num,
                                                      only_update_vanadium=only_update_vanadium)
            else:
                self.start_calibration_worker(vanadium_file, sample_file,
                                              plot_output, self.rb_num, only_update_vanadium=only_update_vanadium)
        elif self.view.get_load_checked():
            if not self.validate_path():
                logger.notice("Invalid path")
                return
            filename = self.view.get_path_filename()
            instrument, vanadium_file, sample_file, grp_ws_name, roi_text, banks = \
                self.model.load_existing_calibration_files(filename)
            self.pending_calibration.set_calibration(vanadium_file, sample_file, instrument)
            self.pending_calibration.set_roi_info_load(banks, grp_ws_name, roi_text)
            self.set_current_calibration()
            set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                        "last_calibration_path", filename)

    def start_calibration_worker(self, vanadium_path, sample_path, plot_output, rb_num, bank=None, calfile=None,
                                 spectrum_numbers=None, only_update_vanadium=False):
        """
        Calibrate the data in a separate thread so as to not freeze the GUI.
        :param vanadium_path: Path to vanadium data file.
        :param sample_path: Path to sample data file.
        :param plot_output: Whether to plot the output.
        :param rb_num: The current RB number set in the GUI.
        :param bank: Optional parameter to crop by bank.
        :param calfile: Custom calibration file the user can supply for the calibration region of interest.
        :param spectrum_numbers: Optional parameter to crop by spectrum number.
        """
        self.worker = AsyncTask(self.model.create_new_calibration, (vanadium_path, sample_path),
                                {
                                "plot_output": plot_output,
                                "instrument": self.instrument,
                                "rb_num": rb_num,
                                "bank": bank,
                                "calfile": calfile,
                                "spectrum_numbers": spectrum_numbers,
                                "only_update_vanadium": only_update_vanadium
                                },
                                error_cb=self._on_error,
                                success_cb=self._on_success)
        self.pending_calibration.set_calibration(vanadium_path, sample_path, self.instrument)
        self.pending_calibration.set_roi_info(bank, calfile, spectrum_numbers)
        self.set_calibrate_controls_enabled(False)
        self.worker.start()

    def start_cropped_calibration_worker(self, vanadium_path, sample_path, plot_output, rb_num, only_update_vanadium):
        if self.cropping_widget.get_custom_calfile_enabled():
            calfile = self.cropping_widget.get_custom_calfile()
            self.start_calibration_worker(vanadium_path, sample_path, plot_output, rb_num,
                                          calfile=calfile, only_update_vanadium=only_update_vanadium)
        elif self.cropping_widget.get_custom_spectra_enabled():
            spec_nums = self.cropping_widget.get_custom_spectra()
            self.start_calibration_worker(vanadium_path, sample_path, plot_output, rb_num,
                                          spectrum_numbers=spec_nums, only_update_vanadium=only_update_vanadium)
        else:
            bank = str(self.cropping_widget.get_bank())
            self.start_calibration_worker(vanadium_path, sample_path, plot_output, rb_num,
                                          bank=bank, only_update_vanadium=only_update_vanadium)

    def set_current_calibration(self, success_info=None):
        if success_info:
            logger.information("Thread executed in " + str(success_info.elapsed_time) + " seconds.")
        self.current_calibration = deepcopy(self.pending_calibration)
        self.calibration_notifier.notify_subscribers(self.current_calibration)
        self.emit_update_fields_signal()
        self.pending_calibration.clear()

    def load_last_calibration(self) -> None:
        """
        Loads the most recently created or loaded calibration into the interface instance. To be used on interface
        startup.
        """
        last_cal_path = get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX,
                                    "last_calibration_path")
        if last_cal_path:
            self.view.set_load_checked(True)
            self.view.set_file_text_with_search(last_cal_path)

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
        if not self.validate_run_numbers() and not self.view.get_update_vanadium_checked():
            create_error_message(self.view, "Check run numbers/path is valid.")
            return False
        if self.view.get_update_vanadium_checked() and not self.view.get_vanadium_valid():
            create_error_message(self.view, "Check vanadium run number/path is valid.")
            return False
        if self.view.get_crop_checked():
            if self.cropping_widget.get_custom_calfile_enabled() and not self.cropping_widget.is_calfile_valid():
                create_error_message(self.view, "Check custom calfile path is valid.")
                return False
            if self.cropping_widget.get_custom_spectra_enabled() and not self.cropping_widget.is_spectra_valid():
                create_error_message(self.view, "Check custom spectra are valid.")
                return False
        return True

    def validate_run_numbers(self):
        return self.view.get_sample_valid() and self.view.get_vanadium_valid()

    def validate_path(self):
        return self.view.get_path_valid()

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def emit_enable_update_van(self, cal_success):
        self.view.sig_enable_only_update_van.emit(cal_success)

    def emit_update_fields_signal(self):
        self.view.sig_update_fields.emit()

    def set_calibrate_controls_enabled(self, enabled):
        self.view.set_calibrate_button_enabled(enabled)
        self.view.set_check_plot_output_enabled(enabled)

    def _on_error(self, error_info):
        logger.error(str(error_info))
        self.emit_enable_button_signal()
        self.emit_enable_update_van(False)

    def _on_success(self, success_info):
        self.set_current_calibration(success_info)
        self.emit_enable_button_signal()
        self.emit_enable_update_van(True)

    def set_create_new_enabled(self, enabled):
        self.view.set_vanadium_enabled(enabled)
        if enabled:
            self.set_calibrate_button_text("Calibrate")
            self.view.set_check_plot_output_enabled(True)
            self.disable_sample_and_crop(self.view.get_update_vanadium_checked())
            self.view.set_check_update_vanadium_enabled(self.last_calibration_successful)
            self.find_files()
        else:
            self.view.set_sample_enabled(enabled)

    def set_load_existing_enabled(self, enabled):
        self.view.set_path_enabled(enabled)
        if enabled:
            self.set_calibrate_button_text("Load")
            self.view.set_check_plot_output_enabled(False)
            self.view.set_check_cropping_enabled(False)
            self.view.set_check_cropping_checked(False)
            self.view.set_check_update_vanadium_enabled(False)

    def set_calibrate_button_text(self, text):
        self.view.set_calibrate_button_text(text)

    def find_files(self):
        self.view.find_sample_files()
        self.view.find_vanadium_files()

    def show_cropping(self, show):
        self.view.set_cropping_widget_visibility(show)

    def disable_sample_and_crop(self, disable):
        show = not disable
        self.view.set_sample_enabled(show)
        self.view.set_check_cropping_enabled(show)
        if disable:
            self.view.set_check_cropping_checked(show)
            self.set_calibrate_button_text("Update Vanadium")
        else:
            self.set_calibrate_button_text("Calibrate")

    def try_enable_update_vanadium(self, cal_success):
        self.last_calibration_successful = cal_success
        if cal_success and self.view.get_new_checked():
            self.view.set_check_update_vanadium_enabled(True)
        else:
            self.view.set_check_update_vanadium_enabled(False)

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
