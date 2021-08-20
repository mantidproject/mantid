# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_view import QuickEditView
from Muon.GUI.Common.plot_widget.quick_edit.quick_edit_presenter import QuickEditPresenter, DuelQuickEditPresenter


class QuickEditWidgetInterface(object):

    def __init__(self, context, view, presenter, parent=None):
        self._view = view
        self._presenter = presenter
        self._context = context

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

    def disable_plot_selection(self):
        self._view.disable_plot_selection()

    def add_subplot(self, name):
        self._presenter.add_subplot(name)

    @property
    def get_all_subplots(self):
        return self._presenter.get_all_subplots

    def clear_subplots(self):
        self._presenter.clear_subplots()

    def get_selection(self):
        return self._presenter.get_selection()

    def get_selection_index(self):
        return self._presenter.get_selection_index()

    def set_selection_by_index(self, index=0):
        self._presenter.set_selection_by_index(index)

    def set_autoscale(self, state:bool):
        self._presenter.set_autoscale(state)

    def set_plot_x_range(self, x_range):
        self._presenter.set_plot_x_range(x_range)

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


class QuickEditWidget(QuickEditWidgetInterface):

    def __init__(self, context, parent=None):
        view = QuickEditView(None, parent)
        super().__init__(context, view, QuickEditPresenter(view, context))


class DuelQuickEditWidget(QuickEditWidgetInterface):

    def __init__(self, context, parent=None):
        view = QuickEditView(None, parent, "Time domains")
        super().__init__(context, view, DuelQuickEditPresenter(view, context))

    def add_subplot(self, name):
        self._presenter.add_subplot(name)
