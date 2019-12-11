# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtWidgets, QtCore


class PlottingDockWidget(QtWidgets.QDockWidget):

    def __init__(self, plotting_widget, parent=None):
        super(PlottingDockWidget, self).__init__("Plotting Window", parent=parent)

        self.setFeatures(
            QtWidgets.QDockWidget.DockWidgetFloatable | QtWidgets.QDockWidget.DockWidgetMovable)
        self.setAllowedAreas(QtCore.Qt.RightDockWidgetArea | QtCore.Qt.LeftDockWidgetArea)
        # cthe widget to be stored within
        self.dockable_plot_widget = plotting_widget
        # set the widget for this dockable plot window
        self.setWidget(self.dockable_plot_widget)

        self.dockLocationChanged.connect(self.window_dock_status_changed)

    # this function makes the plot rescale after being docked or un-docked,
    # to avoid the x labels, y labels and title disappearing upon this action
    def window_dock_status_changed(self, area):
        self.dockable_plot_widget.get_fig().tight_layout()
