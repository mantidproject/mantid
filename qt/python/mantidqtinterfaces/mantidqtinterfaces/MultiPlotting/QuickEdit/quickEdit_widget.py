# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.MultiPlotting.QuickEdit.quickEdit_view import QuickEditView
from mantidqtinterfaces.MultiPlotting.QuickEdit.quickEdit_presenter import QuickEditPresenter


class QuickEditWidget(object):

    def __init__(self, parent=None, auto_btn = False):
        view = QuickEditView(None, parent, auto_btn)
        self._presenter = QuickEditPresenter(view)

    @property
    def widget(self):
        return self._presenter.widget

    @property
    def autoscale(self):
        return self._presenter.autoscale

    def disable_yaxis_changer(self):
        self._presenter.disable_yaxis_changer()

    def enable_yaxis_changer(self):
        self._presenter.enable_yaxis_changer()

    def disable_autoscale(self):
        self._presenter.disable_autoscale()

    def enable_autoscale(self):
        self._presenter.enable_autoscale()

    def uncheck_autoscale(self):
        self._presenter.uncheck_autoscale()

    """ connect statements"""

    def connect_autoscale_changed(self, slot):
        self._presenter.connect_autoscale_changed(slot)

    def connect_errors_changed(self, slot):
        self._presenter.connect_errors_changed(slot)

    def connect_x_range_changed(self, slot):
        self._presenter.connect_x_range_changed(slot)

    def connect_y_range_changed(self, slot):
        self._presenter.connect_y_range_changed(slot)

    def connect_plot_selection(self, slot):
        self._presenter.connect_plot_selection(slot)

    def add_subplot(self, name):
        self._presenter.add_subplot(name)

    def rm_subplot(self, name):
        self._presenter.rm_subplot(name)

    def clear_subplots(self):
        self._presenter.clear_subplots()

    def get_subplots(self):
        return self._presenter.all()

    def get_selection(self):
        name = self._presenter.widget.current_selection()
        if name == "All":
            return self.get_subplots()
        return [name]

    def set_plot_x_range(self, range):
        self._presenter.set_plot_x_range(range)

    def set_plot_y_range(self, y_range):
        self._presenter.set_plot_y_range(y_range)

    def get_plot_x_range(self):
        return self._presenter.get_plot_x_range()

    def get_plot_y_range(self):
        return self._presenter.get_plot_y_range()

    def set_errors(self, state):
        self._presenter.set_errors(state)

    def get_errors(self):
        return self._presenter.get_errors()

    def set_mock(self, mock_presenter):
        self._presenter = mock_presenter
