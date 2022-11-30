# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar, MantidStandardNavigationTools, MantidNavigationTool
from qtpy import QtWidgets, QtCore


class GSAS2PlotToolbar(MantidNavigationToolbar):
    sig_home_clicked = QtCore.Signal()
    sig_toggle_fit_triggered = QtCore.Signal()

    toolitems = (
        MantidNavigationTool('Home', 'Center display on contents', 'mdi.home', 'on_home_clicked', None),
        MantidStandardNavigationTools.BACK,
        MantidStandardNavigationTools.FORWARD,
        MantidStandardNavigationTools.SEPARATOR,
        MantidStandardNavigationTools.PAN,
        MantidStandardNavigationTools.ZOOM,
    )

    def __init__(self, canvas, parent, coordinates=True):
        super().__init__(canvas, parent, coordinates)

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(int(24 * dpi_ratio), int(24 * dpi_ratio)))

    def on_home_clicked(self):
        self.sig_home_clicked.emit()
        self.push_current()
