import unittest
import sys
from mantid import ConfigService
from ui.sans_isis.add_runs_page import AddRunsPage
from sans.gui_logic.presenter.add_runs_presenter import AddRunsPagePresenter
from sans.gui_logic.models.run_summation import RunSummation
from sans.gui_logic.models.binning_type import BinningType
from sans.gui_logic.models.summation_settings import SummationSettings
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter
from sans.gui_logic.presenter.run_selector_presenter import RunSelectorPresenter
from fake_signal import FakeSignal

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class AddRunsPagePresenterTest(unittest.TestCase):
    def setUp(self):
        self.run_summation = self._make_mock_run_summation()
        self.run_selector_presenter = self._make_mock_run_selector_presenter()
        self.summation_settings_presenter = self._make_mock_summation_settings_presenter()
        self.view = self._make_mock_view()
        self.presenter = self._make_presenter(self.run_summation,
                                              self._just_use(self.run_selector_presenter),
                                              self._just_use(self.summation_settings_presenter))

    def _make_presenter(self,
                        run_summation,
                        make_run_selector_presenter,
                        make_summation_settings_presenter):
        return AddRunsPagePresenter(run_summation,
                                    make_run_selector_presenter,
                                    make_summation_settings_presenter,
                                    self.view,
                                    None)

    def _just_use(self, presenter):
        return lambda view, parent: presenter

    def _make_mock_view(self):
        mock_view = mock.create_autospec(AddRunsPage, spec_set=True)
        mock_view.sum = FakeSignal()
        return mock_view

    def _make_mock_run_selector_presenter(self):
        return mock.create_autospec(RunSelectorPresenter, spec_set=True)

    def _make_mock_summation_settings_presenter(self):
        return mock.create_autospec(SummationSettingsPresenter, spec_set=True)

    def _make_mock_run_summation(self):
        return mock.create_autospec(RunSummation, spec_set=True)

    def test_creates_run_selector_with_child_of_view(self):
        fake_run_selector_view = 'Fake Run Selector View'
        self.view.run_selector_view.return_value = fake_run_selector_view

        make_run_selector_presenter = mock.Mock(return_value=self.run_selector_presenter)

        self.presenter = self._make_presenter(self.run_summation,
                                              make_run_selector_presenter,
                                              self._just_use(self.summation_settings_presenter))
        make_run_selector_presenter.assert_called_with(fake_run_selector_view, self.view)

    def test_creates_summation_settings_with_child_of_view(self):
        fake_summation_settings_view = 'Fake Summation Settings View'
        self.view.summation_settings_view.return_value = fake_summation_settings_view

        make_summation_settings_presenter = mock.Mock(return_value=self.run_selector_presenter)
        self.presenter = self._make_presenter(self.run_summation,
                                              self._just_use(self.run_selector_presenter),
                                              make_summation_settings_presenter)
        make_summation_settings_presenter.assert_called_with(fake_summation_settings_view, self.view)

    def test_invokes_model_with_config_when_summation_requested(self):
        run_summation = mock.Mock()
        self.presenter = self._make_presenter(run_summation,
                                              self._just_use(self.run_selector_presenter),
                                              self._just_use(self.summation_settings_presenter))
        fake_run_selection = ['1', '2', '3']
        fake_summation_settings = SummationSettings(BinningType.SaveAsEventData)
        self.run_selector_presenter.run_selection.return_value = fake_run_selection
        self.summation_settings_presenter.settings.return_value = fake_summation_settings
        self.view.sum.emit()
        run_summation.assert_called_with(fake_run_selection,
                                         ConfigService.getString("default.instrument"),
                                         fake_summation_settings)


if __name__ == '__main__': unittest.main()
