# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget


class MainPlotWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view,
                 plot_modes):
        """
        :param view: A reference to the QWidget object for plotting
        """
        self._view = view

        self._plot_modes = {}
        for mode in plot_modes:
            self._plot_modes[mode.name] = mode
            self._plot_modes[mode.name].hide()

        self._view.add_panes(self._plot_modes)
        self._current_plot_mode = self._view.get_plot_mode
        self._plot_modes[self._current_plot_mode].show()

    def set_plot_mode_changed_slot(self, slot):
        self._view.plot_mode_change_connect(slot)

    @property
    def get_plot_mode(self):
        return self._view.get_plot_mode

    @property
    def data_changed_observers(self):
        return [self._plot_modes[mode].data_changed_observer for mode in list(self._plot_modes.keys())]

    @property
    def rebin_options_set_observers(self):
        return [self._plot_modes[mode].rebin_options_set_observer for mode in list(self._plot_modes.keys())]

    @property
    def workspace_replaced_in_ads_observers(self):
        return [self._plot_modes[mode].workspace_replaced_in_ads_observer for mode in list(self._plot_modes.keys())]

    @property
    def workspace_deleted_from_ads_observers(self):
        return [self._plot_modes[mode].workspace_deleted_from_ads_observer for mode in list(self._plot_modes.keys())]

    def show(self, plot_mode):
        self._plot_modes[plot_mode].show()

    def hide(self, plot_mode):
        self._plot_modes[plot_mode].hide()
