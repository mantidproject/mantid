# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import SANSadd2
from ui.sans_isis.work_handler import WorkHandler


class RunSummation(object):
    def __init__(self, work_handler, view=None):
        self._work_handler = work_handler
        self._view = view

    class Listener(WorkHandler.WorkListener):
        def __init__(self, presenter):
            self._presenter = presenter

        def on_processing_finished(self, result):
            self._presenter.on_processing_finished(result)

        def on_processing_error(self, error):
            # currently no different functionality required between
            # processing finished and processing error,
            # so call the same method
            self._presenter.on_processing_finished(error)

    def __call__(self, run_selection, settings, base_file_name):
        self._work_handler.process(RunSummation.Listener(self), self.run, 0, run_selection, settings, base_file_name)

    def run(self, run_selection, settings, base_file_name):
        run_selection = self._run_selection_as_path_list(run_selection)
        binning = self._bin_settings_or_monitors(settings)
        additional_time_shifts = self._time_shifts_or_empty_string(settings)
        overlay_event_workspaces = self._is_overlay_event_workspaces_enabled(settings)
        save_as_event = self._should_save_as_event_workspaces(settings)

        file_name = base_file_name + '.nxs'
        monitors_file_name = base_file_name + '_monitors.nxs'

        SANSadd2.add_runs(
            tuple(run_selection),
            settings.instrument(),
            lowMem=True,
            binning=binning,
            isOverlay=overlay_event_workspaces,
            saveAsEvent=save_as_event,
            time_shifts=additional_time_shifts,
            outFile=file_name,
            outFile_monitors=monitors_file_name,
            save_directory=settings.save_directory,
            estimate_logs=True)

    def _run_selection_as_path_list(self, run_selection):
        return [run.file_path() for run in run_selection]

    def _bin_settings_or_monitors(self, settings):
        return settings.bin_settings if settings.has_bin_settings() \
            else 'Monitors'

    def _time_shifts_or_empty_string(self, settings):
        return settings.additional_time_shifts if settings.has_additional_time_shifts() \
            else ""

    def _is_overlay_event_workspaces_enabled(self, settings):
        return settings.is_overlay_event_workspaces_enabled() \
            if settings.has_overlay_event_workspaces() \
            else False

    def _should_save_as_event_workspaces(self, settings):
        return settings.should_save_as_event_workspaces()

    def on_processing_finished(self, result):
        if self._view:
            self._view.enable_sum()
