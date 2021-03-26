# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
from qtpy.QtWidgets import QVBoxLayout
from workbench.plugins.base import PluginWidget
from workbench.plotting.globalfiguremanager import GlobalFigureManager
from workbench.widgets.plotselector.presenter import PlotSelectorPresenter


class PlotSelector(PluginWidget):
    """Provides an algorithm selector widget for selecting algorithms to run"""
    def __init__(self, parent):
        super(PlotSelector, self).__init__(parent)

        plot_selector_presenter = PlotSelectorPresenter(GlobalFigureManager)

        # layout
        self.plot_selector_widget = plot_selector_presenter.view
        layout = QVBoxLayout()
        layout.addWidget(self.plot_selector_widget)
        self.setLayout(layout)

    def register_plugin(self):
        self.main.add_dockwidget(self)

    def get_plugin_title(self):
        return "Plots"

    def readSettings(self, _):
        pass

    def writeSettings(self, _):
        pass
