class PeakSelectorPresenter(object):
    def __init__(self, view):
        self.view = view

    def update_peak_data(self, data):
        self.view.update_peak_data(data)

    def on_finished(self, slot):
        self.view.on_finished(slot)

    def unreg_on_finished(self, slot):
        self.view.unreg_on_finished(slot)
