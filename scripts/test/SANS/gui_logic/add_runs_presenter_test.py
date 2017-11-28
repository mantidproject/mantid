import unittest
import sys
from mantid import ConfigService
from ui.sans_isis.add_runs_page import AddRunsPage
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from sans.gui_logic.models.run_summation import RunSummation
from sans.gui_logic.models.run_file import RunFile
from sans.gui_logic.models.run_selection import RunSelection
from sans.gui_logic.models.binning_type import BinningType
from sans.gui_logic.models.summation_settings import SummationSettings
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter
from sans.gui_logic.presenter.run_selector_presenter import RunSelectorPresenter
from fake_signal import FakeSignal

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class AddRunsPagePresenterTestCase(unittest.TestCase):
    def _make_mock_view(self):
        mock_view = mock.create_autospec(AddRunsPage, spec_set=True)
        mock_view.sum = FakeSignal()
        mock_view.outFileChanged = FakeSignal()
        return mock_view

    def _just_use(self, presenter):
        return lambda *args: presenter

    def _make_mock_run_selector_presenter(self):
        return mock.create_autospec(RunSelectorPresenter, spec_set=True)

    def _make_mock_summation_settings_presenter(self):
        return mock.create_autospec(SummationSettingsPresenter, spec_set=True)

    def _make_mock_run_summation(self):
        return mock.create_autospec(RunSummation, spec_set=True)

    def _make_mock_run_selection(self):
        return mock.create_autospec(RunSelection, spec_set=True)

    def _make_run_file(self, path):
        return RunFile(path)


class AddRunsPagePresenterInitializationTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.set_up_fake_views()
        self.set_up_mock_child_presenters()
        self.run_summation = self._make_mock_run_summation()
        self.view = self._make_mock_view()

    def set_up_fake_views(self):
        self.fake_run_selector_view = 'Fake Run Selector View'
        self.fake_summation_settings_view = 'Fake Summation Settings View'

    def _make_presenter_with_child_presenters(self,
                                              run_selection,
                                              summation_settings):
        return AddRunsPagePresenter(self.run_summation,
                                    run_selection,
                                    summation_settings,
                                    self.view,
                                    None)

    def set_up_mock_child_presenters(self):
        self.summation_settings_presenter = \
            self._make_mock_summation_settings_presenter()
        self.run_selector_presenter = \
            self._make_mock_run_selector_presenter()

    def _just_use_run_selector_presenter(self):
        return self._just_use(self.run_selector_presenter)

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self.summation_settings_presenter)

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
            mock.Mock(return_value=self.summation_settings_presenter)

        self.presenter = self._make_presenter_with_child_presenters(
                                 self._just_use_run_selector_presenter(),
                                 make_summation_settings_presenter)

        call_args = make_summation_settings_presenter.call_args[0]
        self.assertEqual(self.fake_summation_settings_view, call_args[0])
        self.assertEqual(self.view, call_args[1])


class AddRunsPagePresenterModelConfigTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.set_up_mock_child_presenters()
        self.view = self._make_mock_view()

    def _make_presenter(self,
                        run_summation,
                        run_selection,
                        summation_settings):
        return AddRunsPagePresenter(run_summation,
                                    run_selection,
                                    summation_settings,
                                    self.view,
                                    None)

    def set_up_mock_child_presenters(self):
        self.summation_settings_presenter = \
            self._make_mock_summation_settings_presenter()
        self.run_selector_presenter = \
            self._make_mock_run_selector_presenter()

    def _make_fake_runs(self, run_paths):
        fake_selection = self._make_mock_run_selection()
        fake_selection.__iter__.return_value = \
            [RunFile(path) for path in run_paths]
        return fake_selection

    def _capture_on_change_callback(self, presenter):
        def lam(view, callback, parent_view):
            self._on_change_callback = callback
            return presenter
        return lam

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self.summation_settings_presenter)

    def test_passes_correct_config_when_summation_requested(self):
        run_summation = mock.Mock()
        self.presenter = self._make_presenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter())

        fake_run_selection = self._make_fake_runs(['3'])
        summation_settings_model = SummationSettings(BinningType.SaveAsEventData)
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self.summation_settings_presenter.settings.return_value = summation_settings_model
        self._on_change_callback(fake_run_selection)
        self.view.sum.emit()
        run_summation.assert_called_with(fake_run_selection,
                                         ConfigService.getString("default.instrument"),
                                         summation_settings_model,
                                         '3')


class AddRunsPagePresenterBaseFileNameTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        self.set_up_mock_child_presenters()
        self.view = self._make_mock_view()

    def _make_presenter(self,
                        run_summation,
                        run_selection,
                        summation_settings):
        return AddRunsPagePresenter(run_summation,
                                    run_selection,
                                    summation_settings,
                                    self.view,
                                    None)

    def set_up_mock_child_presenters(self):
        self.summation_settings_presenter = \
            self._make_mock_summation_settings_presenter()
        self.run_selector_presenter = \
            self._make_mock_run_selector_presenter()

    def _make_fake_runs(self, run_paths):
        fake_selection = self._make_mock_run_selection()
        fake_selection.__iter__.return_value = \
            [RunFile(path) for path in run_paths]
        return fake_selection

    def _capture_on_change_callback(self, presenter):
        def lam(view, callback, parent_view):
            self._on_change_callback = callback
            return presenter
        return lam

    def _just_use_summation_settings_presenter(self):
        return self._just_use(self.summation_settings_presenter)

    def retrieve_generated_name_for(self, run_paths):
        run_summation = mock.Mock()
        presenter = self._make_presenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter())
        fake_run_selection = self._make_fake_runs(run_paths)
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self.summation_settings_presenter.settings.return_value = SummationSettings(BinningType.SaveAsEventData)
        self._on_change_callback(fake_run_selection)
        self.view.sum.emit()
        return run_summation.call_args[0][3]

    def test_generates_correct_base_name(self):
        generated_name = self.retrieve_generated_name_for(['1', '2', '3'])
        self.assertEqual('3', generated_name)

    def test_regenerates_correct_base_name_after_highest_removed(self):
        run_summation = mock.Mock()
        presenter = self._make_presenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter())
        fake_run_selection = self._make_fake_runs(['4', '5', '6'])
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self.summation_settings_presenter.settings.return_value = SummationSettings(BinningType.SaveAsEventData)
        self._on_change_callback(fake_run_selection)

        fake_run_selection_without_highest = self._make_fake_runs(['4', '5'])
        self.run_selector_presenter.run_selection.return_value = fake_run_selection_without_highest
        self._on_change_callback(fake_run_selection_without_highest)
        self.view.sum.emit()
        self.assertEqual('5', run_summation.call_args[0][3])

    def test_correct_base_name_after_set_by_user(self):
        user_out_file_name = 'Output'
        run_summation = mock.Mock()
        presenter = self._make_presenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter())
        fake_run_selection = self._make_fake_runs(['4', '5', '6'])
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self.summation_settings_presenter.settings.return_value = SummationSettings(BinningType.SaveAsEventData)
        self._on_change_callback(fake_run_selection)

        self.view.out_file_name.return_value = user_out_file_name
        self.view.outFileChanged.emit()
        self.view.sum.emit()
        self.assertEqual(user_out_file_name, run_summation.call_args[0][3])

    def test_base_name_not_reset_after_set_by_user(self):
        run_summation = mock.Mock()
        presenter = self._make_presenter(
            run_summation,
            self._capture_on_change_callback(self.run_selector_presenter),
            self._just_use_summation_settings_presenter())
        fake_run_selection = self._make_fake_runs(['4', '5', '6'])
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self.summation_settings_presenter.settings.return_value = SummationSettings(BinningType.SaveAsEventData)
        self._on_change_callback(fake_run_selection)

        user_out_file_name = 'Output'
        self.view.out_file_name.return_value = user_out_file_name
        self.view.outFileChanged.emit()

        fake_run_selection_without_highest = self._make_fake_runs(['4', '5'])
        self.run_selector_presenter.run_selection.return_value = fake_run_selection_without_highest
        self._on_change_callback(fake_run_selection_without_highest)
        self.view.sum.emit()
        self.assertEqual(user_out_file_name, run_summation.call_args[0][3])

# TODO Remember to test setting the name to the view when it is generated!


class AddRunsPagePresenterSumButtonTest(AddRunsPagePresenterTestCase):
    def setUp(self):
        pass

    def test_enables_sum_button_when_row_added(self):
        pass

    def test_disables_sum_button_when_row_removed(self):
        pass
        
if __name__ == '__main__': unittest.main()
