class PeakSelectorPresenter(object):
    def __init__(self, view):
        self.view = view
        self.primary_checkboxes = self.view.primary_checkboxes
        self.secondary_checkboxes = self.view.secondary_checkboxes

    def finish_selection(self):
        self.view.finish_selection()

    def update_peak_data(self, data):
        self.view.update_peak_data(data)

    def on_finished(self, slot):
        self.view.on_finished(slot)

    def unreg_on_finished(self, slot):
        self.view.unreg_on_finished(slot)
