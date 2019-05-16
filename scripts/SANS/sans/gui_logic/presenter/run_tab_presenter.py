# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The run tab presenter.

This presenter is essentially the brain of the reduction gui. It controls other presenters and is mainly responsible
for presenting and generating the reduction settings.
"""

from __future__ import (absolute_import, division, print_function)

import copy
import csv
import os
import time
import traceback

from mantid.api import (FileFinder)
from mantid.kernel import Logger, ConfigService, ConfigPropertyObserver
from mantid.py3compat import csv_open_type

from sans.command_interface.batch_csv_file_parser import BatchCsvParser
from sans.common.constants import ALL_PERIODS
from sans.common.enums import (BatchReductionEntry, FitType, RangeStepType, RowState, SampleShape,
                               SaveType, SANSInstrument)
from sans.gui_logic.gui_common import (get_reduction_mode_strings_for_gui, get_string_for_gui_from_instrument,
                                       add_dir_to_datasearch, remove_dir_from_datasearch, SANSGuiPropertiesHandler)
from sans.gui_logic.models.batch_process_runner import BatchProcessRunner
from sans.gui_logic.models.beam_centre_model import BeamCentreModel
from sans.gui_logic.models.create_state import create_states
from sans.gui_logic.models.diagnostics_page_model import run_integral, create_state
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel, TableIndexModel
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from sans.gui_logic.presenter.masking_table_presenter import (MaskingTablePresenter)
from sans.gui_logic.presenter.save_other_presenter import SaveOtherPresenter
from sans.gui_logic.presenter.settings_diagnostic_presenter import (SettingsDiagnosticPresenter)
from sans.sans_batch import SANSCentreFinder
from sans.user_file.user_file_reader import UserFileReader

from ui.sans_isis import SANSSaveOtherWindow
from ui.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
from ui.sans_isis.work_handler import WorkHandler

from qtpy import PYQT4
IN_MANTIDPLOT = False
if PYQT4:
    try:
        from mantidplot import graph, newGraph
        IN_MANTIDPLOT = True
    except ImportError:
        pass
else:
    from mantidqt.plotting.functions import get_plot_fig

row_state_to_colour_mapping = {RowState.Unprocessed: '#FFFFFF', RowState.Processed: '#d0f4d0',
                               RowState.Error: '#accbff'}


def log_times(func):
    """
    Generic decorator to time the execution of the function and
    print it to the logger.
    """

    def run(*args, **kwargs):
        t0 = time.time()
        result = func(*args, **kwargs)
        t1 = time.time()
        time_taken = t1 - t0
        # args[0] is the self parameter
        args[0].sans_logger.information("The generation of all states took {}s".format(time_taken))
        return result

    return run


class SaveDirectoryObserver(ConfigPropertyObserver):
    def __init__(self, callback):
        super(SaveDirectoryObserver, self).__init__("defaultsave.directory")
        self.callback = callback

    def onPropertyValueChanged(self, new_value, old_value):
        self.callback(new_value)


class RunTabPresenter(object):
    class ConcreteRunTabListener(SANSDataProcessorGui.RunTabListener):
        def __init__(self, presenter):
            super(RunTabPresenter.ConcreteRunTabListener, self).__init__()
            self._presenter = presenter

        def on_user_file_load(self):
            self._presenter.on_user_file_load()

        def on_mask_file_add(self):
            self._presenter.on_mask_file_add()

        def on_batch_file_load(self):
            self._presenter.on_batch_file_load()

        def on_process_selected_clicked(self):
            self._presenter.on_process_selected_clicked()

        def on_process_all_clicked(self):
            self._presenter.on_process_all_clicked()

        def on_load_clicked(self):
            self._presenter.on_load_clicked()

        def on_export_table_clicked(self):
            self._presenter.on_export_table_clicked()

        def on_multi_period_selection(self, show_periods):
            self._presenter.on_multiperiod_changed(show_periods)

        def on_reduction_dimensionality_changed(self, is_1d):
            self._presenter.on_reduction_dimensionality_changed(is_1d)

        def on_output_mode_changed(self):
            self._presenter.on_output_mode_changed()

        def on_data_changed(self, row, column, new_value, old_value):
            self._presenter.on_data_changed(row, column, new_value, old_value)

        def on_manage_directories(self):
            self._presenter.on_manage_directories()

        def on_instrument_changed(self):
            self._presenter.on_instrument_changed()

        def on_row_inserted(self, index, row):
            self._presenter.on_row_inserted(index, row)

        def on_rows_removed(self, rows):
            self._presenter.on_rows_removed(rows)

        def on_copy_rows_requested(self):
            self._presenter.on_copy_rows_requested()

        def on_paste_rows_requested(self):
            self._presenter.on_paste_rows_requested()

        def on_insert_row(self):
            self._presenter.on_insert_row()

        def on_erase_rows(self):
            self._presenter.on_erase_rows()

        def on_cut_rows(self):
            self._presenter.on_cut_rows_requested()

        def on_save_other(self):
            self._presenter.on_save_other()

        def on_sample_geometry_selection(self, show_geometry):
            self._presenter.on_sample_geometry_view_changed(show_geometry)

    class ProcessListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(RunTabPresenter.ProcessListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error(error)

    def __init__(self, facility, view=None):
        self._facility = facility
        # Logger
        self.sans_logger = Logger("SANS")
        # Name of graph to output to
        self.output_graph = 'SANS-Latest'
        # For matplotlib continuous plotting
        self.output_fig = None
        self.progress = 0

        # Models that are being used by the presenter
        self._state_model = None
        self._table_model = TableModel()
        self._table_model.subscribe_to_model_changes(self)

        # Presenter needs to have a handle on the view since it delegates it
        self._view = None
        self.set_view(view)
        self._processing = False
        self.work_handler = WorkHandler()
        self.batch_process_runner = BatchProcessRunner(self.notify_progress,
                                                       self.on_processing_finished,
                                                       self.on_processing_error)

        # File information for the first input
        self._file_information = None
        self._clipboard = []

        # Settings diagnostic tab presenter
        self._settings_diagnostic_tab_presenter = SettingsDiagnosticPresenter(self)

        # Masking table presenter
        self._masking_table_presenter = MaskingTablePresenter(self)
        self._table_model.subscribe_to_model_changes(self._masking_table_presenter)

        # Beam centre presenter
        self._beam_centre_presenter = BeamCentrePresenter(self, WorkHandler, BeamCentreModel,
                                                          SANSCentreFinder)
        self._table_model.subscribe_to_model_changes(self._beam_centre_presenter)

        # Workspace Diagnostic page presenter
        self._workspace_diagnostic_presenter = DiagnosticsPagePresenter(self, WorkHandler,
                                                                        run_integral, create_state,
                                                                        self._facility)

        # Check save dir for display
        self._save_directory_observer = \
            SaveDirectoryObserver(self._handle_output_directory_changed)

    def _default_gui_setup(self):
        """
        Provides a default setup of the GUI. This is important for the initial start up, when the view is being set.
        """
        # Set the possible reduction modes
        reduction_mode_list = get_reduction_mode_strings_for_gui()
        self._view.set_reduction_modes(reduction_mode_list)

        # Set the step type options for wavelength
        range_step_types = [RangeStepType.to_string(RangeStepType.Lin),
                            RangeStepType.to_string(RangeStepType.Log),
                            RangeStepType.to_string(RangeStepType.RangeLog),
                            RangeStepType.to_string(RangeStepType.RangeLin)]
        self._view.wavelength_step_type = range_step_types

        # Set the geometry options. This needs to include the option to read the sample shape from file.
        sample_shape = ["Read from file",
                        SampleShape.Cylinder,
                        SampleShape.FlatPlate,
                        SampleShape.Disc]
        self._view.sample_shape = sample_shape

        # Set the q range
        self._view.q_1d_step_type = [RangeStepType.to_string(RangeStepType.Lin),
                                     RangeStepType.to_string(RangeStepType.Log)]
        self._view.q_xy_step_type = [RangeStepType.to_string(RangeStepType.Lin),
                                     RangeStepType.to_string(RangeStepType.Log)]

        # Set the fit options
        fit_types = [FitType.to_string(FitType.Linear),
                     FitType.to_string(FitType.Logarithmic),
                     FitType.to_string(FitType.Polynomial)]
        self._view.transmission_sample_fit_type = fit_types
        self._view.transmission_can_fit_type = fit_types

    def _handle_output_directory_changed(self, new_directory):
        """
        Update the gui to display the new save location for workspaces
        :param new_directory: string. Current save directory for files
        :return:
        """
        self._view.set_out_file_directory(new_directory)
        # Update add runs save location. We want distinct reduction save/add runs save locations,
        # but the add runs directory change when the main directory is, to avoid users having to
        # remember to update in two places.
        self._view.add_runs_presenter.handle_new_save_directory(new_directory)

    # ------------------------------------------------------------------------------------------------------------------
    # Table + Actions
    # ------------------------------------------------------------------------------------------------------------------
    def set_view(self, view):
        """
        Sets the view
        :param view: the view is the SANSDataProcessorGui. The presenter needs to access some of the API
        """
        if view is not None:
            self._view = view

            # Add a listener to the view
            listener = RunTabPresenter.ConcreteRunTabListener(self)
            self._view.add_listener(listener)

            # Default gui setup
            self._default_gui_setup()
            self._view.disable_process_buttons()

            # Set appropriate view for the state diagnostic tab presenter
            self._settings_diagnostic_tab_presenter.set_view(self._view.settings_diagnostic_tab)

            # Set appropriate view for the masking table presenter
            self._masking_table_presenter.set_view(self._view.masking_table)

            # Set the appropriate view for the beam centre presenter
            self._beam_centre_presenter.set_view(self._view.beam_centre)

            # Set the appropriate view for the diagnostic page
            self._workspace_diagnostic_presenter.set_view(self._view.diagnostic_page,
                                                          self._view.instrument)

            self._view.setup_layout()

            self._view.set_out_file_directory(ConfigService.Instance().getString("defaultsave.directory"))

            self._view.set_out_default_output_mode()
            self._view.set_out_default_save_can()

            self._view.set_hinting_line_edit_for_column(
                self._table_model.column_name_converter.index('sample_shape'),
                self._table_model.get_sample_shape_hint_strategy())

            self._view.set_hinting_line_edit_for_column(
                self._table_model.column_name_converter.index('options_column_model'),
                self._table_model.get_options_hint_strategy())

            self._view.gui_properties_handler = SANSGuiPropertiesHandler(
                                                    {"user_file": (self._view.set_out_default_user_file,
                                                                   str)},
                                                    line_edits={"user_file":
                                                                self._view.user_file_line_edit})

    def on_user_file_load(self):
        """
        Loads the user file. Populates the models and the view.
        """
        error_msg = "Loading of the user file failed"
        try:
            # 1. Get the user file path from the view
            user_file_path = self._view.get_user_file_path()

            if not user_file_path:
                return
            # 2. Get the full file path
            user_file_path = FileFinder.getFullPath(user_file_path)
            if not os.path.exists(user_file_path):
                raise RuntimeError(
                    "The user path {} does not exist. Make sure a valid user file path"
                    " has been specified.".format(user_file_path))
        except RuntimeError as path_error:
            # This exception block runs if user file does not exist
            self._on_user_file_load_failure(path_error, error_msg + " when finding file.")
        else:
            try:
                self._table_model.user_file = user_file_path
                # Clear out the current view
                self._view.reset_all_fields_to_default()

                # 3. Read and parse the user file
                user_file_reader = UserFileReader(user_file_path)
                user_file_items = user_file_reader.read_user_file()
            except (RuntimeError, ValueError) as e:
                # It is in this exception block that loading fails if the file is invalid (e.g. a csv)
                self._on_user_file_load_failure(e, error_msg + " when reading file.", use_error_name=True)
            else:
                try:
                    # 4. Populate the model
                    self._state_model = StateGuiModel(user_file_items)
                    # 5. Update the views.
                    self._update_view_from_state_model()
                    self._beam_centre_presenter.update_centre_positions(self._state_model)

                    self._beam_centre_presenter.on_update_rows()
                    self._masking_table_presenter.on_update_rows()
                    self._workspace_diagnostic_presenter.on_user_file_load(user_file_path)

                    # 6. Warning if user file did not contain a recognised instrument
                    if self._view.instrument == SANSInstrument.NoInstrument:
                        raise RuntimeError("User file did not contain a SANS Instrument.")

                except RuntimeError as instrument_e:
                    # This exception block runs if the user file does not contain an parsable instrument
                    self._on_user_file_load_failure(instrument_e, error_msg + " when reading instrument.")
                except Exception as other_error:
                    # If we don't catch all exceptions, SANS can fail to open if last loaded
                    # user file contains an error that would not otherwise be caught
                    traceback.print_exc()
                    self._on_user_file_load_failure(other_error, "Unknown error in loading user file.",
                                                    use_error_name=True)

    def _on_user_file_load_failure(self, e, message, use_error_name=False):
        self._setup_instrument_specific_settings(SANSInstrument.NoInstrument)
        self._view.instrument = SANSInstrument.NoInstrument
        self._view.on_user_file_load_failure()
        self.display_errors(e, message, use_error_name)

    def on_batch_file_load(self):
        """
        Loads a batch file and populates the batch table based on that.
        """
        try:
            # 1. Get the batch file from the view
            batch_file_path = self._view.get_batch_file_path()

            if not batch_file_path:
                return

            datasearch_dirs = ConfigService["datasearch.directories"]
            batch_file_directory, datasearch_dirs = add_dir_to_datasearch(batch_file_path, datasearch_dirs)
            ConfigService["datasearch.directories"] = datasearch_dirs

            if not os.path.exists(batch_file_path):
                raise RuntimeError(
                    "The batch file path {} does not exist. Make sure a valid batch file path"
                    " has been specified.".format(batch_file_path))

            self._table_model.batch_file = batch_file_path

            # 2. Read the batch file
            batch_file_parser = BatchCsvParser(batch_file_path)
            parsed_rows = batch_file_parser.parse_batch_file()

            # 3. Populate the table
            self._table_model.clear_table_entries()
            for index, row in enumerate(parsed_rows):
                self._add_row_to_table_model(row, index)
            self._table_model.remove_table_entries([len(parsed_rows)])
        except RuntimeError as e:
            if batch_file_directory:
                # Remove added directory from datasearch.directories
                ConfigService["datasearch.directories"] = remove_dir_from_datasearch(batch_file_directory, datasearch_dirs)

            self.sans_logger.error("Loading of the batch file failed. {}".format(str(e)))
            self.display_warning_box('Warning', 'Loading of the batch file failed', str(e))

    def _add_row_to_table_model(self, row, index):
        """
        Adds a row to the table
        """

        def get_string_entry(_tag, _row):
            _element = ""
            if _tag in _row:
                _element = _row[_tag]
            return _element

        def get_string_period(_tag):
            return "" if _tag == ALL_PERIODS else str(_tag)

        # 1. Pull out the entries
        sample_scatter = get_string_entry(BatchReductionEntry.SampleScatter, row)
        sample_scatter_period = get_string_period(
            get_string_entry(BatchReductionEntry.SampleScatterPeriod, row))
        sample_transmission = get_string_entry(BatchReductionEntry.SampleTransmission, row)
        sample_transmission_period = \
            get_string_period(get_string_entry(BatchReductionEntry.SampleTransmissionPeriod, row))
        sample_direct = get_string_entry(BatchReductionEntry.SampleDirect, row)
        sample_direct_period = get_string_period(
            get_string_entry(BatchReductionEntry.SampleDirectPeriod, row))
        can_scatter = get_string_entry(BatchReductionEntry.CanScatter, row)
        can_scatter_period = get_string_period(
            get_string_entry(BatchReductionEntry.CanScatterPeriod, row))
        can_transmission = get_string_entry(BatchReductionEntry.CanTransmission, row)
        can_transmission_period = get_string_period(
            get_string_entry(BatchReductionEntry.CanScatterPeriod, row))
        can_direct = get_string_entry(BatchReductionEntry.CanDirect, row)
        can_direct_period = get_string_period(
            get_string_entry(BatchReductionEntry.CanDirectPeriod, row))
        output_name = get_string_entry(BatchReductionEntry.Output, row)
        user_file = get_string_entry(BatchReductionEntry.UserFile, row)

        row_entry = [sample_scatter, sample_scatter_period, sample_transmission,
                     sample_transmission_period,
                     sample_direct, sample_direct_period, can_scatter, can_scatter_period,
                     can_transmission, can_transmission_period,
                     can_direct, can_direct_period,
                     output_name, user_file, '', '']

        table_index_model = TableIndexModel(*row_entry)

        self._table_model.add_table_entry(index, table_index_model)

    def on_update_rows(self):
        self.update_view_from_table_model()

    def update_view_from_table_model(self):
        self._view.clear_table()
        self._view.hide_period_columns()
        for row_index, row in enumerate(self._table_model._table_entries):
            row_entry = [str(x) for x in row.to_list()]
            self._view.add_row(row_entry)
            self._view.change_row_color(row_state_to_colour_mapping[row.row_state], row_index + 1)
            self._view.set_row_tooltip(row.tool_tip, row_index + 1)
            if row.isMultiPeriod():
                self._view.show_period_columns()
        self._view.remove_rows([0])
        self._view.clear_selection()

    def on_data_changed(self, row, column, new_value, old_value):
        self._table_model.update_table_entry(row, column, new_value)
        self._view.change_row_color(row_state_to_colour_mapping[RowState.Unprocessed], row)
        self._view.set_row_tooltip('', row)
        self._beam_centre_presenter.on_update_rows()
        self._masking_table_presenter.on_update_rows()

    def on_instrument_changed(self):
        self._setup_instrument_specific_settings()

    # ----------------------------------------------------------------------------------------------
    # Processing
    # ----------------------------------------------------------------------------------------------

    def _handle_get_states(self, rows):
        """
        Return the states for the supplied rows, calling on_processing_error for any errors
        which occur.
        """
        states, errors = self.get_states(row_index=rows)
        error_msg = "\n\n"
        for row, error in errors.items():
            self.on_processing_error(row, error)
            error_msg += "{}\n".format(error)
        return states, error_msg

    def _plot_graph(self):
        """
        Plot a graph if continuous output specified.
        """
        if self._view.plot_results:
            if IN_MANTIDPLOT:
                if not graph(self.output_graph):
                    newGraph(self.output_graph)
            elif not PYQT4:
                ax_properties = {'yscale': 'log',
                                 'xscale': 'log'}
                fig, _ = get_plot_fig(ax_properties=ax_properties, window_title=self.output_graph)
                fig.show()
                self.output_fig = fig

    def _set_progress_bar_min_max(self, min, max):
        """
        The progress of the progress bar is given by min / max
        :param min: Current value of the progress bar.
        :param max: The value at which the progress bar is full
        """
        setattr(self._view, 'progress_bar_value', min)
        setattr(self._view, 'progress_bar_maximum', max)

    def _process_rows(self, rows):
        """
        Processes a list of rows. Any errors cause the row to be coloured red.
        """
        error_msg = ""
        try:
            # Trip up early if output modes are invalid
            self._validate_output_modes()

            for row in rows:
                self._table_model.reset_row_state(row)
            self.update_view_from_table_model()

            self._view.disable_buttons()
            self._processing = True
            self.sans_logger.information("Starting processing of batch table.")

            states, error_msg = self._handle_get_states(rows)
            if not states:
                raise Exception("No states found")

            self._plot_graph()
            self.progress = 0
            self._set_progress_bar_min_max(self.progress, len(states))
            save_can = self._view.save_can

            # MantidPlot and Workbench have different approaches to plotting
            output_graph = self.output_graph if PYQT4 else self.output_fig
            self.batch_process_runner.process_states(states,
                                                     self._view.use_optimizations,
                                                     self._view.output_mode,
                                                     self._view.plot_results,
                                                     output_graph,
                                                     save_can)

        except Exception as e:
            self.on_processing_finished(None)
            self.sans_logger.error("Process halted due to: {}".format(str(e)))
            self.display_warning_box('Warning', 'Process halted', str(e) + error_msg)

    def on_reduction_dimensionality_changed(self, is_1d):
        """
        Unchecks and disabled canSAS output mode if switching to 2D reduction.
        Enabled canSAS if switching to 1D.
        :param is_1d: bool. If true then switching TO 1D reduction.
        """
        if not self._view.output_mode_memory_radio_button.isChecked():
            # If we're in memory mode, all file types should always be disabled
            if is_1d:
                self._view.can_sas_checkbox.setEnabled(True)
            else:
                if self._view.can_sas_checkbox.isChecked():
                    self._view.can_sas_checkbox.setChecked(False)
                    self.sans_logger.information("2D reductions are incompatible with canSAS output. "
                                                 "canSAS output has been unchecked.")
                self._view.can_sas_checkbox.setEnabled(False)

    def _validate_output_modes(self):
        """
        Check which output modes has been checked (memory, file, both), and which
        file types. If no file types have been selected and output mode is not memory,
        we want to raise an error here. (If we don't, an error will be raised on attempting
        to save after performing the reductions)
        """
        if (self._view.output_mode_file_radio_button.isChecked() or
                self._view.output_mode_both_radio_button.isChecked()):
            if self._view.save_types == [SaveType.NoType]:
                raise RuntimeError("You have selected an output mode which saves to file, "
                                   "but no file types have been selected.")

    def on_output_mode_changed(self):
        """
        When output mode changes, dis/enable file type buttons
        based on the output mode and reduction dimensionality
        """
        if self._view.output_mode_memory_radio_button.isChecked():
            # If in memory mode, disable all buttons regardless of dimension
            self._view.disable_file_type_buttons()
        else:
            self._view.nx_can_sas_checkbox.setEnabled(True)
            self._view.rkh_checkbox.setEnabled(True)
            if self._view.reduction_dimensionality_1D.isChecked():
                self._view.can_sas_checkbox.setEnabled(True)

    def on_process_all_clicked(self):
        """
        Process all entries in the table, regardless of selection.
        """
        all_rows = range(self._table_model.get_number_of_rows())
        all_rows = self._table_model.get_non_empty_rows(all_rows)
        if all_rows:
            self._process_rows(all_rows)

    def on_process_selected_clicked(self):
        """
        Process selected table entries.
        """
        selected_rows = self._view.get_selected_rows()
        selected_rows = self._table_model.get_non_empty_rows(selected_rows)
        if selected_rows:
            self._process_rows(selected_rows)

    def on_processing_error(self, row, error_msg):
        """
        An error occurs while processing the row with index row, error_msg is displayed as a
        tooltip on the row.
        """
        self.increment_progress()
        self._table_model.set_row_to_error(row, error_msg)
        self.update_view_from_table_model()

    def on_processing_finished(self, result):
        self._view.enable_buttons()
        self._processing = False

    def on_load_clicked(self):
        error_msg = "\n\n"
        try:
            self._view.disable_buttons()
            self._processing = True
            self.sans_logger.information("Starting load of batch table.")

            selected_rows = self._get_selected_rows()
            selected_rows = self._table_model.get_non_empty_rows(selected_rows)
            states, errors = self.get_states(row_index=selected_rows)

            for row, error in errors.items():
                self.on_processing_error(row, error)
                error_msg += "{}\n".format(error)

            if not states:
                self.on_processing_finished(None)
                return

            self.progress = 0
            setattr(self._view, 'progress_bar_value', self.progress)
            setattr(self._view, 'progress_bar_maximum', len(states))
            self.batch_process_runner.load_workspaces(states)
        except Exception as e:
            self._view.enable_buttons()
            self.sans_logger.error("Process halted due to: {}".format(str(e)))
            self.display_warning_box("Warning", "Process halted", str(e) + error_msg)

    @staticmethod
    def _get_filename_to_save(filename):
        if filename in (None, ''):
            return None

        if isinstance(filename, tuple):
            # Filenames returned as tuple of (filename, file ending) in qt5
            filename = filename[0]
            if filename in (None, ''):
                return None

        if filename[-4:] != '.csv':
            filename += '.csv'
        return filename

    def on_export_table_clicked(self):
        non_empty_rows = self.get_row_indices()
        if len(non_empty_rows) == 0:
            self.sans_logger.notice("Cannot export table as it is empty.")
            return

        try:
            self._view.disable_buttons()

            default_filename = self._table_model.batch_file
            filename = self.display_save_file_box("Save table as", default_filename, "*.csv")
            filename = self._get_filename_to_save(filename)
            if filename is not None:
                self.sans_logger.information("Starting export of table. Filename: {}".format(filename))
                with open(filename, csv_open_type) as outfile:
                    # Pass filewriting object rather than filename to make testing easier
                    writer = csv.writer(outfile)
                    self._export_table(writer, non_empty_rows)
                    self.sans_logger.information("Table exporting finished.")

            self._view.enable_buttons()
        except Exception as e:
            self._view.enable_buttons()
            self.sans_logger.error("Export halted due to : {}".format(str(e)))
            self.display_warning_box("Warning", "Export halted", str(e))

    def on_multiperiod_changed(self, show_periods):
        if show_periods:
            self._view.show_period_columns()
        else:
            self._view.hide_period_columns()

    def display_errors(self, error, context_msg, use_error_name=False):
        """
        Code for alerting the user to a caught error
        :param error: a caught exception
        :param context_msg: string. Text to explain what SANS was trying to do
                            when the error occurred. e.g. 'Loading of the user file failed'.
        :param use_error_name: bool. If True, append type of error (e.g. RuntimeError) to context_msg
        :return:
        """
        logger_msg = context_msg
        if use_error_name:
            logger_msg += " {}:".format(type(error).__name__)
        logger_msg += " {}"
        self.sans_logger.error(logger_msg.format(str(error)))
        self.display_warning_box('Warning', context_msg, str(error))

    def display_warning_box(self, title, text, detailed_text):
        self._view.display_message_box(title, text, detailed_text)

    def display_save_file_box(self, title, default_path, file_filter):
        filename = self._view.display_save_file_box(title, default_path, file_filter)
        return filename

    def notify_progress(self, row, out_shift_factors, out_scale_factors):
        self.increment_progress()
        if out_scale_factors and out_shift_factors:
            self._table_model.set_option(row, 'MergeScale', round(out_scale_factors[0], 3))
            self._table_model.set_option(row, 'MergeShift', round(out_shift_factors[0], 3))

        self._table_model.set_row_to_processed(row, '')

    def increment_progress(self):
        self.progress = self.progress + 1
        setattr(self._view, 'progress_bar_value', self.progress)

    # ----------------------------------------------------------------------------------------------
    # Row manipulation
    # ----------------------------------------------------------------------------------------------

    def num_rows(self):
        return self._table_model.get_number_of_rows()

    def on_row_inserted(self, index, row):
        """
        Insert a row at a selected point
        """
        row_table_index = TableIndexModel(*row)
        self._table_model.add_table_entry(index, row_table_index)

    def on_insert_row(self):
        """
        Add an empty row to the table after the first selected row (or at the end of the table
        if nothing is selected).
        """
        selected_rows = self._view.get_selected_rows()

        selected_row = selected_rows[0] + 1 if selected_rows else self.num_rows()
        empty_row = self._table_model.create_empty_row()
        self._table_model.add_table_entry(selected_row, empty_row)

    def on_erase_rows(self):
        """
        Make all selected rows empty.
        """
        selected_rows = self._view.get_selected_rows()
        empty_row = self._table_model.create_empty_row()
        for row in selected_rows:
            empty_row = TableModel.create_empty_row()
            self._table_model.replace_table_entries([row], [empty_row])

    def on_rows_removed(self, rows):
        """
        Remove rows from the table
        """
        self._table_model.remove_table_entries(rows)

    def on_copy_rows_requested(self):
        selected_rows = self._view.get_selected_rows()
        self._clipboard = []
        for row in selected_rows:
            data_from_table_model = self._table_model.get_table_entry(row).to_list()
            self._clipboard.append(data_from_table_model)

    def on_cut_rows_requested(self):
        self.on_copy_rows_requested()
        rows = self._view.get_selected_rows()
        self.on_rows_removed(rows)

    def on_paste_rows_requested(self):
        if self._clipboard:
            selected_rows = self._view.get_selected_rows()
            selected_rows = selected_rows if selected_rows else [self.num_rows()]
            replacement_table_index_models = [TableIndexModel(*x) for x in self._clipboard]
            self._table_model.replace_table_entries(selected_rows, replacement_table_index_models)

    def on_manage_directories(self):
        self._view.show_directory_manager()

    def on_sample_geometry_view_changed(self, show_geometry):
        if show_geometry:
            self._view.show_geometry()
        else:
            self._view.hide_geometry()

    def get_row_indices(self):
        """
        Gets the indices of row which are not empty.
        :return: a list of row indices.
        """
        row_indices_which_are_not_empty = []
        number_of_rows = self._table_model.get_number_of_rows()
        for row in range(number_of_rows):
            if not self.is_empty_row(row):
                row_indices_which_are_not_empty.append(row)
        return row_indices_which_are_not_empty

    def on_mask_file_add(self):
        """
        We get the added mask file name and add it to the list of masks
        """
        new_mask_file = self._view.get_mask_file()
        if not new_mask_file:
            return
        new_mask_file_full_path = FileFinder.getFullPath(new_mask_file)
        if not new_mask_file_full_path:
            return

        # Add the new mask file to state model
        mask_files = self._state_model.mask_files

        mask_files.append(new_mask_file)
        self._state_model.mask_files = mask_files

        # Make sure that the sub-presenters are up to date with this change
        self._masking_table_presenter.on_update_rows()
        self._settings_diagnostic_tab_presenter.on_update_rows()
        self._beam_centre_presenter.on_update_rows()

    def is_empty_row(self, row):
        """
        Checks if a row has no entries. These rows will be ignored.
        :param row: the row index
        :return: True if the row is empty.
        """
        return self._table_model.is_empty_row(row)

    def on_save_other(self):
        self.save_other_presenter = SaveOtherPresenter(parent_presenter=self)
        save_other_view = SANSSaveOtherWindow.SANSSaveOtherDialog(self._view)
        self.save_other_presenter.set_view(save_other_view)
        self.save_other_presenter.show()

    # def _validate_rows(self):
    #     """
    #     Validation of the rows. A minimal setup requires that ScatterSample is set.
    #     """
    #     # If SampleScatter is empty, then don't run the reduction.
    #     # We allow empty rows for now, since we cannot remove them from Python.
    #     number_of_rows = self._table_model.get_number_of_rows()
    #     for row in range(number_of_rows):
    #         if not self.is_empty_row(row):
    #             sample_scatter = self._view.get_cell(row, 0)
    #             if not sample_scatter:
    #                 raise RuntimeError("Row {} has not SampleScatter specified. Please correct this.".format(row))

    # ------------------------------------------------------------------------------------------------------------------
    # Controls
    # ------------------------------------------------------------------------------------------------------------------
    def disable_controls(self):
        """
        Disable all input fields and buttons during the execution of the reduction.
        """
        # TODO: think about enabling and disable some controls during reduction
        pass

    def enable_controls(self):
        """
        Enable all input fields and buttons after the execution has completed.
        """
        # TODO: think about enabling and disable some controls during reduction
        pass

    # ----------------------------------------------------------------------------------------------
    # Table Model and state population
    # ------------------------------------------------------------------------------------------------------------------
    def _get_selected_rows(self):
        selected_rows = self._view.get_selected_rows()
        selected_rows = selected_rows if selected_rows else range(self._table_model.get_number_of_rows())
        for row in selected_rows:
            self._table_model.reset_row_state(row)
        self.update_view_from_table_model()

        return selected_rows

    @log_times
    def get_states(self, row_index=None, file_lookup=True, suppress_warnings=False):
        """
        Gathers the state information for all rows.
        :param row_index: if a single row is selected, then only this row is returned,
                          else all the state for all rows is returned.
        :param suppress_warnings: bool. If true don't propagate errors.
                                  This variable is introduced to stop repeated errors
                                  when filling in a row in the table.
                                  This parameter is a temporary fix to the problem of errors being reported
                                  while data is still being input. A long-term fix is to reassess how frequently
                                  SANS calls get_states.
        :return: a list of states.
        """
        # 1. Update the state model
        state_model_with_view_update = self._get_state_model_with_view_update()
        # 2. Update the table model
        table_model = self._table_model
        # 3. Go through each row and construct a state object
        states, errors = None, None
        if table_model and state_model_with_view_update:
            states, errors = create_states(state_model_with_view_update, table_model,
                                           self._view.instrument,
                                           self._facility,
                                           row_index=row_index,
                                           file_lookup=file_lookup,
                                           user_file=self._view.get_user_file_path())

        if errors and not suppress_warnings:
            self.sans_logger.warning("Errors in getting states...")
            for _, v in errors.items():
                self.sans_logger.warning("{}".format(v))

        return states, errors

    def get_state_for_row(self, row_index, file_lookup=True, suppress_warnings=False):
        """
        Creates the state for a particular row.
        :param row_index: the row index
        :param suppress_warnings: bool. If True don't propagate errors from get_states.
                                  This parameter is a temporary fix to the problem of errors being reported
                                  while data is still being input. A long-term fix is to reassess how frequently
                                  SANS calls get_states.
        :return: a state if the index is valid and there is a state else None
        """
        states, errors = self.get_states(row_index=[row_index], file_lookup=file_lookup,
                                         suppress_warnings=suppress_warnings)
        if states is None:
            self.sans_logger.warning(
                "There does not seem to be data for a row {}.".format(row_index))
            return None

        if row_index in states:
            if states:
                return states[row_index]
        return None

    def _update_view_from_state_model(self):
        self._set_on_view("instrument")

        # Front tab view
        self._set_on_view("zero_error_free")
        self._set_on_view("save_types")
        self._set_on_view("compatibility_mode")
        self._set_on_view("merge_scale")
        self._set_on_view("merge_shift")
        self._set_on_view("merge_scale_fit")
        self._set_on_view("merge_shift_fit")
        self._set_on_view("merge_q_range_start")
        self._set_on_view("merge_q_range_stop")
        self._set_on_view("merge_max")
        self._set_on_view("merge_min")

        # Settings tab view
        self._set_on_view("reduction_dimensionality")
        self._set_on_view("reduction_mode")
        self._set_on_view("event_slices")
        self._set_on_view("event_binning")
        self._set_on_view("merge_mask")

        self._set_on_view("wavelength_step_type")
        self._set_on_view("wavelength_min")
        self._set_on_view("wavelength_max")
        self._set_on_view("wavelength_step")

        self._set_on_view("absolute_scale")
        self._set_on_view("z_offset")

        # Adjustment tab
        self._set_on_view("normalization_incident_monitor")
        self._set_on_view("normalization_interpolate")

        self._set_on_view("transmission_incident_monitor")
        self._set_on_view("transmission_interpolate")
        self._set_on_view("transmission_roi_files")
        self._set_on_view("transmission_mask_files")
        self._set_on_view("transmission_radius")
        self._set_on_view("transmission_monitor")
        self._set_on_view("transmission_mn_shift")

        self._set_on_view_transmission_fit()

        self._set_on_view("pixel_adjustment_det_1")
        self._set_on_view("pixel_adjustment_det_2")
        self._set_on_view("wavelength_adjustment_det_1")
        self._set_on_view("wavelength_adjustment_det_2")

        # Q tab
        self._set_on_view_q_rebin_string()
        self._set_on_view("q_xy_max")
        self._set_on_view("q_xy_step")
        self._set_on_view("q_xy_step_type")

        self._set_on_view("gravity_on_off")
        self._set_on_view("gravity_extra_length")

        self._set_on_view("use_q_resolution")
        self._set_on_view_q_resolution_aperture()
        self._set_on_view("q_resolution_delta_r")
        self._set_on_view("q_resolution_collimation_length")
        self._set_on_view("q_resolution_moderator_file")

        self._set_on_view("r_cut")
        self._set_on_view("w_cut")

        # Mask
        self._set_on_view("phi_limit_min")
        self._set_on_view("phi_limit_max")
        self._set_on_view("phi_limit_use_mirror")
        self._set_on_view("radius_limit_min")
        self._set_on_view("radius_limit_max")

    def _set_on_view_transmission_fit_sample_settings(self):
        # Set transmission_sample_use_fit
        fit_type = self._state_model.transmission_sample_fit_type
        use_fit = fit_type is not FitType.NoFit
        self._view.transmission_sample_use_fit = use_fit

        # Set the polynomial order for sample
        polynomial_order = self._state_model.transmission_sample_polynomial_order if fit_type is FitType.Polynomial else 2  # noqa
        self._view.transmission_sample_polynomial_order = polynomial_order

        # Set the fit type for the sample
        fit_type = fit_type if fit_type is not FitType.NoFit else FitType.Linear
        self._view.transmission_sample_fit_type = fit_type

        # Set the wavelength
        wavelength_min = self._state_model.transmission_sample_wavelength_min
        wavelength_max = self._state_model.transmission_sample_wavelength_max
        if wavelength_min and wavelength_max:
            self._view.transmission_sample_use_wavelength = True
            self._view.transmission_sample_wavelength_min = wavelength_min
            self._view.transmission_sample_wavelength_max = wavelength_max

    def _set_on_view_transmission_fit(self):
        # Steps for adding the transmission fit to the view
        # 1. Check if individual settings exist. If so then set the view to separate, else set them to both
        # 2. Apply the settings
        separate_settings = self._state_model.has_transmission_fit_got_separate_settings_for_sample_and_can()
        self._view.set_fit_selection(use_separate=separate_settings)

        if separate_settings:
            self._set_on_view_transmission_fit_sample_settings()

            # Set transmission_sample_can_fit
            fit_type_can = self._state_model.transmission_can_fit_type()
            use_can_fit = fit_type_can is FitType.NoFit
            self._view.transmission_can_use_fit = use_can_fit

            # Set the polynomial order for can
            polynomial_order_can = self._state_model.transmission_can_polynomial_order if fit_type_can is FitType.Polynomial else 2  # noqa
            self._view.transmission_can_polynomial_order = polynomial_order_can

            # Set the fit type for the can
            fit_type_can = fit_type_can if fit_type_can is not FitType.NoFit else FitType.Linear
            self.transmission_can_fit_type = fit_type_can

            # Set the wavelength
            wavelength_min = self._state_model.transmission_can_wavelength_min
            wavelength_max = self._state_model.transmission_can_wavelength_max
            if wavelength_min and wavelength_max:
                self._view.transmission_can_use_wavelength = True
                self._view.transmission_can_wavelength_min = wavelength_min
                self._view.transmission_can_wavelength_max = wavelength_max
        else:
            self._set_on_view_transmission_fit_sample_settings()

    def _set_on_view_q_resolution_aperture(self):
        self._set_on_view("q_resolution_source_a")
        self._set_on_view("q_resolution_sample_a")
        self._set_on_view("q_resolution_source_h")
        self._set_on_view("q_resolution_sample_h")
        self._set_on_view("q_resolution_source_w")
        self._set_on_view("q_resolution_sample_w")

        # If we have h1, h2, w1, and w2 selected then we want to select the rectangular aperture.
        is_rectangular = self._state_model.q_resolution_source_h and self._state_model.q_resolution_sample_h and \
                         self._state_model.q_resolution_source_w and self._state_model.q_resolution_sample_w  # noqa
        self._view.set_q_resolution_shape_to_rectangular(is_rectangular)

    def _set_on_view_q_rebin_string(self):
        """
        Maps the q_1d_rebin_string of the model to the q_1d_step and q_1d_step_type property of the view.
        """
        rebin_string = self._state_model.q_1d_rebin_string
        # Extract the min, max and step and step type from the rebin string
        elements = rebin_string.split(",")
        # If we have three elements then we want to set only the
        if len(elements) == 3:
            step_element = float(elements[1])
            step = abs(step_element)
            step_type = RangeStepType.Lin if step_element >= 0 else RangeStepType.Log

            # Set on the view
            self._view.q_1d_min_or_rebin_string = float(elements[0])
            self._view.q_1d_max = float(elements[2])
            self._view.q_1d_step = step
            self._view.q_1d_step_type = step_type
        else:
            # Set the rebin string
            self._view.q_1d_min_or_rebin_string = rebin_string
            self._view.q_1d_step_type = self._view.VARIABLE

    def _set_on_view(self, attribute_name):
        attribute = getattr(self._state_model, attribute_name)
        if attribute or isinstance(attribute,
                                   bool):  # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values. # noqa
            setattr(self._view, attribute_name, attribute)

    def _set_on_view_with_view(self, attribute_name, view):
        attribute = getattr(self._state_model, attribute_name)
        if attribute or isinstance(attribute,
                                   bool):  # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values. # noqa
            setattr(view, attribute_name, attribute)

    def _get_state_model_with_view_update(self):
        """
        Goes through all sub presenters and update the state model based on the views.

        Note that at the moment we have set up the view and the model such that the name of a property must be the same
        in the view and the model. This can be easily changed, but it also provides a good cohesion.
        """
        state_model = copy.deepcopy(self._state_model)

        # If we don't have a state model then return None
        if state_model is None:
            return state_model
        # Run tab view
        self._set_on_state_model("zero_error_free", state_model)
        self._set_on_state_model("save_types", state_model)
        self._set_on_state_model("compatibility_mode", state_model)
        self._set_on_state_model("event_slice_mode", state_model)
        self._set_on_state_model("merge_scale", state_model)
        self._set_on_state_model("merge_shift", state_model)
        self._set_on_state_model("merge_scale_fit", state_model)
        self._set_on_state_model("merge_shift_fit", state_model)
        self._set_on_state_model("merge_q_range_start", state_model)
        self._set_on_state_model("merge_q_range_stop", state_model)
        self._set_on_state_model("merge_mask", state_model)
        self._set_on_state_model("merge_max", state_model)
        self._set_on_state_model("merge_min", state_model)

        # Settings tab
        self._set_on_state_model("reduction_dimensionality", state_model)
        self._set_on_state_model("reduction_mode", state_model)
        self._set_on_state_model("event_slices", state_model)
        self._set_on_state_model("event_binning", state_model)

        self._set_on_state_model("wavelength_step_type", state_model)
        self._set_on_state_model("wavelength_min", state_model)
        self._set_on_state_model("wavelength_max", state_model)
        self._set_on_state_model("wavelength_step", state_model)
        self._set_on_state_model("wavelength_range", state_model)

        self._set_on_state_model("absolute_scale", state_model)
        self._set_on_state_model("z_offset", state_model)

        # Adjustment tab
        self._set_on_state_model("normalization_incident_monitor", state_model)
        self._set_on_state_model("normalization_interpolate", state_model)

        self._set_on_state_model("transmission_incident_monitor", state_model)
        self._set_on_state_model("transmission_interpolate", state_model)
        self._set_on_state_model("transmission_roi_files", state_model)
        self._set_on_state_model("transmission_mask_files", state_model)
        self._set_on_state_model("transmission_radius", state_model)
        self._set_on_state_model("transmission_monitor", state_model)
        self._set_on_state_model("transmission_mn_shift", state_model)

        self._set_on_state_model_transmission_fit(state_model)

        self._set_on_state_model("pixel_adjustment_det_1", state_model)
        self._set_on_state_model("pixel_adjustment_det_2", state_model)
        self._set_on_state_model("wavelength_adjustment_det_1", state_model)
        self._set_on_state_model("wavelength_adjustment_det_2", state_model)

        # Q tab
        self._set_on_state_model_q_1d_rebin_string(state_model)
        self._set_on_state_model("q_xy_max", state_model)
        self._set_on_state_model("q_xy_step", state_model)
        self._set_on_state_model("q_xy_step_type", state_model)

        self._set_on_state_model("gravity_on_off", state_model)
        self._set_on_state_model("gravity_extra_length", state_model)

        self._set_on_state_model("use_q_resolution", state_model)
        self._set_on_state_model("q_resolution_source_a", state_model)
        self._set_on_state_model("q_resolution_sample_a", state_model)
        self._set_on_state_model("q_resolution_source_h", state_model)
        self._set_on_state_model("q_resolution_sample_h", state_model)
        self._set_on_state_model("q_resolution_source_w", state_model)
        self._set_on_state_model("q_resolution_sample_w", state_model)
        self._set_on_state_model("q_resolution_delta_r", state_model)
        self._set_on_state_model("q_resolution_collimation_length", state_model)
        self._set_on_state_model("q_resolution_moderator_file", state_model)

        self._set_on_state_model("r_cut", state_model)
        self._set_on_state_model("w_cut", state_model)

        # Mask
        self._set_on_state_model("phi_limit_min", state_model)
        self._set_on_state_model("phi_limit_max", state_model)
        self._set_on_state_model("phi_limit_use_mirror", state_model)
        self._set_on_state_model("radius_limit_min", state_model)
        self._set_on_state_model("radius_limit_max", state_model)

        # Beam Centre
        self._beam_centre_presenter.set_on_state_model("lab_pos_1", state_model)
        self._beam_centre_presenter.set_on_state_model("lab_pos_2", state_model)

        return state_model

    def _set_on_state_model_transmission_fit(self, state_model):
        # Behaviour depends on the selection of the fit
        if self._view.use_same_transmission_fit_setting_for_sample_and_can():
            use_fit = self._view.transmission_sample_use_fit
            fit_type = self._view.transmission_sample_fit_type
            polynomial_order = self._view.transmission_sample_polynomial_order
            state_model.transmission_sample_fit_type = fit_type if use_fit else FitType.NoFit
            state_model.transmission_can_fit_type = fit_type if use_fit else FitType.NoFit
            state_model.transmission_sample_polynomial_order = polynomial_order
            state_model.transmission_can_polynomial_order = polynomial_order

            # Wavelength settings
            if self._view.transmission_sample_use_wavelength:
                wavelength_min = self._view.transmission_sample_wavelength_min
                wavelength_max = self._view.transmission_sample_wavelength_max
                state_model.transmission_sample_wavelength_min = wavelength_min
                state_model.transmission_sample_wavelength_max = wavelength_max
                state_model.transmission_can_wavelength_min = wavelength_min
                state_model.transmission_can_wavelength_max = wavelength_max
        else:
            # Sample
            use_fit_sample = self._view.transmission_sample_use_fit
            fit_type_sample = self._view.transmission_sample_fit_type
            polynomial_order_sample = self._view.transmission_sample_polynomial_order
            state_model.transmission_sample_fit_type = fit_type_sample if use_fit_sample else FitType.NoFit
            state_model.transmission_sample_polynomial_order = polynomial_order_sample

            # Wavelength settings
            if self._view.transmission_sample_use_wavelength:
                wavelength_min = self._view.transmission_sample_wavelength_min
                wavelength_max = self._view.transmission_sample_wavelength_max
                state_model.transmission_sample_wavelength_min = wavelength_min
                state_model.transmission_sample_wavelength_max = wavelength_max

            # Can
            use_fit_can = self._view.transmission_can_use_fit
            fit_type_can = self._view.transmission_can_fit_type
            polynomial_order_can = self._view.transmission_can_polynomial_order
            state_model.transmission_can_fit_type = fit_type_can if use_fit_can else FitType.NoFit
            state_model.transmission_can_polynomial_order = polynomial_order_can

            # Wavelength settings
            if self._view.transmission_can_use_wavelength:
                wavelength_min = self._view.transmission_can_wavelength_min
                wavelength_max = self._view.transmission_can_wavelength_max
                state_model.transmission_can_wavelength_min = wavelength_min
                state_model.transmission_can_wavelength_max = wavelength_max

    def _set_on_state_model_q_1d_rebin_string(self, state_model):
        q_1d_step_type = self._view.q_1d_step_type

        # If we are dealing with a simple rebin string then the step type is None
        if self._view.q_1d_step_type is None:
            state_model.q_1d_rebin_string = self._view.q_1d_min_or_rebin_string
        else:
            q_1d_min = self._view.q_1d_min_or_rebin_string
            q_1d_max = self._view.q_1d_max
            q_1d_step = self._view.q_1d_step
            if q_1d_min and q_1d_max and q_1d_step and q_1d_step_type:
                q_1d_rebin_string = str(q_1d_min) + ","
                q_1d_step_type_factor = -1. if q_1d_step_type is RangeStepType.Log else 1.
                q_1d_rebin_string += str(q_1d_step_type_factor * q_1d_step) + ","
                q_1d_rebin_string += str(q_1d_max)
                state_model.q_1d_rebin_string = q_1d_rebin_string

    def _set_on_state_model(self, attribute_name, state_model):
        attribute = getattr(self._view, attribute_name)
        if attribute is not None and attribute != '':
            setattr(state_model, attribute_name, attribute)

    def get_cell_value(self, row, column):
        return self._view.get_cell(row=row, column=self.table_index[column], convert_to=str)

    def _export_table(self, filewriter, rows):
        """
        Take the current table model, and create a comma delimited csv file
        :param filewriter: File object to be written to
        :param rows: list of indices for non-empty rows
        :return: Nothing
        """
        for row in rows:
            table_row = self._table_model.get_table_entry(row).to_batch_list()
            batch_file_row = self._create_batch_entry_from_row(table_row)
            filewriter.writerow(batch_file_row)

    @staticmethod
    def _create_batch_entry_from_row(row):
        batch_file_keywords = ["sample_sans",
                               "sample_trans",
                               "sample_direct_beam",
                               "can_sans",
                               "can_trans",
                               "can_direct_beam",
                               "output_as",
                               "user_file"]
        new_row = []
        for key, value in zip(batch_file_keywords, row):
            new_row.append(key)
            new_row.append(value)

        return new_row

    # ------------------------------------------------------------------------------------------------------------------
    # Settings
    # ------------------------------------------------------------------------------------------------------------------
    def _setup_instrument_specific_settings(self, instrument=None):
        if not instrument:
            instrument = self._view.instrument

        if instrument == SANSInstrument.NoInstrument:
            self._view.disable_process_buttons()
        else:
            instrument_string = get_string_for_gui_from_instrument(instrument)
            ConfigService["default.instrument"] = instrument_string
            self._view.enable_process_buttons()

        self._view.set_instrument_settings(instrument)
        self._beam_centre_presenter.on_update_instrument(instrument)
        self._workspace_diagnostic_presenter.set_instrument_settings(instrument)
