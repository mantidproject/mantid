from sans.gui_logic.models.run_selection import has_any_event_data
from mantid.kernel import ConfigService, ConfigPropertyObserver


class OutputDirectoryObserver(ConfigPropertyObserver):
    def __init__(self, callback):
        super(OutputDirectoryObserver, self).__init__("defaultsave.directory")
        self.callback = callback

    def onPropertyValueChanged(self, new_value, old_value):
        self.callback(new_value)


class AddRunsPagePresenter(object):
    def __init__(self,
                 sum_runs,
                 make_run_selector_presenter,
                 make_run_summation_presenter,
                 view,
                 parent_view):
        self._view = view
        self._sum_runs = sum_runs
        self._use_generated_file_name = True
        self._run_selector_presenter = \
            make_run_selector_presenter(view.run_selector_view(),
                                        self._handle_selection_changed, view)
        self._summation_settings_presenter = \
            make_run_summation_presenter(view.summation_settings_view(),
                                         view)

        self._connect_to_view(view)
        self._output_directory_observer = \
            OutputDirectoryObserver(self._handle_output_directory_changed)

    def _init_views(self, view, parent_view):
        self._view = view
        self._parent = parent_view

    def _connect_to_view(self, view):
        view.sum.connect(self._handle_sum)
        view.outFileChanged.connect(self._handle_out_file_changed)
        self._view.set_out_file_directory(ConfigService.Instance().getString("defaultsave.directory"))

    def _make_base_file_name_from_selection(self, run_selection):
        # Aims to use the run with the highest run number.
        # Therefore assumes that file names all have the same number of
        # leading zeroes since a shorter string is sorted after a longer one.
        names = [run.display_name() for run in run_selection]
        return (max(names) + '-add' if names else '')

    def _sum_base_file_name(self, run_selection):
        if self._use_generated_file_name:
            return self._generated_output_file_name
        else:
            return self._view.out_file_name()

    def _refresh_view(self, run_selection):
        self._update_output_filename(run_selection)
        self._update_histogram_binning(run_selection)
        if run_selection.has_any_runs():
            self._view.enable_sum()
        else:
            self._view.disable_sum()

    def _update_output_filename(self, run_selection):
        self._generated_output_file_name = self._make_base_file_name_from_selection(run_selection)
        if self._use_generated_file_name:
            self._view.set_out_file_name(self._generated_output_file_name)

    def _update_histogram_binning(self, run_selection):
        if has_any_event_data(run_selection):
            self._view.enable_summation_settings()
        else:
            self._view.disable_summation_settings()

    def _handle_selection_changed(self, run_selection):
        self._refresh_view(run_selection)

    def _handle_out_file_changed(self):
        self._use_generated_file_name = False

    def _output_directory_is_not_empty(self, settings):
        return settings.save_directory() != ''

    def _handle_output_directory_changed(self, new_directory):
        self._view.set_out_file_directory(new_directory)

    def _handle_sum(self):
        run_selection = self._run_selector_presenter.run_selection()
        settings = self._summation_settings_presenter.settings()
        if self._output_directory_is_not_empty(settings):
            self._sum_runs(run_selection,
                           settings,
                           self._sum_base_file_name(run_selection))
        else:
            self._view.no_save_directory()
