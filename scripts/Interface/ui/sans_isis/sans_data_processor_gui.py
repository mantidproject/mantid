# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

"""Main view for the ISIS SANS reduction interface."""

from abc import ABCMeta, abstractmethod

from qtpy.QtCore import QRegExp
from qtpy.QtGui import QDoubleValidator, QIntValidator, QRegExpValidator
from qtpy.QtWidgets import QListWidgetItem, QMessageBox, QFileDialog, QMainWindow, QLineEdit
from mantid.kernel import Logger, UsageService, FeatureType
from enum import Enum

from mantidqt import icons
from mantidqt.interfacemanager import InterfaceManager
from mantidqt.utils.qt import load_ui
from mantidqt.widgets import jobtreeview, manageuserdirectories
from reduction_gui.reduction.scripter import execute_script
from sans.common.enums import ReductionDimensionality, OutputMode, SANSInstrument, RangeStepType, ReductionMode, FitType
from sans.gui_logic.gui_common import (
    get_reduction_mode_from_gui_selection,
    get_reduction_mode_strings_for_gui,
    get_string_for_gui_from_reduction_mode,
    GENERIC_SETTINGS,
    load_file,
    load_property,
    set_setting,
    get_instrument_from_gui_selection,
)
from sans.gui_logic.models.POD.save_options import SaveOptions
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.sum_runs_model import SumRunsModel
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from ui.sans_isis.SANSSaveOtherWindow import SANSSaveOtherDialog
from ui.sans_isis.modified_qt_field_factory import ModifiedQtFieldFactory

from ui.sans_isis.sans_gui_observable import SansGuiObservable

Ui_SansDataProcessorWindow, _ = load_ui(__file__, "sans_data_processor_window.ui")


# ----------------------------------------------------------------------------------------------------------------------
# Gui Classes
# ----------------------------------------------------------------------------------------------------------------------
class SANSDataProcessorGui(QMainWindow, Ui_SansDataProcessorWindow):
    data_processor_table = None
    INSTRUMENTS = None
    VARIABLE = "Variable"

    # This dictates the order displayed on the GUI
    COLUMN_NAMES = [
        "Sample Scatter",
        "Sample Transmission",
        "Sample Direct",
        "Can Scatter",
        "Can Transmission",
        "Can Direct",
        "Output Name",
        "User File",
        "Sample Thickness",
        "Options",
        "Sample Shape",
        "Sample Height",
        "Sample Width",
        "Background Workspace",
        "Scale Factor",
        "SSP",
        "STP",
        "SDP",
        "CSP",
        "CTP",
        "CDP",
    ]

    MULTI_PERIOD_COLUMNS = ["SSP", "STP", "SDP", "CSP", "CTP", "CDP"]

    class RunTabListener(metaclass=ABCMeta):
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
        def on_reduction_mode_selection_has_changed(self, selection):
            pass

        @abstractmethod
        def on_mask_file_add(self):
            pass

        @abstractmethod
        def on_process_selected_clicked(self):
            pass

        @abstractmethod
        def on_process_all_clicked(self):
            pass

        @abstractmethod
        def on_load_clicked(self):
            pass

        @abstractmethod
        def on_multi_period_selection(self, show_periods):
            pass

        @abstractmethod
        def on_sample_geometry_selection(self, show_geometry):
            pass

        @abstractmethod
        def on_background_subtraction_selection(self, show_background_subtraction):
            pass

        @abstractmethod
        def on_output_mode_changed(self):
            pass

        @abstractmethod
        def on_data_changed(self, index, row):
            pass

        @abstractmethod
        def on_manage_directories(self):
            pass

        @abstractmethod
        def on_instrument_changed(self):
            pass

        @abstractmethod
        def on_rows_removed(self):
            pass

        @abstractmethod
        def on_copy_rows_requested(self):
            pass

        @abstractmethod
        def on_paste_rows_requested(self):
            pass

        @abstractmethod
        def on_insert_row(self):
            pass

        @abstractmethod
        def on_erase_rows(self):
            pass

        @abstractmethod
        def on_cut_rows(self):
            pass

        @abstractmethod
        def on_save_other(self):
            pass

        @abstractmethod
        def on_field_edit(self):
            """
            Emitted when the view changes off settings tab
            """
            pass

    def __init__(self, parent=None, window_flags=None):
        """
        Initialise the interface
        """
        super(QMainWindow, self).__init__(parent)
        if window_flags:
            self.setWindowFlags(window_flags)
        self.setupUi(self)

        # Listeners allow us to to notify all presenters
        self._settings_listeners = []

        # Q Settings
        self.__generic_settings = GENERIC_SETTINGS
        self.__path_key = "sans_path"
        self.__user_file_key = "user_file"
        self.__batch_file_key = "batch_file"
        self.__mask_file_input_path_key = "mask_files"
        self.__output_mode_key = "output_mode"
        self.__save_can_key = "save_can"

        # Logger
        self.gui_logger = Logger("SANS GUI LOGGER")

        # Track if we need to refresh monitor 5 shift box when we update
        self._has_monitor_5 = False

        # Instrument
        SANSDataProcessorGui.INSTRUMENTS = ",".join(
            item.value for item in [SANSInstrument.SANS2D, SANSInstrument.LOQ, SANSInstrument.LARMOR, SANSInstrument.ZOOM]
        )

        self._column_index_map = None

        self.instrument = SANSInstrument.NO_INSTRUMENT

        self.paste_button.setIcon(icons.get_icon("mdi.content-paste"))
        self.copy_button.setIcon(icons.get_icon("mdi.content-copy"))
        self.cut_button.setIcon(icons.get_icon("mdi.content-cut"))
        self.erase_button.setIcon(icons.get_icon("mdi.eraser"))
        self.delete_row_button.setIcon(icons.get_icon("mdi.table-row-remove"))
        self.insert_row_button.setIcon(icons.get_icon("mdi.table-row-plus-after"))
        self.export_table_button.setIcon(icons.get_icon("mdi.file-export"))

        self._attach_signal_slots()
        self._create_observables()

        # Attach validators
        self._attach_validators()

        self._setup_progress_bar()
        self._setup_add_runs_page()

        # At a later date we can drop new once we confirm the old GUI is not using "ISIS SANS"
        UsageService.registerFeatureUsage(FeatureType.Interface, "ISIS SANS (new)", False)

    def _attach_signal_slots(self):
        self.paste_button.clicked.connect(self._paste_rows_requested)
        self.copy_button.clicked.connect(self._copy_rows_requested)
        self.erase_button.clicked.connect(self._erase_rows)
        self.cut_button.clicked.connect(self._cut_rows)
        self.delete_row_button.clicked.connect(self._remove_rows_requested_from_button)
        self.insert_row_button.clicked.connect(self._on_insert_button_pressed)
        self.export_table_button.clicked.connect(self._export_table_clicked)
        self.save_other_pushButton.clicked.connect(self._on_save_other_button_pressed)
        self.save_can_checkBox.clicked.connect(self._on_save_can_clicked)

        modified_field_factory = ModifiedQtFieldFactory(self._on_field_edit)
        modified_field_factory.attach_to_children(self.settings_page)

    def _create_observables(self):
        self._observable_items = SansGuiObservable()
        for save_checkbox in [self.can_sas_checkbox, self.nx_can_sas_checkbox, self.rkh_checkbox]:
            save_checkbox.clicked.connect(self._observable_items.save_options.notify_subscribers)

        for reduction_checkboxes in [self.reduction_dimensionality_1D, self.reduction_dimensionality_2D]:
            reduction_checkboxes.clicked.connect(self._observable_items.reduction_dim.notify_subscribers)

    def get_observable(self) -> SansGuiObservable:
        return self._observable_items

    def _setup_progress_bar(self):
        self.batch_progress_bar.setMinimum(0)
        self.batch_progress_bar.setMaximum(1)
        self.batch_progress_bar.setValue(0)

    def add_listener(self, listener):
        if not isinstance(listener, SANSDataProcessorGui.RunTabListener):
            raise ValueError("The listener is not of type RunTabListener but rather {}".format(type(listener)))
        self._settings_listeners.append(listener)

    def clear_listeners(self):
        self._settings_listeners = []

    def _call_settings_listeners(self, target):
        for listener in self._settings_listeners:
            target(listener)

    def set_current_page(self, index):
        self.main_stacked_widget.setCurrentIndex(index)

    def _setup_add_runs_page(self):
        self.add_runs_presenter = AddRunsPagePresenter(SumRunsModel(self.add_runs_page), self.add_runs_page, self)

    def setup_layout(self):
        """
        Do further setup that could not be done in the designer.
        So far only two menus have been added, we need to add the processing table manually.
        """
        # --------------------------------------------------------------------------------------------------------------
        # Tab selection
        # --------------------------------------------------------------------------------------------------------------
        self.tab_choice_list.setAlternatingRowColors(True)
        self.tab_choice_list.setSpacing(10)
        self.tab_choice_list.currentRowChanged.connect(self.set_current_page)
        self.set_current_page(0)

        runs_icon = icons.get_icon("mdi.play")
        _ = QListWidgetItem(runs_icon, "Runs", self.tab_choice_list)

        settings_icon = icons.get_icon("mdi.settings")
        _ = QListWidgetItem(settings_icon, "Settings", self.tab_choice_list)

        centre_icon = icons.get_icon("mdi.adjust")
        _ = QListWidgetItem(centre_icon, "Beam Centre", self.tab_choice_list)

        add_runs_page_icon = icons.get_icon("mdi.plus")
        _ = QListWidgetItem(add_runs_page_icon, "Sum Runs", self.tab_choice_list)

        diagnostic_icon = icons.get_icon("mdi.stethoscope")
        _ = QListWidgetItem(diagnostic_icon, "Diagnostic Page", self.tab_choice_list)

        # Set the 0th row enabled
        self.tab_choice_list.setCurrentRow(0)

        # --------------------------------------------------------------------------------------------------------------
        # Main Tab
        # --------------------------------------------------------------------------------------------------------------
        self.create_data_table()

        self._setup_main_tab()

        self.multi_period_check_box.stateChanged.connect(self._on_multi_period_selection)
        self.sample_geometry_checkbox.stateChanged.connect(self._on_sample_geometry_selection)
        self.background_subtraction_checkbox.stateChanged.connect(self._on_background_subtraction_selection)

        self.wavelength_step_type_combo_box.currentIndexChanged.connect(self._on_wavelength_step_type_changed)

        self.process_selected_button.clicked.connect(self._process_selected_clicked)
        self.process_all_button.clicked.connect(self._process_all_clicked)

        self.load_button.clicked.connect(self._load_clicked)

        self.help_button.clicked.connect(self._on_help_button_clicked)

        # Output mode radio buttons
        self.output_mode_memory_radio_button.clicked.connect(self._on_output_mode_clicked)
        self.output_mode_file_radio_button.clicked.connect(self._on_output_mode_clicked)
        self.output_mode_both_radio_button.clicked.connect(self._on_output_mode_clicked)

        # --------------------------------------------------------------------------------------------------------------
        # Settings tabs
        # --------------------------------------------------------------------------------------------------------------
        self.reset_all_fields_to_default()
        self.pixel_adjustment_det_1_push_button.clicked.connect(self._on_load_pixel_adjustment_det_1)
        self.pixel_adjustment_det_2_push_button.clicked.connect(self._on_load_pixel_adjustment_det_2)
        self.wavelength_adjustment_det_1_push_button.clicked.connect(self._on_load_wavelength_adjustment_det_1)
        self.wavelength_adjustment_det_2_push_button.clicked.connect(self._on_load_wavelength_adjustment_det_2)

        # Set the merge settings
        self.reduction_mode_combo_box.currentIndexChanged.connect(self._on_reduction_mode_selection_has_changed)
        self._on_reduction_mode_selection_has_changed()  # Disable the merge settings initially

        # Mask file input settings
        self.mask_file_browse_push_button.clicked.connect(self._on_load_mask_file)
        self.mask_file_add_push_button.clicked.connect(self._on_mask_file_add)

        self.manage_directories_button.clicked.connect(self._on_manage_directories)

        # Set the q step type settings
        self.q_1d_step_type_combo_box.currentIndexChanged.connect(self._on_q_1d_step_type_has_changed)
        self._on_q_1d_step_type_has_changed()

        # Set the q resolution aperture shape settings
        self.q_resolution_shape_combo_box.currentIndexChanged.connect(self._on_q_resolution_shape_has_changed)
        self.q_resolution_group_box.toggled.connect(self._on_q_resolution_shape_has_changed)
        self._on_q_resolution_shape_has_changed()

        # Set the transmission fit selection
        self.fit_selection_combo_box.currentIndexChanged.connect(self._on_fit_selection_has_changed)
        self._on_fit_selection_has_changed()

        # Set the transmission polynomial order
        self.fit_sample_fit_type_combo_box.currentIndexChanged.connect(self._on_transmission_fit_type_has_changed)
        self.fit_can_fit_type_combo_box.currentIndexChanged.connect(self._on_transmission_fit_type_has_changed)
        self._on_transmission_fit_type_has_changed()

        # Set the transmission target
        self.transmission_target_combo_box.currentIndexChanged.connect(self._on_transmission_target_has_changed)
        self._on_transmission_target_has_changed()

        # Roi and Mask files
        self.transmission_roi_files_push_button.clicked.connect(self._on_load_transmission_roi_files)
        self.transmission_mask_files_push_button.clicked.connect(self._on_load_transmission_mask_files)

        # Q Resolution
        self.q_resolution_moderator_file_push_button.clicked.connect(self._on_load_moderator_file)

        self.wavelength_stacked_widget.setCurrentIndex(0)
        self.hide_geometry()
        self.hide_background_subtraction()

        # Hide broken functionality: https://github.com/mantidproject/mantid/issues/37836
        self.event_slice_optimisation_checkbox.setChecked(False)
        self.event_slice_optimisation_checkbox.setHidden(True)
        self.event_slice_optimisation_label.setHidden(True)

        return True

    def _on_wavelength_step_type_changed(self):
        if self.wavelength_step_type == RangeStepType.RANGE_LIN:
            self.wavelength_stacked_widget.setCurrentIndex(1)
            self.wavelength_step_label.setText("Step [\u00c5]")
        elif self.wavelength_step_type == RangeStepType.RANGE_LOG:
            self.wavelength_stacked_widget.setCurrentIndex(1)
            self.wavelength_step_label.setText("Step [d\u03bb/\u03bb]")
        elif self.wavelength_step_type == RangeStepType.LOG:
            self.wavelength_stacked_widget.setCurrentIndex(0)
            self.wavelength_step_label.setText("Step [d\u03bb/\u03bb]")
        elif self.wavelength_step_type == RangeStepType.LIN:
            self.wavelength_stacked_widget.setCurrentIndex(0)
            self.wavelength_step_label.setText("Step [\u00c5]")

    def create_data_table(self):
        # Delete an already existing table
        if self.data_processor_table:
            self.data_processor_table.setParent(None)

        self._column_index_map = {col_name: i for i, col_name in enumerate(self.COLUMN_NAMES)}
        self.data_processor_table = jobtreeview.JobTreeView(self.COLUMN_NAMES, self.cell(""), self)
        self.hide_period_columns()

        # Default QTreeView size is too small
        font = self.data_processor_table.font()
        font.setPointSize(13)
        self.data_processor_table.setFont(font)

        self.data_processor_table.setRootIsDecorated(False)

        self.add_empty_row()

        self.table_signals = jobtreeview.JobTreeViewSignalAdapter(self.data_processor_table, self)
        # The signal adapter subscribes to events from the table
        # and emits signals whenever it is notified.

        self.data_processor_widget_layout.addWidget(self.data_processor_table)
        self.table_signals.cellTextChanged.connect(self._data_changed)
        self.table_signals.rowInserted.connect(self._row_inserted)
        self.table_signals.appendAndEditAtChildRowRequested.connect(self._append_and_edit_at_child_row_requested)
        self.table_signals.appendAndEditAtRowBelowRequested.connect(self._append_and_edit_at_row_below_requested)
        self.table_signals.editAtRowAboveRequested.connect(self._edit_at_row_above_requested)
        self.table_signals.removeRowsRequested.connect(self._remove_rows_requested)
        self.table_signals.copyRowsRequested.connect(self._copy_rows_requested)
        self.table_signals.pasteRowsRequested.connect(self._paste_rows_requested)

    def cell(self, text):
        # Cell will only accept strings
        text = str(text) if text else ""

        background_color = "white"
        border_thickness = 1
        border_color = "black"
        border_opacity = 255
        is_editable = True
        return jobtreeview.Cell(text, background_color, border_thickness, border_color, border_opacity, is_editable)

    def row(self, path):
        return jobtreeview.RowLocation(path)

    def _setup_main_tab(self):
        self.user_file_button.clicked.connect(self._on_user_file_load)
        self.batch_button.clicked.connect(self._on_batch_file_load)

        # Disable the line edit fields. The user should not edit the paths manually.
        # They have to use the button.
        self.user_file_line_edit.setDisabled(True)
        self.batch_line_edit.setDisabled(True)

    def _process_selected_clicked(self):
        """
        Process runs
        """
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Process Selected"], False)
        self._call_settings_listeners(lambda listener: listener.on_process_selected_clicked())

    def _process_all_clicked(self):
        """
        Process All button clicked
        """
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Process All"], False)
        self._call_settings_listeners(lambda listener: listener.on_process_all_clicked())

    def _load_clicked(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Load"], False)
        self._call_settings_listeners(lambda listener: listener.on_load_clicked())

    def _export_table_clicked(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Export Table"], False)
        self._call_settings_listeners(lambda listener: listener.on_export_table_clicked())

    def _processing_finished(self):
        """
        Clean up
        """
        self._call_settings_listeners(lambda listener: listener.on_processing_finished())

    def _data_changed(self, row_location, column, old_value, new_value):
        row = row_location.rowRelativeToParent()
        row_obj = self._convert_to_row_state(self.get_row(row_location))

        self._call_settings_listeners(lambda listener: listener.on_data_changed(row, row_obj))

    def _row_inserted(self, row_location):
        if row_location.depth() > 1:
            self.data_processor_table.removeRowAt(row_location)
        self._call_settings_listeners(lambda listener: listener.on_insert_row())

    def _append_and_edit_at_child_row_requested(self):
        self.data_processor_table.appendAndEditAtChildRow()

    def _append_and_edit_at_row_below_requested(self):
        self.data_processor_table.appendAndEditAtRowBelow()

    def _edit_at_row_above_requested(self):
        self.data_processor_table.editAtRowAbove()

    def _remove_rows_requested(self, rows):
        rows = [item.rowRelativeToParent() for item in rows]
        self._call_settings_listeners(lambda listener: listener.on_rows_removed(rows))

    def _remove_rows_requested_from_button(self):
        rows = self.get_selected_rows()
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Rows removed button"], False)
        self._call_settings_listeners(lambda listener: listener.on_rows_removed(rows))

    def _copy_rows_requested(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Copy rows button"], False)
        self._call_settings_listeners(lambda listener: listener.on_copy_rows_requested())

    def _erase_rows(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Erase rows button"], False)
        self._call_settings_listeners(lambda listener: listener.on_erase_rows())

    def _cut_rows(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Cut rows button"], False)
        self._call_settings_listeners(lambda listener: listener.on_cut_rows())

    def _paste_rows_requested(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Paste rows button"], False)
        self._call_settings_listeners(lambda listener: listener.on_paste_rows_requested())

    def _instrument_changed(self):
        self._call_settings_listeners(lambda listener: listener.on_instrument_changed())

    def _on_insert_button_pressed(self):
        self._call_settings_listeners(lambda listener: listener.on_insert_row())

    def _on_save_other_button_pressed(self):
        self._call_settings_listeners(lambda listener: listener.on_save_other())

    def _on_field_edit(self):
        sender = self.sender()
        if isinstance(sender, QLineEdit) and not sender.isModified():
            # We only want editingFinished to fire if a user has modified. This
            # prevents a warning appearing after the first one - commonly when
            # clicking another line edit with invalid input or attempting to close the interface
            return

        self._call_settings_listeners(lambda listener: listener.on_field_edit())

    @staticmethod
    def _on_help_button_clicked():
        InterfaceManager().showHelpPage("qthelp://org.sphinx.mantidproject/doc/" "interfaces/isis_sans/ISIS%20SANS.html")

    def _on_output_mode_clicked(self):
        """This method is called when an output mode is clicked on the gui"""
        if self.output_mode_memory_radio_button.isChecked():
            output_mode = "PublishToADS"
        elif self.output_mode_file_radio_button.isChecked():
            output_mode = "SaveToFile"
        elif self.output_mode_both_radio_button.isChecked():
            output_mode = "Both"
        else:
            output_mode = None
        set_setting(self.__generic_settings, self.__output_mode_key, output_mode)

        # Notify the presenter
        self._call_settings_listeners(lambda listener: listener.on_output_mode_changed())

    def _on_save_can_clicked(self, value):
        self.save_can_checkBox.setChecked(value)
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Save Can Toggled"], False)
        set_setting(self.__generic_settings, self.__save_can_key, value)

    def _on_user_file_load(self):
        """
        Load the user file
        """
        # Load the user file
        load_file(
            self.user_file_line_edit,
            "TOML Files (*.toml *.Toml *.TOML);; Text Files (*.txt *.Txt *.TXT)",
            self.__generic_settings,
            self.__path_key,
            self.get_user_file_path,
        )

        # Set full user file path for default loading
        self.gui_properties_handler.set_setting("user_file", self.get_user_file_path())

        # Notify presenters
        self._call_settings_listeners(lambda listener: listener.on_user_file_load())

    def on_user_file_load_failure(self):
        try:
            self.gui_properties_handler.set_setting("user_file", "")
        except AttributeError:
            pass
        self.user_file_line_edit.setText("")

    def set_out_default_user_file(self):
        """
        Load a default user file, called on view set-up
        """
        if self.get_user_file_path() != "":
            self._call_settings_listeners(lambda listener: listener.on_user_file_load())

    def set_out_default_output_mode(self):
        try:
            default_output_mode = OutputMode(load_property(self.__generic_settings, self.__output_mode_key))
        except ValueError:
            pass
        else:
            self._check_output_mode(default_output_mode)

    def _check_output_mode(self, value):
        """
        Set the output mode radio button from a SANS enum.
        This method is called when:
        1. The gui is launched
        2. Via the presenter, from state
        :param value: An OutputMode (SANS enum) object
        """
        if value is OutputMode.PUBLISH_TO_ADS:
            self.output_mode_memory_radio_button.setChecked(True)
        elif value is OutputMode.SAVE_TO_FILE:
            self.output_mode_file_radio_button.setChecked(True)
        elif value is OutputMode.BOTH:
            self.output_mode_both_radio_button.setChecked(True)

        # Notify the presenter
        self._call_settings_listeners(lambda listener: listener.on_output_mode_changed())

    def set_out_default_save_can(self):
        try:
            default_save_can = load_property(self.__generic_settings, self.__save_can_key, type=bool)
        except RuntimeError:
            pass
        else:
            self._on_save_can_clicked(default_save_can)

    def _on_batch_file_load(self):
        """
        Load the batch file
        """
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Loaded Batch File"], False)
        load_file(self.batch_line_edit, "CSV Files(*.csv)", self.__generic_settings, self.__batch_file_key, self.get_batch_file_path)
        self._call_settings_listeners(lambda listener: listener.on_batch_file_load())

    def disable_buttons(self):
        self.process_selected_button.setEnabled(False)
        self.process_all_button.setEnabled(False)
        self.batch_button.setEnabled(False)
        self.user_file_button.setEnabled(False)
        self.manage_directories_button.setEnabled(False)
        self.load_button.setEnabled(False)
        self.export_table_button.setEnabled(False)

    def set_monitor_5_enabled(self, new_state):
        self._has_monitor_5 = new_state
        self.transmission_mn_5_shift_label.setEnabled(new_state)
        self.transmission_mn_5_shift_line_edit.setEnabled(new_state)

    def enable_buttons(self):
        self.process_selected_button.setEnabled(True)
        self.process_all_button.setEnabled(True)
        self.batch_button.setEnabled(True)
        self.user_file_button.setEnabled(True)
        self.manage_directories_button.setEnabled(True)
        self.load_button.setEnabled(True)
        self.export_table_button.setEnabled(True)

    def disable_file_type_buttons(self):
        self.can_sas_checkbox.setEnabled(False)
        self.nx_can_sas_checkbox.setEnabled(False)
        self.rkh_checkbox.setEnabled(False)

    def enable_file_type_buttons(self):
        self.can_sas_checkbox.setEnabled(True)
        self.nx_can_sas_checkbox.setEnabled(True)
        self.rkh_checkbox.setEnabled(True)

    def disable_can_sas_1D_button(self):
        self.can_sas_checkbox.setEnabled(False)

    def enable_can_sas_1D_button(self):
        self.can_sas_checkbox.setEnabled(True)

    def disable_process_buttons(self):
        self.process_selected_button.setEnabled(False)
        self.process_all_button.setEnabled(False)
        self.load_button.setEnabled(False)

    def enable_process_buttons(self):
        self.process_selected_button.setEnabled(True)
        self.process_all_button.setEnabled(True)
        self.load_button.setEnabled(True)

    def display_message_box(self, title, message, details):
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Warning)

        message_length = len(message)

        # This is to ensure that the QMessage box if wide enough to display nicely.
        msg.setText(10 * " " + message + " " * (30 - message_length))
        msg.setWindowTitle(title)
        msg.setDetailedText(details)
        msg.setStandardButtons(QMessageBox.Ok)
        msg.setDefaultButton(QMessageBox.Ok)
        msg.setEscapeButton(QMessageBox.Ok)
        msg.exec_()

    def display_save_file_box(self, title, default_path, file_filter):
        filename = QFileDialog.getSaveFileName(self, title, default_path, filter=file_filter)
        return filename

    def get_user_file_path(self):
        return str(self.user_file_line_edit.text())

    def get_batch_file_path(self):
        return str(self.batch_line_edit.text())

    def set_out_file_directory(self, out_file_directory):
        self.output_directory_location.setText("{}".format(out_file_directory))

    def _on_load_pixel_adjustment_det_1(self):
        load_file(self.pixel_adjustment_det_1_line_edit, "*.*", self.__generic_settings, self.__path_key, self.get_pixel_adjustment_det_1)

    def get_pixel_adjustment_det_1(self):
        return str(self.pixel_adjustment_det_1_line_edit.text())

    def _on_load_pixel_adjustment_det_2(self):
        load_file(self.pixel_adjustment_det_2_line_edit, "*.*", self.__generic_settings, self.__path_key, self.get_pixel_adjustment_det_2)

    def get_pixel_adjustment_det_2(self):
        return str(self.pixel_adjustment_det_2_line_edit.text())

    def _on_load_wavelength_adjustment_det_1(self):
        load_file(
            self.wavelength_adjustment_det_1_line_edit,
            "*.*",
            self.__generic_settings,
            self.__path_key,
            self.get_wavelength_adjustment_det_1,
        )

    def get_wavelength_adjustment_det_1(self):
        return str(self.wavelength_adjustment_det_1_line_edit.text())

    def _on_load_wavelength_adjustment_det_2(self):
        load_file(
            self.wavelength_adjustment_det_2_line_edit,
            "*.*",
            self.__generic_settings,
            self.__path_key,
            self.get_wavelength_adjustment_det_2,
        )

    def get_wavelength_adjustment_det_2(self):
        return str(self.wavelength_adjustment_det_2_line_edit.text())

    def _on_reduction_mode_selection_has_changed(self):
        selection = self.reduction_mode_combo_box.currentText()
        try:
            is_merged = ReductionMode(selection) is ReductionMode.MERGED
        except ValueError:
            is_merged = False

        self.merged_settings.setEnabled(is_merged)
        self._call_settings_listeners(lambda listener: listener.on_reduction_mode_selection_has_changed(selection))

    def _on_q_resolution_shape_has_changed(self):
        shape_selection = self.q_resolution_shape_combo_box.currentIndex()
        use_q_resolution = self.q_resolution_group_box.isChecked()
        has_circular_aperture_been_selected = shape_selection == 0

        enable_circular = has_circular_aperture_been_selected and use_q_resolution
        enable_rectangular = not has_circular_aperture_been_selected and use_q_resolution

        self.q_resolution_source_a_line_edit.setEnabled(enable_circular)
        self.q_resolution_sample_a_line_edit.setEnabled(enable_circular)
        self.q_resolution_source_a_label.setEnabled(enable_circular)
        self.q_resolution_sample_a_label.setEnabled(enable_circular)

        self.q_resolution_source_h_line_edit.setEnabled(enable_rectangular)
        self.q_resolution_sample_h_line_edit.setEnabled(enable_rectangular)
        self.q_resolution_source_w_line_edit.setEnabled(enable_rectangular)
        self.q_resolution_sample_w_line_edit.setEnabled(enable_rectangular)
        self.q_resolution_source_h_label.setEnabled(enable_rectangular)
        self.q_resolution_sample_h_label.setEnabled(enable_rectangular)
        self.q_resolution_source_w_label.setEnabled(enable_rectangular)
        self.q_resolution_sample_w_label.setEnabled(enable_rectangular)

    def _on_q_1d_step_type_has_changed(self):
        selection = self.q_1d_step_type_combo_box.currentText()
        is_variable = selection == self.VARIABLE
        self.q_1d_max_line_edit.setEnabled(not is_variable)
        self.q_1d_step_line_edit.setEnabled(not is_variable)
        if is_variable:
            comma_separated_floats_regex_string = r"^(\s*[-+]?[0-9]*\.?[0-9]*)(\s*,\s*[-+]?[0-9]*\.?[0-9]*)+\s*$"
            reg_ex = QRegExp(comma_separated_floats_regex_string)
            validator = QRegExpValidator(reg_ex)
            self.q_1d_min_line_edit.setValidator(validator)

            self.q_min_label.setText("Rebin String")
            self.q_step_label.setText("Step [\u00c5^-1]")
        else:
            # If rebin string data
            data_q_min = str(self.q_1d_min_line_edit.text())
            if "," in data_q_min:
                self.q_1d_min_line_edit.setText("")
            validator = QDoubleValidator()
            validator.setBottom(0.0)
            self.q_1d_min_line_edit.setValidator(validator)

            label = "Min [\u00c5^-1]"
            self.q_min_label.setText(label)

            step_label = "dQ/Q" if "Log" in selection else "Step [\u00c5^-1]"
            self.q_step_label.setText(step_label)

    def set_q_resolution_shape_to_rectangular(self, is_rectangular):
        index = 1 if is_rectangular else 0
        self.q_resolution_shape_combo_box.setCurrentIndex(index)

    def _on_fit_selection_has_changed(self):
        fit_selection = self.fit_selection_combo_box.currentIndex()
        use_settings_for_sample_and_can = fit_selection == 0
        # If we use the same settings for the sample and the can, then we don't need the inputs for the can, hence
        # we can hide them, else we need to make sure they are shown.
        if use_settings_for_sample_and_can:
            self.fit_sample_label.setText("Sample and Can")
        else:
            self.fit_sample_label.setText("Sample               ")

        show_can = not use_settings_for_sample_and_can
        self.fit_can_label.setVisible(show_can)
        self.fit_can_use_fit_check_box.setVisible(show_can)
        self.fit_can_fit_type_combo_box.setVisible(show_can)
        self.fit_can_wavelength_combo_box.setVisible(show_can)
        self.fit_can_polynomial_order_spin_box.setVisible(show_can)

    def set_fit_selection(self, use_separate):
        index = 1 if use_separate else 0
        self.fit_selection_combo_box.setCurrentIndex(index)

    def use_same_transmission_fit_setting_for_sample_and_can(self):
        return self.fit_selection_combo_box.currentIndex() == 0

    def _on_transmission_fit_type_has_changed(self):
        # Check the sample settings
        fit_type_sample_as_string = self.fit_sample_fit_type_combo_box.currentText()
        fit_type_sample = FitType(fit_type_sample_as_string)
        is_sample_polynomial = fit_type_sample is FitType.POLYNOMIAL
        self.fit_sample_polynomial_order_spin_box.setEnabled(is_sample_polynomial)

        # Check the can settings
        fit_type_can_as_string = self.fit_can_fit_type_combo_box.currentText()
        fit_type_can = FitType(fit_type_can_as_string)
        is_can_polynomial = fit_type_can is FitType.POLYNOMIAL
        self.fit_can_polynomial_order_spin_box.setEnabled(is_can_polynomial)

    def _on_transmission_target_has_changed(self):
        use_monitor = self.transmission_target_combo_box.currentIndex() == 0
        use_roi = not use_monitor

        self.transmission_monitor_label.setEnabled(use_monitor)
        self.transmission_monitor_line_edit.setEnabled(use_monitor)
        self.transmission_mn_4_shift_label.setEnabled(use_monitor)
        self.transmission_mn_4_shift_line_edit.setEnabled(use_monitor)

        if self._has_monitor_5:
            self.transmission_mn_5_shift_label.setEnabled(use_monitor)
            self.transmission_mn_5_shift_line_edit.setEnabled(use_monitor)

        self.transmission_radius_label.setEnabled(use_roi)
        self.transmission_radius_line_edit.setEnabled(use_roi)
        self.transmission_roi_files_label.setEnabled(use_roi)
        self.transmission_roi_files_line_edit.setEnabled(use_roi)
        self.transmission_roi_files_push_button.setEnabled(use_roi)
        self.transmission_mask_files_label.setEnabled(use_roi)
        self.transmission_mask_files_line_edit.setEnabled(use_roi)
        self.transmission_mask_files_push_button.setEnabled(use_roi)

    def get_transmission_roi_files(self):
        return str(self.transmission_roi_files_line_edit.text())

    def _on_load_transmission_roi_files(self):
        load_file(self.transmission_roi_files_line_edit, "*.*", self.__generic_settings, self.__path_key, self.get_transmission_roi_files)

    def get_transmission_mask_files(self):
        return str(self.transmission_mask_files_line_edit.text())

    def _on_load_transmission_mask_files(self):
        load_file(self.transmission_mask_files_line_edit, "*.*", self.__generic_settings, self.__path_key, self.get_transmission_mask_files)

    def get_moderator_file(self):
        return str(self.q_resolution_moderator_file_line_edit.text())

    def _on_load_moderator_file(self):
        load_file(self.q_resolution_moderator_file_line_edit, "*.*", self.__generic_settings, self.__path_key, self.get_moderator_file)

    def get_mask_file(self):
        return str(self.mask_file_input_line_edit.text())

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories.openManageUserDirectories()

    def _on_load_mask_file(self):
        load_file(self.mask_file_input_line_edit, "*.*", self.__generic_settings, self.__mask_file_input_path_key, self.get_mask_file)

    def _on_mask_file_add(self):
        self._call_settings_listeners(lambda listener: listener.on_mask_file_add())

    def _on_multi_period_selection(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Multiple Period Toggled"], False)
        self._call_settings_listeners(lambda listener: listener.on_multi_period_selection(self.is_multi_period_view()))

    def _on_sample_geometry_selection(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Sample Geometry Toggled"], False)
        self._call_settings_listeners(lambda listener: listener.on_sample_geometry_selection(self.is_sample_geometry()))

    def _on_background_subtraction_selection(self):
        UsageService.registerFeatureUsage(FeatureType.Feature, ["ISIS SANS", "Background Subtraction Toggled"], False)
        self._call_settings_listeners(lambda listener: listener.on_background_subtraction_selection(self.is_background_subtraction()))

    def _on_manage_directories(self):
        self._call_settings_listeners(lambda listener: listener.on_manage_directories())

    # ------------------------------------------------------------------------------------------------------------------
    # Elements which can be set and read by the model
    # ------------------------------------------------------------------------------------------------------------------
    def set_instrument_settings(self, instrument):
        if instrument:
            reduction_mode_list = get_reduction_mode_strings_for_gui(instrument)
            self.set_reduction_modes(reduction_mode_list)

    def update_gui_combo_box(self, value, expected_type, combo_box):
        # There are three types of values that can be passed:
        # Lists: we set the combo box to the values in the list
        # expected_type: we set the expected type
        # str (in the case of "Variable" Q rebin): We set the combo box to the text if it is an option
        gui_element = getattr(self, combo_box)
        if isinstance(value, list):
            gui_element.clear()
            for element in value:
                self._add_list_element_to_combo_box(gui_element=gui_element, element=element)
        elif isinstance(value, Enum):
            self._set_enum_as_element_in_combo_box(gui_element=gui_element, element=value)
        elif isinstance(value, str):
            index = gui_element.findText(value)
            if index != -1:
                gui_element.setCurrentIndex(index)
        else:
            raise RuntimeError("Expected an input of type {}, but got {}".format(expected_type, type(value)))

    def _add_list_element_to_combo_box(self, gui_element, element):
        if isinstance(element, Enum):
            self._add_enum_as_element_in_combo_box(gui_element=gui_element, element=element)
        else:
            gui_element.addItem(element)

    @staticmethod
    def _set_enum_as_element_in_combo_box(gui_element, element):
        value_as_string = element.value
        index = gui_element.findText(value_as_string)
        if index != -1:
            gui_element.setCurrentIndex(index)

    def _add_enum_as_element_in_combo_box(self, gui_element, element):
        value_as_string = element.value
        gui_element.addItem(value_as_string)

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

    def update_simple_line_edit_field(self, line_edit, value):
        gui_element = getattr(self, line_edit)
        gui_element.setText(str(value))

    def is_multi_period_view(self):
        return self.multi_period_check_box.isChecked()

    def set_multi_period_view_mode(self, mode):
        self.multi_period_check_box.setChecked(mode)

    def is_sample_geometry(self):
        return self.sample_geometry_checkbox.isChecked()

    def set_sample_geometry_mode(self, mode):
        self.sample_geometry_checkbox.setChecked(mode)

    def is_background_subtraction(self):
        return self.background_subtraction_checkbox.isChecked()

    def set_background_subtraction_mode(self, mode):
        self.background_subtraction_checkbox.setChecked(mode)

    def set_plot_results_checkbox_visibility(self, visibility):
        self.plot_results_checkbox.setVisible(visibility)

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

    @property
    def user_file(self):
        return self.user_file_line_edit.text()

    @user_file.setter
    def user_file(self, val):
        self.user_file_line_edit.setText(val)

    @property
    def batch_file(self):
        return self.batch_line_edit.text()

    @batch_file.setter
    def batch_file(self, val):
        self.batch_line_edit.setText(val)

    # -----------------------------------------------------------------
    # Save Options
    # -----------------------------------------------------------------
    @property
    def save_types(self) -> SaveOptions:
        selected_options = SaveOptions()
        selected_options.can_sas_1d = self.can_sas_checkbox.isChecked()
        selected_options.nxs_can_sas = self.nx_can_sas_checkbox.isChecked()
        selected_options.rkh = self.rkh_checkbox.isChecked()
        return selected_options

    @save_types.setter
    def save_types(self, values: SaveOptions):
        self.can_sas_checkbox.setChecked(values.can_sas_1d)
        self.nx_can_sas_checkbox.setChecked(values.nxs_can_sas)
        self.rkh_checkbox.setChecked(values.rkh)

    @property
    def zero_error_free(self):
        return self.save_zero_error_free.isChecked()

    @zero_error_free.setter
    def zero_error_free(self, value):
        self.save_zero_error_free.setChecked(value)

    @property
    def save_can(self):
        return self.save_can_checkBox.isChecked()

    @save_can.setter
    def save_can(self, value):
        self._on_save_can_clicked(value)

    @property
    def progress_bar_minimum(self):
        return self.batch_progress_bar.minimum()

    @progress_bar_minimum.setter
    def progress_bar_minimum(self, value):
        self.batch_progress_bar.setMinimum(value)

    @property
    def progress_bar_maximum(self):
        return self.batch_progress_bar.maximum()

    @progress_bar_maximum.setter
    def progress_bar_maximum(self, value):
        self.batch_progress_bar.setMaximum(value)

    @property
    def progress_bar_value(self):
        return self.batch_progress_bar.value()

    @progress_bar_value.setter
    def progress_bar_value(self, progress):
        self.batch_progress_bar.setValue(progress)

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
    def plot_results(self):
        return self.plot_results_checkbox.isChecked()

    @plot_results.setter
    def plot_results(self, value):
        self.plot_results_checkbox.setChecked(value)

    @property
    def output_mode(self):
        if self.output_mode_memory_radio_button.isChecked():
            return OutputMode.PUBLISH_TO_ADS
        elif self.output_mode_file_radio_button.isChecked():
            return OutputMode.SAVE_TO_FILE
        elif self.output_mode_both_radio_button.isChecked():
            return OutputMode.BOTH
        else:
            self.gui_logger.warning("The output format was not specified. Defaulting to saving to memory only.")
            return OutputMode.PUBLISH_TO_ADS

    @output_mode.setter
    def output_mode(self, value):
        self._check_output_mode(value)
        try:
            set_setting(self.__generic_settings, self.__output_mode_key, value.value)
        except RuntimeError:
            pass

    @property
    def compatibility_mode(self):
        return self.event_binning_group_box.isChecked()

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self.event_binning_group_box.setChecked(value)

    @property
    def event_slice_optimisation(self):
        return self.event_slice_optimisation_checkbox.isChecked()

    @event_slice_optimisation.setter
    def event_slice_optimisation(self, value):
        # Functionality is broken, so the checkbox for this setter is hidden. See here for more information:
        # https://github.com/mantidproject/mantid/issues/37836
        pass

    @property
    def instrument(self):
        return get_instrument_from_gui_selection(self.instrument_type.text())

    @instrument.setter
    def instrument(self, value):
        assert isinstance(value, Enum), "Expected InstrumentEnum, got %r" % value
        instrument_string = value.value
        self.instrument_type.setText("{}".format(instrument_string))
        self._instrument_changed()

    # ==================================================================================================================
    # ==================================================================================================================
    # General TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # General group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def reduction_dimensionality(self) -> ReductionDimensionality:
        return ReductionDimensionality.ONE_DIM if self.reduction_dimensionality_1D.isChecked() else ReductionDimensionality.TWO_DIM

    @reduction_dimensionality.setter
    def reduction_dimensionality(self, value: ReductionDimensionality):
        is_1d = value is ReductionDimensionality.ONE_DIM
        self.reduction_dimensionality_1D.setChecked(is_1d)
        self.reduction_dimensionality_2D.setChecked(not is_1d)

    @property
    def reduction_mode(self):
        reduction_mode_as_string = self.reduction_mode_combo_box.currentText()
        return get_reduction_mode_from_gui_selection(reduction_mode_as_string)

    @reduction_mode.setter
    def reduction_mode(self, value):
        # There are two types of values that can be passed:
        # String: we look for string and we set it
        # Convert the value to the correct GUI string

        # Set the correct selection of reduction modes which are available
        reduction_mode_list = get_reduction_mode_strings_for_gui(self.instrument)
        self.set_reduction_modes(reduction_mode_list)

        reduction_mode_as_string = get_string_for_gui_from_reduction_mode(value, self.instrument)
        if reduction_mode_as_string:
            index = self.reduction_mode_combo_box.findText(reduction_mode_as_string)
            if index != -1:
                self.reduction_mode_combo_box.setCurrentIndex(index)

    def set_reduction_modes(self, reduction_mode_list):
        current_index = self.reduction_mode_combo_box.currentIndex()
        self.reduction_mode_combo_box.clear()
        for element in reduction_mode_list:
            self.reduction_mode_combo_box.addItem(element)
        if current_index != -1:
            self.reduction_mode_combo_box.setCurrentIndex(current_index)

    @property
    def merge_scale(self):
        return self.get_simple_line_edit_field(line_edit="merged_scale_line_edit", expected_type=float)

    @merge_scale.setter
    def merge_scale(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_scale_line_edit", value=value)

    @property
    def merge_shift(self):
        return self.get_simple_line_edit_field(line_edit="merged_shift_line_edit", expected_type=float)

    @merge_shift.setter
    def merge_shift(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_shift_line_edit", value=value)

    @property
    def merge_scale_fit(self):
        return self.merged_scale_use_fit_check_box.isChecked()

    @merge_scale_fit.setter
    def merge_scale_fit(self, value):
        self.merged_scale_use_fit_check_box.setChecked(value)

    @property
    def merge_shift_fit(self):
        return self.merged_shift_use_fit_check_box.isChecked()

    @merge_shift_fit.setter
    def merge_shift_fit(self, value):
        self.merged_shift_use_fit_check_box.setChecked(value)

    @property
    def merge_q_range_start(self):
        return self.get_simple_line_edit_field(line_edit="merged_q_range_start_line_edit", expected_type=float)

    @merge_q_range_start.setter
    def merge_q_range_start(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_q_range_start_line_edit", value=value)

    @property
    def merge_q_range_stop(self):
        return self.get_simple_line_edit_field(line_edit="merged_q_range_stop_line_edit", expected_type=float)

    @merge_q_range_stop.setter
    def merge_q_range_stop(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_q_range_stop_line_edit", value=value)

    @property
    def merge_mask(self):
        return self.merge_mask_check_box.isChecked()

    @merge_mask.setter
    def merge_mask(self, value):
        self.merge_mask_check_box.setChecked(value)

    @property
    def merge_max(self):
        return self.get_simple_line_edit_field(line_edit="merged_max_line_edit", expected_type=float)

    @merge_max.setter
    def merge_max(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_max_line_edit", value=value)

    @property
    def merge_min(self):
        return self.get_simple_line_edit_field(line_edit="merged_min_line_edit", expected_type=float)

    @merge_min.setter
    def merge_min(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_min_line_edit", value=value)

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
    # Event slices group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def event_binning(self):
        return str(self.event_binning_line_edit.text())

    @event_binning.setter
    def event_binning(self, value):
        self.event_binning_line_edit.setText(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Wavelength Group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def wavelength_step_type(self):
        step_type_as_string = self.wavelength_step_type_combo_box.currentText()
        return RangeStepType(step_type_as_string)

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

    @property
    def wavelength_range(self):
        return str(self.wavelength_slices_line_edit.text())

    @wavelength_range.setter
    def wavelength_range(self, value):
        self.wavelength_slices_line_edit.setText(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale Group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def absolute_scale(self):
        return self.get_simple_line_edit_field(line_edit="absolute_scale_line_edit", expected_type=float)

    @absolute_scale.setter
    def absolute_scale(self, value):
        self.update_simple_line_edit_field(line_edit="absolute_scale_line_edit", value=value)

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
    def transmission_incident_monitor(self):
        return self.get_simple_line_edit_field(line_edit="transmission_line_edit", expected_type=int)

    @transmission_incident_monitor.setter
    def transmission_incident_monitor(self, value):
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
        return self.get_simple_line_edit_field(line_edit="transmission_monitor_line_edit", expected_type=int)

    @transmission_monitor.setter
    def transmission_monitor(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_monitor_line_edit", value=value)

    @property
    def transmission_mn_4_shift(self):
        return self.get_simple_line_edit_field(line_edit="transmission_mn_4_shift_line_edit", expected_type=float)

    @transmission_mn_4_shift.setter
    def transmission_mn_4_shift(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_mn_4_shift_line_edit", value=value)

    @property
    def transmission_mn_5_shift(self):
        return self.get_simple_line_edit_field(line_edit="transmission_mn_5_shift_line_edit", expected_type=float)

    @transmission_mn_5_shift.setter
    def transmission_mn_5_shift(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_mn_5_shift_line_edit", value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Transmission fit
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def transmission_sample_use_fit(self):
        return self.fit_sample_use_fit_check_box.isChecked()

    @transmission_sample_use_fit.setter
    def transmission_sample_use_fit(self, value):
        self.fit_sample_use_fit_check_box.setChecked(value)

    @property
    def transmission_can_use_fit(self):
        return self.fit_can_use_fit_check_box.isChecked()

    @transmission_can_use_fit.setter
    def transmission_can_use_fit(self, value):
        self.fit_can_use_fit_check_box.setChecked(value)

    @property
    def transmission_sample_fit_type(self):
        fit_type_as_string = self.fit_sample_fit_type_combo_box.currentText()
        try:
            return FitType(fit_type_as_string)
        except (RuntimeError, ValueError):
            return

    @transmission_sample_fit_type.setter
    def transmission_sample_fit_type(self, value):
        if value is None:
            self.fit_sample_fit_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=FitType, combo_box="fit_sample_fit_type_combo_box")

    @property
    def transmission_can_fit_type(self):
        fit_type_as_string = self.fit_can_fit_type_combo_box.currentText()
        try:
            return FitType(fit_type_as_string)
        except (RuntimeError, ValueError):
            return None

    @transmission_can_fit_type.setter
    def transmission_can_fit_type(self, value):
        if value is None:
            self.fit_sample_fit_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=FitType, combo_box="fit_can_fit_type_combo_box")

    @staticmethod
    def _set_polynomial_order(spin_box, value):
        minimum = spin_box.minimum()
        maximum = spin_box.maximum()
        if value < minimum or value > maximum:
            raise ValueError("The value for the polynomial order {} has " "to be in the range of {} and {}".format(value, minimum, maximum))
        spin_box.setValue(value)

    @property
    def transmission_sample_polynomial_order(self):
        return self.fit_sample_polynomial_order_spin_box.value()

    @transmission_sample_polynomial_order.setter
    def transmission_sample_polynomial_order(self, value):
        self._set_polynomial_order(spin_box=self.fit_sample_polynomial_order_spin_box, value=value)

    @property
    def transmission_can_polynomial_order(self):
        return self.fit_can_polynomial_order_spin_box.value()

    @transmission_can_polynomial_order.setter
    def transmission_can_polynomial_order(self, value):
        self._set_polynomial_order(spin_box=self.fit_can_polynomial_order_spin_box, value=value)

    @property
    def transmission_sample_wavelength_min(self):
        return self.get_simple_line_edit_field(line_edit="fit_sample_wavelength_min_line_edit", expected_type=float)

    @transmission_sample_wavelength_min.setter
    def transmission_sample_wavelength_min(self, value):
        self.update_simple_line_edit_field(line_edit="fit_sample_wavelength_min_line_edit", value=value)

    @property
    def transmission_sample_wavelength_max(self):
        return self.get_simple_line_edit_field(line_edit="fit_sample_wavelength_max_line_edit", expected_type=float)

    @transmission_sample_wavelength_max.setter
    def transmission_sample_wavelength_max(self, value):
        self.update_simple_line_edit_field(line_edit="fit_sample_wavelength_max_line_edit", value=value)

    @property
    def transmission_can_wavelength_min(self):
        return self.get_simple_line_edit_field(line_edit="fit_can_wavelength_min_line_edit", expected_type=float)

    @transmission_can_wavelength_min.setter
    def transmission_can_wavelength_min(self, value):
        self.update_simple_line_edit_field(line_edit="fit_can_wavelength_min_line_edit", value=value)

    @property
    def transmission_can_wavelength_max(self):
        return self.get_simple_line_edit_field(line_edit="fit_can_wavelength_max_line_edit", expected_type=float)

    @transmission_can_wavelength_max.setter
    def transmission_can_wavelength_max(self, value):
        self.update_simple_line_edit_field(line_edit="fit_can_wavelength_max_line_edit", value=value)

    @property
    def transmission_sample_use_wavelength(self):
        return self.fit_sample_wavelength_combo_box.isChecked()

    @transmission_sample_use_wavelength.setter
    def transmission_sample_use_wavelength(self, value):
        self.fit_sample_wavelength_combo_box.setChecked(value)

    @property
    def transmission_can_use_wavelength(self):
        return self.fit_can_wavelength_combo_box.isChecked()

    @transmission_can_use_wavelength.setter
    def transmission_can_use_wavelength(self, value):
        self.fit_can_wavelength_combo_box.setChecked(value)

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

    # ==================================================================================================================
    # ==================================================================================================================
    # Q TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Q limit
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def q_1d_min_or_rebin_string(self):
        gui_element = self.q_1d_min_line_edit
        value_as_string = gui_element.text()
        if not value_as_string:
            return None
        try:
            value = float(value_as_string)
        except ValueError:
            value = value_as_string
        return value

    @q_1d_min_or_rebin_string.setter
    def q_1d_min_or_rebin_string(self, value):
        self.update_simple_line_edit_field(line_edit="q_1d_min_line_edit", value=value)

    @property
    def q_1d_max(self):
        return self.get_simple_line_edit_field(line_edit="q_1d_max_line_edit", expected_type=float)

    @q_1d_max.setter
    def q_1d_max(self, value):
        self.update_simple_line_edit_field(line_edit="q_1d_max_line_edit", value=value)

    @property
    def q_1d_step(self):
        return self.get_simple_line_edit_field(line_edit="q_1d_step_line_edit", expected_type=float)

    @q_1d_step.setter
    def q_1d_step(self, value):
        self.update_simple_line_edit_field(line_edit="q_1d_step_line_edit", value=value)

    @property
    def q_1d_step_type(self):
        q_1d_step_type_as_string = self.q_1d_step_type_combo_box.currentText()
        # Hedge for trying to read out
        try:
            return RangeStepType(q_1d_step_type_as_string)
        except ValueError:
            return None

    @q_1d_step_type.setter
    def q_1d_step_type(self, value):
        if value is None:
            # Set to the default
            self.q_1d_step_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=RangeStepType, combo_box="q_1d_step_type_combo_box")
            # Set the list
            if isinstance(value, list):
                gui_element = self.q_1d_step_type_combo_box
                gui_element.clear()
                value.append(self.VARIABLE)
                for element in value:
                    self._add_list_element_to_combo_box(gui_element=gui_element, element=element)

    @property
    def q_xy_max(self):
        return self.get_simple_line_edit_field(line_edit="q_xy_max_line_edit", expected_type=float)

    @q_xy_max.setter
    def q_xy_max(self, value):
        self.update_simple_line_edit_field(line_edit="q_xy_max_line_edit", value=value)

    @property
    def q_xy_step(self):
        return self.get_simple_line_edit_field(line_edit="q_xy_step_line_edit", expected_type=float)

    @q_xy_step.setter
    def q_xy_step(self, value):
        self.update_simple_line_edit_field(line_edit="q_xy_step_line_edit", value=value)

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def gravity_extra_length(self):
        return self.get_simple_line_edit_field(line_edit="gravity_extra_length_line_edit", expected_type=float)

    @gravity_extra_length.setter
    def gravity_extra_length(self, value):
        self.update_simple_line_edit_field(line_edit="gravity_extra_length_line_edit", value=value)

    @property
    def gravity_on_off(self):
        return self.gravity_group_box.isChecked()

    @gravity_on_off.setter
    def gravity_on_off(self, value):
        self.gravity_group_box.setChecked(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Q Resolution
    # ------------------------------------------------------------------------------------------------------------------
    def _get_q_resolution_aperture(self, current_index, line_edit):
        if self.q_resolution_shape_combo_box.currentIndex() == current_index:
            return self.get_simple_line_edit_field(line_edit=line_edit, expected_type=float)
        else:
            return ""

    @property
    def use_q_resolution(self):
        return self.q_resolution_group_box.isChecked()

    @use_q_resolution.setter
    def use_q_resolution(self, value):
        self.q_resolution_group_box.setChecked(value)

    @property
    def q_resolution_source_a(self):
        # We expected a current index 0 (since this is a circular aperture)
        return self._get_q_resolution_aperture(current_index=0, line_edit="q_resolution_source_a_line_edit")

    @q_resolution_source_a.setter
    def q_resolution_source_a(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_source_a_line_edit", value=value)

    @property
    def q_resolution_sample_a(self):
        return self._get_q_resolution_aperture(current_index=0, line_edit="q_resolution_sample_a_line_edit")

    @q_resolution_sample_a.setter
    def q_resolution_sample_a(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_sample_a_line_edit", value=value)

    @property
    def q_resolution_source_h(self):
        return self._get_q_resolution_aperture(current_index=1, line_edit="q_resolution_source_h_line_edit")

    @q_resolution_source_h.setter
    def q_resolution_source_h(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_source_h_line_edit", value=value)

    @property
    def q_resolution_sample_h(self):
        return self._get_q_resolution_aperture(current_index=1, line_edit="q_resolution_sample_h_line_edit")

    @q_resolution_sample_h.setter
    def q_resolution_sample_h(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_sample_h_line_edit", value=value)

    @property
    def q_resolution_source_w(self):
        return self._get_q_resolution_aperture(current_index=1, line_edit="q_resolution_source_w_line_edit")

    @q_resolution_source_w.setter
    def q_resolution_source_w(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_source_w_line_edit", value=value)

    @property
    def q_resolution_sample_w(self):
        return self._get_q_resolution_aperture(current_index=1, line_edit="q_resolution_sample_w_line_edit")

    @q_resolution_sample_w.setter
    def q_resolution_sample_w(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_sample_w_line_edit", value=value)

    @property
    def q_resolution_delta_r(self):
        return self.get_simple_line_edit_field(line_edit="q_resolution_delta_r_line_edit", expected_type=float)

    @q_resolution_delta_r.setter
    def q_resolution_delta_r(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_delta_r_line_edit", value=value)

    @property
    def q_resolution_collimation_length(self):
        return self.get_simple_line_edit_field(line_edit="q_resolution_collimation_length_line_edit", expected_type=float)

    @q_resolution_collimation_length.setter
    def q_resolution_collimation_length(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_collimation_length_line_edit", value=value)

    @property
    def q_resolution_moderator_file(self):
        return self.get_simple_line_edit_field(line_edit="q_resolution_moderator_file_line_edit", expected_type=str)

    @q_resolution_moderator_file.setter
    def q_resolution_moderator_file(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_moderator_file_line_edit", value=value)

    @property
    def r_cut(self):
        return self.get_simple_line_edit_field(line_edit="r_cut_line_edit", expected_type=float)

    @r_cut.setter
    def r_cut(self, value):
        self.update_simple_line_edit_field(line_edit="r_cut_line_edit", value=value)

    @property
    def w_cut(self):
        return self.get_simple_line_edit_field(line_edit="w_cut_line_edit", expected_type=float)

    @w_cut.setter
    def w_cut(self, value):
        self.update_simple_line_edit_field(line_edit="w_cut_line_edit", value=value)

    # ==================================================================================================================
    # ==================================================================================================================
    # MASK TAB
    # ==================================================================================================================
    # ==================================================================================================================

    # ------------------------------------------------------------------------------------------------------------------
    # Phi Limit
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def phi_limit_min(self):
        return self.get_simple_line_edit_field(line_edit="phi_limit_min_line_edit", expected_type=float)

    @phi_limit_min.setter
    def phi_limit_min(self, value):
        self.update_simple_line_edit_field(line_edit="phi_limit_min_line_edit", value=value)

    @property
    def phi_limit_max(self):
        return self.get_simple_line_edit_field(line_edit="phi_limit_max_line_edit", expected_type=float)

    @phi_limit_max.setter
    def phi_limit_max(self, value):
        self.update_simple_line_edit_field(line_edit="phi_limit_max_line_edit", value=value)

    @property
    def phi_limit_use_mirror(self):
        return self.phi_limit_use_mirror_check_box.isChecked()

    @phi_limit_use_mirror.setter
    def phi_limit_use_mirror(self, value):
        self.phi_limit_use_mirror_check_box.setChecked(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Radius Limit
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def radius_limit_min(self):
        return self.get_simple_line_edit_field(line_edit="radius_limit_min_line_edit", expected_type=float)

    @radius_limit_min.setter
    def radius_limit_min(self, value):
        self.update_simple_line_edit_field(line_edit="radius_limit_min_line_edit", value=value)

    @property
    def radius_limit_max(self):
        return self.get_simple_line_edit_field(line_edit="radius_limit_max_line_edit", expected_type=float)

    @radius_limit_max.setter
    def radius_limit_max(self, value):
        self.update_simple_line_edit_field(line_edit="radius_limit_max_line_edit", value=value)

    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # END ACCESSORS
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    # $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    def _attach_validators(self):
        # Setup the list of validators
        double_validator = QDoubleValidator()
        positive_double_validator = QDoubleValidator()
        positive_double_validator.setBottom(0.0)
        positive_integer_validator = QIntValidator()
        positive_integer_validator.setBottom(1)

        # -------------------------------
        # General tab
        # -------------------------------
        self.merged_shift_line_edit.setValidator(double_validator)
        self.merged_scale_line_edit.setValidator(double_validator)
        self.merged_q_range_start_line_edit.setValidator(double_validator)
        self.merged_q_range_stop_line_edit.setValidator(double_validator)
        self.merged_max_line_edit.setValidator(double_validator)
        self.merged_min_line_edit.setValidator(double_validator)

        self.wavelength_min_line_edit.setValidator(positive_double_validator)
        self.wavelength_max_line_edit.setValidator(positive_double_validator)
        self.wavelength_step_line_edit.setValidator(positive_double_validator)

        self.absolute_scale_line_edit.setValidator(double_validator)
        self.z_offset_line_edit.setValidator(double_validator)

        # --------------------------------
        # Adjustment tab
        # --------------------------------
        self.monitor_normalization_line_edit.setValidator(positive_integer_validator)
        self.transmission_line_edit.setValidator(positive_integer_validator)
        self.transmission_monitor_line_edit.setValidator(positive_integer_validator)
        self.transmission_radius_line_edit.setValidator(positive_double_validator)
        self.transmission_mn_4_shift_line_edit.setValidator(double_validator)
        self.transmission_mn_5_shift_line_edit.setValidator(double_validator)

        self.fit_sample_wavelength_min_line_edit.setValidator(positive_double_validator)
        self.fit_sample_wavelength_max_line_edit.setValidator(positive_double_validator)
        self.fit_can_wavelength_min_line_edit.setValidator(positive_double_validator)
        self.fit_can_wavelength_max_line_edit.setValidator(positive_double_validator)

        # --------------------------------
        # Q tab
        # --------------------------------
        self.q_1d_min_line_edit.setValidator(double_validator)
        self.q_1d_max_line_edit.setValidator(double_validator)
        self.q_1d_step_line_edit.setValidator(positive_double_validator)
        self.q_xy_max_line_edit.setValidator(positive_double_validator)  # Yes, this should be positive!
        self.q_xy_step_line_edit.setValidator(positive_double_validator)

        self.r_cut_line_edit.setValidator(positive_double_validator)
        self.w_cut_line_edit.setValidator(positive_double_validator)

        self.gravity_extra_length_line_edit.setValidator(double_validator)

        self.q_resolution_source_a_line_edit.setValidator(positive_double_validator)
        self.q_resolution_sample_a_line_edit.setValidator(positive_double_validator)
        self.q_resolution_source_h_line_edit.setValidator(positive_double_validator)
        self.q_resolution_sample_h_line_edit.setValidator(positive_double_validator)
        self.q_resolution_source_w_line_edit.setValidator(positive_double_validator)
        self.q_resolution_sample_w_line_edit.setValidator(positive_double_validator)
        self.q_resolution_delta_r_line_edit.setValidator(positive_double_validator)
        self.q_resolution_collimation_length_line_edit.setValidator(double_validator)

    def reset_all_fields_to_default(self):
        # ------------------------------
        # General tab
        # ------------------------------
        self.reduction_dimensionality_1D.setChecked(True)
        self.reduction_mode_combo_box.setCurrentIndex(0)

        self.merged_q_range_start_line_edit.setText("")
        self.merged_q_range_stop_line_edit.setText("")
        self.merged_max_line_edit.setText("")
        self.merged_min_line_edit.setText("")
        self.merged_scale_line_edit.setText("")
        self.merged_shift_line_edit.setText("")
        self.merged_shift_use_fit_check_box.setChecked(False)
        self.merged_scale_use_fit_check_box.setChecked(False)

        self.event_binning_group_box.setChecked(False)
        self.slice_event_line_edit.setText("")
        self.event_binning_line_edit.setText("")

        self.wavelength_min_line_edit.setText("")
        self.wavelength_max_line_edit.setText("")
        self.wavelength_step_line_edit.setText("")
        self.wavelength_step_type_combo_box.setCurrentIndex(0)
        self.wavelength_slices_line_edit.setText("")

        self.absolute_scale_line_edit.setText("")
        self.z_offset_line_edit.setText("")

        # --------------------------------
        # Adjustment tab
        # --------------------------------
        self.monitor_normalization_line_edit.setText("")
        self.monitor_normalization_interpolating_rebin_check_box.setChecked(False)

        self.transmission_line_edit.setText("")
        self.transmission_interpolating_rebin_check_box.setChecked(False)
        self.transmission_target_combo_box.setCurrentIndex(0)
        self.transmission_monitor_line_edit.setText("")
        self.transmission_mn_4_shift_line_edit.setText("")
        self.transmission_mn_5_shift_line_edit.setText("")
        self.transmission_radius_line_edit.setText("")
        self.transmission_roi_files_line_edit.setText("")
        self.transmission_mask_files_line_edit.setText("")

        self.fit_selection_combo_box.setCurrentIndex(0)
        self.fit_sample_use_fit_check_box.setChecked(False)
        self.fit_sample_fit_type_combo_box.setCurrentIndex(0)
        self.fit_sample_polynomial_order_spin_box.setValue(2)
        self.fit_sample_wavelength_combo_box.setChecked(False)
        self.fit_sample_wavelength_min_line_edit.setText("")
        self.fit_sample_wavelength_max_line_edit.setText("")

        self.fit_can_use_fit_check_box.setChecked(False)
        self.fit_can_fit_type_combo_box.setCurrentIndex(0)
        self.fit_can_polynomial_order_spin_box.setValue(2)
        self.fit_can_wavelength_combo_box.setChecked(False)
        self.fit_can_wavelength_min_line_edit.setText("")
        self.fit_can_wavelength_max_line_edit.setText("")

        self.pixel_adjustment_det_1_line_edit.setText("")
        self.pixel_adjustment_det_2_line_edit.setText("")

        self.wavelength_adjustment_det_1_line_edit.setText("")
        self.wavelength_adjustment_det_2_line_edit.setText("")
        # --------------------------------
        # Q tab
        # --------------------------------

        self.radius_limit_min_line_edit.setText("")
        self.radius_limit_max_line_edit.setText("")

        self.phi_limit_min_line_edit.setText("-90")
        self.phi_limit_max_line_edit.setText("90")
        self.phi_limit_use_mirror_check_box.setChecked(True)

        self.wavelength_min_line_edit.setText("")
        self.wavelength_max_line_edit.setText("")
        self.wavelength_slices_line_edit.setText("")
        self.wavelength_step_line_edit.setText("")
        self.wavelength_step_type_combo_box.setCurrentIndex(0)

        self.r_cut_line_edit.setText("")
        self.w_cut_line_edit.setText("")

        self.q_1d_min_line_edit.setText("")
        self.q_1d_max_line_edit.setText("")
        self.q_1d_step_line_edit.setText("")
        self.q_1d_step_type_combo_box.setCurrentIndex(0)

        self.q_xy_max_line_edit.setText("")
        self.q_xy_step_line_edit.setText("")

        self.gravity_group_box.setChecked(False)
        self.gravity_extra_length_line_edit.setText("")

        self.q_resolution_group_box.setChecked(False)
        self.q_resolution_shape_combo_box.setCurrentIndex(0)
        self.q_resolution_source_a_line_edit.setText("")
        self.q_resolution_sample_a_line_edit.setText("")
        self.q_resolution_source_h_line_edit.setText("")
        self.q_resolution_sample_h_line_edit.setText("")
        self.q_resolution_source_w_line_edit.setText("")
        self.q_resolution_sample_w_line_edit.setText("")
        self.q_resolution_delta_r_line_edit.setText("")
        self.q_resolution_collimation_length_line_edit.setText("")
        self.q_resolution_moderator_file_line_edit.setText("")

        self.mask_file_input_line_edit.setText("")

    # ----------------------------------------------------------------------------------------------
    # Table interaction
    # ----------------------------------------------------------------------------------------------

    def get_cell(self, row, column):
        row_location = self.row([row])
        value = self.data_processor_table.cellAt(row_location, column).contentText()
        return value

    def set_cell(self, value, row, column):
        value_as_str = str(value)
        cell = self.data_processor_table.cellAt(row, column)
        cell.setContentText(value_as_str)
        self.data_processor_table.setCellAt(row, column, cell)

    def change_row_color(self, color, row_index):
        row_location = self.row([row_index])
        cell_data = self.data_processor_table.cellsAt(row_location)
        for index, cell in enumerate(cell_data):
            cell.setBackgroundColor(color)
            self.data_processor_table.setCellAt(row_location, index, cell)

    def set_row_tooltip(self, tool_tip, row):
        if not tool_tip:
            tool_tip = ""

        row_location = self.row([row])
        cell_data = self.data_processor_table.cellsAt(row_location)
        for index, cell in enumerate(cell_data):
            cell.setToolTip(tool_tip)
            self.data_processor_table.setCellAt(row_location, index, cell)

    def get_selected_rows(self):
        row_locations = self.data_processor_table.selectedRowLocations()
        rows = [x.rowRelativeToParent() for x in row_locations]
        return rows

    def get_row(self, row_location):
        cell_data = self.data_processor_table.cellsAt(row_location)
        return [str(x.contentText()) for x in cell_data]

    def clear_table(self):
        self.data_processor_table.removeAllRows()

    def clear_selection(self):
        self.data_processor_table.clearSelection()

    def update_table_selection(self, row_locations):
        row_locations = [self.row([x]) for x in row_locations]
        self.data_processor_table.setSelectedRowLocations(row_locations)

    def add_row(self, row_entries):
        value = [self.cell(x) for x in self._convert_from_row_state(row_entries)]
        self.data_processor_table.appendChildRowOf(self.row([]), value)

    def remove_rows(self, rows):
        rows = [self.row([item]) for item in rows]
        self.data_processor_table.removeRows(rows)

    def add_empty_row(self):
        value = [self.cell("") for _ in self._column_index_map]
        self.data_processor_table.appendChildRowOf(self.row([]), value)

    def insert_empty_row(self, row_index):
        self.data_processor_table.insertChildRowOf(self.row([]), row_index)

    def set_hinting_line_edit_for_column(self, column, hint_strategy):
        self.data_processor_table.setHintsForColumn(column, hint_strategy)

    def _run_python_code(self, text):
        """
        Re-emits 'runPytonScript' signal
        """
        execute_script(text)

    def hide_geometry(self):
        self.data_processor_table.hideColumn(self._column_index_map["Sample Shape"])
        self.data_processor_table.hideColumn(self._column_index_map["Sample Height"])
        self.data_processor_table.hideColumn(self._column_index_map["Sample Width"])

    def show_geometry(self):
        self.data_processor_table.showColumn(self._column_index_map["Sample Shape"])
        self.data_processor_table.showColumn(self._column_index_map["Sample Height"])
        self.data_processor_table.showColumn(self._column_index_map["Sample Width"])

    def hide_background_subtraction(self):
        self.data_processor_table.hideColumn(self._column_index_map["Background Workspace"])
        self.data_processor_table.hideColumn(self._column_index_map["Scale Factor"])

    def show_background_subtraction(self):
        self.data_processor_table.showColumn(self._column_index_map["Background Workspace"])
        self.data_processor_table.showColumn(self._column_index_map["Scale Factor"])

    def hide_period_columns(self):
        self.multi_period_check_box.setChecked(False)

        for col in self.MULTI_PERIOD_COLUMNS:
            self.data_processor_table.hideColumn(self._column_index_map[col])

    def show_period_columns(self):
        self.multi_period_check_box.setChecked(True)

        for col in self.MULTI_PERIOD_COLUMNS:
            self.data_processor_table.showColumn(self._column_index_map[col])

    def _convert_from_row_state(self, row_state_obj):
        assert isinstance(row_state_obj, RowEntries)
        array = ["" for _ in self._column_index_map]
        array[self._column_index_map["Can Direct"]] = row_state_obj.can_direct
        array[self._column_index_map["Can Scatter"]] = row_state_obj.can_scatter
        array[self._column_index_map["Can Transmission"]] = row_state_obj.can_transmission

        array[self._column_index_map["Sample Direct"]] = row_state_obj.sample_direct
        array[self._column_index_map["Sample Scatter"]] = row_state_obj.sample_scatter
        array[self._column_index_map["Sample Transmission"]] = row_state_obj.sample_transmission

        array[self._column_index_map["Options"]] = row_state_obj.options.get_displayed_text()
        array[self._column_index_map["Output Name"]] = row_state_obj.output_name
        array[self._column_index_map["User File"]] = row_state_obj.user_file

        array[self._column_index_map["Sample Height"]] = row_state_obj.sample_height

        sample_shape = row_state_obj.sample_shape.value if row_state_obj.sample_shape else ""
        array[self._column_index_map["Sample Shape"]] = sample_shape
        array[self._column_index_map["Sample Thickness"]] = row_state_obj.sample_thickness
        array[self._column_index_map["Sample Width"]] = row_state_obj.sample_width

        array[self._column_index_map["Background Workspace"]] = row_state_obj.background_ws
        array[self._column_index_map["Scale Factor"]] = row_state_obj.scale_factor

        array[self._column_index_map["CDP"]] = row_state_obj.can_direct_period
        array[self._column_index_map["CSP"]] = row_state_obj.can_scatter_period
        array[self._column_index_map["CTP"]] = row_state_obj.can_transmission_period

        array[self._column_index_map["SDP"]] = row_state_obj.sample_direct_period
        array[self._column_index_map["SSP"]] = row_state_obj.sample_scatter_period
        array[self._column_index_map["STP"]] = row_state_obj.sample_transmission_period
        return array

    def _convert_to_row_state(self, row_array):
        entry = RowEntries()
        entry.can_direct = row_array[self._column_index_map["Can Direct"]]
        entry.can_scatter = row_array[self._column_index_map["Can Scatter"]]
        entry.can_transmission = row_array[self._column_index_map["Can Transmission"]]

        entry.sample_direct = row_array[self._column_index_map["Sample Direct"]]
        entry.sample_scatter = row_array[self._column_index_map["Sample Scatter"]]
        entry.sample_transmission = row_array[self._column_index_map["Sample Transmission"]]

        entry.output_name = row_array[self._column_index_map["Output Name"]]
        entry.user_file = row_array[self._column_index_map["User File"]]

        entry.sample_height = row_array[self._column_index_map["Sample Height"]]
        entry.sample_shape = row_array[self._column_index_map["Sample Shape"]]
        entry.sample_thickness = row_array[self._column_index_map["Sample Thickness"]]
        entry.sample_width = row_array[self._column_index_map["Sample Width"]]

        entry.background_ws = row_array[self._column_index_map["Background Workspace"]]
        entry.scale_factor = row_array[self._column_index_map["Scale Factor"]]

        entry.can_direct_period = row_array[self._column_index_map["CDP"]]
        entry.can_scatter_period = row_array[self._column_index_map["CSP"]]
        entry.can_transmission_period = row_array[self._column_index_map["CTP"]]

        entry.sample_direct_period = row_array[self._column_index_map["SDP"]]
        entry.sample_scatter_period = row_array[self._column_index_map["SSP"]]
        entry.sample_transmission_period = row_array[self._column_index_map["STP"]]

        entry.options.set_user_options(row_array[self._column_index_map["Options"]])

        return entry

    def closeEvent(self, event):
        for child in self.children():
            if isinstance(child, SANSSaveOtherDialog):
                child.done(0)
        super(QMainWindow, self).closeEvent(event)
