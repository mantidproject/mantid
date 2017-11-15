from mantidqtpython import MantidQt
from mantid import ConfigService

class SummationSettingsPresenter(object):
    def __init__(self, model, view, parent_view):
        self.model = model
        self.view = view
        self.parent_view = parent_view
        self.connect_to_view(view)

    def connect_to_view(self, view):
        view.binningTypeChanged.connect(self.handle_binning_type_changed)
        view.preserveEventsChanged.connect(self.handle_preserve_events_changed)

    def handle_binning_type_changed(self, type_of_binning):
        print "Binning Type Changed to {}".format(type_of_binning)

    def handle_preserve_events_changed(self, is_enabled):
        print "preserveEventsChanged is now {}".format(is_enabled)
