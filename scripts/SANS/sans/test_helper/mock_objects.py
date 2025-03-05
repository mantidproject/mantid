# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from functools import partial

from unittest import mock
from sans.gui_logic.presenter.run_tab_presenter import RunTabPresenter
from sans.common.enums import RangeStepType, OutputMode, SANSFacility, SANSInstrument
from sans.state.JsonSerializable import JsonSerializable
from sans.test_helper.test_director import TestDirector
from ui.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
from ui.sans_isis.settings_diagnostic_tab import SettingsDiagnosticTab
from ui.sans_isis.diagnostics_page import DiagnosticsPage
from ui.sans_isis.masking_table import MaskingTable
from ui.sans_isis.beam_centre import BeamCentre


def create_mock_settings_diagnostic_tab():
    view = mock.create_autospec(SettingsDiagnosticTab, spec_set=False, instance=True)
    view.get_current_row = mock.MagicMock(return_value=0)
    return view


def create_mock_masking_table():
    view = mock.create_autospec(MaskingTable, spec_set=False, instance=True)
    view.get_current_row = mock.MagicMock(return_value=0)
    return view


def create_mock_beam_centre_tab():
    view = mock.create_autospec(BeamCentre, spec_set=False, instance=True)
    view.r_min_line_edit = mock.Mock()
    view.r_max_line_edit = mock.Mock()
    return view


def create_mock_diagnostics_tab():
    view = mock.create_autospec(DiagnosticsPage, spec_set=False, instance=True)
    return view


def get_cell_mock(row, column, convert_to=None, user_file_path=""):
    _ = convert_to
    if row == 0:
        # For the first row we return the
        # all of the sample data
        if column == 0:
            return "SANS2D00022024"
        elif column == 2:
            return "SANS2D00022048"
        elif column == 4:
            return "SANS2D00022048"
        else:
            return ""
    elif row == 1:
        if column == 0:
            return "SANS2D00022024"
        if column == 13:
            return user_file_path
        else:
            return ""
    else:
        # For the other rows, we only return sample scatter
        if column == 0:
            return "SANS2D00022024"
        else:
            return ""


mock_listener_list = []


def on_load_user_file_mock():
    for listener in mock_listener_list:
        listener.on_user_file_load()


def on_load_batch_file_mock():
    for listener in mock_listener_list:
        listener.on_batch_file_load()


def add_listener_mock(listener):
    mock_listener_list.append(listener)


def create_mock_view(user_file_path, batch_file_path=None, row_user_file_path=""):
    get_cell_mock_with_path = partial(get_cell_mock, user_file_path=row_user_file_path)

    view = mock.create_autospec(SANSDataProcessorGui, spec_set=False, instance=True)
    view.get_user_file_path = mock.Mock(return_value=user_file_path)
    view.get_cell = mock.MagicMock(side_effect=get_cell_mock_with_path)
    view.get_batch_file_path = mock.MagicMock(return_value=batch_file_path)
    view.batch_file = batch_file_path
    view.get_number_of_rows = mock.MagicMock(return_value=2)

    # Older unit test files which do not specify this will cause Mock to substitute
    # the MagicMock object instead. This cannot be deep copied causing
    # the unit tests to fail over. This only affects testing for force to None type to avoid
    view.transmission_mn_5_shift = None

    # Add the settings diagnostic mock
    settings_diagnostic_tab = create_mock_settings_diagnostic_tab()
    view.settings_diagnostic_tab = settings_diagnostic_tab

    # Add the masking table view
    masking_table = create_mock_masking_table()
    view.masking_table = masking_table

    # Add the beam centre view
    beam_centre = create_mock_beam_centre_tab()
    view.beam_centre = beam_centre

    # Add the data diagnostic tab
    diagnostic_page = create_mock_diagnostics_tab()
    view.diagnostic_page = diagnostic_page

    view.halt_process_flag = mock.MagicMock()

    # Mock objects used in the properties handler
    view.user_file_line_edit = mock.Mock()

    # Mock the add runs presenter
    view.add_runs_presenter = mock.Mock()

    # ---------------------
    # Mocking properties
    # ---------------------
    _event_slices = mock.PropertyMock(return_value="")
    type(view).event_slices = _event_slices

    _merge_scale = mock.PropertyMock(return_value=1.0)
    type(view).merge_scale = _merge_scale

    _merge_shift = mock.PropertyMock(return_value=0.0)
    type(view).merge_shift = _merge_shift

    _merge_q_range_start = mock.PropertyMock(return_value=None)
    type(view).merge_q_range_start = _merge_q_range_start

    _merge_q_range_stop = mock.PropertyMock(return_value=None)
    type(view).merge_q_range_stop = _merge_q_range_stop

    _merge_q_range_stop = mock.PropertyMock(return_value=None)
    type(view).merge_q_range_stop = _merge_q_range_stop

    _merge_min = mock.PropertyMock(return_value=None)
    type(view).merge_min = _merge_min

    _merge_max = mock.PropertyMock(return_value=None)
    type(view).merge_max = _merge_max

    _sample_height = mock.PropertyMock(return_value=None)
    type(view).sample_height = _sample_height

    _sample_width = mock.PropertyMock(return_value=None)
    type(view).sample_width = _sample_width

    _sample_thickness = mock.PropertyMock(return_value=None)
    type(view).sample_thickness = _sample_thickness

    _sample_shape = mock.PropertyMock(return_value=None)
    type(view).sample_shape = _sample_shape

    _pixel_adjustment_det_1 = mock.PropertyMock(return_value="")
    type(view).pixel_adjustment_det_1 = _pixel_adjustment_det_1

    _pixel_adjustment_det_2 = mock.PropertyMock(return_value="")
    type(view).pixel_adjustment_det_2 = _pixel_adjustment_det_2

    _wavelength_adjustment_det_1 = mock.PropertyMock(return_value="")
    type(view).wavelength_adjustment_det_1 = _wavelength_adjustment_det_1

    _wavelength_adjustment_det_2 = mock.PropertyMock(return_value="")
    type(view).wavelength_adjustment_det_2 = _wavelength_adjustment_det_2

    _gravity_extra_length = mock.PropertyMock(return_value=None)
    type(view).gravity_extra_length = _gravity_extra_length

    _q_resolution_source_h = mock.PropertyMock(return_value=None)
    type(view).q_resolution_source_h = _q_resolution_source_h

    _q_resolution_sample_h = mock.PropertyMock(return_value=None)
    type(view).q_resolution_sample_h = _q_resolution_sample_h

    _q_resolution_source_w = mock.PropertyMock(return_value=None)
    type(view).q_resolution_source_w = _q_resolution_source_w

    _q_resolution_sample_w = mock.PropertyMock(return_value=None)
    type(view).q_resolution_sample_w = _q_resolution_sample_w

    _phi_limit_min = mock.PropertyMock(return_value=None)
    type(view).phi_limit_min = _phi_limit_min

    _phi_limit_max = mock.PropertyMock(return_value=None)
    type(view).phi_limit_max = _phi_limit_max

    _phi_limit_max = mock.PropertyMock(return_value=None)
    type(view).phi_limit_max = _phi_limit_max

    _q_1d_step = mock.PropertyMock(return_value=0.001)
    type(view).q_1d_step = _q_1d_step

    _q_1d_step_type = mock.PropertyMock(return_value=RangeStepType.LIN)
    type(view)._q_1d_step_type = _q_1d_step_type

    _output_mode = mock.PropertyMock(return_value=OutputMode.PUBLISH_TO_ADS)
    type(view).output_mode = _output_mode

    _wavelength_range = mock.PropertyMock(return_value="")
    type(view).wavelength_range = _wavelength_range

    _instrument = mock.PropertyMock(return_value=SANSInstrument.SANS2D)
    type(view).instrument = _instrument

    _event_slice_optimisation = mock.PropertyMock(return_value=False)
    type(view).event_slice_optimisation = _event_slice_optimisation

    return view, settings_diagnostic_tab, masking_table


def create_mock_view2(user_file_path, batch_file_path=None):
    global mock_listener_list
    mock_listener_list = []
    view, _, _ = create_mock_view(user_file_path, batch_file_path)

    view.add_listener = mock.MagicMock(side_effect=add_listener_mock)
    view._on_user_file_load = mock.MagicMock(side_effect=on_load_user_file_mock)
    view._on_batch_file_load = mock.MagicMock(side_effect=on_load_batch_file_mock)

    _output_mode = mock.PropertyMock(return_value=OutputMode.PUBLISH_TO_ADS)
    type(view).output_mode = _output_mode

    return view


class FakeState(metaclass=JsonSerializable):
    def __init__(self):
        super(FakeState, self).__init__()
        self.dummy_state = "dummy_state"

    @property
    def property_manager(self):
        return self.dummy_state


def get_state_for_row_mock(row_index, file_lookup=True, suppress_warnings=False):
    return FakeState()


def get_state_for_row_mock_with_real_state(row_index, file_lookup=True, suppress_warnings=False):
    _ = row_index
    test_director = TestDirector()
    return test_director.construct()


def create_run_tab_presenter_mock(use_fake_state=True):
    presenter = mock.create_autospec(RunTabPresenter, spec_set=False, instance=True)
    presenter.get_row_indices = mock.MagicMock(return_value=[0, 1, 3])
    presenter._table_model = mock.MagicMock()
    presenter._facility = SANSFacility.ISIS
    if use_fake_state:
        presenter.get_state_for_row = mock.MagicMock(side_effect=get_state_for_row_mock)
    else:
        presenter.get_state_for_row = mock.MagicMock(side_effect=get_state_for_row_mock_with_real_state)
    return presenter


class FakeParentPresenter(object):
    def __init__(self):
        super(FakeParentPresenter, self).__init__()

    def get_row_indices(self):
        # We assume that row 2 is empty
        return [0, 1, 3]

    def get_state_for_row(self, row_index):
        return FakeState() if row_index == 3 else ""
