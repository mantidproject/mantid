from mantidqtpython import MantidQt
from mantid import ConfigService

class AddRunsPagePresenter(object):
    def __init__(self, model, view, parent):
        self.model = model
        self.view = view
        self.parent = parent
        self.connect_to_view(view)

    def connect_to_view(self, view):
        view.manageDirectories.connect(self.handle_manage_directories)
        view.addRuns.connect(self.handle_add_items)
        view.removeRuns.connect(self.handle_remove_items)
        view.removeAllRuns.connect(self.handle_remove_all_items)
        view.browse.connect(self.handle_browse)
        view.sum.connect(self.handle_sum)

        view.binningTypeChanged.connect(self.handle_binning_type_changed)
        view.preserveEventsChanged.connect(self.handle_preserve_events_changed)

    def handle_remove_all_items(self):
        self.model.clear_all_runs()
        self.refresh()

    def handle_sum(self):
        print "Sum All Items"

    def handle_remove_items(self):
        selected = self.view.selected_runs()
        selected.sort(reverse=True)
        for index in selected:
            self.model.remove_run(index)
        self.refresh()

    def parse_runs_from_input(self, input):
        return self.model.find_all_from_query(input.replace(':', '-'))

    def add_runs(self, runs):
        for run in runs:
            self.model.add_run(run)
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
        self.view.draw_runs(self.model)

    def handle_binning_type_changed(self, type_of_binning):
        print "Binning Type Changed to {}".format(type_of_binning)

    def handle_preserve_events_changed(self, is_enabled):
        print "preserveEventsChanged is now {}".format(is_enabled)

    def handle_manage_directories(self):
        self.view.show_directories_manager()

    def handle_browse(self):
        search_directories = ConfigService.Instance().getDataSearchDirs()
        files = self.view.show_file_picker([".nxs"], search_directories)
        # TODO: Get file extensions from a reputable source.
        self.add_runs(self.model.create_run_from_path(file) for file in files)
        self.refresh()
