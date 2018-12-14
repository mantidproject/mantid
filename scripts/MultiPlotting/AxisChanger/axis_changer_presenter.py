# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


class AxisChangerPresenter(object):

    def __init__(self, view):
        self.view = view

    def set_enabled(self, state):
        self.view.set_enabled(state)

    def hide(self):
        self.view.hide()

    def show(self):
        self.view.show()

    def get_bounds(self):
        return self.view.get_bounds()

    def set_bounds(self, bounds):
        self.view.set_bounds(bounds)

    def clear_bounds(self):
        self.view.clear_bounds()

    def on_bound_changed(self, slot):
        self.view.on_bound_changed(slot)

    def unreg_on_bound_changed(self, slot):
        try:
            self.view.unreg_bound_changed(slot)
        except TypeError:
            return
