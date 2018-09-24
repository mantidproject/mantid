import unittest
import sys
from ui.sans_isis.add_runs_page import AddRunsPage
from ui.sans_isis.sans_data_processor_gui import SANSDataProcessorGui
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from sans.gui_logic.models.run_summation import RunSummation
from sans.gui_logic.models.run_file import SummableRunFile
from sans.gui_logic.models.run_selection import RunSelection
from sans.gui_logic.models.summation_settings import SummationSettings
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter
from sans.gui_logic.presenter.run_selector_presenter import RunSelectorPresenter
from fake_signal import FakeSignal
from assert_called import assert_called
from sans.common.enums import SANSInstrument

if sys.version_info.major == 2:
    import mock
else:
    from unittest import mock


class AddRunsPagePresenterTestCase(unittest.TestCase):
    def _make_mock_view(self):
        mock_view = mock.create_autospec(AddRunsPage, spec_set=True)
        mock_view.sum = FakeSignal()
        mock_view.outFileChanged = FakeSignal()
        return mock_view

    def _make_mock_parent_view(self):
        mock_parent_view = mock.create_autospec(SANSDataProcessorGui, spec_set=True)
        mock_parent_view.instrument.to_string.return_value = 'LOQ'
        return mock_parent_view

    def setUpMockChildPresenters(self):
        self._summation_settings_presenter = \
            self._make_mock_summation_settings_presenter()
        self.run_selector_presenter = \
            self._make_mock_run_selector_presenter()

    def setUpFakeChildViews(self):
        self.fake_run_selector_view = 'Fake Run Selector View'
        self.fake_summation_settings_view = 'Fake Summation Settings View'

    def setUpMockChildPresentersWithDefaultSummationSettings(self):
        self.setUpMockChildPresenters()
        self._summation_settings = self._summation_settings_with_save_directory('/dev/null')
        self._summation_settings_presenter.settings.return_value = self._summation_settings

    def _just_use(self, presenter):
        return lambda *args: presenter

    def _make_mock_run_selector_presenter(self):
        return mock.create_autospec(RunSelectorPresenter, spec_set=True)

    def _make_mock_summation_settings_presenter(self):
        return mock.create_autospec(SummationSettingsPresenter, spec_set=True)

    def _just_use_run_selector_presenter(self):
        return self._just_use(self.run_selector_presenter)

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self._summation_settings_presenter)

    def _make_mock_run_selection(self, iterable):
        mock_runs = mock.create_autospec(RunSelection, spec_set=True)
        mock_runs.__iter__.return_value = list(iterable)
        return mock_runs

    def _make_mock_run_summation(self):
        return mock.create_autospec(RunSummation, spec_set=True)

    def _make_fake_run(self, path, is_event_data=False):
        return SummableRunFile(path, path, is_event_data)

    def _make_fake_histogram_run(self, path):
        return self._make_fake_run(path, path, False)

    def _make_fake_event_run(self, path):
        return self._make_fake_run(path, path, True)

    def _summation_settings_with_save_directory(self, directory):
        mock_summation_settings = mock.create_autospec(SummationSettings, spec_set=True)
        mock_summation_settings.save_directory.return_value = directory
        return mock_summation_settings


class InitializationTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.setUpFakeChildViews()
        self.setUpMockChildPresentersWithDefaultSummationSettings()
        self.run_summation = self._make_mock_run_summation()
        self.view = self._make_mock_view()
        self._parent_view = self._make_mock_parent_view()

    def _make_presenter_with_child_presenters(self,
                                              run_selection,
                                              summation_settings):
        return AddRunsPagePresenter(self.run_summation,
                                    run_selection,
                                    summation_settings,
                                    self.view,
                                    None)

    def test_creates_run_selector_with_child_view(self):
        self.view.run_selector_view.return_value = self.fake_run_selector_view
        make_run_selector_presenter = \
            mock.Mock(return_value=self.run_selector_presenter)

        self.presenter = self._make_presenter_with_child_presenters(
            make_run_selector_presenter,
            self._just_use_summation_settings_presenter())

        call_args = make_run_selector_presenter.call_args[0]
        self.assertEqual(self.fake_run_selector_view, call_args[0])
        self.assertEqual(self.view, call_args[2])

    def test_creates_summation_settings_with_child_view(self):
        self.view.summation_settings_view.return_value = \
            self.fake_summation_settings_view
        make_summation_settings_presenter = \
            mock.Mock(return_value=self._summation_settings_presenter)

        self.presenter = self._make_presenter_with_child_presenters(
                                 self._just_use_run_selector_presenter(),
                                 make_summation_settings_presenter)

        call_args = make_summation_settings_presenter.call_args[0]
        self.assertEqual(self.fake_summation_settings_view, call_args[0])
        self.assertEqual(self.view, call_args[1])


class SelectionMockingTestCase(AddRunsPagePresenterTestCase):
    # Creates a factory function which stores the run summation
    # change callback allowing us to notify the presenter when
    # the view should be synchronised with the model.
    def _capture_on_change_callback(self, presenter):
        def run_selector_presenter_factory(view, callback, parent_view):
            self._on_model_updated = callback
            return presenter
        return run_selector_presenter_factory

    def _update_selection_model(self, new_selection):
        self.run_selector_presenter.run_selection.return_value = new_selection
        self._on_model_updated(new_selection)

    def _make_mock_run_selection_from_paths(self, paths):
        return self._make_mock_run_selection(self._make_fake_run(path) for path in paths)


class SummationSettingsViewEnablednessTest(SelectionMockingTestCase):
    def setUp(self):
        self.setUpMockChildPresenters()
        self._view = self._make_mock_view()
        self._parent_view = self._make_mock_parent_view()
        self._event_run = self._make_fake_event_run()
        self._histogram_run = self._make_fake_histogram_run()
        self._make_presenter()

    def _make_fake_event_run(self):
        run = mock.create_autospec(SummableRunFile, spec_set=True)
        run.display_name.return_value = '14'
        run.is_event_data.return_value = True
        return run

    def _make_fake_histogram_run(self):
        run = mock.create_autospec(SummableRunFile, spec_set=True)
        run.display_name.return_value = '10'
        run.is_event_data.return_value = False
        return run

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self._summation_settings_presenter)

    def _make_presenter(self):
        return AddRunsPagePresenter(mock.Mock(),
                                    self._capture_on_change_callback(self.run_selector_presenter),
                                    self._just_use_summation_settings_presenter(),
                                    self._view,
                                    self._parent_view)

    def test_disables_summation_settings_when_no_event_data(self):
        runs = self._make_mock_run_selection([self._histogram_run,
                                              self._histogram_run])
        self._on_model_updated(runs)
        assert_called(self._view.disable_summation_settings)

    def test_enables_summation_settings_when_event_data(self):
        runs = self._make_mock_run_selection([self._event_run,
                                              self._event_run])
        self._on_model_updated(runs)
        assert_called(self._view.enable_summation_settings)

    def test_enables_summation_settings_when_event_and_histogram_data(self):
        runs = self._make_mock_run_selection([self._histogram_run,
                                              self._event_run])
        self._on_model_updated(runs)
        assert_called(self._view.enable_summation_settings)


class SummationConfigurationTest(SelectionMockingTestCase):
    def setUp(self):
        self.setUpMockChildPresentersWithDefaultSummationSettings()
        self.view = self._make_mock_view()
        self.parent_view = self._make_mock_parent_view()

    def _make_presenter(self,
                        run_summation,
                        run_selection,
                        summation_settings):
        return AddRunsPagePresenter(run_summation,
                                    run_selection,
                                    summation_settings,
                                    self.view,
                                    self.parent_view)

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self._summation_settings_presenter)

    def test_passes_correct_config_when_summation_requested(self):
        run_summation = mock.Mock()

        self.presenter = self._make_presenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter())

        fake_run_selection = self._make_mock_run_selection_from_paths(['3'])
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self._on_model_updated(fake_run_selection)
        self.view.sum.emit()
        run_summation.assert_called_with(fake_run_selection,
                                         self._summation_settings,
                                         'LOQ3-add')

    def test_shows_error_when_empty_default_directory(self):
        summation_settings_model = self._summation_settings_with_save_directory('')
        self._summation_settings_presenter.settings.return_value = summation_settings_model
        self.presenter = self._make_presenter(
            mock.Mock(),
            self._just_use_run_selector_presenter(),
            self._just_use_summation_settings_presenter())

        self.view.sum.emit()
        assert_called(self.view.no_save_directory)


class BaseFileNameTest(SelectionMockingTestCase):
    def setUp(self):
        self.setUpMockChildPresentersWithDefaultSummationSettings()
        self.view = self._make_mock_view()
        self.parent_view = self._make_mock_parent_view()

    def _make_presenter(self,
                        run_summation):
        return AddRunsPagePresenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter(),
            self.view,
            self.parent_view)

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self._summation_settings_presenter)

    def _update_selection_model(self, new_selection):
        self.run_selector_presenter.run_selection.return_value = new_selection
        self._on_model_updated(new_selection)

    def _base_file_name_arg(self, run_summation_mock):
        first_call = 0
        return run_summation_mock.call_args[first_call][2]

    def _retrieve_generated_name_for(self, run_paths):
        run_summation = mock.Mock()
        presenter = self._make_presenter(run_summation)
        fake_run_selection = self._make_mock_run_selection_from_paths(run_paths)
        self._update_selection_model(fake_run_selection)
        self.view.sum.emit()
        return self._base_file_name_arg(run_summation)

    def test_generates_correct_base_name(self):
        generated_name = self._retrieve_generated_name_for(['1', '2', '3'])
        self.assertEqual('LOQ3-add', generated_name)

    def test_regenerates_correct_base_name_after_highest_removed(self):
        run_summation = mock.Mock()
        presenter = self._make_presenter(run_summation)
        self._update_selection_model(self._make_mock_run_selection_from_paths(['4', '5', '6']))
        self._update_selection_model(self._make_mock_run_selection_from_paths(['4', '5']))
        self.view.sum.emit()
        self.assertEqual('LOQ5-add', self._base_file_name_arg(run_summation))

    def test_correct_base_name_after_set_by_user(self):
        user_out_file_name = 'Output'
        run_summation = mock.Mock()
        presenter = self._make_presenter(run_summation)
        self._update_selection_model(self._make_mock_run_selection_from_paths(['4', '5', '6']))

        self.view.out_file_name.return_value = user_out_file_name
        self.view.outFileChanged.emit()
        self.view.sum.emit()
        self.assertEqual(user_out_file_name,
                         self._base_file_name_arg(run_summation))

    def test_base_name_not_reset_after_set_by_user(self):
        run_summation = mock.Mock()
        presenter = self._make_presenter(run_summation)
        self._update_selection_model(self._make_mock_run_selection_from_paths(['4', '5', '6']))

        user_out_file_name = 'Output'
        self.view.out_file_name.return_value = user_out_file_name
        self.view.outFileChanged.emit()

        self._update_selection_model(self._make_mock_run_selection_from_paths(['4', '5']))
        self.view.sum.emit()
        self.assertEqual(user_out_file_name,
                         self._base_file_name_arg(run_summation))

    def test_sets_name_in_view_after_selection_update(self):
        run_summation = mock.Mock()
        presenter = self._make_presenter(run_summation)
        self._update_selection_model(self._make_mock_run_selection_from_paths(['4', '6', '5']))
        self.view.set_out_file_name.assert_called_with('LOQ6-add')
        self._update_selection_model(self._make_mock_run_selection_from_paths(['5', '4']))
        self.view.set_out_file_name.assert_called_with('LOQ5-add')


class SumButtonTest(SelectionMockingTestCase):
    def setUp(self):
        self.setUpMockChildPresentersWithDefaultSummationSettings()
        self.run_summation = self._make_mock_run_summation()
        self.view = self._make_mock_view()
        self.parent_view = self._make_mock_parent_view()
        self.presenter = self._make_presenter()

    def _make_presenter(self):
        return AddRunsPagePresenter(
            self._make_mock_run_summation(),
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter(),
            self.view,
            self.parent_view)

    def test_enables_sum_button_when_row_added(self):
        fake_run_selection = self._make_mock_run_selection_from_paths(['5'])
        fake_run_selection.has_any_runs.return_value = True
        self._update_selection_model(fake_run_selection)
        assert_called(self.view.enable_sum)

    def test_disables_sum_button_when_no_rows(self):
        fake_run_selection = self._make_mock_run_selection_from_paths(['5'])
        fake_run_selection.has_any_runs.return_value = True
        self._update_selection_model(fake_run_selection)
        fake_run_selection.has_any_runs.return_value = False
        self._update_selection_model(fake_run_selection)
        assert_called(self.view.disable_sum)

if __name__ == '__main__': unittest.main()
