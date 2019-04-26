# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtCore, QtWidgets

from mantidqt.utils.qt import load_ui
from mantid.api import AnalysisDataService
import Muon.GUI.Common.utilities.run_string_utils as run_utils

ui_workspace_selector, _ = load_ui(__file__, "workspace_selector.ui")


class WorkspaceSelectorView(QtWidgets.QDialog, ui_workspace_selector):
    def __init__(self, current_runs, instrument, parent_widget=None):
        super(QtWidgets.QDialog, self).__init__(parent=parent_widget)
        self.setupUi(self)
        self.current_runs = current_runs
        self.instrument = instrument

        self.select_button.clicked.connect(self.accept)
        self.cancel_button.clicked.connect(self.reject)

        self.data_selection_list_widget.addItems(self.get_workspace_list())

    def get_workspace_list(self):
        filter_types = ['Asymmetry', 'Pair', 'PhaseQuad']
        filtered_list = AnalysisDataService.getObjectNames()

        current_run_strings = [run_utils.run_list_to_string(run) for run in self.current_runs]

        filtered_list = [item for item in filtered_list if any([self.instrument + run + ';' in item for run in current_run_strings])]

        filtered_list = [item for item in filtered_list if any([filter in item for filter in filter_types])]

        return filtered_list

    def get_selected_list(self):
        return [str(item.text()) for item in self.data_selection_list_widget.selectedItems()]

    @staticmethod
    def get_selected_data(current_runs, instrument, parent):
        dialog = WorkspaceSelectorView(current_runs, instrument, parent)

        result = dialog.exec_()

        selected_list = dialog.get_selected_list()

        return(selected_list, result == QtWidgets.QDialog.Accepted)
