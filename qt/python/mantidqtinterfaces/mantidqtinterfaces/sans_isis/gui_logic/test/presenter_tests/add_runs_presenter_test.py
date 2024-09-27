# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from mantid.kernel import ConfigService
from unittest import mock
from sans_core.common.enums import SANSInstrument
from mantidqtinterfaces.sans_isis.gui_logic.models.sum_runs_model import SumRunsModel
from mantidqtinterfaces.sans_isis.gui_logic.models.SummationSettingsModel import SummationSettingsModel
from mantidqtinterfaces.sans_isis.gui_logic.models.run_file import SummableRunFile
from mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter, AddRunsFilenameManager
from mantidqtinterfaces.sans_isis.gui_logic.test.fake_signal import FakeSignal
from mantidqtinterfaces.sans_isis.views.add_runs_page import AddRunsPage
from mantidqtinterfaces.sans_isis.views.sans_data_processor_gui import SANSDataProcessorGui


class MockedOutAddRunsFilenameManager(AddRunsFilenameManager):
    def __init__(self):
        super(MockedOutAddRunsFilenameManager, self).__init__("LOQ")
        self.instrument_string = "LOQ"

    def _get_leading_zeroes(self, run_number):
        # Return four 0s as all examples we are using in testing
        # Would normally require 4
        return 4 * "0"

    def make_filename(self, run_numbers):
        return "LOQ0000" + str(max(run_numbers)) + "-add"


class AddRunsPagePresenterTestCase(unittest.TestCase):
    def _make_mock_view(self):
        mock_view = mock.create_autospec(AddRunsPage, spec_set=True)
        mock_view.sum = FakeSignal()
        mock_view.customOutFileChanged = FakeSignal()
        mock_view.saveDirectoryClicked = FakeSignal()
        return mock_view

    def _make_mock_parent_view(self):
        mock_parent_view = mock.create_autospec(SANSDataProcessorGui, spec_set=True)
        mock_parent_view.instrument = SANSInstrument.LOQ
        return mock_parent_view

    def setUpFakeChildViews(self):
        self.fake_run_selector_view = "Fake Run Selector View"
        self.fake_summation_settings_view = "Fake Summation Settings View"

    def _make_mock_run_summation(self):
        return mock.create_autospec(SumRunsModel, spec_set=True)

    def _make_fake_run(self, path, is_event_data=False):
        return SummableRunFile(path, path, is_event_data)

    def _summation_settings_with_save_directory(self, directory):
        mock_summation_settings = mock.create_autospec(SummationSettingsModel, spec_set=True)
        mock_summation_settings.save_directory = directory
        return mock_summation_settings


class InitializationTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.setUpFakeChildViews()

    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.RunSelectionModel", autospec=True)
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.RunSelectorPresenter", autospec=True)
    def test_creates_run_selector_with_child_view(self, patched_presenter, patched_model):
        view = self._make_mock_view()
        parent_view = self._make_mock_parent_view()

        AddRunsPagePresenter(sum_runs_model=mock.Mock(), view=view, parent_view=parent_view)

        patched_model.assert_called_once_with(mock.ANY)
        patched_presenter.assert_called_once_with(mock.ANY, patched_model.return_value, mock.ANY, mock.ANY, view)

    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.SummationSettingsModel", autospec=True)
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.SummationSettingsPresenter", autospec=True)
    def test_creates_run_selector_summation_model_with_child_view(self, patched_presenter, patched_model):
        view = self._make_mock_view()
        parent_view = self._make_mock_parent_view()

        AddRunsPagePresenter(sum_runs_model=mock.Mock(), view=view, parent_view=parent_view)

        patched_model.assert_called_once_with(mock.ANY)
        patched_presenter.assert_called_once_with(patched_model.return_value, mock.ANY, view)


class SummationSettingsViewEnablednessTest(AddRunsPagePresenterTestCase):
    def _make_fake_event_run(self):
        run = mock.create_autospec(SummableRunFile, spec_set=True)
        run.display_name.return_value = "14"
        run.is_event_data.return_value = True
        return run

    def _make_fake_histogram_run(self):
        run = mock.create_autospec(SummableRunFile, spec_set=True)
        run.display_name.return_value = "10"
        run.is_event_data.return_value = False
        return run

    def _make_presenter(self):
        self.view = self._make_mock_view()
        parent_view = self._make_mock_parent_view()
        presenter = AddRunsPagePresenter(mock.Mock(), self.view, parent_view)
        presenter._get_filename_manager = mock.Mock(return_value=MockedOutAddRunsFilenameManager())
        return presenter

    @mock.patch(
        "mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.AddRunsPagePresenter._handle_custom_outfile_check_changed"
    )
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.RunSelectionModel", autospec=True)
    def test_enables_summation_settings_when_event_data(self, _patched_init_run_selector, _):
        histogram_run = self._make_fake_histogram_run()
        runs = mock.MagicMock()
        runs.has_any_runs.return_value = True
        runs.__iter__.return_value = [histogram_run, histogram_run]

        presenter = self._make_presenter()
        presenter._handle_selection_changed(run_selection=runs)
        self.view.enable_summation_settings.assert_called_once()

    @mock.patch(
        "mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.AddRunsPagePresenter._handle_custom_outfile_check_changed"
    )
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.RunSelectionModel", autospec=True)
    def test_enables_summation_settings_when_event_and_histogram_data(self, _, _handle_check):
        histogram_run = self._make_fake_histogram_run()
        event_run = self._make_fake_event_run()

        runs = mock.MagicMock()
        runs.has_any_runs.return_value = True
        runs.__iter__.return_value = [event_run, histogram_run]

        presenter = self._make_presenter()
        presenter._handle_selection_changed(run_selection=runs)
        self.view.enable_summation_settings.assert_called_once()


class SummationConfigurationTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.view = self._make_mock_view()
        self.parent_view = self._make_mock_parent_view()

    @mock.patch(
        "mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.AddRunsPagePresenter._handle_custom_outfile_check_changed"
    )
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.RunSelectorPresenter", autospec=True)
    def test_passes_correct_config_when_summation_requested(self, patched_run_selector, _):
        # Ensure we know the type that was returned by the constructor
        run_selector_mock = mock.Mock()
        patched_run_selector.return_value = run_selector_mock

        ConfigService["defaultsave.directory"] = "someDir/"
        run_summation = mock.Mock()

        presenter = AddRunsPagePresenter(run_summation, self.view, self.parent_view)
        presenter._get_filename_manager = mock.Mock(return_value=MockedOutAddRunsFilenameManager())

        mocked_run_selector = mock.MagicMock()
        mocked_run_selector.has_any_runs.return_value = True
        run_number_mock = mock.Mock()
        run_number_mock.display_name.return_value = 3
        mocked_run_selector.__iter__.return_value = [run_number_mock]

        run_selector_mock.run_selection.return_value = mocked_run_selector
        self.view.sum.emit()
        run_summation.assert_called_with(mock.ANY, mock.ANY, "LOQ00003-add")

    @mock.patch(
        "mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.AddRunsPagePresenter._handle_custom_outfile_check_changed"
    )
    @mock.patch("mantidqtinterfaces.sans_isis.gui_logic.presenter.add_runs_presenter.RunSelectionModel", autospec=True)
    def test_shows_error_when_empty_default_directory(self, _, _handle_check):
        view = self._make_mock_view()
        presenter = AddRunsPagePresenter(mock.MagicMock(), view, mock.Mock())
        presenter.save_directory = ""

        view.sum.emit()
        view.no_save_directory.assert_any_call()


def create_mocked_runs(start, len):
    run_selection = [mock.Mock() for _ in range(len)]
    for x, mock_instance in enumerate(run_selection):
        mock_instance.display_name.return_value = start + x
    return run_selection


class BaseFileNameTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.view = self._make_mock_view()
        self.parent_view = self._make_mock_parent_view()
        ConfigService["defaultsave.directory"] = "someDir/"

    def tearDown(self):
        ConfigService["defaultsave.directory"] = ""

    def _make_presenter(self, run_summation):
        presenter = AddRunsPagePresenter(run_summation, self.view, self.parent_view)
        presenter._get_filename_manager = mock.Mock(return_value=MockedOutAddRunsFilenameManager())
        return presenter

    def test_generates_correct_base_name(self):
        run_summation = mock.MagicMock()
        presenter = self._make_presenter(run_summation)

        run_summation.has_any_runs.return_value = True

        run_selection = mock.Mock()
        run_selection.display_name.return_value = 3
        run_summation.__iter__.return_value = [run_selection]
        returned = presenter._sum_base_file_name(run_summation)

        self.assertEqual("LOQ00003-add", returned)

    def test_regenerates_correct_base_name_after_highest_removed(self):
        run_summation = mock.MagicMock()
        presenter = self._make_presenter(run_summation)

        run_summation.has_any_runs.return_value = True

        # Runs 4 / 5 / 6
        run_selection = create_mocked_runs(start=4, len=3)

        run_summation.__iter__.return_value = run_selection
        returned = presenter._sum_base_file_name(run_summation)
        self.assertEqual("LOQ00006-add", returned)

        # Drop run 6
        del run_selection[-1]
        returned = presenter._sum_base_file_name(run_summation)

        self.assertEqual("LOQ00005-add", returned)

    def test_correct_base_name_after_set_by_user_and_custom_selected(self):
        user_out_file_name = "Output"
        run_summation = mock.MagicMock()
        presenter = self._make_presenter(run_summation)

        # Runs 4 / 5 / 6
        run_selection = create_mocked_runs(start=4, len=3)

        run_summation.__iter__.return_value = run_selection

        self.view.out_file_name.return_value = user_out_file_name
        self.view.customOutFileChanged.emit(True)

        returned = presenter._sum_base_file_name(run_selection=run_selection)

        self.assertEqual(returned, user_out_file_name)

    def test_base_name_not_reset_when_custom_selected(self):
        run_summation = mock.MagicMock()
        presenter = self._make_presenter(run_summation)
        # Runs 4 / 5 / 6
        run_selection = create_mocked_runs(start=4, len=3)

        user_out_file_name = "Output"
        self.view.out_file_name.return_value = user_out_file_name
        self.view.customOutFileChanged.emit(True)

        run_summation.__iter__.return_value = run_selection
        returned = presenter._sum_base_file_name(run_selection=run_selection)
        self.assertEqual(user_out_file_name, returned)

        new_selection = mock.Mock()
        new_selection.display_name.return_value = 1

        run_summation.__iter__.return_value = run_selection
        returned = presenter._sum_base_file_name(run_selection=run_selection)
        self.assertEqual(user_out_file_name, returned)

    def test_sets_name_in_view_after_selection_update(self):
        run_summation = mock.MagicMock()
        presenter = self._make_presenter(run_summation)
        # Runs 4 / 5 / 6
        run_selection = create_mocked_runs(start=4, len=3)

        run_summation.__iter__.return_value = run_selection
        presenter._refresh_view(run_selection=run_summation)

        self.view.set_out_file_name.assert_called_with("LOQ00006-add")

    def test_custom_filename_box_disabled_and_repopulated_when_custom_filename_unchecked(self):
        run_summation = mock.MagicMock()
        presenter = self._make_presenter(run_summation)

        run_summation.has_any_runs.return_value = True

        # Runs 4 / 5 / 6
        run_selection = create_mocked_runs(start=4, len=3)

        run_summation.__iter__.return_value = run_selection
        presenter._run_selector_presenter.run_selection = mock.MagicMock()
        presenter._run_selector_presenter.run_selection.return_value = run_summation
        presenter._handle_custom_outfile_check_changed(False)

        self.view.disable_output_file_name_edit.assert_called()
        self.assertEqual(self.view.disable_output_file_name_edit.call_count, 2)
        self.view.set_out_file_name.assert_called_with("LOQ00006-add")

    def test_custom_filename_box_enabled_when_custom_filename_checked(self):
        run_summation = mock.MagicMock()
        _ = self._make_presenter(run_summation)
        self.view.customOutFileChanged.emit(True)

        self.view.enable_output_file_name_edit.assert_called_once()
        self.view.clear_output_file_name_edit.assert_called_once()


class SumButtonTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.run_summation = self._make_mock_run_summation()
        self.view = self._make_mock_view()
        self.parent_view = self._make_mock_parent_view()
        self.presenter = self._make_presenter()

    def _make_presenter(self):
        presenter = AddRunsPagePresenter(self._make_mock_run_summation(), self.view, self.parent_view)
        presenter._get_filename_manager = mock.Mock(return_value=MockedOutAddRunsFilenameManager())
        return presenter

    def test_enables_sum_button_when_row_added(self):
        fake_run_list = create_mocked_runs(start=5, len=1)
        fake_run_selection = mock.MagicMock()

        fake_run_selection.has_any_runs.return_value = True
        fake_run_selection.__iter__.return_value = fake_run_list

        self.presenter._handle_selection_changed(run_selection=fake_run_selection)
        self.view.enable_sum.assert_called_once()

    def test_disables_sum_button_when_no_rows(self):
        fake_run_selection = mock.Mock()
        fake_run_selection.has_any_runs.return_value = False

        self.view.disable_sum.reset_mock()
        self.presenter._handle_selection_changed(run_selection=fake_run_selection)
        self.view.disable_sum.assert_called_once()


class AddRunsFilenameManagerTest(unittest.TestCase):
    def _get_filename_manager(self, inst_string):
        return AddRunsFilenameManager(inst_string)

    def test_that_filename_manager_selects_correct_run_for_name(self):
        filename_manager = self._get_filename_manager("LOQ")
        expected_run = "105476"
        actual_run = filename_manager._select_max_run([105475, 105476, 105475, 99999])
        self.assertEqual(actual_run, expected_run)

    def test_that_filename_manager_gets_correct_zeros(self):
        filename_manager = self._get_filename_manager("LOQ")

        expected_zeroes = 2 * "0"
        actual_zeroes = filename_manager._get_leading_zeroes("105476")
        self.assertEqual(actual_zeroes, expected_zeroes)

    def test_that_filename_manager_gets_facility_zeros_for_run_before_definition(self):
        filename_manager = self._get_filename_manager("LOQ")
        expected_zeroes = 1 * "0"
        actual_zeroes = filename_manager._get_leading_zeroes("7777")
        self.assertEqual(actual_zeroes, expected_zeroes)

    def test_that_make_filename_return_empty_string_if_no_runs(self):
        filename_manager = self._get_filename_manager("LOQ")
        expected_name = ""
        actual_name = filename_manager.make_filename([])
        self.assertEqual(actual_name, expected_name)

    def test_that_make_filename_returns_correct_string_if_runs_present(self):
        filename_manager = self._get_filename_manager("LOQ")
        expected_name = "LOQ00105476-add"
        runs = ["105476", "105466"]
        actual_name = filename_manager.make_filename(runs)
        self.assertEqual(actual_name, expected_name)

    def test_that_make_filename_returns_fullname_if_one_is_given(self):
        filename_manager = self._get_filename_manager("LOQ")
        expected_name = "LOQ74044-add"
        runs = ["74045", "74065", "LOQ74044"]
        actual_name = filename_manager.make_filename(runs)
        self.assertEqual(actual_name, expected_name)


class AddRunsDefaultSettingsTest(unittest.TestCase):
    def setUp(self):
        mock_parent_view = mock.Mock()
        mock_parent_view.instrument = SANSInstrument.LOQ
        self.presenter = AddRunsPagePresenter(parent_view=mock_parent_view, sum_runs_model=mock.Mock(), view=mock.Mock())

    def test_that_presenter_calls_properties_handler_to_update_directory_on_directory_changed(self):
        new_dir_name = os.path.join("some", "dir", "path")
        self.presenter._view.display_save_directory_box = mock.Mock(return_value=new_dir_name)
        self.presenter.gui_properties_handler.set_setting = mock.Mock()
        self.presenter.set_output_directory = mock.Mock()

        self.presenter._handle_output_directory_changed()
        self.presenter.gui_properties_handler.set_setting.assert_called_once_with("add_runs_output_directory", new_dir_name + os.sep)
        self.presenter.set_output_directory.assert_called_once_with(new_dir_name + os.sep)

    def test_that_if_output_directory_is_empty_default_save_directory_is_used_instead(self):
        default_dir = os.path.join("default", "save", "directory")
        ConfigService["defaultsave.directory"] = default_dir
        # ConfigService may do some conversion in the background.
        default_dir = ConfigService["defaultsave.directory"]

        output_dir = self.presenter.set_output_directory("")
        ConfigService["defaultsave.directory"] = ""

        self.assertEqual(
            output_dir,
            default_dir,
            "Because directory input was an empty string, we expected the output directory "
            "to use the default save directory {} instead. "
            "Directory actually used was {}".format(default_dir, output_dir),
        )
        self.assertEqual(self.presenter.save_directory, default_dir)

    def test_that_if_output_directory_is_not_empty_it_is_used(self):
        dir = os.path.join("a", "save", "directory")
        output_dir = self.presenter.set_output_directory(dir)

        self.assertEqual(output_dir, dir)
        self.assertEqual(self.presenter.save_directory, dir)


if __name__ == "__main__":
    unittest.main()
