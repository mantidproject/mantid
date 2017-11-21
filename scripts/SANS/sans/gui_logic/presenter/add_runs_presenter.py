from mantid import ConfigService


class AddRunsPagePresenter(object):
    def __init__(self,
                 sum_runs,
                 make_run_selector_presenter,
                 make_run_summation_presenter,
                 view,
                 parent_view):
        self._view = view
        self._sum_runs = sum_runs
        self._run_selector_presenter = \
            make_run_selector_presenter(view.run_selector_view(), view)
        self._summation_settings_presenter = \
            make_run_summation_presenter(view.summation_settings_view(), view) \

        self._connect_to_view(view)

    def _init_views(self, view, parent_view):
        self._view = view
        self._parent = parent_view

    def _connect_to_view(self, view):
        view.sum.connect(self._handle_sum)

    def _handle_sum(self):
        self._sum_runs(self._run_selector_presenter.run_selection(),
                       self._default_instrument(),
                       self._summation_settings_presenter.settings())

    def _default_instrument(self):
        return ConfigService.getString("default.instrument")
