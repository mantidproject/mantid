# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.plotting.mantid_navigation_toolbar import MantidNavigationToolbar, MantidStandardNavigationTools, MantidNavigationTool
from qtpy import QtWidgets, QtCore


class FittingPlotToolbar(MantidNavigationToolbar):
    sig_home_clicked = QtCore.Signal()
    sig_toggle_fit_triggered = QtCore.Signal()
    sig_serial_fit_clicked = QtCore.Signal()
    sig_seq_fit_clicked = QtCore.Signal()
    sig_toggle_legend = QtCore.Signal()
    sig_find_peaks_convolve = QtCore.Signal()

    toolitems = (
        MantidNavigationTool("Home", "Center display on contents", "mdi.home", "on_home_clicked", None),
        MantidStandardNavigationTools.BACK,
        MantidStandardNavigationTools.FORWARD,
        MantidStandardNavigationTools.SEPARATOR,
        MantidStandardNavigationTools.PAN,
        MantidStandardNavigationTools.ZOOM,
        MantidNavigationTool("Fit", "Open/close fitting tab", None, "toggle_fit", False),
        MantidNavigationTool("Serial Fit", "Fit each spec with the starting guess.", None, "serial_fit", None),
        MantidNavigationTool("Sequential Fit", "Fit each spec using the output of a previous run", None, "seq_fit", None),
        MantidNavigationTool("Hide Legend", "Toggle the legend box on/off", None, "toggle_legend", False),
        MantidNavigationTool(
            "FindPeaksConvolve", "Run FindPeaksConvolve algorithm on the selected workspace", None, "run_find_peak_convolve", None
        ),
    )

    def __init__(self, canvas, parent, coordinates=True):
        super().__init__(canvas, parent, coordinates)

        # Adjust icon size or they are too small in PyQt5 by default
        dpi_ratio = QtWidgets.QApplication.instance().desktop().physicalDpiX() / 100
        self.setIconSize(QtCore.QSize(int(24 * dpi_ratio), int(24 * dpi_ratio)))

    def on_home_clicked(self):
        self.sig_home_clicked.emit()
        self.push_current()

    def toggle_fit(self):
        fit_action = self._actions["toggle_fit"]
        if fit_action.isChecked():
            # disable pan and zoom
            if self._actions["zoom"].isChecked():
                self.zoom()
            if self._actions["pan"].isChecked():
                self.pan()
        self.sig_toggle_fit_triggered.emit()

    def serial_fit(self):
        self.sig_serial_fit_clicked.emit()

    def seq_fit(self):
        self.sig_seq_fit_clicked.emit()

    def toggle_legend(self):
        self.sig_toggle_legend.emit()

    def run_find_peak_convolve(self):
        self.sig_find_peaks_convolve.emit()

    def get_show_legend_value(self):
        return not self._actions["toggle_legend"].isChecked()

    def handle_fit_browser_close(self):
        """
        Respond to a signal that user closed self.fit_browser by
        clicking the [x] button.
        """
        self._actions["toggle_fit"].trigger()

    def trigger_fit_toggle_action(self):
        self._actions["toggle_fit"].trigger()

    def set_fit_checkstate(self, checkstate):
        self._actions["toggle_fit"].setChecked(checkstate)
