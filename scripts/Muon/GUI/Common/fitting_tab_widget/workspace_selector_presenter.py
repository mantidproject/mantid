# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AnalysisDataService
import Muon.GUI.Common.utilities.run_string_utils as run_utils
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_view import FittingTabView
from qtpy import QtWidgets

class WorkspaceSelectorPresenter(object):
    def __init__(self, current_runs, instrument):
        self.current_runs = current_runs
        self.view = FittingTabView()
        self.instrument = instrument

        display_list = self.get_workspace_list()
        self.view.update_display_list(display_list)

    def get_workspace_list(self):
        filter_types = ['Asymmetry', 'Pair', 'PhaseQuad']
        filtered_list = AnalysisDataService.getObjectNames()

        current_run_strings = [run_utils.run_list_to_string(run) for run in self.current_runs]

        filtered_list = [item for item in filtered_list if any([self.instrument + run + ';' in item for run in current_run_strings])]

        filtered_list = [item for item in filtered_list if any([filter in item for filter in filter_types])]

        return filtered_list

    def get_selected_list(self):
        return self.view.data_selection_list_widget.selectedItems()

    @staticmethod
    def get_selected_data(current_runs, instrument):
        presenter = WorkspaceSelectorPresenter(current_runs, instrument)

        result = presenter.view.exec_()

        selected_list = presenter.get_selected_list()

        return(selected_list, result == QtWidgets.QDialog.Accepted)




