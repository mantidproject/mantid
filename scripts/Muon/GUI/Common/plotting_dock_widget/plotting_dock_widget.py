# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore


class PlottingDockWidget(QtWidgets.QDockWidget):

    def __init__(self, plotting_widget, parent=None):
        super(PlottingDockWidget, self).__init__("Plotting Window", parent=parent)

        self.setFeatures(
            QtWidgets.QDockWidget.DockWidgetFloatable | QtWidgets.QDockWidget.DockWidgetMovable)
        self.setAllowedAreas(QtCore.Qt.RightDockWidgetArea | QtCore.Qt.LeftDockWidgetArea)
        # the widget to be stored within
        self.dockable_plot_widget = plotting_widget
        # set the widget for this dockable plot window
        self.setWidget(self.dockable_plot_widget)
