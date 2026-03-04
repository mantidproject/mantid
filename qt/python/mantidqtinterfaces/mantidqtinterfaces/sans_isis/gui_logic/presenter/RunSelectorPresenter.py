# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import ConfigService


class RunSelectorPresenter(object):
    file_extensions = [".nxs"]

    def __init__(self, title, run_selection, run_finder, view, parent_view):
        self._run_selection = run_selection
        self._run_finder = run_finder
        self.view = view
        self.view.title = title
        self.parent_view = parent_view
        self._connect_to_view(view)
        self._refresh()

    def run_selection(self):
        return self._run_selection

    def _connect_to_view(self, view):
        view.manageDirectories.connect(self._handle_manage_directories)
        view.addRuns.connect(self._handle_add_items)
        view.removeRuns.connect(self._handle_remove_items)
        view.removeAllRuns.connect(self._handle_remove_all_items)
        view.browse.connect(self._handle_browse)

    def _handle_remove_all_items(self):
        self._run_selection.clear_all_runs()
        self._refresh()

    def _handle_remove_items(self):
        selected = self.view.selected_runs()
        selected.sort(reverse=True)
        for index in selected:
            self._run_selection.remove_run(index)
        self._refresh()

    def _parse_runs_from_input(self, input):
        sanitized_input = input.replace(":", "-")
        return self._run_finder.find_all_from_query(sanitized_input)

    def _handle_add_items(self):
        input = self.view.run_list()
        error, runs = self._parse_runs_from_input(input)
        if error:
            self.view.invalid_run_query(error)
        else:
            if runs:
                self._add_runs(runs)
            else:
                self.view.run_not_found()

    def _add_runs(self, runs):
        for run in runs:
            self._run_selection.add_run(run)
        self._refresh()

    def _refresh(self):
        self.view.draw_runs(self._run_selection)

    def _handle_manage_directories(self):
        self.view.show_directories_manager()

    def find_from_file_path(self, file_path):
        return self._run_finder.find_from_file_path(file_path)

    def _handle_browse(self):  # Add Error handling
        search_directories = ConfigService.Instance().getDataSearchDirs()
        file_paths = self.view.show_file_picker(RunSelectorPresenter.file_extensions, search_directories)
        self._add_runs(self.find_from_file_path(file_path) for file_path in file_paths)
        self._refresh()
