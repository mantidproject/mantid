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

        self._plot_modes={}
        for mode in plot_modes:
            self._plot_modes[mode.name] = mode
            self._plot_modes[mode.name].hide()

        self._view.add_panes(self._plot_modes)
        self._current_plot_mode = self._view.get_plot_mode
        self._plot_modes[self._current_plot_mode].show()
        self._view.plot_mode_change_connect(self.handle_plot_mode_changed_by_user)
        # gui observers
        self._setup_gui_observers()
        self._setup_view_connections()
        #self._view

    def _setup_gui_observers(self):
        """"Setup GUI observers, e.g fit observers"""
        return

    def _setup_view_connections(self):
        return #self._view.on_plot_mode_changed(self.handle_plot_mode_changed_by_user)

    def handle_plot_mode_changed_by_user(self):
        old_plot_mode = self._current_plot_mode
        self._current_plot_mode = self._view.get_plot_mode
        self._plot_modes[old_plot_mode].hide()
        self._plot_modes[self._current_plot_mode].show()
