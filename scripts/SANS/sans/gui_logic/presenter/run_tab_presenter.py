# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
""" The run tab presenter.

This presenter is essentially the brain of the reduction gui. It controls other presenters and is mainly responsible
for presenting and generating the reduction settings.
"""
import os
import time
import traceback
from contextlib import contextmanager
from functools import wraps
from typing import Optional

from mantidqt.utils.observer_pattern import GenericObserver
from sans.user_file.toml_parsers.toml_v1_schema import TomlValidationError
from ui.sans_isis import SANSSaveOtherWindow
from ui.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
from ui.sans_isis.sans_gui_observable import SansGuiObservable

from mantid.api import FileFinder
from mantid.kernel import Logger, ConfigService, ConfigPropertyObserver
from sans.command_interface.batch_csv_parser import BatchCsvParser
from sans.common.enums import (
    ReductionMode,
    RangeStepType,
    RowState,
    SampleShape,
    SaveType,
    SANSInstrument,
    ReductionDimensionality,
    OutputMode,
)
from sans.gui_logic.gui_common import (
    add_dir_to_datasearch,
    get_reduction_mode_from_gui_selection,
    get_reduction_mode_strings_for_gui,
    get_string_for_gui_from_instrument,
    SANSGuiPropertiesHandler,
)
from sans.gui_logic.models.RowEntries import RowEntries
from sans.gui_logic.models.async_workers.sans_run_tab_async import SansRunTabAsync
from sans.gui_logic.models.create_state import create_states
from sans.gui_logic.models.file_loading import FileLoading, UserFileLoadException
from sans.gui_logic.models.run_tab_model import RunTabModel
from sans.gui_logic.models.settings_adjustment_model import SettingsAdjustmentModel
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel
from sans.gui_logic.presenter.Observers.run_tab_observers import RunTabObservers
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from sans.gui_logic.presenter.masking_table_presenter import MaskingTablePresenter
from sans.gui_logic.presenter.presenter_common import PresenterCommon
from sans.gui_logic.presenter.save_other_presenter import SaveOtherPresenter
from sans.gui_logic.presenter.settings_adjustment_presenter import SettingsAdjustmentPresenter
from sans.gui_logic.presenter.settings_diagnostic_presenter import SettingsDiagnosticPresenter
from sans.state.AllStates import AllStates
from mantid.plots.plotfunctions import get_plot_fig

row_state_to_colour_mapping = {RowState.UNPROCESSED: "#FFFFFF", RowState.PROCESSED: "#d0f4d0", RowState.ERROR: "#accbff"}


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


@contextmanager
def disable_buttons(presenter):
    presenter._view.disable_buttons()
    try:
        yield
    finally:
        presenter._view.enable_buttons()


@contextmanager
def disable_model_updates(presenter):
    method = presenter.update_model_from_view
    try:
        presenter.update_model_from_view = lambda: None
        yield
    finally:
        presenter.update_model_from_view = method


def disable_model_updates_decorator(f):
    @wraps(f)
    def wrapper(presenter, *args, **kwargs):
        with disable_model_updates(presenter):
            return f(presenter, *args, **kwargs)

    return wrapper


class SaveDirectoryObserver(ConfigPropertyObserver):
    def __init__(self, callback):
        super(SaveDirectoryObserver, self).__init__("defaultsave.directory")
        self.callback = callback

    def onPropertyValueChanged(self, new_value, old_value):
        self.callback(new_value)


class RunTabPresenter(PresenterCommon):
    DEFAULT_DECIMAL_PLACES_MM = 1

    class ConcreteRunTabListener(SANSDataProcessorGui.RunTabListener):
        def __init__(self, presenter):
            super(RunTabPresenter.ConcreteRunTabListener, self).__init__()
            self._presenter: RunTabPresenter = presenter

        def on_user_file_load(self):
            self._presenter.on_user_file_load()

        def on_mask_file_add(self):
            self._presenter.on_mask_file_add()

        def on_batch_file_load(self):
            self._presenter.on_batch_file_load()

        def on_reduction_mode_selection_has_changed(self, selection):
            self._presenter.on_reduction_mode_selection_has_changed(selection)

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

        def on_output_mode_changed(self):
            self._presenter.on_output_mode_changed()

        def on_data_changed(self, index, row):
            self._presenter.on_data_changed(index, row)

        def on_manage_directories(self):
            self._presenter.on_manage_directories()

        def on_instrument_changed(self):
            self._presenter.on_instrument_changed()

        def on_row_inserted(self):
            self._presenter.on_row_appended()

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

        def on_background_subtraction_selection(self, show_background):
            self._presenter.on_background_subtraction_view_changed(show_background)

        def on_field_edit(self):
            self._presenter.update_model_from_view()

    def __init__(self, facility, run_tab_model, model=None, table_model=None, view=None):
        # We don't have access to state model really at this point
        super(RunTabPresenter, self).__init__(view, None)

        self._facility = facility
        # Logger
        self.sans_logger = Logger("SANS")
        # Name of graph to output to
        self.output_graph = "SANS-Latest"
        # For matplotlib continuous plotting
        self.output_fig = None
        self.progress = 0

        # Models that are being used by the presenter
        self._model = model if model else StateGuiModel(all_states=AllStates())
        self._run_tab_model: RunTabModel = run_tab_model
        self._table_model = table_model if table_model else TableModel()
        self._table_model.subscribe_to_model_changes(self)

        self._processing = False
        self.batch_process_runner = SansRunTabAsync(self.notify_progress, self.on_processing_finished, self.on_processing_error)

        # File information for the first input
        self._file_information = None
        self._csv_parser = BatchCsvParser()

        self._setup_sub_presenters()

        # Presenter needs to have a handle on the view since it delegates it
        with disable_model_updates(self):
            self.set_view(view)

        # Check save dir for display
        self._save_directory_observer = SaveDirectoryObserver(self._handle_output_directory_changed)
        self._register_observers()

    def _setup_sub_presenters(self):
        # Holds a list of sub presenters which uses the CommonPresenter class to set views
        # models and call update methods applicable to all presenters
        # N.B. although this class implements the methods don't add it, as set_view will recurse
        self._common_sub_presenters = []

        # Settings diagnostic tab presenter
        self._settings_diagnostic_tab_presenter = SettingsDiagnosticPresenter(self)

        # Masking table presenter
        self._masking_table_presenter = MaskingTablePresenter(self)
        self._table_model.subscribe_to_model_changes(self._masking_table_presenter)

        # Beam centre presenter
        self._beam_centre_presenter = BeamCentrePresenter(self)
        self._table_model.subscribe_to_model_changes(self._beam_centre_presenter)

        # Workspace Diagnostic page presenter
        self._workspace_diagnostic_presenter = DiagnosticsPagePresenter(self, self._facility)
        # Adjustment Tab presenter
        self._settings_adjustment_presenter = SettingsAdjustmentPresenter(model=SettingsAdjustmentModel(), view=self._view)
        self._common_sub_presenters.append(self._settings_adjustment_presenter)

    def _register_observers(self):
        self._observers = RunTabObservers(
            reduction_dim=GenericObserver(callback=self.on_reduction_dimensionality_changed),
            save_options=GenericObserver(callback=self.on_save_options_change),
        )

        view_observable: SansGuiObservable = self._view.get_observable()
        view_observable.reduction_dim.add_subscriber(self._observers.reduction_dim)
        view_observable.save_options.add_subscriber(self._observers.save_options)

    def default_gui_setup(self):
        """
        Provides a default setup of the GUI. This is important for the initial start up, when the view is being set.
        """
        # Set the possible reduction modes
        reduction_mode_list = get_reduction_mode_strings_for_gui()
        self._view.set_reduction_modes(reduction_mode_list)

        # Set the step type options for wavelength
        range_step_types = [RangeStepType.LIN.value, RangeStepType.LOG.value, RangeStepType.RANGE_LOG.value, RangeStepType.RANGE_LIN.value]
        self._view.wavelength_step_type = range_step_types

        # Set the geometry options. This needs to include the option to read the sample shape from file.
        sample_shape = ["Read from file", SampleShape.CYLINDER, SampleShape.FLAT_PLATE, SampleShape.DISC]
        self._view.sample_shape = sample_shape

        # Set the q range
        self._view.q_1d_step_type = [RangeStepType.LIN.value, RangeStepType.LOG.value]

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
        if not view:
            self.sans_logger.warning("Empty view passed to set view at:")
            traceback.print_stack()
            return

        self._view: SANSDataProcessorGui = view

        listener = RunTabPresenter.ConcreteRunTabListener(self)
        self._view.add_listener(listener)

        self.default_gui_setup()
        self._view.disable_process_buttons()

        for presenter in self._common_sub_presenters:
            # TODO when the below presenters are converted to use a common presenter
            # use this for in loop
            presenter.set_view(view)
            presenter.default_gui_setup()

        # Set appropriate view for the state diagnostic tab presenter
        self._settings_diagnostic_tab_presenter.set_view(self._view.settings_diagnostic_tab)

        # Set appropriate view for the masking table presenter
        self._masking_table_presenter.set_view(self._view.masking_table)

        # Set the appropriate view for the beam centre presenter
        self._beam_centre_presenter.set_view(self._view.beam_centre)

        # Set the appropriate view for the diagnostic page
        self._workspace_diagnostic_presenter.set_view(self._view.diagnostic_page, self._view.instrument)

        self._view.setup_layout()

        self.hide_or_show_plot_results_checkbox_based_on_user_properties()

        self._view.set_out_file_directory(ConfigService.Instance().getString("defaultsave.directory"))

        self._view.set_out_default_output_mode()
        self._view.set_out_default_save_can()

        self._view.set_hinting_line_edit_for_column(
            self._table_model.column_name_converter.index("sample_shape"), self._table_model.get_sample_shape_hint_strategy()
        )

        self._view.set_hinting_line_edit_for_column(
            self._table_model.column_name_converter.index("options_column_model"), self._table_model.get_options_hint_strategy()
        )

        self._view.gui_properties_handler = SANSGuiPropertiesHandler(
            {"user_file": (self._view.set_out_default_user_file, str)}, line_edits={"user_file": self._view.user_file_line_edit}
        )
        if self._view.get_user_file_path() == "":
            self._view.gui_properties_handler.set_setting("user_file", "")

    @property
    def instrument(self):
        return self._model.instrument

    def on_save_options_change(self):
        selected_save_types = self._view.save_types
        self._run_tab_model.update_save_types(selected_save_types)

    def on_user_file_load(self):
        """
        Loads the user file. Populates the models and the view.
        """
        error_msg = "Loading of the user file failed"
        # 1. Get the user file path from the view
        user_file_path = self._view.get_user_file_path()

        if not user_file_path:
            return

        # 2. Get the full file path
        user_file_path = FileFinder.getFullPath(user_file_path)
        if not os.path.exists(user_file_path):
            path_error = f"The user path {user_file_path} does not exist. Make sure a valid user file path has been specified."
            self._on_user_file_load_failure(path_error, error_msg + " when finding file.")
            return

        # Clear out the current view
        self._view.reset_all_fields_to_default()
        try:
            # Always set the instrument to NoInstrument unless otherwise specified as our fallback
            user_file_items = FileLoading.load_user_file(file_path=user_file_path, file_information=self._file_information)
        except (UserFileLoadException, TomlValidationError) as e:
            # It is in this exception block that loading fails if the file is invalid (e.g. a csv)
            self._on_user_file_load_failure(e, error_msg + " when reading file.", use_error_name=True)
            return

        try:
            # 4. Populate the model and update sub-presenters
            self._model = StateGuiModel(user_file_items)
            self._model.user_file = user_file_path
            self._settings_adjustment_presenter.set_model(SettingsAdjustmentModel(all_states=user_file_items))
            # 5. Update the views.
            self.update_view_from_model()

            self._beam_centre_presenter.copy_centre_positions(self._model)
            self._beam_centre_presenter.update_centre_positions()
            self._beam_centre_presenter.set_meters_mode_enabled(self._view.instrument == SANSInstrument.LARMOR)

            self._masking_table_presenter.on_update_rows()
            self._workspace_diagnostic_presenter.on_user_file_load(user_file_path)

            # 6. Warning if user file did not contain a recognised instrument
            if self._view.instrument == SANSInstrument.NO_INSTRUMENT:
                raise RuntimeError("User file did not contain a SANS Instrument.")

        except RuntimeError as instrument_e:
            # This exception block runs if the user file does not contain an parsable instrument
            self._on_user_file_load_failure(instrument_e, error_msg + " when reading instrument.")
        except Exception as other_error:
            # If we don't catch all exceptions, SANS can fail to open if last loaded
            # user file contains an error that would not otherwise be caught
            traceback.print_exc()
            self._on_user_file_load_failure(other_error, "Unknown error in loading user file.", use_error_name=True)

    def hide_or_show_plot_results_checkbox_based_on_user_properties(self):
        """
        Hide the plot results checkbox if it has not been explicitly enabled in the user properties file.

        When performing merged reduction, if both the scale option and the plot result option are selected,
        it causes an issue. The agreed temporary fix is to hide the plot result checkbox by default since
        the functionality is rarely used. Users can enable the visibility of the plot result checkbox
        from their properties file if needed. This experiment will help us determine whether to
        permanently hide the checkbox or fix the underlying issue.
        """
        visibility = ConfigService.getString("sans.isis_sans.plotResults")
        self._view.set_plot_results_checkbox_visibility(visibility == "On")

    def _on_user_file_load_failure(self, e, message, use_error_name=False):
        self._setup_instrument_specific_settings(SANSInstrument.NO_INSTRUMENT)
        self._view.instrument = SANSInstrument.NO_INSTRUMENT
        self._view.on_user_file_load_failure()
        self.display_errors(e, message, use_error_name)

    def on_batch_file_load(self):
        """
        Loads a batch file and populates the batch table based on that.
        """
        # 1. Get the batch file from the view
        batch_file_path = self._view.get_batch_file_path()

        if not batch_file_path:
            return

        datasearch_dirs = ConfigService["datasearch.directories"]
        batch_file_directory, datasearch_dirs = add_dir_to_datasearch(batch_file_path, datasearch_dirs)
        ConfigService["datasearch.directories"] = datasearch_dirs

        try:
            if not os.path.exists(batch_file_path):
                raise RuntimeError(
                    "The batch file path {} does not exist. Make sure a valid batch file path"
                    " has been specified.".format(batch_file_path)
                )

            # 2. Read the batch file
            parsed_rows = self._csv_parser.parse_batch_file(batch_file_path)

            # 3. Populate the table
            self._table_model.clear_table_entries()
            self._add_multiple_rows_to_table_model(rows=parsed_rows)

            # 4. Set the batch file path in the model
            self._model.batch_file = batch_file_path

        except (RuntimeError, ValueError, SyntaxError, IOError, KeyError) as e:
            self.sans_logger.error("Loading of the batch file failed. {}".format(str(e)))
            self.display_warning_box("Warning", "Loading of the batch file failed", str(e))

        self.on_update_rows()

    def _add_multiple_rows_to_table_model(self, rows):
        self._table_model.add_multiple_table_entries(table_index_model_list=rows)

    def on_update_rows(self):
        self.update_view_from_table_model()
        self._get_current_file_information()

    def _get_current_file_information(self):
        # We shouldn't have to do this hack, but it solves the problem of trying to load in
        # file information before we need it when the user adds details to a row entry
        file_info = None
        for row in self._table_model.get_non_empty_rows():
            try:
                file_info = row.file_information
            except (ValueError, RuntimeError, OSError):
                pass

        if self._file_information != file_info:
            # Reload everything now our file information has updated
            # so we get our IPF / IDF content. This should be made more granular long-term
            self._file_information = file_info
            self.on_user_file_load()

    def update_view_from_table_model(self):
        self._view.clear_table()
        self._view.hide_period_columns()

        num_rows = self._table_model.get_number_of_rows()
        for row_index in range(num_rows):
            row = self._table_model.get_row(row_index)
            self._view.add_row(row)
            self._view.change_row_color(row_state_to_colour_mapping[row.state], row_index + 1)
            self._view.set_row_tooltip(row.tool_tip, row_index + 1)
            if row.is_multi_period():
                self._view.show_period_columns()
        self._view.remove_rows([0])
        self._view.clear_selection()

    def on_data_changed(self, index, row_entries):
        # TODO this is inefficient, but we should only be parsing the user input
        # when they hit a button, not as they type
        self._table_model.replace_table_entry(index, row_entries)
        self._view.change_row_color(row_state_to_colour_mapping[RowState.UNPROCESSED], index)
        self._view.set_row_tooltip("", index)
        self._beam_centre_presenter.on_update_rows()
        self._masking_table_presenter.on_update_rows()
        self.update_view_from_table_model()

    def on_instrument_changed(self):
        self._setup_instrument_specific_settings()
        self._beam_centre_presenter.on_update_instrument(self.instrument)

    # ----------------------------------------------------------------------------------------------
    # Processing
    # ----------------------------------------------------------------------------------------------

    def _plot_graph(self):
        """
        Plot a graph if continuous output specified.
        """
        if self._view.plot_results:
            ax_properties = {"yscale": "log", "xscale": "log"}
            fig, _ = get_plot_fig(ax_properties=ax_properties, window_title=self.output_graph)
            fig.show()
            self.output_fig = fig

    def _set_progress_bar(self, current, number_steps):
        """
        The progress of the progress bar is given by min / max
        :param current: Current value of the progress bar.
        :param number_steps: The value at which the progress bar is full
        """
        self.progress = current
        setattr(self._view, "progress_bar_value", current)
        setattr(self._view, "progress_bar_maximum", number_steps)

    def _process_rows(self, rows):
        """
        Processes a list of rows. Any errors cause the row to be coloured red.
        """
        self._set_progress_bar(current=0, number_steps=len(rows))

        # Trip up early if output modes are invalid
        try:
            self._validate_output_modes()
        except ValueError as e:
            return self.on_processing_error(str(e))

        row_index_pair = []

        for row in rows:
            row.reset_row_state()
            row_index_pair.append((row, self._table_model.get_row_index(row)))

        self.update_view_from_table_model()

        self._view.disable_buttons()
        self._processing = True
        self.sans_logger.information("Starting processing of batch table.")

        self._plot_graph()
        save_can = self._view.save_can

        self.batch_process_runner.process_states_on_thread(
            row_index_pair,
            self.get_states,
            self._view.use_optimizations,
            self._view.output_mode,
            self._view.plot_results,
            self.output_fig,
            save_can,
        )

    def on_reduction_dimensionality_changed(self):
        dimensionality = self._view.reduction_dimensionality
        self._run_tab_model.update_reduction_mode(dimensionality)
        # Update save options in case they've updated in the model
        save_opts = self._run_tab_model.get_save_types()
        self._view.save_types = save_opts
        self._check_if_SaveCanSAS1D_is_usable()

    def _validate_output_modes(self):
        """
        Check which output modes has been checked (memory, file, both), and which
        file types. If no file types have been selected and output mode is not memory,
        we want to raise an error here. (If we don't, an error will be raised on attempting
        to save after performing the reductions)
        """
        if self._view.output_mode_file_radio_button.isChecked() or self._view.output_mode_both_radio_button.isChecked():
            if self._view.save_types == [SaveType.NO_TYPE]:
                raise ValueError("You have selected an output mode which saves to file, " "but no file types have been selected.")

    def _check_if_SaveCanSAS1D_is_usable(self):
        """
        SaveCanSAS1D is only allowed to be used for 1D data. Check that we're in a file saving mode and in one dimension
        before enabling the checkbox.
        """
        if self._view.output_mode is OutputMode.BOTH or self._view.output_mode is OutputMode.SAVE_TO_FILE:
            if self._view.reduction_dimensionality is ReductionDimensionality.TWO_DIM:
                self._view.disable_can_sas_1D_button()
                return
            self._view.enable_can_sas_1D_button()

    def on_output_mode_changed(self):
        """
        When output mode changes, dis/enable file type buttons
        based on the output mode and reduction dimensionality
        """
        if self._view.output_mode_memory_radio_button.isChecked():
            # If in memory mode, disable all buttons regardless of dimension
            self._view.disable_file_type_buttons()
        else:
            self._view.enable_file_type_buttons()
            self._check_if_SaveCanSAS1D_is_usable()

    def on_process_all_clicked(self):
        """
        Process all entries in the table, regardless of selection.
        """
        to_process = self._table_model.get_non_empty_rows()
        if to_process:
            self._process_rows(to_process)

    def _get_selected_non_empty_rows(self):
        selected_rows = self._view.get_selected_rows()
        to_process = [self._table_model.get_row(i) for i in selected_rows]
        return [row for row in to_process if not row.is_empty()]

    def on_process_selected_clicked(self):
        """
        Process selected table entries.
        """
        to_process = self._get_selected_non_empty_rows()

        if to_process:
            self._process_rows(to_process)

    def on_processing_error(self, error_msg):
        """
        An error occurs while processing the row with index row, error_msg is displayed as a
        tooltip on the row.
        """
        self._view.enable_buttons()
        self._processing = False
        self.update_view_from_table_model()
        self.sans_logger.error("Process halted due to: {}".format(str(error_msg)))
        self.display_warning_box("Warning", "Process halted", str(error_msg))

    def on_processing_finished(self):
        self._view.enable_buttons()
        self._processing = False
        self.update_view_from_table_model()

    def on_load_clicked(self):
        selected_rows = self._get_selected_non_empty_rows()
        if len(selected_rows) == 0:
            # Try to process all rows
            selected_rows = self._table_model.get_non_empty_rows()

        if len(selected_rows) == 0:
            return

        self._view.disable_buttons()
        self._processing = True
        self.sans_logger.information("Starting load of batch table.")

        row_index_pair = []
        for row in selected_rows:
            row.reset_row_state()
            row_index_pair.append((row, self._table_model.get_row_index(row)))

        self._set_progress_bar(current=0, number_steps=len(selected_rows))
        self.batch_process_runner.load_workspaces_on_thread(row_index_pairs=row_index_pair, get_states_func=self.get_states)

    @staticmethod
    def _get_filename_to_save(filename):
        if filename in (None, ""):
            return None

        if isinstance(filename, tuple):
            # Filenames returned as tuple of (filename, file ending) in qt5
            filename = filename[0]
            if filename in (None, ""):
                return None

        if filename[-4:] != ".csv":
            filename += ".csv"
        return filename

    def on_export_table_clicked(self):
        non_empty_rows = self._table_model.get_non_empty_rows()
        if len(non_empty_rows) == 0:
            self.sans_logger.warning("Cannot export table as it is empty.")
            return

        with disable_buttons(self):
            default_filename = self._model.batch_file
            filename = self.display_save_file_box("Save table as", default_filename, "*.csv")
            filename = self._get_filename_to_save(filename)

            if not filename:
                return
            self.sans_logger.information("Starting export of table. Filename: {}".format(filename))
            try:
                self._csv_parser.save_batch_file(rows=non_empty_rows, file_path=filename)
            except (PermissionError, IOError) as e:
                self.display_errors(error=e, context_msg="Failed to save the .csv file.", use_error_name=True)
            self.sans_logger.information("Table exporting finished.")

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
        self.display_warning_box("Warning", context_msg, str(error))

    def display_warning_box(self, title, text, detailed_text):
        self._view.display_message_box(title, text, detailed_text)

    def display_save_file_box(self, title, default_path, file_filter):
        filename = self._view.display_save_file_box(title, default_path, file_filter)
        return filename

    def notify_progress(self, row_index, out_shift_factors, out_scale_factors):
        self.progress += 1
        setattr(self._view, "progress_bar_value", self.progress)

        row_entry = self._table_model.get_row(row_index)

        if out_scale_factors and out_shift_factors:
            row_entry.options.set_developer_option("MergeScale", round(out_scale_factors[0], 3))
            row_entry.options.set_developer_option("MergeShift", round(out_shift_factors[0], 3))

        row_entry.state = RowState.PROCESSED
        row_entry.tool_tip = None
        self.update_view_from_table_model()

    def increment_progress(self):
        self.progress += 1
        setattr(self._view, "progress_bar_value", self.progress)

    # ----------------------------------------------------------------------------------------------
    # Row manipulation
    # ----------------------------------------------------------------------------------------------

    def num_rows(self):
        return self._table_model.get_number_of_rows()

    def on_row_appended(self):
        """
        Insert a row at a selected point
        """
        self._table_model.append_table_entry(RowEntries())
        self.update_view_from_table_model()

    def on_insert_row(self):
        """
        Add an empty row to the table after the first selected row (or at the end of the table
        if nothing is selected).
        """
        selected_rows = self._view.get_selected_rows()

        empty_row = RowEntries()
        if len(selected_rows) >= 1:
            self._table_model.insert_row_at(row_index=selected_rows[0] + 1, row_entry=empty_row)
        else:
            self._table_model.append_table_entry(empty_row)

        self.update_view_from_table_model()

    def on_erase_rows(self):
        """
        Make all selected rows empty.
        """
        selected_rows = self._view.get_selected_rows()
        if not selected_rows or len(selected_rows) == self._table_model.get_number_of_rows():
            self._table_model.clear_table_entries()
        else:
            for row in selected_rows:
                empty_row = RowEntries()
                self._table_model.replace_table_entries([row], [empty_row])
        self.update_view_from_table_model()

    def on_rows_removed(self, row_indices):
        """
        Remove rows from the table
        """
        self._table_model.remove_table_entries(row_indices)
        self.update_view_from_table_model()

    def on_copy_rows_requested(self):
        self._table_model.copy_rows(self._view.get_selected_rows())

    def on_cut_rows_requested(self):
        self._table_model.cut_rows(self._view.get_selected_rows())
        self.update_view_from_table_model()

    def on_paste_rows_requested(self):
        self._table_model.paste_rows(self._view.get_selected_rows())
        self.update_view_from_table_model()

    def on_manage_directories(self):
        self._view.show_directory_manager()

    def on_sample_geometry_view_changed(self, show_geometry):
        if show_geometry:
            self._view.show_geometry()
        else:
            self._view.hide_geometry()

    def on_background_subtraction_view_changed(self, show_background):
        if show_background:
            self._view.show_background_subtraction()
        else:
            self._view.hide_background_subtraction()

    def get_row_indices(self):
        """
        Gets the indices of row which are not empty.
        :return: a list of row indices.
        """
        row_indices_which_are_not_empty = []
        rows = self._table_model.get_all_rows()
        for i, row in enumerate(rows):
            if not row.is_empty():
                row_indices_which_are_not_empty.append(i)
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
        mask_files = self._model.mask_files

        mask_files.append(new_mask_file)
        self._model.mask_files = mask_files

        # Make sure that the sub-presenters are up to date with this change
        self._masking_table_presenter.on_update_rows()
        self._settings_diagnostic_tab_presenter.on_update_rows()
        self._beam_centre_presenter.on_update_rows()

    def on_save_other(self):
        self.save_other_presenter = SaveOtherPresenter(parent_presenter=self)
        save_other_view = SANSSaveOtherWindow.SANSSaveOtherDialog(self._view)
        self.save_other_presenter.set_view(save_other_view)
        self.save_other_presenter.show()

    def on_reduction_mode_selection_has_changed(self, selection):
        if not selection:
            return
        mode = get_reduction_mode_from_gui_selection(selection)
        if mode == ReductionMode.HAB:
            self._beam_centre_presenter.update_front_selected()
        elif mode == ReductionMode.LAB:
            self._beam_centre_presenter.update_rear_selected()
        else:
            self._beam_centre_presenter.update_all_selected()

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
        return selected_rows

    @log_times
    def get_states(self, row_entries=None, file_lookup=True, suppress_warnings=False):
        """
        Gathers the state information for all rows.
        :param row_entries: if a single row is selected, then only this row is returned,
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
        state_model_with_view_update = self.update_model_from_view()
        # 2. Update the table model
        table_model = self._table_model
        # 3. Go through each row and construct a state object
        states, errors = None, None
        if table_model and state_model_with_view_update:
            states, errors = create_states(
                state_model_with_view_update, facility=self._facility, row_entries=row_entries, file_lookup=file_lookup
            )

        if errors and not suppress_warnings:
            self.sans_logger.warning("Errors in getting states...")
            for _, v in errors.items():
                self.sans_logger.warning("{}".format(v))

        return states, errors

    def get_row(self, row_index):
        return self._table_model.get_row(index=row_index)

    def get_state_for_row(self, row_index, file_lookup=True, suppress_warnings=False) -> Optional[AllStates]:
        """
        Creates the state for a particular row.
        :param row_index: the row index
        :param suppress_warnings: bool. If True don't propagate errors from get_states.
                                  This parameter is a temporary fix to the problem of errors being reported
                                  while data is still being input. A long-term fix is to reassess how frequently
                                  SANS calls get_states.
        :return: a state if the index is valid and there is a state else None
        """
        row_entry = self._table_model.get_row(row_index)
        states, errors = self.get_states(row_entries=[row_entry], file_lookup=file_lookup, suppress_warnings=suppress_warnings)
        if not states:
            if not suppress_warnings:
                self.sans_logger.warning("There does not seem to be data for a row {}.".format(row_index))
            return None

        return states[row_entry].all_states

    @disable_model_updates_decorator
    def update_view_from_model(self):
        self.sans_logger.debug("Updating SANS View from Model")
        self._set_on_view("instrument")

        for presenter in self._common_sub_presenters:
            presenter.update_view_from_model()

        # Front tab view
        self._view.save_types = self._run_tab_model.get_save_types()
        self._set_on_view("zero_error_free")
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
        self._set_on_view("wavelength_range")

        self._set_on_view("absolute_scale")
        self._set_on_view("z_offset", self.DEFAULT_DECIMAL_PLACES_MM)

        # Q tab
        self._set_on_view_q_rebin_string()
        self._set_on_view("q_xy_max")
        self._set_on_view("q_xy_step")

        self._set_on_view("gravity_on_off")
        self._set_on_view("gravity_extra_length")

        self._set_on_view("use_q_resolution")
        self._set_on_view_q_resolution_aperture()
        self._set_on_view("q_resolution_delta_r", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("q_resolution_collimation_length")
        self._set_on_view("q_resolution_moderator_file")

        self._set_on_view("r_cut", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("w_cut")

        # Mask
        self._set_on_view("phi_limit_min")
        self._set_on_view("phi_limit_max")
        self._set_on_view("phi_limit_use_mirror")
        self._set_on_view("radius_limit_min", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("radius_limit_max", self.DEFAULT_DECIMAL_PLACES_MM)

        # User file and batch file
        self._set_on_view("user_file")
        self._set_on_view("batch_file")

    def _set_on_view_q_resolution_aperture(self):
        self._set_on_view("q_resolution_source_a", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("q_resolution_sample_a", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("q_resolution_source_h", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("q_resolution_sample_h", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("q_resolution_source_w", self.DEFAULT_DECIMAL_PLACES_MM)
        self._set_on_view("q_resolution_sample_w", self.DEFAULT_DECIMAL_PLACES_MM)

        # If we have h1, h2, w1, and w2 selected then we want to select the rectangular aperture.
        is_rectangular = (
            self._model.q_resolution_source_h
            and self._model.q_resolution_sample_h
            and self._model.q_resolution_source_w
            and self._model.q_resolution_sample_w
        )
        self._view.set_q_resolution_shape_to_rectangular(is_rectangular)

    def _set_on_view_q_rebin_string(self):
        """
        Maps the q_1d_rebin_string of the model to the q_1d_step and q_1d_step_type property of the view.
        """
        rebin_string = self._model.q_1d_rebin_string
        # Extract the min, max and step and step type from the rebin string
        elements = rebin_string.split(",")
        # If we have three elements then we want to set only the
        if len(elements) == 3:
            step_element = float(elements[1])
            step = abs(step_element)
            step_type = RangeStepType.LIN if step_element >= 0 else RangeStepType.LOG

            # Set on the view
            self._view.q_1d_min_or_rebin_string = float(elements[0])
            self._view.q_1d_max = float(elements[2])
            self._view.q_1d_step = step
            self._view.q_1d_step_type = step_type
        else:
            # Set the rebin string
            self._view.q_1d_min_or_rebin_string = rebin_string
            self._view.q_1d_step_type = self._view.VARIABLE

    def update_model_from_view(self):
        """
        Goes through all sub presenters and update the state model based on the views.

        Note that at the moment we have set up the view and the model such that the name of a property must be the same
        in the view and the model. This can be easily changed, but it also provides a good cohesion.
        """
        self.sans_logger.debug("Updating SANS Model from View")
        state_model = self._model

        # If we don't have a state model then return None
        if state_model is None:
            return state_model

        for presenter in self._common_sub_presenters:
            presenter.update_model_from_view()

        state_model.save_types = self._run_tab_model.get_save_types().to_all_states()

        try:
            # Run tab view
            self._set_on_custom_model("zero_error_free", state_model)
            self._set_on_custom_model("compatibility_mode", state_model)
            self._set_on_custom_model("event_slice_optimisation", state_model)
            self._set_on_custom_model("merge_scale", state_model)
            self._set_on_custom_model("merge_shift", state_model)
            self._set_on_custom_model("merge_scale_fit", state_model)
            self._set_on_custom_model("merge_shift_fit", state_model)
            self._set_on_custom_model("merge_q_range_start", state_model)
            self._set_on_custom_model("merge_q_range_stop", state_model)
            self._set_on_custom_model("merge_mask", state_model)
            self._set_on_custom_model("merge_max", state_model)
            self._set_on_custom_model("merge_min", state_model)

            # Settings tab
            self._set_on_custom_model("reduction_dimensionality", state_model)
            self._set_on_custom_model("reduction_mode", state_model)
            self._set_on_custom_model("event_slices", state_model)
            self._set_on_custom_model("event_binning", state_model)

            self._set_on_custom_model("wavelength_min", state_model)
            self._set_on_custom_model("wavelength_max", state_model)
            self._set_on_custom_model("wavelength_range", state_model)
            self._set_on_custom_model("wavelength_step", state_model)
            self._set_on_custom_model("wavelength_step_type", state_model)

            self._set_on_custom_model("absolute_scale", state_model)
            self._set_on_custom_model("z_offset", state_model)

            # Q tab
            self._set_on_state_model_q_1d_rebin_string(state_model)
            self._set_on_custom_model("q_xy_max", state_model)
            self._set_on_custom_model("q_xy_step", state_model)

            self._set_on_custom_model("gravity_on_off", state_model)
            self._set_on_custom_model("gravity_extra_length", state_model)

            self._set_on_custom_model("use_q_resolution", state_model)
            self._set_on_custom_model("q_resolution_source_a", state_model)
            self._set_on_custom_model("q_resolution_sample_a", state_model)
            self._set_on_custom_model("q_resolution_source_h", state_model)
            self._set_on_custom_model("q_resolution_sample_h", state_model)
            self._set_on_custom_model("q_resolution_source_w", state_model)
            self._set_on_custom_model("q_resolution_sample_w", state_model)
            self._set_on_custom_model("q_resolution_delta_r", state_model)
            self._set_on_custom_model("q_resolution_collimation_length", state_model)
            self._set_on_custom_model("q_resolution_moderator_file", state_model)

            self._set_on_custom_model("r_cut", state_model)
            self._set_on_custom_model("w_cut", state_model)

            # Mask
            self._set_on_custom_model("phi_limit_min", state_model)
            self._set_on_custom_model("phi_limit_max", state_model)
            self._set_on_custom_model("phi_limit_use_mirror", state_model)
            self._set_on_custom_model("radius_limit_min", state_model)
            self._set_on_custom_model("radius_limit_max", state_model)

            # User file and batch file
            self._set_on_custom_model("user_file", state_model)
            self._set_on_custom_model("batch_file", state_model)

            # Beam Centre
            self._beam_centre_presenter.set_on_state_model("rear_pos_1", state_model)
            self._beam_centre_presenter.set_on_state_model("rear_pos_2", state_model)
            self._beam_centre_presenter.set_on_state_model("front_pos_1", state_model)
            self._beam_centre_presenter.set_on_state_model("front_pos_2", state_model)
        except (RuntimeError, ValueError) as e:
            self.display_warning_box(title="Invalid Settings Entered", text=str(e), detailed_text=str(e))

        return state_model

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
                q_1d_step_type_factor = -1.0 if q_1d_step_type is RangeStepType.LOG else 1.0
                q_1d_rebin_string += str(q_1d_step_type_factor * q_1d_step) + ","
                q_1d_rebin_string += str(q_1d_max)
                state_model.q_1d_rebin_string = q_1d_rebin_string

    # ------------------------------------------------------------------------------------------------------------------
    # Settings
    # ------------------------------------------------------------------------------------------------------------------
    def _setup_instrument_specific_settings(self, instrument=None):
        if ConfigService["default.facility"] != self._facility.value:
            ConfigService["default.facility"] = self._facility.value
            self.sans_logger.notice("Facility changed to ISIS.")

        if not instrument:
            instrument = self._view.instrument

        if instrument == SANSInstrument.NO_INSTRUMENT:
            self._view.disable_process_buttons()
        else:
            instrument_string = get_string_for_gui_from_instrument(instrument)
            ConfigService["default.instrument"] = instrument_string
            self.sans_logger.notice("Instrument changed to {}.".format(instrument_string))
            self._view.enable_process_buttons()

        self._view.set_instrument_settings(instrument)
        self._settings_adjustment_presenter.update_instrument(instrument)
        self._beam_centre_presenter.on_update_instrument(instrument)
        self._workspace_diagnostic_presenter.set_instrument_settings(instrument)
