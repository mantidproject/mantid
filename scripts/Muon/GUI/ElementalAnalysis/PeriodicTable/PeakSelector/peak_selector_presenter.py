class PeakSelectorPresenter(object):
    def __init__(self, view):
        self.view = view

        self.view.on_okay_pressed(self._close)

    def _close(self, element, data):
        self.view.close()

    def update_peak_data(self, data):
        self.view.update_peak_data(data)

    def on_okay_pressed(self, slot):
        self.view.on_okay_pressed(slot)

    def unreg_on_okay_pressed(self, slot):
        self.view.unreg_on_okay_pressed(slot)
