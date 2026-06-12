# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_model import FittingDataModel
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_view import FittingDataView
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.data_handling.data_presenter import FittingDataPresenter

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.fitting_ads_observer import FittingADSObserver
from typing import List, Any


class FittingDataWidget(object):
    def __init__(self, parent: Any, view: FittingDataView | None = None):
        if view is None:
            self.view = FittingDataView(parent)
        else:
            self.view = view

        self.model = FittingDataModel()
        self.presenter = FittingDataPresenter(self.model, self.view)

        self.ads_observer = FittingADSObserver(self.remove_workspace, self.clear_workspaces, self.replace_workspace, self.rename_workspace)

    def get_active_ws_list(self) -> List[str]:
        return self.presenter.get_active_ws_list()

    def get_sorted_active_ws_list(self) -> List[str]:
        return self.presenter.get_sorted_active_ws_list()

    def remove_workspace(self, workspace: str) -> None:
        self.presenter.remove_workspace(workspace)

    def rename_workspace(self, old_name: str, new_name: str) -> None:
        self.presenter.rename_workspace(old_name, new_name)

    def clear_workspaces(self) -> None:
        self.presenter.clear_workspaces()

    def replace_workspace(self, name: str, workspace: Any) -> None:
        self.presenter.replace_workspace(name, workspace)
