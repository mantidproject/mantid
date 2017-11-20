from mantidqtpython import MantidQt
from mantid import ConfigService
from sans.gui_logic.models.run_file import RunFile

class RunSelectorPresenter(object):
    def __init__(self, run_selection, run_finder, view, parent_view):
        self._run_selection = run_selection
        self._run_finder = run_finder
        self.view = view
        self.parent_view = parent_view
        self._connect_to_view(view)
        self.refresh()

    def run_selection(self):
        return self._run_selection

    def _connect_to_view(self, view):
        view.manageDirectories.connect(self.handle_manage_directories)
        view.addRuns.connect(self.handle_add_items)
        view.removeRuns.connect(self.handle_remove_items)
        view.removeAllRuns.connect(self.handle_remove_all_items)
        view.browse.connect(self.handle_browse)

    def handle_remove_all_items(self):
        self._run_selection.clear_all_runs()
        self.refresh()

    def handle_remove_items(self):
        selected = self.view.selected_runs()
        selected.sort(reverse=True)
        for index in selected:
            self._run_selection.remove_run(index)
        self.refresh()

    def parse_runs_from_input(self, input):
        return self._run_finder.find_all_from_query(input.replace(':', '-'))

    def add_runs(self, runs):
        for run in runs:
            self._run_selection.add_run(run)
        self.refresh()

    def handle_add_items(self):
        input = self.view.run_list()
        error, runs = self.parse_runs_from_input(input)
        if error:
            self.view.invalid_run_query(error)
        else:
            if runs:
                self.add_runs(runs)
            else:
                self.view.run_not_found()

    def refresh(self):
        self.view.draw_runs(self._run_selection)

    def handle_manage_directories(self):
        self.view.show_directories_manager()

    def handle_browse(self):
        search_directories = ConfigService.Instance().getDataSearchDirs()
        files = self.view.show_file_picker([".nxs"], search_directories)
        # TODO: Get file extensions from a reputable source.
        self.add_runs(RunFile(file) for file in files)
        self.refresh()
