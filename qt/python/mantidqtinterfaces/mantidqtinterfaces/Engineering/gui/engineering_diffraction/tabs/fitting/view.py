# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations  # this prevents the imports from TYPE_CHECKING from being evaluated at runtime
from qtpy import QtWidgets
from mantidqt.utils.qt import load_ui
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_view import FittingDataView
    from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_view import FittingPlotView

Ui_fitting, _ = load_ui(__file__, "fitting_tab.ui")


class FittingView(QtWidgets.QWidget, Ui_fitting):
    def __init__(self, parent: QtWidgets.QWidget | None = None):
        super(FittingView, self).__init__(parent)
        self.setupUi(self)

    def get_data_widget(self) -> FittingDataView:
        return self.widget_data

    def get_plot_widget(self) -> FittingPlotView:
        return self.widget_plot
