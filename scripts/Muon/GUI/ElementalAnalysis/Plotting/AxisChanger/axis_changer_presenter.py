class AxisChangerPresenter(object):
    def __init__(self, view):
        self.view = view

    def get_bounds(self):
        return self.view.get_bounds()

    def set_bounds(self, bounds):
        self.view.set_bounds(bounds)

    def on_bounds_changed(self, slot):
        self.view.on_bounds_changed(slot)

    def unreg_on_bounds_changed(self, slot):
        try:
            self.view.unreg_on_bounds_changed(slot)
        except TypeError:
            return
