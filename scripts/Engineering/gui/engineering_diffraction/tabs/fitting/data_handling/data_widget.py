# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_model import FittingDataModel
from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_view import FittingDataView
from Engineering.gui.engineering_diffraction.tabs.fitting.data_handling.data_presenter import FittingDataPresenter

from Engineering.gui.engineering_diffraction.tabs.fitting.fitting_ads_observer import FittingADSObserver


class FittingDataWidget(object):
    def __init__(self, parent, view=None):
        if view is None:
            self.view = FittingDataView(parent)
        else:
            self.view = view

        self.model = FittingDataModel()
        self.presenter = FittingDataPresenter(self.model, self.view)

        self.ads_observer = FittingADSObserver(self.remove_workspace, self.clear_workspaces,
                                               self.replace_workspace, self.rename_workspace)

    def get_loaded_workspaces(self):
        return self.presenter.get_loaded_workspaces()

    def remove_workspace(self, workspace):
        self.presenter.remove_workspace(workspace)

    def rename_workspace(self, old_name, new_name):
        self.presenter.rename_workspace(old_name, new_name)

    def clear_workspaces(self):
        self.presenter.clear_workspaces()

    def replace_workspace(self, name, workspace):
        self.presenter.replace_workspace(name, workspace)
