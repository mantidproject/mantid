from mantidqtpython import MantidQt
from mantid import ConfigService
from run_selector_presenter import RunSelectorPresenter
from summation_settings_presenter import SummationSettingsPresenter

class AddRunsPagePresenter(object):
    def __init__(self, sum_runs, run_selection, run_finder, summation_settings, view, parent_view):
        self._view = view
        self._sum_runs = sum_runs
        self._run_selector_presenter = \
            RunSelectorPresenter(run_selection, run_finder, self._view.run_selector, self)
        self._summation_settings_presenter = \
            SummationSettingsPresenter(summation_settings, self._view.summation_settings, self)
        self._connect_to_view(view)

    def _init_views(view, parent_view):
        self._view = view
        self._parent = parent_view

    def _connect_to_view(self, view):
        view.sum.connect(self._handle_sum)

    def _handle_sum(self):
        self._sum_runs(self._run_selector_presenter.run_selection, \
                       self._default_instrument(), \
                       self._summation_settings_presenter.settings)

    def _default_instrument(self):
        return ConfigService.getString("default.instument")
