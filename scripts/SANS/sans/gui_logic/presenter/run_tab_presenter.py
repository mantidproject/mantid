""" The run tab presenter.

This presenter is essentially the brain of the reduction gui. It controlls other presenters and is mainly responsible
for presenting and generating the reduction settings.
"""

from __future__ import (absolute_import, division, print_function)

import os
import copy
import time
from mantid.kernel import Logger
from mantid.api import (AnalysisDataService, FileFinder, WorkspaceFactory)
from mantid.kernel import (Property)

from ui.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
from sans.gui_logic.models.state_gui_model import StateGuiModel
from sans.gui_logic.models.table_model import TableModel, TableIndexModel
from sans.gui_logic.presenter.gui_state_director import (GuiStateDirector)
from sans.gui_logic.presenter.settings_diagnostic_presenter import (SettingsDiagnosticPresenter)
from sans.gui_logic.presenter.masking_table_presenter import (MaskingTablePresenter)
from sans.gui_logic.presenter.beam_centre_presenter import BeamCentrePresenter
from sans.gui_logic.sans_data_processor_gui_algorithm import SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME
from sans.gui_logic.presenter.property_manager_service import PropertyManagerService
from sans.gui_logic.gui_common import (get_reduction_mode_strings_for_gui, generate_table_index, OPTIONS_SEPARATOR,
                                       OPTIONS_EQUAL, get_instrument_strings_for_gui)
from sans.common.enums import (BatchReductionEntry, OutputMode, RangeStepType, SampleShape, FitType)
from sans.user_file.user_file_reader import UserFileReader
from sans.command_interface.batch_csv_file_parser import BatchCsvParser
from sans.common.constants import ALL_PERIODS
from sans.gui_logic.models.beam_centre_model import BeamCentreModel
from ui.sans_isis.work_handler import WorkHandler
from sans.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from sans.gui_logic.models.diagnostics_page_model import run_integral, create_state
from sans.sans_batch import SANSCentreFinder

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

        def on_processing_finished(self):
            self._presenter.on_processing_finished()

        def on_multi_period_selection(self):
            self._presenter.on_multi_period_selection()

        def on_data_changed(self):
            self._presenter.on_data_changed()

        def on_manage_directories(self):
            self._presenter.on_manage_directories()

        def on_instrument_changed(self):
            self._presenter.on_instrument_changed()

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

        # Models that are being used by the presenter
        self._state_model = None
        self._table_model = None

        # Due to the nature of the DataProcessorWidget we need to provide an algorithm with at least one input
        # workspace and at least one output workspace. Our SANS state approach is not compatible with this. Hence
        # we provide a dummy workspace which is not used. We keep it invisible on the ADS and delete it when the
        # main_presenter is deleted.
        # This is not a nice solution but in line with the SANS dummy algorithm approach that we have provided
        # for the
        self._create_dummy_input_workspace()

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

    def __del__(self):
        self._delete_dummy_input_workspace()

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
                        SampleShape.to_string(SampleShape.CylinderAxisUp),
                        SampleShape.to_string(SampleShape.Cuboid),
                        SampleShape.to_string(SampleShape.CylinderAxisAlong)]
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

            # Set up the correct table row indices
            self.on_multi_period_selection()

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

            # Clear out the current view
            self._view.reset_all_fields_to_default()

            # 3. Read and parse the user file
            user_file_reader = UserFileReader(user_file_path)
            user_file_items = user_file_reader.read_user_file()

            # 4. Populate the model
            self._state_model = StateGuiModel(user_file_items)
            # 5. Update the views.
            self._update_view_from_state_model()

            # 6. Perform calls on child presenters
            self._masking_table_presenter.on_update_rows()
            self._settings_diagnostic_tab_presenter.on_update_rows()
            self._beam_centre_presenter.on_update_rows()
            self._workspace_diagnostic_presenter.on_user_file_load(user_file_path)

        except Exception as e:
            self.sans_logger.error("Loading of the user file failed. Ensure that the path to your files has been added "
                                   "to the Mantid search directories! See here for more details: {}".format(str(e)))

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

            # 2. Read the batch file
            batch_file_parser = BatchCsvParser(batch_file_path)
            parsed_rows = batch_file_parser.parse_batch_file()
            # 3. Clear the table
            self._view.clear_table()

            # 4. Populate the table
            for row in parsed_rows:
                self._populate_row_in_table(row)

            # 5. Perform calls on child presenters
            self._masking_table_presenter.on_update_rows()
            self._settings_diagnostic_tab_presenter.on_update_rows()
            self._beam_centre_presenter.on_update_rows()

        except RuntimeError as e:
            self.sans_logger.error("Loading of the batch file failed. Ensure that the path to your files has been added"
                                   " to the Mantid search directories! See here for more details: {}".format(str(e)))

    def on_data_changed(self):
        # 1. Perform calls on child presenters
        self._masking_table_presenter.on_update_rows()
        self._settings_diagnostic_tab_presenter.on_update_rows()
        self._beam_centre_presenter.on_update_rows()

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
            self.sans_logger.information("Starting processing of batch table.")
            # 0. Validate rows
            self._create_dummy_input_workspace()
            self._validate_rows()

            # 1. Set up the states and convert them into property managers
            states = self.get_states()
            if not states:
                raise RuntimeError("There seems to have been an issue with setting the states. Make sure that a user file"
                                   " has been loaded")
            property_manager_service = PropertyManagerService()
            property_manager_service.add_states_to_pmds(states)

            # 2. Add dummy input workspace to Options column
            self._remove_dummy_workspaces_and_row_index()
            self._set_dummy_workspace()

            # 3. Add dummy row index to Options column
            self._set_indices()

            # 4. Create the graph if continuous output is specified
            if mantidplot:
                if self._view.plot_results and not mantidplot.graph(self.output_graph):
                    mantidplot.newGraph(self.output_graph)

        except Exception as e:
            self._view.halt_process_flag()
            self._view.enable_buttons()
            self.sans_logger.error("Process halted due to: {}".format(str(e)))

    def on_processing_finished(self):
        self._remove_dummy_workspaces_and_row_index()
        self._view.enable_buttons()

    def on_multi_period_selection(self):
        multi_period = self._view.is_multi_period_view()
        self.table_index = generate_table_index(multi_period)

    def on_manage_directories(self):
        self._view.show_directory_manager()

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

    def _add_to_hidden_options(self, row, property_name, property_value):
        """
        Adds a new property to the Hidden Options column

        @param row: The row where the Options column is being altered
        @param property_name: The property name on the GUI algorithm.
        @param property_value: The value which is being set for the property.
        """
        entry = property_name + OPTIONS_EQUAL + str(property_value)
        options = self._get_hidden_options(row)
        if options:
            options += OPTIONS_SEPARATOR + entry
        else:
            options = entry
        self._set_hidden_options(options, row)

    def _set_hidden_options(self, value, row):
        self._view.set_cell(value, row, self.table_index['HIDDEN_OPTIONS_INDEX'])

    def _get_options(self, row):
        return self._view.get_cell(row, self.table_index['OPTIONS_INDEX'], convert_to=str)

    def _get_hidden_options(self, row):
        return self._view.get_cell(row, self.table_index['HIDDEN_OPTIONS_INDEX'], convert_to=str)

    def is_empty_row(self, row):
        """
        Checks if a row has no entries. These rows will be ignored.
        :param row: the row index
        :return: True if the row is empty.
        """
        indices = range(self.table_index['OPTIONS_INDEX'] + 1)
        for index in indices:
            cell_value = self._view.get_cell(row, index, convert_to=str)
            if cell_value:
                return False
        return True

    def _remove_from_hidden_options(self, row, property_name):
        """
        Remove the entries in the hidden options column
        :param row: the row index
        :param property_name: the property name which is to be removed
        """
        options = self._get_hidden_options(row)
        # Remove the property entry and the value
        individual_options = options.split(",")
        clean_options = []
        for individual_option in individual_options:
            if property_name not in individual_option:
                clean_options.append(individual_option)
        clean_options = ",".join(clean_options)
        self._set_hidden_options(clean_options, row)

    def _validate_rows(self):
        """
        Validation of the rows. A minimal setup requires that ScatterSample is set.
        """
        # If SampleScatter is empty, then don't run the reduction.
        # We allow empty rows for now, since we cannot remove them from Python.
        number_of_rows = self._view.get_number_of_rows()
        for row in range(number_of_rows):
            if not self.is_empty_row(row):
                sample_scatter = self._view.get_cell(row, 0)
                if not sample_scatter:
                    raise RuntimeError("Row {} has not SampleScatter specified. Please correct this.".format(row))

    def get_processing_options(self):
        """
        Creates a processing string for the data processor widget

        :return: A dict of key:value pairs of processing-algorithm properties and values for the data processor widget
        """
        global_options = {}

        # Check if optimizations should be used
        global_options['UseOptimizations'] = "1" if self._view.use_optimizations else "0"

        # Get the output mode
        output_mode = self._view.output_mode
        global_options['OutputMode'] = OutputMode.to_string(output_mode)

        # Check if results should be plotted
        global_options['PlotResults'] = "1" if self._view.plot_results else "0"

        # Get the name of the graph to output to
        global_options['OutputGraph'] = "{}".format(self.output_graph)
        return global_options

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
    def get_states(self, row_index=None):
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
        table_model = self._get_table_model()

        # 3. Go through each row and construct a state object
        if table_model and state_model_with_view_update:
            states = self._create_states(state_model_with_view_update, table_model, row_index)
        else:
            states = None
        stop_time_state_generation = time.time()
        time_taken = stop_time_state_generation - start_time_state_generation
        self.sans_logger.information("The generation of all states took {}s".format(time_taken))
        return states

    def get_row_indices(self):
        """
        Gets the indices of row which are not empty.
        :return: a list of row indices.
        """
        row_indices_which_are_not_empty = []
        number_of_rows = self._view.get_number_of_rows()
        for row in range(number_of_rows):
            if not self.is_empty_row(row):
                row_indices_which_are_not_empty.append(row)
        return row_indices_which_are_not_empty

    def get_state_for_row(self, row_index):
        """
        Creates the state for a particular row.
        :param row_index: the row index
        :return: a state if the index is valid and there is a state else None
        """
        states = self.get_states(row_index=row_index)
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

        # Beam Centre
        self._beam_centre_presenter.set_on_view('lab_pos_1', self._state_model)
        self._beam_centre_presenter.set_on_view('lab_pos_2', self._state_model)
        self._beam_centre_presenter.set_on_view('hab_pos_1', self._state_model)
        self._beam_centre_presenter.set_on_view('hab_pos_2', self._state_model)

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

    def _get_table_model(self):
        # 1. Create a new table model
        user_file = self._view.get_user_file_path()
        batch_file = self._view.get_batch_file_path()

        table_model = TableModel()
        table_model.user_file = user_file
        self.batch_file = batch_file

        # 2. Iterate over each row, create a table row model and insert it
        number_of_rows = self._view.get_number_of_rows()
        is_multi_period_view = self._view.is_multi_period_view()
        for row in range(number_of_rows):
            sample_scatter = self.get_cell_value(row, 'SAMPLE_SCATTER_INDEX')
            sample_transmission = self.get_cell_value(row, 'SAMPLE_TRANSMISSION_INDEX')
            sample_direct = self.get_cell_value(row, 'SAMPLE_DIRECT_INDEX')

            can_scatter = self.get_cell_value(row, 'CAN_SCATTER_INDEX')
            can_transmission = self.get_cell_value(row, 'CAN_TRANSMISSION_INDEX')
            can_direct = self.get_cell_value(row, 'CAN_DIRECT_INDEX')

            sample_scatter_period = self.get_cell_value(row, 'SAMPLE_SCATTER_PERIOD_INDEX')if is_multi_period_view else ""
            sample_transmission_period = self.get_cell_value(row, 'SAMPLE_TRANSMISSION_PERIOD_INDEX')if is_multi_period_view else ""
            sample_direct_period = self.get_cell_value(row, 'SAMPLE_DIRECT_PERIOD_INDEX')if is_multi_period_view else ""

            can_scatter_period = self.get_cell_value(row, 'CAN_SCATTER_PERIOD_INDEX') if is_multi_period_view else ""
            can_transmission_period = self.get_cell_value(row, 'CAN_TRANSMISSION_PERIOD_INDEX') if is_multi_period_view else ""
            can_direct_period = self.get_cell_value(row, 'CAN_DIRECT_PERIOD_INDEX') if is_multi_period_view else ""

            output_name = self.get_cell_value(row, 'OUTPUT_NAME_INDEX')
            user_file = self.get_cell_value(row, 'USER_FILE_INDEX')

            # Get the options string
            # We don't have to add the hidden column here, since it only contains information for the SANS
            # workflow to operate properly. It however does not contain information for the
            options_string = self._get_options(row)

            table_index_model = TableIndexModel(index=row,
                                                sample_scatter=sample_scatter,
                                                sample_scatter_period=sample_scatter_period,
                                                sample_transmission=sample_transmission,
                                                sample_transmission_period=sample_transmission_period,
                                                sample_direct=sample_direct,
                                                sample_direct_period=sample_direct_period,
                                                can_scatter=can_scatter,
                                                can_scatter_period=can_scatter_period,
                                                can_transmission=can_transmission,
                                                can_transmission_period=can_transmission_period,
                                                can_direct=can_direct,
                                                can_direct_period=can_direct_period,
                                                output_name=output_name,
                                                user_file = user_file,
                                                options_column_string=options_string)
            table_model.add_table_entry(row, table_index_model)
        return table_model

    def get_cell_value(self, row, column):
        return self._view.get_cell(row=row, column=self.table_index[column], convert_to=str)

    def _create_states(self, state_model, table_model, row_index=None):
        """
        Here we create the states based on the settings in the models
        :param state_model: the state model object
        :param table_model: the table model object
        :param row_index: the selected row, if None then all rows are generated
        """
        number_of_rows = self._view.get_number_of_rows()
        if row_index is not None:
            # Check if the selected index is valid
            if row_index >= number_of_rows:
                return None
            rows = [row_index]
        else:
            rows = range(number_of_rows)
        states = {}

        gui_state_director = GuiStateDirector(table_model, state_model, self._facility)
        for row in rows:
            self.sans_logger.information("Generating state for row {}".format(row))
            if not self.is_empty_row(row):
                row_user_file = table_model.get_row_user_file(row)
                if row_user_file:
                    user_file_path = FileFinder.getFullPath(row_user_file)
                    if not os.path.exists(user_file_path):
                        raise RuntimeError("The user path {} does not exist. Make sure a valid user file path"
                                           " has been specified.".format(user_file_path))

                    user_file_reader = UserFileReader(user_file_path)
                    user_file_items = user_file_reader.read_user_file()

                    row_state_model = StateGuiModel(user_file_items)
                    row_gui_state_director = GuiStateDirector(table_model, row_state_model, self._facility)
                    self._create_row_state(row_gui_state_director, states, row)
                else:
                    self._create_row_state(gui_state_director, states, row)
        return states

    def _create_row_state(self, director, states, row):
        try:
            state = director.create_state(row)
            states.update({row: state})
        except (ValueError, RuntimeError) as e:
            raise RuntimeError("There was a bad entry for row {}. Ensure that the path to your files has "
                               "been added to the Mantid search directories! See here for more "
                               "details: {}".format(row, str(e)))

    def _populate_row_in_table(self, row):
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
        sample_scatter_period = get_string_entry(BatchReductionEntry.SampleScatterPeriod, row)
        sample_transmission = get_string_entry(BatchReductionEntry.SampleTransmission, row)
        sample_transmission_period = get_string_entry(BatchReductionEntry.SampleTransmissionPeriod, row)
        sample_direct = get_string_entry(BatchReductionEntry.SampleDirect, row)
        sample_direct_period = get_string_entry(BatchReductionEntry.SampleDirectPeriod, row)
        can_scatter = get_string_entry(BatchReductionEntry.CanScatter, row)
        can_scatter_period = get_string_entry(BatchReductionEntry.CanScatterPeriod, row)
        can_transmission = get_string_entry(BatchReductionEntry.CanTransmission, row)
        can_transmission_period = get_string_entry(BatchReductionEntry.CanScatterPeriod, row)
        can_direct = get_string_entry(BatchReductionEntry.CanDirect, row)
        can_direct_period = get_string_entry(BatchReductionEntry.CanDirectPeriod, row)
        output_name = get_string_entry(BatchReductionEntry.Output, row)

        # If one of the periods is not null, then we should switch the view to multi-period view
        if any ((sample_scatter_period, sample_transmission_period, sample_direct_period, can_scatter_period,
                can_transmission_period, can_direct_period)):
            if not self._view.is_multi_period_view():
                self._view.set_multi_period_view_mode(True)

        # 2. Create entry that can be understood by table
        if self._view.is_multi_period_view():
            row_entry = "SampleScatter:{},ssp:{},SampleTrans:{},stp:{},SampleDirect:{},sdp:{}," \
                        "CanScatter:{},csp:{},CanTrans:{},ctp:{}," \
                        "CanDirect:{},cdp:{},OutputName:{}".format(sample_scatter,
                                                                   get_string_period(sample_scatter_period),
                                                                   sample_transmission,
                                                                   get_string_period(sample_transmission_period),
                                                                   sample_direct,
                                                                   get_string_period(sample_direct_period),
                                                                   can_scatter,
                                                                   get_string_period(can_scatter_period),
                                                                   can_transmission,
                                                                   get_string_period(can_transmission_period),
                                                                   can_direct,
                                                                   get_string_period(can_direct_period),
                                                                   output_name)
        else:
            row_entry = "SampleScatter:{},SampleTrans:{},SampleDirect:{}," \
                        "CanScatter:{},CanTrans:{}," \
                        "CanDirect:{},OutputName:{}".format(sample_scatter,
                                                            sample_transmission,
                                                            sample_direct,
                                                            can_scatter,
                                                            can_transmission,
                                                            can_direct,
                                                            output_name)

        self._view.add_row(row_entry)

    # ------------------------------------------------------------------------------------------------------------------
    # Settings
    # ------------------------------------------------------------------------------------------------------------------
    def _setup_instrument_specific_settings(self, instrument=None):
        if not instrument:
            instrument = self._view.instrument

        self._view.set_instrument_settings(instrument)
        self._beam_centre_presenter.on_update_instrument(instrument)
        self._workspace_diagnostic_presenter.set_instrument_settings(instrument)

    # ------------------------------------------------------------------------------------------------------------------
    # Setting workaround for state in DataProcessorWidget
    # ------------------------------------------------------------------------------------------------------------------
    def _remove_dummy_workspaces_and_row_index(self):
        number_of_rows = self._view.get_number_of_rows()
        for row in range(number_of_rows):
            self._remove_from_hidden_options(row, "InputWorkspace")
            self._remove_from_hidden_options(row, "RowIndex")

    def _set_indices(self):
        number_of_rows = self._view.get_number_of_rows()
        for row in range(number_of_rows):
            to_set = Property.EMPTY_INT if self.is_empty_row(row) else row
            self._add_to_hidden_options(row, "RowIndex", to_set)

    def _set_dummy_workspace(self):
        number_of_rows = self._view.get_number_of_rows()
        for row in range(number_of_rows):
            self._add_to_hidden_options(row, "InputWorkspace", SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME)

    @staticmethod
    def _create_dummy_input_workspace():
        if not AnalysisDataService.doesExist(SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME):
            workspace = WorkspaceFactory.create("Workspace2D", 1, 1, 1)
            AnalysisDataService.addOrReplace(SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME, workspace)

    @staticmethod
    def _delete_dummy_input_workspace():
        if AnalysisDataService.doesExist(SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME):
            AnalysisDataService.remove(SANS_DUMMY_INPUT_ALGORITHM_PROPERTY_NAME)
