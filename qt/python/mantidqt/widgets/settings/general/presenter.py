from mantid.kernel import ConfigService
from mantidqt.widgets.settings.general.view import GeneralSettingsView


class GeneralSettings(object):
    def __init__(self, view=None):
        self.view = view if view else GeneralSettingsView(None, self)

        facilities = ConfigService.Instance().getFacilityNames()
        self.view.facility.addItems(facilities)
        self.action_facility_changed(facilities[0])
        self.view.facility.currentTextChanged.connect(self.action_facility_changed)

    def action_facility_changed(self, new_facility):
        self.view.instrument.clear()
        self.view.instrument.addItems(
            [str(instr) for instr in ConfigService.Instance().getFacility(new_facility).instruments()])
