""" The run tab presenter.

This presenter is essentially the brain of the reduction gui. It controls other presenters and is mainly responsible
for presenting and generating the reduction settings.
"""

from __future__ import (absolute_import, division, print_function)

import os
import copy
import time
from mantid.kernel import Logger
from mantid.api import (FileFinder)

from ui.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel, TableIndexModel
from sans.gui_logic.presenter.settings_diagnostic_presenter import (SettingsDiagnosticPresenter)
from sans.gui_logic.presenter.masking_table_presenter import (MaskingTablePresenter)
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.gui_logic.gui_common import (get_reduction_mode_strings_for_gui, get_instrument_strings_for_gui)
from sans.common.enums import (BatchReductionEntry, RangeStepType, SampleShape, FitType)
from sans.user_file.user_file_reader import UserFileReader
from sans.command_interface.batch_csv_file_parser import BatchCsvParser
from sans.common.constants import ALL_PERIODS
from sans.gui_logic.models.beam_centre_model import BeamCentreModel
from sans.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from sans.gui_logic.models.diagnostics_page_model import run_integral, create_state
from sans.sans_batch import SANSCentreFinder
from sans.gui_logic.models.create_state import create_states
from ui.sans_isis.work_handler import WorkHandler
from sans.common.file_information import SANSFileInformationFactory
from sans.sans_batch import SANSBatchReduction

try:
    import mantidplot
except (Exception, Warning):
    mantidplot = None
    # this should happen when this is called from outside Mantidplot and only then,
    # the result is that attempting to plot will raise an exception


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

        def on_processed_clicked(self):
            self._presenter.on_processed_clicked()

        def on_multi_period_selection(self, show_periods):
            self._presenter.on_multiperiod_changed(show_periods)

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

    class ProcessListener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            super(RunTabPresenter.ProcessListener, self).__init__()
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished(result)

        def on_processing_error(self, error):
            self._presenter.on_processing_error(error)

    def __init__(self, facility, view=None):
        super(RunTabPresenter, self).__init__()
        self._facility = facility
        # Logger
        self.sans_logger = Logger("SANS")
        # Name of grpah to output to
        self.output_graph = 'SANS-Latest'
        # Presenter needs to have a handle on the view since it delegates it
        self._view = None
        self.set_view(view)
        self._processing = False
        self.work_handler = WorkHandler()

        # Models that are being used by the presenter
        self._state_model = None
        self._table_model = TableModel()

        # File information for the first input
        self._file_information = None

        # Settings diagnostic tab presenter
        self._settings_diagnostic_tab_presenter = SettingsDiagnosticPresenter(self)

        # Masking table presenter
        self._masking_table_presenter = MaskingTablePresenter(self)

        # Beam centre presenter
        self._beam_centre_presenter = BeamCentrePresenter(self, WorkHandler, BeamCentreModel, SANSCentreFinder)

        # Workspace Diagnostic page presenter
        self._workspace_diagnostic_presenter = DiagnosticsPagePresenter(self, WorkHandler, run_integral, create_state, self._facility)

    def _default_gui_setup(self):
        """
        Provides a default setup of the GUI. This is important for the initial start up, when the view is being set.
        """
        # Set the possible reduction modes
        reduction_mode_list = get_reduction_mode_strings_for_gui()
        self._view.set_reduction_modes(reduction_mode_list)

        # Set the possible instruments
        instrument_list = get_instrument_strings_for_gui()
        self._view.set_instruments(instrument_list)

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
        self._view.q_1d_step_type = range_step_types
        self._view.q_xy_step_type = range_step_types

        # Set the fit options
        fit_types = [FitType.to_string(FitType.Linear),
                     FitType.to_string(FitType.Logarithmic),
                     FitType.to_string(FitType.Polynomial)]
        self._view.transmission_sample_fit_type = fit_types
        self._view.transmission_can_fit_type = fit_types

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

            # Set appropriate view for the state diagnostic tab presenter
            self._settings_diagnostic_tab_presenter.set_view(self._view.settings_diagnostic_tab)

            # Set appropriate view for the masking table presenter
            self._masking_table_presenter.set_view(self._view.masking_table)

            # Set the appropriate view for the beam centre presenter
            self._beam_centre_presenter.set_view(self._view.beam_centre)

            # Set the appropriate view for the diagnostic page
            self._workspace_diagnostic_presenter.set_view(self._view.diagnostic_page, self._view.instrument)

    def on_user_file_load(self):
        """
        Loads the user file. Populates the models and the view.
        """
        try:
            # 1. Get the user file path from the view
            user_file_path = self._view.get_user_file_path()

            if not user_file_path:
                return
            # 2. Get the full file path
            user_file_path = FileFinder.getFullPath(user_file_path)
            if not os.path.exists(user_file_path):
                raise RuntimeError("The user path {} does not exist. Make sure a valid user file path"
                                   " has been specified.".format(user_file_path))
            self._table_model.user_file = user_file_path
            # Clear out the current view
            self._view.reset_all_fields_to_default()

            # 3. Read and parse the user file
            user_file_reader = UserFileReader(user_file_path)
            user_file_items = user_file_reader.read_user_file()

            # 4. Populate the model
            self._state_model = StateGuiModel(user_file_items)
            # 5. Update the views.
            self._update_view_from_state_model()
            self._beam_centre_presenter.update_centre_positions(self._state_model)

            self._beam_centre_presenter.on_update_rows()
            self._masking_table_presenter.on_update_rows()
            self._workspace_diagnostic_presenter.on_user_file_load(user_file_path)

        except Exception as e:
            self.sans_logger.error("Loading of the user file failed. {}".format(str(e)))
            self.display_warning_box('Warning', 'Loading of the user file failed.', str(e))

    def on_batch_file_load(self):
        """
        Loads a batch file and populates the batch table based on that.
        """
        try:
            # 1. Get the batch file from the view
            batch_file_path = self._view.get_batch_file_path()

            if not batch_file_path:
                return

            if not os.path.exists(batch_file_path):
                raise RuntimeError("The batch file path {} does not exist. Make sure a valid batch file path"
                                   " has been specified.".format(batch_file_path))

            self._table_model.batch_file = batch_file_path

            # 2. Read the batch file
            batch_file_parser = BatchCsvParser(batch_file_path)
            parsed_rows = batch_file_parser.parse_batch_file()
            # 3. Clear the table
            self._view.clear_table()

            # 4. Populate the table
            self._table_model.clear_table_entries()
            for row in parsed_rows:
                self._add_row_to_table_model(row)

            self.update_view_from_table_model()

            self._beam_centre_presenter.on_update_rows()
            self._masking_table_presenter.on_update_rows()

        except RuntimeError as e:
            self.sans_logger.error("Loading of the batch file failed. {}".format(str(e)))
            self.display_warning_box('Warning', 'Loading of the batch file failed', str(e))

    def _add_row_to_table_model(self,row):
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
        sample_scatter_period = get_string_period(get_string_entry(BatchReductionEntry.SampleScatterPeriod, row))
        sample_transmission = get_string_entry(BatchReductionEntry.SampleTransmission, row)
        sample_transmission_period = \
            get_string_period(get_string_entry(BatchReductionEntry.SampleTransmissionPeriod, row))
        sample_direct = get_string_entry(BatchReductionEntry.SampleDirect, row)
        sample_direct_period = get_string_period(get_string_entry(BatchReductionEntry.SampleDirectPeriod, row))
        can_scatter = get_string_entry(BatchReductionEntry.CanScatter, row)
        can_scatter_period = get_string_period(get_string_entry(BatchReductionEntry.CanScatterPeriod, row))
        can_transmission = get_string_entry(BatchReductionEntry.CanTransmission, row)
        can_transmission_period = get_string_period(get_string_entry(BatchReductionEntry.CanScatterPeriod, row))
        can_direct = get_string_entry(BatchReductionEntry.CanDirect, row)
        can_direct_period = get_string_period(get_string_entry(BatchReductionEntry.CanDirectPeriod, row))
        output_name = get_string_entry(BatchReductionEntry.Output, row)
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(sample_scatter)
        sample_thickness = file_information._thickness
        user_file = get_string_entry(BatchReductionEntry.UserFile, row)

        row_entry = [sample_scatter, sample_scatter_period, sample_transmission, sample_transmission_period,
                     sample_direct, sample_direct_period, can_scatter, can_scatter_period, can_transmission, can_transmission_period,
                     can_direct, can_direct_period,
                     output_name, user_file, sample_thickness, '']

        table_index_model = TableIndexModel(*row_entry)

        self._table_model.append_table_entry(table_index_model)

    def update_view_from_table_model(self):
        self._view.hide_period_columns()
        for row in self._table_model._table_entries:
            row_entry = [str(x) for x in row.toList()]
            self._view.add_row(row_entry)
            if row.isMultiPeriod():
                self._view.show_period_columns()

    def on_data_changed(self, row, column, new_value, old_value):
        self._table_model.update_table_entry(row, column, new_value)
        self._beam_centre_presenter.on_update_rows()
        self._masking_table_presenter.on_update_rows()

    def on_instrument_changed(self):
        self._setup_instrument_specific_settings()

    def on_processed_clicked(self):
        """
        Prepares the batch reduction.

        0. Validate rows and create dummy workspace if it does not exist
        1. Sets up the states
        2. Adds a dummy input workspace
        3. Adds row index information
        """
        try:
            self._view.disable_buttons()
            self._processing = True
            self.sans_logger.information("Starting processing of batch table.")

            # 1. Set up the states and convert them into property managers
            selected_rows = self._view.get_selected_rows()
            selected_rows = selected_rows if selected_rows else range(self._table_model.get_number_of_rows())
            states = self.get_states(row_index=selected_rows)
            if not states:
                raise RuntimeError("There seems to have been an issue with setting the states. Make sure that a user file"
                                   " has been loaded")

            # 4. Create the graph if continuous output is specified
            if mantidplot:
                if self._view.plot_results and not mantidplot.graph(self.output_graph):
                    mantidplot.newGraph(self.output_graph)

            # Check if optimizations should be used
            use_optimizations = self._view.use_optimizations

            # Get the output mode
            output_mode = self._view.output_mode

            # Check if results should be plotted
            plot_results =  self._view.plot_results

            # Get the name of the graph to output to
            output_graph = self.output_graph
            sans_batch = SANSBatchReduction()
            listener = RunTabPresenter.ProcessListener(self)

            self.work_handler.process(listener, sans_batch, states=states.values(), use_optimizations=use_optimizations,
                                      output_mode=output_mode, plot_results=plot_results, output_graph=output_graph)

        except Exception as e:
            self._view.enable_buttons()
            self.sans_logger.error("Process halted due to: {}".format(str(e)))
            self.display_warning_box('Warning', 'Process halted', str(e))

    def on_multiperiod_changed(self, show_periods):
        if show_periods:
            self._view.show_period_columns()
        else:
            self._view.hide_period_columns()

    def display_warning_box(self, title, text, detailed_text):
        self._view.display_message_box(title, text, detailed_text)

    def on_processing_finished(self, result):
        self._view.enable_buttons()
        self._processing = False

    def on_processing_error(self, error):
        self._view.enable_buttons()
        self._processing = False

    def on_row_inserted(self, index, row):
        row_table_index = TableIndexModel(*row)
        self._table_model.add_table_entry(index, row_table_index)

    def on_rows_removed(self, rows):
        self._view.remove_rows(rows)
        self._table_model.remove_table_entries(rows)

    def on_manage_directories(self):
        self._view.show_directory_manager()

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

    # ------------------------------------------------------------------------------------------------------------------
    # Table Model and state population
    # ------------------------------------------------------------------------------------------------------------------
    def get_states(self, row_index=None, file_lookup=True):
        """
        Gathers the state information for all rows.
        :param row_index: if a single row is selected, then only this row is returned, else all the state for all
                             rows is returned
        :return: a list of states
        """
        start_time_state_generation = time.time()

        # 1. Update the state model
        state_model_with_view_update = self._get_state_model_with_view_update()
        # 2. Update the table model
        table_model = self._table_model

        # 3. Go through each row and construct a state object
        if table_model and state_model_with_view_update:
            states = create_states(state_model_with_view_update, table_model, self._view.instrument
                                   , self._facility, row_index=row_index, file_lookup=file_lookup)
        else:
            states = None
        stop_time_state_generation = time.time()
        time_taken = stop_time_state_generation - start_time_state_generation
        self.sans_logger.information("The generation of all states took {}s".format(time_taken))
        return states

    def get_state_for_row(self, row_index, file_lookup=True):
        """
        Creates the state for a particular row.
        :param row_index: the row index
        :return: a state if the index is valid and there is a state else None
        """
        states = self.get_states(row_index=[row_index], file_lookup=file_lookup)
        if states is None:
            self.sans_logger.warning("There does not seem to be data for a row {}.".format(row_index))
            return None

        if row_index in list(states.keys()):
            if states:
                return states[row_index]
        return None

    def _update_view_from_state_model(self):
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
        self._set_on_view("sample_shape")
        self._set_on_view("sample_height")
        self._set_on_view("sample_width")
        self._set_on_view("sample_thickness")
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
        self._set_on_view("show_transmission")

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
        if attribute or isinstance(attribute, bool):  # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values. # noqa
            setattr(self._view, attribute_name, attribute)

    def _set_on_view_with_view(self, attribute_name, view):
        attribute = getattr(self._state_model, attribute_name)
        if attribute or isinstance(attribute, bool):  # We need to be careful here. We don't want to set empty strings, or None, but we want to set boolean values. # noqa
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
        self._set_on_state_model("sample_shape", state_model)
        self._set_on_state_model("sample_height", state_model)
        self._set_on_state_model("sample_width", state_model)
        self._set_on_state_model("sample_thickness", state_model)
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
        self._set_on_state_model("show_transmission", state_model)

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
                q_1d_rebin_string += str(q_1d_step_type_factor*q_1d_step) + ","
                q_1d_rebin_string += str(q_1d_max)
                state_model.q_1d_rebin_string = q_1d_rebin_string

    def _set_on_state_model(self, attribute_name, state_model):
        attribute = getattr(self._view, attribute_name)
        if attribute is not None and attribute != '':
            setattr(state_model, attribute_name, attribute)

    def get_cell_value(self, row, column):
        return self._view.get_cell(row=row, column=self.table_index[column], convert_to=str)

    # ------------------------------------------------------------------------------------------------------------------
    # Settings
    # ------------------------------------------------------------------------------------------------------------------
    def _setup_instrument_specific_settings(self, instrument=None):
        if not instrument:
            instrument = self._view.instrument

        self._view.set_instrument_settings(instrument)
        self._beam_centre_presenter.on_update_instrument(instrument)
        self._workspace_diagnostic_presenter.set_instrument_settings(instrument)
