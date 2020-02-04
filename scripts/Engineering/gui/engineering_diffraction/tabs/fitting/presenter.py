# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_widget import FittingDataWidget


class FittingPresenter(object):
    def __init__(self, view):
        self.view = view

        self.data_widget = FittingDataWidget(self.view, view=self.view.get_data_widget())
