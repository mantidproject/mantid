# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#from typing import Dict, List
#from Muon.GUI.Common.ADSHandler.workspace_naming import remove_rebin_from_name, add_rebin_to_name
#from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_model import FitPlotInformation
from Muon.GUI.Common.home_tab.home_tab_presenter import HomeTabSubWidget

#from Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_presenter_interface import \
#    PlottingCanvasPresenterInterface
#from Muon.GUI.Common.contexts.frequency_domain_analysis_context import FrequencyDomainAnalysisContext
#from Muon.GUI.Common.plot_widget.plot_widget_model import PlotWidgetModel
#from Muon.GUI.Common.plot_widget.plot_widget_view_interface import PlotWidgetViewInterface
#from Muon.GUI.Common.contexts.plotting_context import PlotMode
#from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing, GenericObservable
#from mantid.dataobjects import Workspace2D


class PlotWidgetPresenterMain(HomeTabSubWidget):

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
        self._plot_modes[self._current_plot_mode].plot_mode_changed()
