from mantidqtpython import MantidQt
from mantid import ConfigService

class AddRunsPagePresenter(object):
    def __init__(self, model, view, parent):
        self.model = model
        self.view = view
        self.parent = parent
        self.connect_to_view(view)

    def connect_to_view(self, view):
        view.manageDirectoriesPressed.connect(self.handle_manage_directories_pressed)
        view.addRunsPressed.connect(self.handle_add_items_pressed)
        view.removeRunsPressed.connect(self.handle_remove_items_pressed)
        view.removeAllRunsPressed.connect(self.handle_remove_all_items_pressed)
        view.browsePressed.connect(self.handle_browse_pressed)

        view.binningTypeChanged.connect(self.handle_binning_type_changed)
        view.preserveEventsChanged.connect(self.handle_preserve_events_changed)
        view.sumPressed.connect(self.handle_sum_pressed)

    def handle_remove_all_items_pressed(self):
        self.model.clear_all_runs()
        self.refresh()

    def handle_sum_pressed(self):
        print "Sum All Items"

    def handle_remove_items_pressed(self):
        for index in self.view.selected_runs():
            self.model.remove_run(index)
        self.refresh()

    def list_range_to_iterator(self, list_range):
        if len(list_range) == 0:
            return None
        elif len(list_range) == 1:
            return range(int(list_range[0]), int(list_range[0]) + 1)
        elif len(list_range) == 2:
            return range(int(list_range[0]), int(list_range[1]))
        else:
            return None

    def parse_runs_from_input(self, input):
        run_ranges_split_on_commas = input.split(',')
        list_ranges = [run_range.replace(':','-').split('-')\
                       for run_range in run_ranges_split_on_commas]
        run_ranges_expanded = [self.list_range_to_iterator(range)\
                               for range in list_ranges]
        return (run for run_range in run_ranges_expanded for run in run_range)

    def should_continue_adding(self, error):
        if error:
            return self.view.should_continue_adding(error)
        else:
            return True

    def handle_add_items_pressed(self):
        input = self.view.run_list()
        runs = self.parse_runs_from_input(input)
        for run in runs:
            error = self.model.add_run(str(run).strip())
            if not self.should_continue_adding(error):
                self.refresh()
                return
        self.refresh()

    def refresh(self):
        self.view.draw_runs(self.model)

    def handle_binning_type_changed(self, type_of_binning):
        print "Binning Type Changed to {}".format(type_of_binning)

    def handle_preserve_events_changed(self, is_enabled):
        print "preserveEventsChanged is now {}".format(is_enabled)

    def handle_manage_directories_pressed(self):
        self.view.show_directories_manager()

    def handle_browse_pressed(self):
        search_directories = ConfigService.Instance().getDataSearchDirs()
        files = self.view.show_file_picker([".txt", ".nxs"], search_directories)
        for file in files:
            self.model.add_run(file)
        self.refresh()
