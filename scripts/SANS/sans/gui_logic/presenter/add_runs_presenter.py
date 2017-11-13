# class FakeSignal:
#     def __init__():
#         self._handlers = []
#
#     def connect(handler):
#         self._handlers.append(handler)
#
#     def emit(args):
#         for handler in self._handlers:
#             handler(args)

from mantidqtpython import MantidQt

class AddRunsPagePresenter(object):
    def __init__(self, model, view, parent):
        self.model = model
        self.view = view
        self.parent = parent
        self.connect_to_view(view)

    def connect_to_view(self, view):
        view.manageDirectoriesPressed.connect(self.handle_manage_directories_pressed)
        view.browsePressed.connect(self.handle_browse_pressed)
        view.addRunsPressed.connect(self.handle_add_items_pressed)
        view.removeRunsPressed.connect(self.handle_remove_items_pressed)
        view.removeAllRunsPressed.connect(self.handle_remove_all_items_pressed)
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

    def handle_add_items_pressed(self):
        self.model.add_run(self.view.run_list())
        self.refresh()

    def refresh(self):
        self.view.draw_runs(self.model)

    def handle_binning_type_changed(self, type_of_binning):
        print "Binning Type Changed to {}".format(type_of_binning)

    def handle_preserve_events_changed(self, is_enabled):
        print "preserveEventsChanged is now {}".format(is_enabled)

    def handle_manage_directories_pressed(self):
        MantidQt.API.ManageUserDirectories.openUserDirsDialog(self.view)


    def handle_browse_pressed(self):
        print "Browse ..."
