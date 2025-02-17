# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT, create_error_message
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.cropping.cropping_presenter import CroppingPresenter
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.EnggUtils import GROUP

from mantidqt.utils.asynchronous import AsyncTask
from mantid.simpleapi import logger
from mantidqt.utils.observer_pattern import Observable, GenericObservable


class CalibrationPresenter(object):
    def __init__(self, model, view):
        self.model = model
        self.view = view
        self.worker = None
        self.calibration_notifier = self.CalibrationNotifier(self)
        self.prm_filepath_notifier_gsas2 = GenericObservable()
        self.current_calibration = CalibrationInfo()

        self.connect_view_signals()

        # Main Window State Variables
        self.instrument = "ENGINX"
        self.rb_num = None

        # Cropping Options
        self.cropping_widget = CroppingPresenter(parent=self.view, view=self.view.get_cropping_widget())
        self.show_cropping(False)

    def connect_view_signals(self):
        self.view.set_on_calibrate_clicked(self.on_calibrate_clicked)
        self.view.set_enable_controls_connection(self.set_calibrate_controls_enabled)
        self.view.set_update_field_connection(self.set_field_value)
        self.view.set_on_radio_new_toggled(self.set_create_new_enabled)
        self.view.set_on_radio_existing_toggled(self.set_load_existing_enabled)
        self.view.set_on_check_cropping_state_changed(self.show_cropping)

    def update_calibration_from_view(self):
        self.current_calibration.clear()
        if self.view.get_load_checked():
            # loading calibration from path to .prm
            self.current_calibration.set_calibration_from_prm_fname(self.view.get_path_filename())
        else:
            # make a new calibration
            sample_file = self.view.get_sample_filename()
            self.current_calibration.set_calibration_paths(self.instrument, sample_file)
            # set group and any additional parameters needed
            if self.view.get_crop_checked():
                self.current_calibration.set_group(self.cropping_widget.get_group())
                if self.current_calibration.group == GROUP.CUSTOM:
                    self.current_calibration.set_grouping_file(self.cropping_widget.get_custom_groupingfile())
                elif self.current_calibration.group == GROUP.CROPPED:
                    self.current_calibration.set_spectra_list(self.cropping_widget.get_custom_spectra())
            else:
                # default if no cropping
                self.current_calibration.set_group(GROUP.BOTH)

    def on_calibrate_clicked(self):
        if self.view.get_new_checked() and self._validate():
            self.update_calibration_from_view()
            self.start_calibration_worker(self.view.get_plot_output())
        elif self.view.get_load_checked() and self.validate_path():
            self.update_calibration_from_view()
            self.model.load_existing_calibration_files(self.current_calibration)
            self._notify_updated_calibration()

    def start_calibration_worker(self, plot_output):
        """
        Calibrate the data in a separate thread so as to not freeze the GUI.
        """
        self.worker = AsyncTask(
            self.model.create_new_calibration,
            (self.current_calibration, self.rb_num, plot_output),
            error_cb=self._on_error,
            success_cb=self._on_success,
        )
        self.set_calibrate_controls_enabled(False)
        self.worker.start()

    def _on_error(self, error_info):
        logger.error(str(error_info))
        self.emit_enable_button_signal()

    def _on_success(self, success_info):
        self._notify_updated_calibration()
        self.emit_enable_button_signal()

    def _notify_updated_calibration(self):
        self.calibration_notifier.notify_subscribers(self.current_calibration)
        set_setting(
            output_settings.INTERFACES_SETTINGS_GROUP,
            output_settings.ENGINEERING_PREFIX,
            "last_calibration_path",
            self.current_calibration.get_prm_filepath(),
        )
        self.prm_filepath_notifier_gsas2.notify_subscribers(self.model.get_last_prm_file_gsas2())

    def set_field_value(self):
        self.view.set_sample_text(self.current_calibration.get_sample())

    def load_last_calibration(self) -> None:
        """
        Loads the most recently created or loaded calibration into the interface instance. To be used on interface
        startup.
        """
        last_grouping_path = get_setting(
            output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, "last_calibration_path"
        )
        if last_grouping_path:
            self.view.set_load_checked(True)
            self.view.set_file_text_with_search(last_grouping_path)

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
        if not self.view.get_sample_valid():
            create_error_message(self.view, "Check run numbers/path is valid.")
            return False
        if self.view.get_crop_checked():
            if self.cropping_widget.get_custom_groupingfile_enabled() and not self.cropping_widget.is_groupingfile_valid():
                create_error_message(self.view, "Check custom grouping file path is valid.")
                return False
            if self.cropping_widget.get_custom_spectra_enabled() and not self.cropping_widget.is_spectra_valid():
                create_error_message(self.view, "Check custom spectra are valid.")
                return False
        return True

    def validate_path(self):
        return self.view.get_path_valid()

    def emit_enable_button_signal(self):
        self.view.sig_enable_controls.emit(True)

    def set_calibrate_controls_enabled(self, enabled):
        self.view.set_calibrate_button_enabled(enabled)

    def set_create_new_enabled(self, enabled):
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
            self.view.set_check_cropping_enabled(False)
            self.view.set_check_cropping_checked(False)

    def set_calibrate_button_text(self, text):
        self.view.set_calibrate_button_text(text)

    def find_files(self):
        self.view.find_sample_files()

    def show_cropping(self, show):
        self.view.set_cropping_widget_visibility(show)

    def add_prm_gsas2_subscriber(self, obs):
        self.prm_filepath_notifier_gsas2.add_subscriber(obs)

    # -----------------------
    # Observers / Observables
    # -----------------------
    class CalibrationNotifier(Observable):
        def __init__(self, outer):
            Observable.__init__(self)
            self.outer = outer

        def notify_subscribers(self, *args, **kwargs):
            Observable.notify_subscribers(self, *args)
