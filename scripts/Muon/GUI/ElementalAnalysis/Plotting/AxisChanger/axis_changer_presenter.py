class AxisChangerPresenter(object):
    def __init__(self, view):
        self.view = view

    def on_lbound_return_pressed(self, slot):
        self.view.on_lbound_return_pressed(slot)

    def unreg_on_lbound_return_pressed(self, slot):
        self.view.unreg_on_lbound_return_pressed(slot)

    def on_ubound_return_pressed(self, slot):
        self.view.on_ubound_return_pressed(slot)

    def unreg_on_ubound_return_pressed(self, slot):
        self.view.unreg_on_ubound_return_pressed(slot)
