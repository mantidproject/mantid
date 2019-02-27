# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" Main view for the ISIS SANS reduction interface.
"""

from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta, abstractmethod
from inspect import isclass
from qtpy.QtWidgets import (QListWidgetItem, QMessageBox, QFileDialog, QMainWindow)  # noqa
from qtpy.QtCore import (QRegExp, QSettings)  # noqa
from qtpy.QtGui import (QDoubleValidator, QIcon, QIntValidator, QRegExpValidator)  # noqa
from six import with_metaclass

from reduction_gui.reduction.scripter import execute_script
from mantid.kernel import (Logger)
from mantidqt import icons
from mantidqt.utils.qt import load_ui
from mantidqt.widgets import jobtreeview, manageuserdirectories
from sans.common.enums import (BinningType, ReductionDimensionality, OutputMode, SaveType, SANSInstrument,
                               RangeStepType, ReductionMode, FitType)
from sans.common.file_information import SANSFileInformationFactory
from sans.gui_logic.gui_common import (get_reduction_mode_from_gui_selection,
                                       get_reduction_mode_strings_for_gui,
                                       get_string_for_gui_from_reduction_mode, GENERIC_SETTINGS,
                                       load_file, load_default_file, load_property, set_setting,
                                       get_instrument_from_gui_selection)
from sans.gui_logic.models.run_summation import RunSummation
from sans.gui_logic.models.run_selection import RunSelection
from sans.gui_logic.models.run_finder import SummableRunFinder
from sans.gui_logic.models.summation_settings import SummationSettings
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from sans.gui_logic.presenter.run_selector_presenter import RunSelectorPresenter
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter
from ui.sans_isis.work_handler import WorkHandler
from ui.sans_isis.SANSSaveOtherWindow import SANSSaveOtherDialog

from qtpy import PYQT4
if PYQT4:
    IN_MANTIDPLOT = False
    try:
        from pymantidplot import proxies
        IN_MANTIDPLOT = True
    except ImportError:
        # We are not in MantidPlot e.g. testing
        pass

DEFAULT_BIN_SETTINGS = \
    '5.5,45.5,50.0, 50.0,1000.0, 500.0,1500.0, 750.0,99750.0, 255.0,100005.0'

Ui_SansDataProcessorWindow, _ = load_ui(__file__, "sans_data_processor_window.ui")


class RunSelectorPresenterFactory(object):
    def __init__(self, title, run_finder):
        self._title = title
        self._run_finder = run_finder

    def __call__(self,
                 run_selector_view,
                 on_selection_change,
                 parent_view):
        return RunSelectorPresenter(self._title,
                                    RunSelection(on_selection_change),
                                    self._run_finder,
                                    run_selector_view,
                                    parent_view)


def _make_run_summation_settings_presenter(summation_settings_view, parent_view, instrument):
    if instrument != "LOQ":
        binning_type = BinningType.SaveAsEventData
    else:
        binning_type = BinningType.Custom
    summation_settings = SummationSettings(binning_type)
    summation_settings.bin_settings = DEFAULT_BIN_SETTINGS
    return SummationSettingsPresenter(summation_settings,
                                      summation_settings_view,
                                      parent_view)


# ----------------------------------------------------------------------------------------------------------------------
# Gui Classes
# ----------------------------------------------------------------------------------------------------------------------
class SANSDataProcessorGui(QMainWindow,
                           Ui_SansDataProcessorWindow):
    data_processor_table = None
    INSTRUMENTS = None
    VARIABLE = "Variable"

    MULTI_PERIOD_COLUMNS = [1, 3, 5, 7, 9, 11]

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
        def on_reduction_dimensionality_changed(self, is_1d):
            pass

        @abstractmethod
        def on_data_changed(self, row, column, new_value, old_value):
            pass

        @abstractmethod
        def on_manage_directories(self):
            pass

        @abstractmethod
        def on_instrument_changed(self):
            pass

        @abstractmethod
        def on_row_inserted(self):
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
        def on_compatibility_unchecked(self):
            pass

    def __init__(self):
        """
        Initialise the interface
        """
        super(QMainWindow, self).__init__()
        self.setupUi(self)

        # Listeners allow us to to notify all presenters
        self._settings_listeners = []

        # Q Settings
        self.__generic_settings = GENERIC_SETTINGS
        self.__path_key = "sans_path"
        self.__user_file_key = "user_file"
        self.__mask_file_input_path_key = "mask_files"
        self.__output_mode_key = "output_mode"
        self.__save_can_key = "save_can"

        # Logger
        self.gui_logger = Logger("SANS GUI LOGGER")

        # Instrument
        SANSDataProcessorGui.INSTRUMENTS = ",".join([SANSInstrument.to_string(item)
                                                     for item in [SANSInstrument.SANS2D,
                                                                  SANSInstrument.LOQ,
                                                                  SANSInstrument.LARMOR,
                                                                  SANSInstrument.ZOOM]])

        self.instrument = SANSInstrument.NoInstrument

        self.paste_button.setIcon(icons.get_icon("fa.paste"))
        self.copy_button.setIcon(icons.get_icon("fa.copy"))
        self.cut_button.setIcon(icons.get_icon("fa.cut"))
        self.erase_button.setIcon(icons.get_icon("fa.eraser"))
        self.delete_row_button.setIcon(icons.get_icon("fa.trash"))
        self.insert_row_button.setIcon(icons.get_icon("fa.table"))

        self.paste_button.clicked.connect(self._paste_rows_requested)
        self.copy_button.clicked.connect(self._copy_rows_requested)
        self.erase_button.clicked.connect(self._erase_rows)
        self.cut_button.clicked.connect(self._cut_rows)

        self.delete_row_button.clicked.connect(self._remove_rows_requested_from_button)
        self.insert_row_button.clicked.connect(self._on_insert_button_pressed)
        self.save_other_pushButton.clicked.connect(self._on_save_other_button_pressed)

        self.save_can_checkBox.clicked.connect(self._on_save_can_clicked)
        self.reduction_dimensionality_1D.toggled.connect(self._on_reduction_dimensionality_changed)

        # Attach validators
        self._attach_validators()

        self._setup_progress_bar()

    def _setup_progress_bar(self):
        self.batch_progress_bar.setMinimum(0)
        self.batch_progress_bar.setMaximum(1)
        self.batch_progress_bar.setValue(0)

    def add_listener(self, listener):
        if not isinstance(listener, SANSDataProcessorGui.RunTabListener):
            raise ValueError(
                "The listener is not of type RunTabListener but rather {}".format(type(listener)))
        self._settings_listeners.append(listener)

    def clear_listeners(self):
        self._settings_listeners = []

    def _call_settings_listeners(self, target):
        for listener in self._settings_listeners:
            target(listener)

    def set_current_page(self, index):
        self.main_stacked_widget.setCurrentIndex(index)

    def _setup_add_runs_page(self):
        self.add_runs_presenter = AddRunsPagePresenter(RunSummation(WorkHandler(), self.add_runs_page),
                                                       RunSelectorPresenterFactory('Runs To Sum',
                                                                                   SummableRunFinder(
                                                                                       SANSFileInformationFactory())),
                                                       _make_run_summation_settings_presenter,
                                                       self.add_runs_page, self)

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

        runs_icon = icons.get_icon("fa.play-circle-o")
        _ = QListWidgetItem(runs_icon, "Runs", self.tab_choice_list)  # noqa

        settings_icon = icons.get_icon("fa.cog")
        _ = QListWidgetItem(settings_icon, "Settings", self.tab_choice_list)  # noqa

        centre_icon = icons.get_icon("fa.dot-circle-o")
        _ = QListWidgetItem(centre_icon, "Beam Centre", self.tab_choice_list)  # noqa

        add_runs_page_icon = icons.get_icon("fa.plus-circle")
        _ = QListWidgetItem(add_runs_page_icon, "Sum Runs", self.tab_choice_list)  # noqa

        diagnostic_icon = icons.get_icon("fa.question-circle")
        _ = QListWidgetItem(diagnostic_icon, "Diagnostic Page", self.tab_choice_list)  # noqa

        # Set the 0th row enabled
        self.tab_choice_list.setCurrentRow(0)

        # --------------------------------------------------------------------------------------------------------------
        # Main Tab
        # --------------------------------------------------------------------------------------------------------------
        self.create_data_table(show_periods=False)

        self._setup_main_tab()

        self.multi_period_check_box.stateChanged.connect(self._on_multi_period_selection)
        self.sample_geometry_checkbox.stateChanged.connect(self._on_sample_geometry_selection)

        self.wavelength_step_type_combo_box.currentIndexChanged.connect(
            self._on_wavelength_step_type_changed)

        self.process_selected_button.clicked.connect(self._process_selected_clicked)
        self.process_all_button.clicked.connect(self._process_all_clicked)

        self.load_button.clicked.connect(self._load_clicked)

        self.export_table_button.clicked.connect(self._export_table_clicked)

        self.help_button.clicked.connect(self._on_help_button_clicked)

        # Output mode radio buttons
        self.output_mode_memory_radio_button.clicked.connect(self._on_output_mode_clicked)
        self.output_mode_file_radio_button.clicked.connect(self._on_output_mode_clicked)
        self.output_mode_both_radio_button.clicked.connect(self._on_output_mode_clicked)

        # --------------------------------------------------------------------------------------------------------------
        # Settings tabs
        # --------------------------------------------------------------------------------------------------------------
        self.reset_all_fields_to_default()
        self.pixel_adjustment_det_1_push_button.clicked.connect(
            self._on_load_pixel_adjustment_det_1)
        self.pixel_adjustment_det_2_push_button.clicked.connect(
            self._on_load_pixel_adjustment_det_2)
        self.wavelength_adjustment_det_1_push_button.clicked.connect(
            self._on_load_wavelength_adjustment_det_1)
        self.wavelength_adjustment_det_2_push_button.clicked.connect(
            self._on_load_wavelength_adjustment_det_2)

        # Set the merge settings
        self.reduction_mode_combo_box.currentIndexChanged.connect(
            self._on_reduction_mode_selection_has_changed)
        self._on_reduction_mode_selection_has_changed()  # Disable the merge settings initially

        # Mask file input settings
        self.mask_file_browse_push_button.clicked.connect(self._on_load_mask_file)
        self.mask_file_add_push_button.clicked.connect(self._on_mask_file_add)

        self.manage_directories_button.clicked.connect(self._on_manage_directories)

        # Set the q step type settings
        self.q_1d_step_type_combo_box.currentIndexChanged.connect(
            self._on_q_1d_step_type_has_changed)
        self._on_q_1d_step_type_has_changed()

        self.q_xy_step_type_combo_box.currentIndexChanged.connect(
            self._on_q_xy_step_type_has_changed)
        self._on_q_xy_step_type_has_changed()

        # Set the q resolution aperture shape settings
        self.q_resolution_shape_combo_box.currentIndexChanged.connect(
            self._on_q_resolution_shape_has_changed)
        self.q_resolution_group_box.toggled.connect(self._on_q_resolution_shape_has_changed)
        self._on_q_resolution_shape_has_changed()

        # Set the transmission fit selection
        self.fit_selection_combo_box.currentIndexChanged.connect(self._on_fit_selection_has_changed)
        self._on_fit_selection_has_changed()

        # Set the transmission polynomial order
        self.fit_sample_fit_type_combo_box.currentIndexChanged.connect(
            self._on_transmission_fit_type_has_changed)
        self.fit_can_fit_type_combo_box.currentIndexChanged.connect(
            self._on_transmission_fit_type_has_changed)
        self._on_transmission_fit_type_has_changed()

        # Set the transmission target
        self.transmission_target_combo_box.currentIndexChanged.connect(
            self._on_transmission_target_has_changed)
        self._on_transmission_target_has_changed()

        # Roi and Mask files
        self.transmission_roi_files_push_button.clicked.connect(
            self._on_load_transmission_roi_files)
        self.transmission_mask_files_push_button.clicked.connect(
            self._on_load_transmission_mask_files)

        # Q Resolution
        self.q_resolution_moderator_file_push_button.clicked.connect(self._on_load_moderator_file)

        self.wavelength_stacked_widget.setCurrentIndex(0)
        self.hide_geometry()

        return True

    def _on_wavelength_step_type_changed(self):
        if self.wavelength_step_type == RangeStepType.RangeLin:
            self.wavelength_stacked_widget.setCurrentIndex(1)
            self.wavelength_step_label.setText(u'Step [\u00c5^-1]')
        elif self.wavelength_step_type == RangeStepType.RangeLog:
            self.wavelength_stacked_widget.setCurrentIndex(1)
            self.wavelength_step_label.setText(u'Step [d\u03BB/\u03BB]')
        elif self.wavelength_step_type == RangeStepType.Log:
            self.wavelength_stacked_widget.setCurrentIndex(0)
            self.wavelength_step_label.setText(u'Step [d\u03BB/\u03BB]')
        elif self.wavelength_step_type == RangeStepType.Lin:
            self.wavelength_stacked_widget.setCurrentIndex(0)
            self.wavelength_step_label.setText(u'Step [\u00c5^-1]')

    def create_data_table(self, show_periods):
        # Delete an already existing table
        if self.data_processor_table:
            self.data_processor_table.setParent(None)

        self.data_processor_table = jobtreeview.JobTreeView(
            ["Sample Scatter", "ssp", "Sample Transmission", "stp", "Sample Direct", "sdp",
             "Can Scatter", "csp",
             "Can Transmission", "ctp", "Can Direct", "cdp", "Output Name", "User File",
             "Sample Thickness", "Sample Height", "Sample Width", "Sample Shape",
             "Options"]
            , self.cell(""), self)

        # Default QTreeView size is too small
        font = self.data_processor_table.font()
        font.setPointSize(13)
        self.data_processor_table.setFont(font)

        self.data_processor_table.setRootIsDecorated(False)

        row_entry = [''] * 16
        self.add_row(row_entry)
        self._call_settings_listeners(lambda listener: listener.on_row_inserted(0, row_entry))

        self.table_signals = \
            jobtreeview.JobTreeViewSignalAdapter(self.data_processor_table, self)
        # The signal adapter subscribes to events from the table
        # and emits signals whenever it is notified.

        if show_periods:
            self.show_period_columns()
        else:
            self.hide_period_columns()

        self.data_processor_widget_layout.addWidget(self.data_processor_table)
        self.table_signals.cellTextChanged.connect(self._data_changed)
        self.table_signals.rowInserted.connect(self._row_inserted)
        self.table_signals.removeRowsRequested.connect(self._remove_rows_requested)
        self.table_signals.copyRowsRequested.connect(self._copy_rows_requested)
        self.table_signals.pasteRowsRequested.connect(self._paste_rows_requested)

    def cell(self, text):
        background_color = 'white'
        border_thickness = 1
        border_color = "black"
        border_opacity = 255
        is_editable = True
        return jobtreeview.Cell(text, background_color, border_thickness,
                                border_color, border_opacity, is_editable)

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
        self._call_settings_listeners(lambda listener: listener.on_process_selected_clicked())

    def _process_all_clicked(self):
        """
        Process All button clicked
        """
        self._call_settings_listeners(lambda listener: listener.on_process_all_clicked())

    def _load_clicked(self):
        self._call_settings_listeners(lambda listener: listener.on_load_clicked())

    def _export_table_clicked(self):
        self._call_settings_listeners(lambda listener: listener.on_export_table_clicked())

    def _processing_finished(self):
        """
        Clean up
        """
        self._call_settings_listeners(lambda listener: listener.on_processing_finished())

    def _data_changed(self, row_location, column, old_value, new_value):
        row = row_location.rowRelativeToParent()
        self._call_settings_listeners(
            lambda listener: listener.on_data_changed(row, column, str(new_value), (old_value)))

    def _row_inserted(self, row_location):
        if row_location.depth() > 1:
            self.data_processor_table.removeRowAt(row_location)
        else:
            index = row_location.rowRelativeToParent()
            row = self.get_row(row_location)
            self._call_settings_listeners(lambda listener: listener.on_row_inserted(index, row))

    def _remove_rows_requested(self, rows):
        rows = [item.rowRelativeToParent() for item in rows]
        self._call_settings_listeners(lambda listener: listener.on_rows_removed(rows))

    def _remove_rows_requested_from_button(self):
        rows = self.get_selected_rows()
        self._call_settings_listeners(lambda listener: listener.on_rows_removed(rows))

    def _copy_rows_requested(self):
        self._call_settings_listeners(lambda listener: listener.on_copy_rows_requested())

    def _erase_rows(self):
        self._call_settings_listeners(lambda listener: listener.on_erase_rows())

    def _cut_rows(self):
        self._call_settings_listeners(lambda listener: listener.on_cut_rows())

    def _paste_rows_requested(self):
        self._call_settings_listeners(lambda listener: listener.on_paste_rows_requested())

    def _instrument_changed(self):
        self._call_settings_listeners(lambda listener: listener.on_instrument_changed())

    def _on_insert_button_pressed(self):
        self._call_settings_listeners(lambda listener: listener.on_insert_row())

    def _on_save_other_button_pressed(self):
        self._call_settings_listeners(lambda listener: listener.on_save_other())

    def _on_compatibility_unchecked(self):
        self._call_settings_listeners(lambda listener: listener.on_compatibility_unchecked())

    def _on_help_button_clicked(self):
        if PYQT4:
            proxies.showCustomInterfaceHelp('ISIS SANS v2')

    def _on_output_mode_clicked(self):
        if self.output_mode_memory_radio_button.isChecked():
            output_mode = "PublishToADS"
        elif self.output_mode_file_radio_button.isChecked():
            output_mode = "SaveToFile"
        elif self.output_mode_both_radio_button.isChecked():
            output_mode = "Both"
        else:
            output_mode = None
        set_setting(self.__generic_settings, self.__output_mode_key, output_mode)

    def _on_save_can_clicked(self, value):
        self.save_can_checkBox.setChecked(value)
        set_setting(self.__generic_settings, self.__save_can_key, value)

    def _on_reduction_dimensionality_changed(self, is_1d):
        self._call_settings_listeners(lambda listener: listener.on_reduction_dimensionality_changed(is_1d))

    def _on_user_file_load(self):
        """
        Load the user file
        """
        # Load the user file
        load_file(self.user_file_line_edit, "*.*", self.__generic_settings, self.__path_key,
                  self.get_user_file_path)

        # Set full user file path for default loading
        set_setting(self.__generic_settings, self.__user_file_key, self.get_user_file_path())

        # Notify presenters
        self._call_settings_listeners(lambda listener: listener.on_user_file_load())

    def on_user_file_load_failure(self):
        set_setting(self.__generic_settings, self.__user_file_key, "")
        self.user_file_line_edit.setText("")

    def set_out_default_user_file(self):
        """
        Load a default user file, called on view set-up
        """
        if self.get_user_file_path() != "":
            self._call_settings_listeners(lambda listener: listener.on_user_file_load())

    def set_out_default_output_mode(self):
        try:
            default_output_mode = OutputMode.from_string(load_property(self.__generic_settings, self.__output_mode_key))
        except RuntimeError:
            pass
        else:
            self._check_output_mode(default_output_mode)

    def _check_output_mode(self, value):
        if value is OutputMode.PublishToADS:
            self.output_mode_memory_radio_button.setChecked(True)
        elif value is OutputMode.SaveToFile:
            self.output_mode_file_radio_button.setChecked(True)
        elif value is OutputMode.Both:
            self.output_mode_both_radio_button.setChecked(True)

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
        load_file(self.batch_line_edit, "*.*", self.__generic_settings, self.__path_key,
                  self.get_batch_file_path)
        self._call_settings_listeners(lambda listener: listener.on_batch_file_load())

    def disable_buttons(self):

        self.process_selected_button.setEnabled(False)
        self.process_all_button.setEnabled(False)
        self.batch_button.setEnabled(False)
        self.user_file_button.setEnabled(False)
        self.manage_directories_button.setEnabled(False)
        self.load_button.setEnabled(False)
        self.export_table_button.setEnabled(False)

    def enable_buttons(self):
        self.process_selected_button.setEnabled(True)
        self.process_all_button.setEnabled(True)
        self.batch_button.setEnabled(True)
        self.user_file_button.setEnabled(True)
        self.manage_directories_button.setEnabled(True)
        self.load_button.setEnabled(True)
        self.export_table_button.setEnabled(True)

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
        msg.setText(10 * ' ' + message + ' ' * (30 - message_length))
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
        load_file(self.pixel_adjustment_det_1_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_pixel_adjustment_det_1)

    def get_pixel_adjustment_det_1(self):
        return str(self.pixel_adjustment_det_1_line_edit.text())

    def _on_load_pixel_adjustment_det_2(self):
        load_file(self.pixel_adjustment_det_2_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_pixel_adjustment_det_2)

    def get_pixel_adjustment_det_2(self):
        return str(self.pixel_adjustment_det_2_line_edit.text())

    def _on_load_wavelength_adjustment_det_1(self):
        load_file(self.wavelength_adjustment_det_1_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_wavelength_adjustment_det_1)

    def get_wavelength_adjustment_det_1(self):
        return str(self.wavelength_adjustment_det_1_line_edit.text())

    def _on_load_wavelength_adjustment_det_2(self):
        load_file(self.wavelength_adjustment_det_2_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_wavelength_adjustment_det_2)

    def get_wavelength_adjustment_det_2(self):
        return str(self.wavelength_adjustment_det_2_line_edit.text())

    def _on_reduction_mode_selection_has_changed(self):
        selection = self.reduction_mode_combo_box.currentText()
        is_merged = selection == ReductionMode.to_string(ReductionMode.Merged)
        self.merged_settings.setEnabled(is_merged)

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
            comma_separated_floats_regex_string = "^(\s*[-+]?[0-9]*\.?[0-9]*)(\s*,\s*[-+]?[0-9]*\.?[0-9]*)+\s*$"
            reg_ex = QRegExp(comma_separated_floats_regex_string)
            validator = QRegExpValidator(reg_ex)
            self.q_1d_min_line_edit.setValidator(validator)

            self.q_min_label.setText("Rebin String")
            self.q_step_label.setText(u'Step [\u00c5^-1]')
        else:
            # If rebin string data
            data_q_min = str(self.q_1d_min_line_edit.text())
            if "," in data_q_min:
                self.q_1d_min_line_edit.setText("")
            validator = QDoubleValidator()
            validator.setBottom(0.0)
            self.q_1d_min_line_edit.setValidator(validator)

            label = u"Min [\u00c5^-1]"
            self.q_min_label.setText(label)

            step_label = u'dQ/Q' if u'Log' in selection else u'Step [\u00c5^-1]'
            self.q_step_label.setText(step_label)

    def _on_q_xy_step_type_has_changed(self):
        selection = self.q_xy_step_type_combo_box.currentText()
        step_label = u'dQ/Q' if u'Log' in selection else u'Step [\u00c5^-1]'
        self.q_xy_step_label.setText(step_label)

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
        fit_type_sample_as_string = self.fit_sample_fit_type_combo_box.currentText().encode('utf-8')
        fit_type_sample = FitType.from_string(fit_type_sample_as_string)
        is_sample_polynomial = fit_type_sample is FitType.Polynomial
        self.fit_sample_polynomial_order_spin_box.setEnabled(is_sample_polynomial)

        # Check the can settings
        fit_type_can_as_string = self.fit_can_fit_type_combo_box.currentText().encode('utf-8')
        fit_type_can = FitType.from_string(fit_type_can_as_string)
        is_can_polynomial = fit_type_can is FitType.Polynomial
        self.fit_can_polynomial_order_spin_box.setEnabled(is_can_polynomial)

    def _on_transmission_target_has_changed(self):
        use_monitor = self.transmission_target_combo_box.currentIndex() == 0
        use_roi = not use_monitor

        self.transmission_monitor_label.setEnabled(use_monitor)
        self.transmission_monitor_line_edit.setEnabled(use_monitor)
        self.transmission_mn_shift_label.setEnabled(use_monitor)
        self.transmission_mn_shift_line_edit.setEnabled(use_monitor)

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
        load_file(self.transmission_roi_files_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_transmission_roi_files)

    def get_transmission_mask_files(self):
        return str(self.transmission_mask_files_line_edit.text())

    def _on_load_transmission_mask_files(self):
        load_file(self.transmission_mask_files_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_transmission_mask_files)

    def get_moderator_file(self):
        return str(self.q_resolution_moderator_file_line_edit.text())

    def _on_load_moderator_file(self):
        load_file(self.q_resolution_moderator_file_line_edit, "*.*", self.__generic_settings,
                  self.__path_key, self.get_moderator_file)

    def get_mask_file(self):
        return str(self.mask_file_input_line_edit.text())

    def show_directory_manager(self):
        manageuserdirectories.ManageUserDirectories(self).exec_()

    def _on_load_mask_file(self):
        load_file(self.mask_file_input_line_edit, "*.*", self.__generic_settings,
                  self.__mask_file_input_path_key, self.get_mask_file)

    def _on_mask_file_add(self):
        self._call_settings_listeners(lambda listener: listener.on_mask_file_add())

    def _on_multi_period_selection(self):
        self._call_settings_listeners(
            lambda listener: listener.on_multi_period_selection(self.is_multi_period_view()))

    def _on_sample_geometry_selection(self):
        self._call_settings_listeners(lambda listener: listener.on_sample_geometry_selection(self.is_sample_geometry()))

    def _on_manage_directories(self):
        self._call_settings_listeners(lambda listener: listener.on_manage_directories())

    # ------------------------------------------------------------------------------------------------------------------
    # Elements which can be set and read by the model
    # ------------------------------------------------------------------------------------------------------------------
    def set_instrument_settings(self, instrument):
        if instrument:
            reduction_mode_list = get_reduction_mode_strings_for_gui(instrument)
            self.set_reduction_modes(reduction_mode_list)

            if instrument != SANSInstrument.NoInstrument:
                self._setup_add_runs_page()

    def update_gui_combo_box(self, value, expected_type, combo_box):
        # There are three types of values that can be passed:
        # Lists: we set the combo box to the values in the list
        # expected_type: we set the expected type
        # str (in the case of "Variable" Q rebin): We set the combo box to the text if it is an option
        gui_element = getattr(self, combo_box)
        if isinstance(value, list):
            gui_element.clear()
            for element in value:
                self._add_list_element_to_combo_box(gui_element=gui_element, element=element,
                                                    expected_type=expected_type)
        elif expected_type.has_member(value):
            self._set_enum_as_element_in_combo_box(gui_element=gui_element, element=value,
                                                   expected_type=expected_type)
        elif isinstance(value, str):
            index = gui_element.findText(value)
            if index != -1:
                gui_element.setCurrentIndex(index)
        else:
            raise RuntimeError("Expected an input of type {}, but got {}".format(expected_type, type(value)))

    def _add_list_element_to_combo_box(self, gui_element, element, expected_type=None):
        if expected_type is not None and isclass(element) and issubclass(element, expected_type):
            self._add_enum_as_element_in_combo_box(gui_element=gui_element, element=element,
                                                   expected_type=expected_type)
        else:
            gui_element.addItem(element)

    @staticmethod
    def _set_enum_as_element_in_combo_box(gui_element, element, expected_type):
        value_as_string = expected_type.to_string(element)
        index = gui_element.findText(value_as_string)
        if index != -1:
            gui_element.setCurrentIndex(index)

    def _add_enum_as_element_in_combo_box(self, gui_element, element, expected_type):
        value_as_string = expected_type.to_string(element)
        gui_element.addItem(value_as_string)

    def get_simple_line_edit_field(self, expected_type, line_edit):
        gui_element = getattr(self, line_edit)
        value_as_string = gui_element.text()
        return expected_type(value_as_string) if value_as_string else None

    def update_simple_line_edit_field(self, line_edit, value):
        if value:
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
        self._on_save_can_clicked.setChecked(value)

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
            return OutputMode.PublishToADS
        elif self.output_mode_file_radio_button.isChecked():
            return OutputMode.SaveToFile
        elif self.output_mode_both_radio_button.isChecked():
            return OutputMode.Both
        else:
            self.gui_logger.warning(
                "The output format was not specified. Defaulting to saving to memory only.")
            return OutputMode.PublishToADS

    @output_mode.setter
    def output_mode(self, value):
        self._check_output_mode(value)
        try:
            set_setting(self.__generic_settings, self.__output_mode_key, OutputMode.to_string(value))
        except RuntimeError:
            pass

    @property
    def compatibility_mode(self):
        return self.event_binning_group_box.isChecked()

    @compatibility_mode.setter
    def compatibility_mode(self, value):
        self.event_binning_group_box.setChecked(value)
        if not value:
            self._on_compatibility_unchecked()

    @property
    def instrument(self):
        return get_instrument_from_gui_selection(self.instrument_type.text())

    @instrument.setter
    def instrument(self, value):
        instrument_string = SANSInstrument.to_string(value)
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
        return self.get_simple_line_edit_field(line_edit="merged_scale_line_edit",
                                               expected_type=float)

    @merge_scale.setter
    def merge_scale(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_scale_line_edit", value=value)

    @property
    def merge_shift(self):
        return self.get_simple_line_edit_field(line_edit="merged_shift_line_edit",
                                               expected_type=float)

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
        return self.get_simple_line_edit_field(line_edit="merged_q_range_start_line_edit",
                                               expected_type=float)

    @merge_q_range_start.setter
    def merge_q_range_start(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_q_range_start_line_edit",
                                               value=value)

    @property
    def merge_q_range_stop(self):
        return self.get_simple_line_edit_field(line_edit="merged_q_range_stop_line_edit",
                                               expected_type=float)

    @merge_q_range_stop.setter
    def merge_q_range_stop(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_q_range_stop_line_edit",
                                               value=value)

    @property
    def merge_mask(self):
        return self.merge_mask_check_box.isChecked()

    @merge_mask.setter
    def merge_mask(self, value):
        self.merge_mask_check_box.setChecked(value)

    @property
    def merge_max(self):
        return self.get_simple_line_edit_field(line_edit="merged_max_line_edit",
                                               expected_type=float)

    @merge_max.setter
    def merge_max(self, value):
        if value is not None:
            self.update_simple_line_edit_field(line_edit="merged_max_line_edit", value=value)

    @property
    def merge_min(self):
        return self.get_simple_line_edit_field(line_edit="merged_min_line_edit",
                                               expected_type=float)

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
        step_type_as_string = self.wavelength_step_type_combo_box.currentText().encode('utf-8')
        return RangeStepType.from_string(step_type_as_string)

    @wavelength_step_type.setter
    def wavelength_step_type(self, value):
        self.update_gui_combo_box(value=value, expected_type=RangeStepType,
                                  combo_box="wavelength_step_type_combo_box")

    @property
    def wavelength_min(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_min_line_edit",
                                               expected_type=float)

    @wavelength_min.setter
    def wavelength_min(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_min_line_edit", value=value)

    @property
    def wavelength_max(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_max_line_edit",
                                               expected_type=float)

    @wavelength_max.setter
    def wavelength_max(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_max_line_edit", value=value)

    @property
    def wavelength_step(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_step_line_edit",
                                               expected_type=float)

    @wavelength_step.setter
    def wavelength_step(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_step_line_edit", value=value)

    @property
    def wavelength_range(self):
        return str(self.wavelength_slices_line_edit.text())

    @wavelength_range.setter
    def wavelength_range(self, value):
        self.wavelength_slices.setText(value)

    # ------------------------------------------------------------------------------------------------------------------
    # Scale Group
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def absolute_scale(self):
        return self.get_simple_line_edit_field(line_edit="absolute_scale_line_edit",
                                               expected_type=float)

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
        return self.get_simple_line_edit_field(line_edit="monitor_normalization_line_edit",
                                               expected_type=int)

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
        return self.get_simple_line_edit_field(line_edit="transmission_line_edit",
                                               expected_type=int)

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
        return self.get_simple_line_edit_field(line_edit="transmission_roi_files_line_edit",
                                               expected_type=str)

    @transmission_roi_files.setter
    def transmission_roi_files(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_roi_files_line_edit",
                                           value=value)

    @property
    def transmission_mask_files(self):
        return self.get_simple_line_edit_field(line_edit="transmission_mask_files_line_edit",
                                               expected_type=str)

    @transmission_mask_files.setter
    def transmission_mask_files(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_mask_files_line_edit",
                                           value=value)

    @property
    def transmission_radius(self):
        return self.get_simple_line_edit_field(line_edit="transmission_radius_line_edit",
                                               expected_type=float)

    @transmission_radius.setter
    def transmission_radius(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_radius_line_edit", value=value)

    @property
    def transmission_monitor(self):
        return self.get_simple_line_edit_field(line_edit="transmission_monitor_line_edit",
                                               expected_type=int)

    @transmission_monitor.setter
    def transmission_monitor(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_monitor_line_edit", value=value)

    @property
    def transmission_mn_shift(self):
        return self.get_simple_line_edit_field(line_edit="transmission_mn_shift_line_edit",
                                               expected_type=float)

    @transmission_mn_shift.setter
    def transmission_mn_shift(self, value):
        self.update_simple_line_edit_field(line_edit="transmission_mn_shift_line_edit", value=value)

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
        fit_type_as_string = self.fit_sample_fit_type_combo_box.currentText().encode('utf-8')
        return FitType.from_string(fit_type_as_string)

    @transmission_sample_fit_type.setter
    def transmission_sample_fit_type(self, value):
        if value is None:
            self.fit_sample_fit_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=FitType,
                                      combo_box="fit_sample_fit_type_combo_box")

    @property
    def transmission_can_fit_type(self):
        fit_type_as_string = self.fit_can_fit_type_combo_box.currentText().encode('utf-8')
        return FitType.from_string(fit_type_as_string)

    @transmission_can_fit_type.setter
    def transmission_can_fit_type(self, value):
        if value is None:
            self.fit_sample_fit_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=FitType,
                                      combo_box="fit_can_fit_type_combo_box")

    @staticmethod
    def _set_polynomial_order(spin_box, value):
        minimum = spin_box.minimum()
        maximum = spin_box.maximum()
        if value < minimum or value > maximum:
            raise ValueError("The value for the polynomial order {} has "
                             "to be in the range of {} and {}".format(value, minimum, maximum))
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
        return self.get_simple_line_edit_field(line_edit="fit_sample_wavelength_min_line_edit",
                                               expected_type=float)

    @transmission_sample_wavelength_min.setter
    def transmission_sample_wavelength_min(self, value):
        self.update_simple_line_edit_field(line_edit="fit_sample_wavelength_min_line_edit",
                                           value=value)

    @property
    def transmission_sample_wavelength_max(self):
        return self.get_simple_line_edit_field(line_edit="fit_sample_wavelength_max_line_edit",
                                               expected_type=float)

    @transmission_sample_wavelength_max.setter
    def transmission_sample_wavelength_max(self, value):
        self.update_simple_line_edit_field(line_edit="fit_sample_wavelength_max_line_edit",
                                           value=value)

    @property
    def transmission_can_wavelength_min(self):
        return self.get_simple_line_edit_field(line_edit="fit_can_wavelength_min_line_edit",
                                               expected_type=float)

    @transmission_can_wavelength_min.setter
    def transmission_can_wavelength_min(self, value):
        self.update_simple_line_edit_field(line_edit="fit_can_wavelength_min_line_edit",
                                           value=value)

    @property
    def transmission_can_wavelength_max(self):
        return self.get_simple_line_edit_field(line_edit="fit_can_wavelength_max_line_edit",
                                               expected_type=float)

    @transmission_can_wavelength_max.setter
    def transmission_can_wavelength_max(self, value):
        self.update_simple_line_edit_field(line_edit="fit_can_wavelength_max_line_edit",
                                           value=value)

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
        return self.get_simple_line_edit_field(line_edit="pixel_adjustment_det_1_line_edit",
                                               expected_type=str)

    @pixel_adjustment_det_1.setter
    def pixel_adjustment_det_1(self, value):
        self.update_simple_line_edit_field(line_edit="pixel_adjustment_det_1_line_edit",
                                           value=value)

    @property
    def pixel_adjustment_det_2(self):
        return self.get_simple_line_edit_field(line_edit="pixel_adjustment_det_2_line_edit",
                                               expected_type=str)

    @pixel_adjustment_det_2.setter
    def pixel_adjustment_det_2(self, value):
        self.update_simple_line_edit_field(line_edit="pixel_adjustment_det_2_line_edit",
                                           value=value)

    @property
    def wavelength_adjustment_det_1(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_adjustment_det_1_line_edit",
                                               expected_type=str)

    @wavelength_adjustment_det_1.setter
    def wavelength_adjustment_det_1(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_adjustment_det_1_line_edit",
                                           value=value)

    @property
    def wavelength_adjustment_det_2(self):
        return self.get_simple_line_edit_field(line_edit="wavelength_adjustment_det_2_line_edit",
                                               expected_type=str)

    @wavelength_adjustment_det_2.setter
    def wavelength_adjustment_det_2(self, value):
        self.update_simple_line_edit_field(line_edit="wavelength_adjustment_det_2_line_edit",
                                           value=value)

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
            value = value_as_string.encode('utf-8')
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
        q_1d_step_type_as_string = self.q_1d_step_type_combo_box.currentText().encode('utf-8')
        # Hedge for trying to read out
        try:
            return RangeStepType.from_string(q_1d_step_type_as_string)
        except RuntimeError:
            return None

    @q_1d_step_type.setter
    def q_1d_step_type(self, value):
        if value is None:
            # Set to the default
            self.q_1d_step_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=RangeStepType,
                                      combo_box="q_1d_step_type_combo_box")
            # Set the list
            if isinstance(value, list):
                gui_element = self.q_1d_step_type_combo_box
                gui_element.clear()
                value.append(self.VARIABLE)
                for element in value:
                    self._add_list_element_to_combo_box(gui_element=gui_element, element=element,
                                                        expected_type=RangeStepType)

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

    @property
    def q_xy_step_type(self):
        q_xy_step_type_as_string = self.q_xy_step_type_combo_box.currentText().encode('utf-8')
        try:
            return RangeStepType.from_string(q_xy_step_type_as_string)
        except RuntimeError:
            return None

    @q_xy_step_type.setter
    def q_xy_step_type(self, value):
        if value is None:
            # Set to the default
            self.q_xy_step_type_combo_box.setCurrentIndex(0)
        else:
            self.update_gui_combo_box(value=value, expected_type=RangeStepType,
                                      combo_box="q_xy_step_type_combo_box")

            if isinstance(value, list):
                gui_element = self.q_xy_step_type_combo_box
                gui_element.clear()
                for element in value:
                    self._add_list_element_to_combo_box(gui_element=gui_element, element=element,
                                                        expected_type=RangeStepType)

    # ------------------------------------------------------------------------------------------------------------------
    # Gravity
    # ------------------------------------------------------------------------------------------------------------------
    @property
    def gravity_extra_length(self):
        return self.get_simple_line_edit_field(line_edit="gravity_extra_length_line_edit",
                                               expected_type=float)

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
        return self._get_q_resolution_aperture(current_index=0,
                                               line_edit="q_resolution_source_a_line_edit")

    @q_resolution_source_a.setter
    def q_resolution_source_a(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_source_a_line_edit", value=value)

    @property
    def q_resolution_sample_a(self):
        return self._get_q_resolution_aperture(current_index=0,
                                               line_edit="q_resolution_sample_a_line_edit")

    @q_resolution_sample_a.setter
    def q_resolution_sample_a(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_sample_a_line_edit", value=value)

    @property
    def q_resolution_source_h(self):
        return self._get_q_resolution_aperture(current_index=1,
                                               line_edit="q_resolution_source_h_line_edit")

    @q_resolution_source_h.setter
    def q_resolution_source_h(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_source_h_line_edit", value=value)

    @property
    def q_resolution_sample_h(self):
        return self._get_q_resolution_aperture(current_index=1,
                                               line_edit="q_resolution_sample_h_line_edit")

    @q_resolution_sample_h.setter
    def q_resolution_sample_h(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_sample_h_line_edit", value=value)

    @property
    def q_resolution_source_w(self):
        return self._get_q_resolution_aperture(current_index=1,
                                               line_edit="q_resolution_source_w_line_edit")

    @q_resolution_source_w.setter
    def q_resolution_source_w(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_source_w_line_edit", value=value)

    @property
    def q_resolution_sample_w(self):
        return self._get_q_resolution_aperture(current_index=1,
                                               line_edit="q_resolution_sample_w_line_edit")

    @q_resolution_sample_w.setter
    def q_resolution_sample_w(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_sample_w_line_edit", value=value)

    @property
    def q_resolution_delta_r(self):
        return self.get_simple_line_edit_field(line_edit="q_resolution_delta_r_line_edit",
                                               expected_type=float)

    @q_resolution_delta_r.setter
    def q_resolution_delta_r(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_delta_r_line_edit", value=value)

    @property
    def q_resolution_collimation_length(self):
        return self.get_simple_line_edit_field(
            line_edit="q_resolution_collimation_length_line_edit",
            expected_type=float)

    @q_resolution_collimation_length.setter
    def q_resolution_collimation_length(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_collimation_length_line_edit",
                                           value=value)

    @property
    def q_resolution_moderator_file(self):
        return self.get_simple_line_edit_field(line_edit="q_resolution_moderator_file_line_edit",
                                               expected_type=str)

    @q_resolution_moderator_file.setter
    def q_resolution_moderator_file(self, value):
        self.update_simple_line_edit_field(line_edit="q_resolution_moderator_file_line_edit",
                                           value=value)

    @property
    def r_cut(self):
        return self.get_simple_line_edit_field(line_edit="r_cut_line_edit",
                                               expected_type=float)

    @r_cut.setter
    def r_cut(self, value):
        self.update_simple_line_edit_field(line_edit="r_cut_line_edit", value=value)

    @property
    def w_cut(self):
        return self.get_simple_line_edit_field(line_edit="w_cut_line_edit",
                                               expected_type=float)

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
        return self.get_simple_line_edit_field(line_edit="phi_limit_min_line_edit",
                                               expected_type=float)

    @phi_limit_min.setter
    def phi_limit_min(self, value):
        self.update_simple_line_edit_field(line_edit="phi_limit_min_line_edit", value=value)

    @property
    def phi_limit_max(self):
        return self.get_simple_line_edit_field(line_edit="phi_limit_max_line_edit",
                                               expected_type=float)

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
        return self.get_simple_line_edit_field(line_edit="radius_limit_min_line_edit",
                                               expected_type=float)

    @radius_limit_min.setter
    def radius_limit_min(self, value):
        self.update_simple_line_edit_field(line_edit="radius_limit_min_line_edit", value=value)

    @property
    def radius_limit_max(self):
        return self.get_simple_line_edit_field(line_edit="radius_limit_max_line_edit",
                                               expected_type=float)

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
        self.transmission_mn_shift_line_edit.setValidator(double_validator)

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
        self.q_xy_max_line_edit.setValidator(
            positive_double_validator)  # Yes, this should be positive!
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
        self.transmission_mn_shift_line_edit.setText("")
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
        self.q_xy_step_type_combo_box.setCurrentIndex(0)

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

    def get_cell(self, row, column, convert_to=None):
        row_location = self.row([row])
        value = self.data_processor_table.cellAt(row_location, column).contentText()
        return value if convert_to is None else convert_to(value)

    def set_cell(self, value, row, column):
        value_as_str = str(value)
        cell = self.data_processor_table.cellAt(row, column)
        cell.setContentText(value_as_str)
        self.data_processor_table.setCellAt(row, column, cell)

    def change_row_color(self, color, row):
        row_location = self.row([row])
        cell_data = self.data_processor_table.cellsAt(row_location)
        for index, cell in enumerate(cell_data):
            cell.setBackgroundColor(color)
            self.data_processor_table.setCellAt(row_location, index, cell)

    def set_row_tooltip(self, tool_tip, row):
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

    def add_row(self, value):
        value = [self.cell(x) for x in value]
        self.data_processor_table.appendChildRowOf(self.row([]), value)

    def remove_rows(self, rows):
        rows = [self.row([item]) for item in rows]
        self.data_processor_table.removeRows(rows)

    def insert_empty_row(self, row_index):
        self.data_processor_table.insertChildRowOf(self.row([]), row_index)

    def set_hinting_line_edit_for_column(self, column, hint_strategy):
        self.data_processor_table.setHintsForColumn(column, hint_strategy)

    def _run_python_code(self, text):
        """
        Re-emits 'runPytonScript' signal
        """
        execute_script(text)

    def hide_period_columns(self):
        self.multi_period_check_box.setChecked(False)
        for col in self.MULTI_PERIOD_COLUMNS:
            self.data_processor_table.hideColumn(col)

    def show_period_columns(self):
        self.multi_period_check_box.setChecked(True)

        for col in self.MULTI_PERIOD_COLUMNS:
            self.data_processor_table.showColumn(col)

    def show_geometry(self):
        self.data_processor_table.showColumn(15)
        self.data_processor_table.showColumn(16)
        self.data_processor_table.showColumn(17)

    def hide_geometry(self):
        self.data_processor_table.hideColumn(15)
        self.data_processor_table.hideColumn(16)
        self.data_processor_table.hideColumn(17)

    def closeEvent(self, event):
        for child in self.children():
            if isinstance(child, SANSSaveOtherDialog):
                child.done(0)
        super(QMainWindow, self).closeEvent(event)
