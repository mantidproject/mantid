# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from qtpy import QtWidgets

from mantidqt.utils.qt import load_ui

Ui_fitting, _ = load_ui(__file__, "fitting_tab.ui")


class FittingView(QtWidgets.QWidget, Ui_fitting):
    def __init__(self, parent=None):
        super(FittingView, self).__init__(parent)
        self.setupUi(self)

    def get_data_widget(self):
        return self.widget_data
