#from __future__ import (absolute_import, division, print_function)
try:
    from mantidplot import *
except ImportError:
    canMantidPlot = False

import os
import ui_sans_data_processor_window
from PyQt4 import QtGui, QtCore
from mantid.simpleapi import *
from mantid.kernel import Logger
from abc import ABCMeta, abstractmethod
from six import with_metaclass
from inspect import isclass
from mantidqtpython import MantidQt
from sans.common.enums import (ReductionDimensionality, OutputMode, SaveType, SANSInstrument, RebinType,
                               RangeStepType, SampleShape, ReductionMode)
from sans.gui_logic.gui_common import (get_reduction_mode_from_gui_selection,
                                       get_string_for_gui_from_reduction_mode, OPTIONS_SEPARATOR)
canMantidPlot = True

# ----------------------------------------------------------------------------------------------------------------------
# Globals
# ----------------------------------------------------------------------------------------------------------------------
gui_logger = Logger("SANS GUI LOGGER")


# ----------------------------------------------------------------------------------------------------------------------
# Free Functions
# ----------------------------------------------------------------------------------------------------------------------
def open_file_dialog(line_edit, filter_text, directory):
    dlg = QtGui.QFileDialog()
    dlg.setFileMode(QtGui.QFileDialog.AnyFile)
    dlg.setFilter(filter_text)
    dlg.setDirectory(directory)
    if dlg.exec_():
        file_names = dlg.selectedFiles()
        if file_names:
            line_edit.setText(file_names[0])


# ----------------------------------------------------------------------------------------------------------------------
# Gui Classes
# ----------------------------------------------------------------------------------------------------------------------
class SANSDataProcessorGui(QtGui.QMainWindow, ui_sans_data_processor_window.Ui_SansDataProcessorWindow):
    data_processor_table = None
    main_presenter = None

    class RunTabListener(with_metaclass(ABCMeta, object)):
        """
        Defines the elements which a presenter can listen to in this View
        """
        @abstractmethod
        def on_user_file_load(self):
            pass

        @abstractmethod
        def on_batch_file_load(self):
            pass

        @abstractmethod
        def on_processed_clicked(self):
            pass

        @abstractmethod
        def on_processing_finished(self):
            pass

    def __init__(self, main_presenter):
        """
        Initialise the interface
        """
        super(QtGui.QMainWindow, self).__init__()
        self.setupUi(self)

        # Main presenter

        self._main_presenter = main_presenter

        # Algorithm configuration
        self._gui_algorithm_name = self._main_presenter.get_gui_algorithm_name()
        self._white_list_entries = self._main_presenter.get_white_list()
        self._black_list = self._main_presenter.get_black_list()

        # Listeners allow us to to notify all presenters
        self._settings_listeners = []

        # Q Settings
        self.__generic_settings = "Mantid/ISISSANS"
        self.__user_file_path_key = "user_file_path"
        self.__batch_file_path_key = "batch_file_path"
        self.__pixel_adjustment_det_1_path_key = "pixel_adjustment_det_1_path"
        self.__pixel_adjustment_det_2_path_key = "pixel_adjustment_det_2_path"
        self.__wavelength_adjustment_det_1_path_key = "wavelength_adjustment_det_1_path"
        self.__wavelength_adjustment_det_2_path_key = "wavelength_adjustment_det_2_path"

        # Instrument
        self._instrument = None

        # Attach validators
        self._attach_validators()

    def add_listener(self, listener):
        if not isinstance(listener, SANSDataProcessorGui.RunTabListener):
            raise ValueError("The listener ist not of type RunTabListener but rather {}".format(type(listener)))
        self._settings_listeners.append(listener)

    def clear_listeners(self):
        self._settings_listeners = []

    def _call_settings_listeners(self, target):
        for listener in self._settings_listeners:
            target(listener)

    def set_current_page(self, index):
        self.main_stacked_widget.setCurrentIndex(index)

    def setup_layout(self):
        """
        Do further setup that could not be done in the designer.
        So far only two menus have been added, we need to add the processing table manually.
        """
        # --------------------------------------------------------------------------------------------------------------
        # Tab selection
        # --------------------------------------------------------------------------------------------------------------
        self.tab_choice_list.addItem("Runs")
        self.tab_choice_list.addItem("Settings")
        self.tab_choice_list.setAlternatingRowColors(True)
        self.tab_choice_list.setSpacing(10)
        self.tab_choice_list.currentRowChanged.connect(self.set_current_page)
        self.set_current_page(0)

        # --------------------------------------------------------------------------------------------------------------
        # Algorithm setup
        # --------------------------------------------------------------------------------------------------------------
        # Setup white list
        white_list = MantidQt.MantidWidgets.DataProcessorWhiteList()
        for entry in self._white_list_entries:
            # If there is a column name specified, then it is a white list entry.
            if entry.column_name:
                white_list.addElement(entry.column_name, entry.algorithm_property, entry.description,
                                      entry.show_value, entry.prefix)

        # Setup the black list, ie the properties which should not appear in the Options column

        # Processing algorithm (mandatory)
        alg = MantidQt.MantidWidgets.DataProcessorProcessingAlgorithm(self._gui_algorithm_name, 'unused_',
                                                                      self._black_list)

        # --------------------------------------------------------------------------------------------------------------
        # Main Tab
        # --------------------------------------------------------------------------------------------------------------
        self.data_processor_table = MantidQt.MantidWidgets.QDataProcessorWidget(white_list, alg, self)
        self._setup_main_tab()

        # --------------------------------------------------------------------------------------------------------------
        # Settings tabs
        # --------------------------------------------------------------------------------------------------------------
        self.reset_all_fields_to_default()
        self.pixel_adjustment_det_1_push_button.clicked.connect(self._on_load_pixel_adjustment_det_1)
        self.pixel_adjustment_det_2_push_button.clicked.connect(self._on_load_pixel_adjustment_det_2)
        self.wavelength_adjustment_det_1_push_button.clicked.connect(self._on_load_wavelength_adjustment_det_1)
        self.wavelength_adjustment_det_2_push_button.clicked.connect(self._on_load_wavelength_adjustment_det_2)

        # Set the merge settings
        self.reduction_mode_combo_box.currentIndexChanged .connect(self._on_reduction_mode_selection_has_changed)
        self._on_reduction_mode_selection_has_changed()  # Disable the merge settings initially

        return True

    def _setup_main_tab(self):
        # --------------------------------------------------------------------------------------------------------------
        # Header setup
        # --------------------------------------------------------------------------------------------------------------
        self.user_file_button.clicked.connect(self._on_user_file_load)
        self.batch_button.clicked.connect(self._on_batch_file_load)

        # --------------------------------------------------------------------------------------------------------------
        # Table setup
        # --------------------------------------------------------------------------------------------------------------
        # Add the presenter to the data processor
        self.data_processor_table.accept(self._main_presenter)

        # Set the list of available instruments in the widget and the default instrument
        no_instrument = SANSInstrument.to_string(SANSInstrument.NoInstrument)
        self.data_processor_table.setInstrumentList(no_instrument, no_instrument)

        # The widget will emit a 'runAsPythonScript' signal to run python code
        self.data_processor_table.runAsPythonScript.connect(self._run_python_code)
        self.data_processor_table.processButtonClicked.connect(self._processed_clicked)
        self.data_processor_table.processingFinished.connect(self._processing_finished)
        self.data_processor_widget_layout.addWidget(self.data_processor_table)

    def _processed_clicked(self):
        """
        Process runs
        """
        self._call_settings_listeners(lambda listener: listener.on_processed_clicked())

    def _processing_finished(self):
        """
        Clean up
        """
        self._call_settings_listeners(lambda listener: listener.on_processing_finished())

    def _on_user_file_load(self):
        """
        Load the user file
        """
        # Load the user file
        self._load_file(self.user_file_line_edit, "*.*", self.__generic_settings, self.__user_file_path_key,
                        self.get_user_file_path)

        # Notify presenters
        self._call_settings_listeners(lambda listener: listener.on_user_file_load())

    def _on_batch_file_load(self):
        """
        Load the batch file
        """
        self._load_file(self.batch_line_edit, "*.*", self.__generic_settings, self.__batch_file_path_key,
                        self.get_batch_file_path)
        self._call_settings_listeners(lambda listener: listener.on_batch_file_load())

    def get_user_file_path(self):
        return str(self.user_file_line_edit.text())

    def get_batch_file_path(self):
        return str(self.batch_line_edit.text())

    def _load_file(self, line_edit_field, filter_for_dialog, q_settings_group_key, q_settings_key, func):
        # Get the last location of the user file
        settings = QtCore.QSettings()
        settings.beginGroup(q_settings_group_key)
        last_path = settings.value(q_settings_key, "", type=str)
        settings.endGroup()

        # Open the dialog
        open_file_dialog(line_edit_field, filter_for_dialog, last_path)

        # Save the new location
        new_path, _ = os.path.split(func())
        if new_path:
            settings = QtCore.QSettings()
            settings.beginGroup(q_settings_group_key)
            settings.setValue(q_settings_key, new_path)
            settings.endGroup()

    def _on_load_pixel_adjustment_det_1(self):
        self._load_file(self.pixel_adjustment_det_1_line_edit, "*.*", self.__generic_settings,
                        self.__pixel_adjustment_det_1_path_key,  self.get_pixel_adjustment_det_1)

    def get_pixel_adjustment_det_1(self):
        return str(self.pixel_adjustment_det_1_line_edit.text())

    def _on_load_pixel_adjustment_det_2(self):
        self._load_file(self.pixel_adjustment_det_2_line_edit, "*.*", self.__generic_settings,
                        self.__pixel_adjustment_det_2_path_key,  self.get_pixel_adjustment_det_2)

    def get_pixel_adjustment_det_2(self):
        return str(self.pixel_adjustment_det_2_line_edit.text())

    def _on_load_wavelength_adjustment_det_1(self):
        self._load_file(self.wavelength_adjustment_det_1_line_edit, "*.*", self.__generic_settings,
                        self.__wavelength_adjustment_det_1_path_key,  self.get_wavelength_adjustment_det_1)

    def get_wavelength_adjustment_det_1(self):
        return str(self.wavelength_adjustment_det_1_line_edit.text())

    def _on_load_wavelength_adjustment_det_2(self):
        self._load_file(self.wavelength_adjustment_det_2_line_edit, "*.*", self.__generic_settings,
                        self.__wavelength_adjustment_det_2_path_key,  self.get_wavelength_adjustment_det_2)

    def get_wavelength_adjustment_det_2(self):
        return str(self.wavelength_adjustment_det_2_line_edit.text())

    def _on_reduction_mode_selection_has_changed(self):
        selection = self.reduction_mode_combo_box.currentText()
        is_merged = selection == ReductionMode.to_string(ReductionMode.Merged)
        self.merged_settings.setEnabled(is_merged)

    # ------------------------------------------------------------------------------------------------------------------
    # Elements which can be set and read by the model
    # ------------------------------------------------------------------------------------------------------------------
    def set_instrument_settings(self, instrument):
        if instrument:
            self._instrument = instrument
            self.data_processor_table.setInstrumentList(instrument, instrument)

    def update_gui_combo_box(self, value, expected_type, combo_box):
        # There are two types of values that can be passed:
        # Lists: we set the combo box to the values in the list
        # expected_type: we set the expected type
        if isinstance(value, list):
            gui_element = getattr(self, combo_box)
            gui_element.clear()
            for element in value:
                self._add_list_element_to_combo_box(gui_element=gui_element, element=element,
                                                    expected_type=expected_type)
        else:
            # Convert the value to the correct GUI string
            if issubclass(value, expected_type):
                gui_element = getattr(self, combo_box)
                self._set_enum_as_element_in_combo_box(gui_element=gui_element, element=value,
                                                       expected_type=expected_type)
            else:
                raise RuntimeError("Expected an input of type {}, but got {}".format(expected_type, type(value)))

    def _add_list_element_to_combo_box(self, gui_element, element, expected_type=None):
        if expected_type is not None and isclass(element) and issubclass(element, expected_type):
            self._set_enum_as_element_in_combo_box(gui_element=gui_element, element=element,
                                                   expected_type=expected_type)
        else:
            gui_element.addItem(element)

    @staticmethod
    def _set_enum_as_element_in_combo_box(gui_element, element, expected_type):
        value_as_string = expected_type.to_string(element)
        index = gui_element.findText(value_as_string)
        if index != -1:
            gui_element.setCurrentIndex(index)

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

    def update_simple_line_edit_field(self, line_edit, value):
        if value:
            gui_element = getattr(self, line_edit)
            gui_element.setText(str(value))

    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # START ACCESSORS
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    # ==================================================================================================================
    # ==================================================================================================================
    # FRONT TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # -----------------------------------------------------------------
    # Save Options
    # -----------------------------------------------------------------
    @property
    def save_types(self):
        checked_save_types = []
        if self.can_sas_checkbox.isChecked():
            checked_save_types.append(SaveType.CanSAS)
        if self.nx_can_sas_checkbox.isChecked():
            checked_save_types.append(SaveType.NXcanSAS)
        if self.rkh_checkbox.isChecked():
            checked_save_types.append(SaveType.RKH)
        if self.nist_qxy_checkbox.isChecked():
            checked_save_types.append(SaveType.NistQxy)
        return checked_save_types

    @save_types.setter
    def save_types(self, values):
        for value in values:
            if value is SaveType.CanSAS:
                self.can_sas_checkbox.setChecked(True)
            elif value is SaveType.NXcanSAS:
                self.nx_can_sas_checkbox.setChecked(True)
            elif value is SaveType.RKH:
                self.rkh_checkbox.setChecked(True)
            elif value is SaveType.NistQxy:
                self.nist_qxy_checkbox.setChecked(True)

    @property
    def zero_error_free(self):
        return self.save_zero_error_free.isChecked()

    @zero_error_free.setter
    def zero_error_free(self, value):
        self.save_zero_error_free.setChecked(value)

    # -----------------------------------------------------------------
    # Global options
    # -----------------------------------------------------------------
    @property
    def use_optimizations(self):
        return self.use_optimizations_checkbox.isChecked()

    @use_optimizations.setter
    def use_optimizations(self, value):
        self.use_optimizations_checkbox.setChecked(value)

    @property
    def output_mode(self):
        if self.output_mode_memory_radio_button.isChecked():
            return OutputMode.PublishToADS
        elif self.output_mode_file_radio_button.isChecked():
            return OutputMode.SaveToFile()
        elif self.output_mode_both_radio_button.isChecked():
            return OutputMode.Both
        else:
            gui_logger.warning("The output format was not specified. Defaulting to saving to memory only.")
            return OutputMode.PublishToADS

    @output_mode.setter
    def output_mode(self, value):
        if value is OutputMode.PublishToADS:
            self.output_mode_memory_radio_button.setChecked(True)
        elif value is OutputMode.SaveToFile:
            self.output_mode_file_radio_button.setChecked(True)
        elif value is OutputMode.Both:
            self.output_mode_both_radio_button.setCheck(True)

    @property
    def compatibility_mode(self):
        return self.compatibility_mode_check_box.isChecked()

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self.compatibility_mode_check_box.setChecked(value)

    # ==================================================================================================================
    # ==================================================================================================================
    # General TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # General group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def reduction_dimensionality(self):
        return ReductionDimensionality.OneDim if self.reduction_dimensionality_1D.isChecked() \
            else ReductionDimensionality.TwoDim

    @reduction_dimensionality.setter
    def reduction_dimensionality(self, value):
        is_1d = value is ReductionDimensionality.OneDim
        self.reduction_dimensionality_1D.setChecked(is_1d)
        self.reduction_dimensionality_2D.setChecked(not is_1d)

    @property
    def reduction_mode(self):
        reduction_mode_as_string = self.reduction_mode_combo_box.currentText()
        return get_reduction_mode_from_gui_selection(reduction_mode_as_string)

    @reduction_mode.setter
    def reduction_mode(self, value):
        # There are two types of values that can be passed:
        # Lists: we set the combo box to the values in the list
        # String: we look for string and we set it
        if isinstance(value, list):
            self.reduction_mode_combo_box.clear()
            for element in value:
                self.reduction_mode_combo_box.addItem(element)
        else:
            # Convert the value to the correct GUI string
            reduction_mode_as_string = get_string_for_gui_from_reduction_mode(value, self._instrument)
            if reduction_mode_as_string:
                index = self.reduction_mode_combo_box.findText(reduction_mode_as_string)
                if index != -1:
                    self.reduction_mode_combo_box.setCurrentIndex(index)

    # ------------------------------------------------------------------------------------------------------------------
    # Event slices group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def event_slices(self):
        return str(self.slice_event_line_edit.text())

    @event_slices.setter
    def event_slices(self, value):
        self.slice_event_line_edit.setText(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength Group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def wavelength_step_type(self):
        step_type_as_string = self.wavelength_step_type_combo_box.currentText().encode('utf-8')
        return RangeStepType.from_string(step_type_as_string)

    @wavelength_step_type.setter
    def wavelength_step_type(self, value):
        self.update_gui_combo_box(value=value, expected_type=RangeStepType, combo_box="wavelength_step_type_combo_box")

    @property
    def wavelength_min(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_min_line_edit", expected_type=float)

    @wavelength_min.setter
    def wavelength_min(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_min_line_edit", value=value)

    @property
    def wavelength_max(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_max_line_edit", expected_type=float)

    @wavelength_max.setter
    def wavelength_max(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_max_line_edit", value=value)

    @property
    def wavelength_step(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_step_line_edit", expected_type=float)

    @wavelength_step.setter
    def wavelength_step(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_step_line_edit", value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale Group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def sample_shape(self):
        geometry_as_string = self.geometry_combo_box.currentText().encode('utf-8')
        # Either the selection is something that can be converted to a SampleShape or we need to read from file
        try:
            return SampleShape.from_string(geometry_as_string)
        except RuntimeError:
            return None

    @sample_shape.setter
    def sample_shape(self, value):
        if value is None:
            # Set to the default
            self.geometry_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=SampleShape, combo_box="geometry_combo_box")

    @property
    def absolute_scale(self):
        return self.get_simple_line_edit_field(line_edit="absolute_scale_line_edit", expected_type=float)

    @absolute_scale.setter
    def absolute_scale(self, value):
        self.update_simple_line_edit_field(line_edit="absolute_scale_line_edit", value=value)

    @property
    def sample_height(self):
        return self.get_simple_line_edit_field(line_edit="height_line_edit", expected_type=float)

    @sample_height.setter
    def sample_height(self, value):
        self.update_simple_line_edit_field(line_edit="height_line_edit", value=value)

    @property
    def sample_width(self):
        return self.get_simple_line_edit_field(line_edit="width_line_edit", expected_type=float)

    @sample_width.setter
    def sample_width(self, value):
        self.update_simple_line_edit_field(line_edit="width_line_edit", value=value)

    @property
    def sample_thickness(self):
        return self.get_simple_line_edit_field(line_edit="thickness_line_edit", expected_type=float)

    @sample_thickness.setter
    def sample_thickness(self, value):
        self.update_simple_line_edit_field(line_edit="thickness_line_edit", value=value)

    @property
    def z_offset(self):
        return self.get_simple_line_edit_field(line_edit="z_offset_line_edit", expected_type=float)

    @z_offset.setter
    def z_offset(self, value):
        self.update_simple_line_edit_field(line_edit="z_offset_line_edit", value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # ADJUSTMENT TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Monitor normalization
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def normalization_incident_monitor(self):
        return self.get_simple_line_edit_field(line_edit="monitor_normalization_line_edit", expected_type=int)

    @normalization_incident_monitor.setter
    def normalization_incident_monitor(self, value):
        self.update_simple_line_edit_field(line_edit="monitor_normalization_line_edit", value=value)

    @property
    def normalization_interpolate(self):
        return self.monitor_normalization_interpolating_rebin_check_box.isChecked()

    @normalization_interpolate.setter
    def normalization_interpolate(self, value):
        self.monitor_normalization_interpolating_rebin_check_box.setChecked(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def normalization_incident_monitor(self):
        return self.get_simple_line_edit_field(line_edit="transmission_line_edit", expected_type=int)

    @normalization_incident_monitor.setter
    def normalization_incident_monitor(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_line_edit", value=value)

    @property
    def transmission_interpolate(self):
        return self.transmission_interpolating_rebin_check_box.isChecked()

    @transmission_interpolate.setter
    def transmission_interpolate(self, value):
        self.transmission_interpolating_rebin_check_box.setChecked(value)

    @property
    def transmission_roi_files(self):
        return self.get_simple_line_edit_field(line_edit="transmission_roi_files_line_edit", expected_type=str)

    @transmission_roi_files.setter
    def transmission_roi_files(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_roi_files_line_edit", value=value)

    @property
    def transmission_mask_files(self):
        return self.get_simple_line_edit_field(line_edit="transmission_mask_files_line_edit", expected_type=str)

    @transmission_mask_files.setter
    def transmission_mask_files(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_mask_files_line_edit", value=value)

    @property
    def transmission_radius(self):
        return self.get_simple_line_edit_field(line_edit="transmission_radius_line_edit", expected_type=float)

    @transmission_radius.setter
    def transmission_radius(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_radius_line_edit", value=value)

    @property
    def transmission_monitor(self):
        return 3 if self.transmission_m3_radio_button.isChecked() else 4

    @transmission_monitor.setter
    def transmission_monitor(self, value):
        if value == 3:
            self.transmission_m3_radio_button.setChecked(True)
        else:
            self.transmission_m4_radio_button.setChecked(True)

    @property
    def transmission_m4_shift(self):
        return self.get_simple_line_edit_field(line_edit="transmission_m4_shift_line_edit", expected_type=float)

    @transmission_m4_shift.setter
    def transmission_m4_shift(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_m4_shift_line_edit", value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength- and pixel-adjustment files
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def pixel_adjustment_det_1(self):
        return self.get_simple_line_edit_field(line_edit="pixel_adjustment_det_1_line_edit", expected_type=str)

    @pixel_adjustment_det_1.setter
    def pixel_adjustment_det_1(self, value):
        self.update_simple_line_edit_field(line_edit="pixel_adjustment_det_1_line_edit", value=value)

    @property
    def pixel_adjustment_det_2(self):
        return self.get_simple_line_edit_field(line_edit="pixel_adjustment_det_2_line_edit", expected_type=str)

    @pixel_adjustment_det_2.setter
    def pixel_adjustment_det_2(self, value):
        self.update_simple_line_edit_field(line_edit="pixel_adjustment_det_2_line_edit", value=value)

    @property
    def wavelength_adjustment_det_1(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_adjustment_det_1_line_edit", expected_type=str)

    @wavelength_adjustment_det_1.setter
    def wavelength_adjustment_det_1(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_adjustment_det_1_line_edit", value=value)

    @property
    def wavelength_adjustment_det_2(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_adjustment_det_2_line_edit", expected_type=str)

    @wavelength_adjustment_det_2.setter
    def wavelength_adjustment_det_2(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_adjustment_det_2_line_edit", value=value)


    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # END ACCESSORS
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    def _attach_validators(self):
        # Setup the list of validators
        double_validator = QtGui.QDoubleValidator()
        positive_double_validator = QtGui.QDoubleValidator()
        positive_double_validator.setBottom(0.0)
        positive_integer_validator = QtGui.QIntValidator()
        positive_integer_validator.setBottom(1)

        # -------------------------------
        # General tab
        # -------------------------------
        self.wavelength_min_line_edit.setValidator(positive_double_validator)
        self.wavelength_max_line_edit.setValidator(positive_double_validator)
        self.wavelength_step_line_edit.setValidator(positive_double_validator)

        self.absolute_scale_line_edit.setValidator(double_validator)
        self.height_line_edit.setValidator(positive_double_validator)
        self.width_line_edit.setValidator(positive_double_validator)
        self.thickness_line_edit.setValidator(positive_double_validator)
        self.z_offset_line_edit.setValidator(double_validator)

        # --------------------------------
        # Adjustment tab
        # --------------------------------
        self.monitor_normalization_line_edit.setValidator(positive_integer_validator)
        self.transmission_line_edit.setValidator(positive_integer_validator)
        self.transmission_radius_line_edit.setValidator(positive_double_validator)
        self.transmission_m4_shift_line_edit.setValidator(double_validator)

    def reset_all_fields_to_default(self):
        # ------------------------------
        # General tab
        # ------------------------------
        self.reduction_dimensionality_1D.setChecked(True)
        self.reduction_mode_combo_box.setCurrentIndex(0)

        self.merged_q_range_start_line_edit.setText("")
        self.merged_q_range_stop_line_edit.setText("")
        self.merged_scale_line_edit.setText("")
        self.merged_shift_line_edit.setText("")
        self.merged_shift_use_fit_check_box.setChecked(False)
        self.merged_scale_use_fit_check_box.setChecked(False)
        self.merged_use_q_range_check_box.setChecked(False)

        self.slice_event_line_edit.setText("")

        self.wavelength_min_line_edit.setText("")
        self.wavelength_max_line_edit.setText("")
        self.wavelength_step_line_edit.setText("")
        self.wavelength_step_type_combo_box.setCurrentIndex(0)

        self.absolute_scale_line_edit.setText("")
        self.geometry_combo_box.setCurrentIndex(0)
        self.height_line_edit.setText("")
        self.width_line_edit.setText("")
        self.thickness_line_edit.setText("")
        self.z_offset_line_edit.setText("")

        # --------------------------------
        # Adjustment tab
        # --------------------------------
        self.monitor_normalization_line_edit.setText("")
        self.monitor_normalization_interpolating_rebin_check_box.setChecked(False)

        self.transmission_line_edit.setText("")
        self.transmission_interpolating_rebin_check_box.setChecked(False)
        self.transmission_target_combo_box.setCurrentIndex(0)
        self.transmission_m3_radio_button.setChecked(True)
        self.transmission_m4_shift_line_edit.setText("")
        self.transmission_radius_line_edit.setText("")
        self.transmission_roi_files_line_edit.setText("")
        self.transmission_mask_files_line_edit.setText("")

        self.pixel_adjustment_det_1_line_edit.setText("")
        self.pixel_adjustment_det_2_line_edit.setText("")

        self.wavelength_adjustment_det_1_line_edit.setText("")
        self.wavelength_adjustment_det_2_line_edit.setText("")

    # ------------------------------------------------------------------------------------------------------------------
    # Table interaction
    # ------------------------------------------------------------------------------------------------------------------
    def get_cell(self, row, column, convert_to=None):
        value = self.data_processor_table.getCell(row, column)
        return value if convert_to is None else convert_to(value)

    def set_cell(self, value, row, column):
        value_as_str = str(value)
        self.data_processor_table.setCell(value_as_str, row, column)

    def get_number_of_rows(self):
        return self.data_processor_table.getNumberOfRows()

    def clear_table(self):
        self.data_processor_table.clearTable()

    def add_row(self, value):
        """
        Inserts a row in to the table.

        The value needs to have the form: "Input:test,Output:test,Options:OutputWorkspace=2", where the keys
        are the names of the column
        :param value: the value specifying a row
        """
        self.data_processor_table.transfer([value])

    # ------------------------------------------------------------------------------------------------------------------
    # NON-ESSENTIAL (Should we get rid of it?)
    # ------------------------------------------------------------------------------------------------------------------
    def add_actions_to_menus(self, workspace_list):
        """
        Initialize table actions. Some table actions are not shown with the widget but they can be added to
        external menus.
        In this interface we have a 'File' menu and an 'Edit' menu
        """
        self.menuEdit.clear()
        self.menuFile.clear()

        # Actions that go in the 'Edit' menu
        self._create_action(MantidQt.MantidWidgets.DataProcessorProcessCommand(self.data_processor_table),
                            self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorPlotRowCommand(self.data_processor_table),
                            self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorAppendRowCommand(self.data_processor_table),
                            self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorCopySelectedCommand(self.data_processor_table
                                                                                    ), self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorCutSelectedCommand(self.data_processor_table),
                            self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorPasteSelectedCommand(self.data_processor_table),
                            self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorClearSelectedCommand(self.data_processor_table),
                            self.menuEdit)
        self._create_action(MantidQt.MantidWidgets.DataProcessorDeleteRowCommand(self.data_processor_table),
                            self.menuEdit)

        # Actions that go in the 'File' menu
        self._create_action(MantidQt.MantidWidgets.DataProcessorOpenTableCommand(self.data_processor_table),
                            self.menuFile, workspace_list)
        self._create_action(MantidQt.MantidWidgets.DataProcessorNewTableCommand(self.data_processor_table),
                            self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessorSaveTableCommand(self.data_processor_table),
                            self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessorSaveTableAsCommand(self.data_processor_table),
                            self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessorImportTableCommand(self.data_processor_table),
                            self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessorExportTableCommand(self.data_processor_table),
                            self.menuFile)
        self._create_action(MantidQt.MantidWidgets.DataProcessorOptionsCommand(self.data_processor_table),
                            self.menuFile)

    def _create_action(self, command, menu, workspace_list=None):
        """
        Create an action from a given DataProcessorCommand and add it to a given menu
        A 'workspace_list' can be provided but it is only intended to be used with DataProcessorOpenTableCommand.
        It refers to the list of table workspaces in the ADS that could be loaded into the widget. Note that only
        table workspaces with an appropriate number of columns and column types can be loaded.
        """
        if (workspace_list is not None and command.name() == "Open Table"):
            submenu = QtGui.QMenu(command.name(), self)
            submenu.setIcon(QtGui.QIcon(command.icon()))

            for ws in workspace_list:
                ws_command = MantidQt.MantidWidgets.DataProcessorWorkspaceCommand(self.data_processor_table, ws)
                action = QtGui.QAction(QtGui.QIcon(ws_command.icon()), ws_command.name(), self)
                action.triggered.connect(lambda: self._connect_action(ws_command))
                submenu.addAction(action)

            menu.addMenu(submenu)
        else:
            action = QtGui.QAction(QtGui.QIcon(command.icon()), command.name(), self)
            action.setShortcut(command.shortcut())
            action.setStatusTip(command.tooltip())
            action.triggered.connect(lambda: self._connect_action(command))
            menu.addAction(action)

    def _connect_action(self, command):
        """
        Executes an action
        """
        command.execute()

    def _run_python_code(self, text):
        """
        Re-emits 'runPytonScript' signal
        """
        mantidplot.runPythonScript(text, True)
