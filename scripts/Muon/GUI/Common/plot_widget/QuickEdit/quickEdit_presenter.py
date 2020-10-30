# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class QuickEditPresenter(object):

    def __init__(self, view, plotting_context):
        self._view = view
        self._context = plotting_context
        self.set_plot_x_range(self._context.default_xlims)
        self.set_plot_y_range(self._context.default_ylims)

    @property
    def widget(self):
        return self._view

    def add_subplot(self, name):
        current = self._view.current_selection()
        self._view.add_subplot(name)
        index = self._view.find_subplot(current)
        self._view.set_selection(index)

    def clear_subplots(self):
        self._view.clear_subplots()





    @property
    def autoscale(self):
        return self._view.autoscale_state

    def disable_yaxis_changer(self):
        self._view.disable_yaxis_changer()

    def enable_yaxis_changer(self):
        self._view.enable_yaxis_changer()

    def disable_autoscale(self):
        self._view.disable_autoscale()

    def enable_autoscale(self):
        self._view.enable_autoscale()

    def uncheck_autoscale(self):
        self._view.uncheck_autoscale()

    def connect_autoscale_changed(self, slot):
        self._view.connect_autoscale_changed(slot)

    def connect_errors_changed(self, slot):
        self._view.connect_errors_changed(slot)

    def connect_x_range_changed(self, slot):
        self._view.connect_x_range_changed(slot)

    def connect_y_range_changed(self, slot):
        self._view.connect_y_range_changed(slot)

    def connect_plot_selection(self, slot):
        self._view.connect_plot_selection(slot)

    def all(self):
        return [self._view.plot_at_index(index) for index in range(1, self._view.number_of_plots())]

    def set_to_all(self):
        self._view.set_index(0)

    def set_plot_x_range(self, range):
        self._view.set_plot_x_range(range)

    def set_plot_y_range(self,y_range):
        self._view.set_plot_y_range(y_range)

    def get_plot_x_range(self):
        return self._view.get_x_bounds()

    def get_plot_y_range(self):
        return self._view.get_y_bounds()

    def set_errors(self, state):
        previous = self._view.get_errors()
        if previous == state:
            return
        self._view.set_errors(state)

    def get_errors(self):
        return self._view.get_errors()

    def get_selection(self):
        name = self.widget.current_selection()
        if name == "All":
            return self.all()
        return [name]

    def rm_subplot(self, name):
        current = self._view.current_selection()
        if current == name:
            current = "All"
        to_remove = self._view.find_index(name)
        self._view.rm_subplot(to_remove)
        index = self._view.find_index(current)
        self._view.set_index(index)
