# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from mantidqt.widgets.plotconfigdialog.plotconfigdialogview import PlotConfigDialogView
from mantidqt.widgets.plotconfigdialog.axestabwidget.axestabwidgetpresenter import AxesTabWidgetPresenter
from mantidqt.widgets.plotconfigdialog.curvestabwidget.curvestabwidgetview import CurvesTabWidgetView


class PlotConfigDialogPresenter:

    def __init__(self, fig, parent=None):
        self.fig = fig
        self.view = PlotConfigDialogView(parent)
        self.view.show()

        # Initialise tabs
        axes_tab = AxesTabWidgetPresenter(self.fig, parent=self.view)

        # Create list of tab presenters
        self.tab_widget_presenters = []
        self.tab_widget_presenters.append(axes_tab)

        # Create list of tab views and add them to the parent view
        self.tab_widget_views = []
        self.tab_widget_views.append((axes_tab.view, "Axes"))
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
