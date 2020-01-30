# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model import FittingDataModel
from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_view import FittingDataView
from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_presenter import FittingDataPresenter


class FittingDataWidget(object):
    def __init__(self, parent, view=None):
        if view is None:
            self.view = FittingDataView(parent)
        else:
            self.view = view

        self.model = FittingDataModel()
        self.presenter = FittingDataPresenter(self.model, self.view)

    def get_loaded_workspaces(self):
        return self.presenter.get_loaded_workspaces()

    def remove_workspace(self, workspace):
        self.presenter.remove_workspace(workspace)
