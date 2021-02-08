# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtWidgets import QStackedWidget, QWidget
from mantidqt.utils.qt import load_ui

ui_fitting_tab, _ = load_ui(__file__, "fitting_tab2.ui")


class FittingTabView(QWidget, ui_fitting_tab):
    def __init__(self, parent=None, general_fitting_view=None):
        super(FittingTabView, self).__init__(parent)
        self.setupUi(self)

        self.fit_type_stacked_widget = QStackedWidget()
        self.fit_type_stacked_widget.addWidget(general_fitting_view)

        self.layout().addWidget(self.fit_type_stacked_widget)
