# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget

from MultiPlotting.multi_plotting_widget import MultiPlotWindow

class HomePlotWidgetPresenter(HomeTabSubWidget):

    def __init__(self, view, model):
        self._view = view
        self._model = model
        self._plot_window = None
        self._view.on_plot_button_clicked(self.handle_plot_button_clicked)
        self.keep = False

    def show(self):
        self._view.show()

    def update_view_from_model(self):
        pass

    def handle_plot_button_clicked(self):
        is_raw = self._view.if_raw()
        ws_list = self._model.context.get_names_of_workspaces_to_fit(runs='All',
                                                               group_and_pair=self._model.context.gui_context[
                                                               'selected_group_pair'], phasequad=False, rebin=not is_raw)
        new_plot = False
        if self._plot_window is None:
           new_plot = True

        if new_plot or not self._view.if_keep():
            self._plot_window = MultiPlotWindow("Muon plots")
            self._plot_window.windowClosedSignal.connect(self._close_plot)

        self.plotting = self._plot_window.multi_plot
        if self._view.if_overlay():
            # need to check if subplot exists first
            # probably want a subplot for each x label
            if not self.keep:
                if not self.plotting.has_subplot("Time domain"):
                    self.plotting.add_subplot("Time domain")
                self.keep = True
            for ws_name in ws_list:
                self.plotting.plot("Time domain", ws_name)
        else:
            for ws_name in ws_list:
                self.plotting.add_subplot(ws_name)
                self.plotting.plot(ws_name, ws_name)
        # only do below line if its a new multi plot
        if new_plot:
            self.plotting.set_all_values_to([0.0,15.0],[-30.,30])
        self._plot_window.show()

    def _close_plot(self):
        self._plot_window = None
        self.plotting = None
        self.keep = False

    # example code for forcing plots to close when GUI is closed
    #def closeEvent(self, event):
    #    if self._plot_window is not None:
    #        self._plot_window.closeEvent(event)
    #    super(HomePlotWidgetPresenter, self).closeEvent(event)
