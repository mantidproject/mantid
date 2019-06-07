# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog import curves_in_figure
from mantidqt.widgets.plotconfigdialog.view import PlotConfigDialogView
from mantidqt.widgets.plotconfigdialog.axestabwidget.presenter import AxesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.presenter import CurvesTabWidgetPresenter


class PlotConfigDialogPresenter:

    def __init__(self, fig, parent=None):
        self.fig = fig
        self.view = PlotConfigDialogView(parent)
        self.view.show()

        self.tab_widget_presenters = []
        self.tab_widget_views = []

        # Axes tab
        axes_tab = AxesTabWidgetPresenter(self.fig, parent=self.view)
        self.tab_widget_presenters.append(axes_tab)
        self.tab_widget_views.append((axes_tab.view, "Axes"))
        # Curves tab (only add if curves present in figure)
        if curves_in_figure(self.fig):
            curves_tab = CurvesTabWidgetPresenter(self.fig, parent=self.view)
            self.tab_widget_presenters.append(curves_tab)
            self.tab_widget_views.append((curves_tab.view, "Curves"))

        self._add_tab_widget_views()

        # Signals
        self.view.ok_button.clicked.connect(self.apply_properties_and_exit)
        self.view.apply_button.clicked.connect(self.apply_properties)
        self.view.cancel_button.clicked.connect(self.exit)

    def _add_tab_widget_views(self):
        self.view.add_tab_widgets(self.tab_widget_views)

    def apply_properties(self):
        for tab in self.tab_widget_presenters:
            tab.apply_properties()
        self.fig.canvas.draw()

    def apply_properties_and_exit(self):
        self.apply_properties()
        self.view.close()

    def exit(self):
        self.view.close()
