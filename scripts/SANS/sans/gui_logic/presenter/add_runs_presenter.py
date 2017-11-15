from mantidqtpython import MantidQt
from mantid import ConfigService
from run_selector_presenter import RunSelectorPresenter
from summation_settings_presenter import SummationSettingsPresenter

class AddRunsPagePresenter(object):
    def __init__(self, model, view, parent_view):
        self.model = model
        self.view = view
        self.parent = parent_view
        self.run_selector_presenter = RunSelectorPresenter(model, self.view.run_selector, self)
        self.summation_settings_presenter = SummationSettingsPresenter(model, self.view.summation_settings, self)
        self.connect_to_view(view)

    def connect_to_view(self, view):
        # view.sum.connect(self.handle_sum)
        pass

    def handle_sum(self):
        print "Sum All Items"
