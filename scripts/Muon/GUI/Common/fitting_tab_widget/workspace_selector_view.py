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
from Muon.GUI.Common.list_selector.list_selector_presenter import ListSelectorPresenter
from Muon.GUI.Common.list_selector.list_selector_view import ListSelectorView
ui_workspace_selector, _ = load_ui(__file__, "workspace_selector.ui")


class WorkspaceSelectorView(QtWidgets.QDialog, ui_workspace_selector):
    def __init__(self, current_runs, instrument, current_workspaces, context, parent_widget=None):
        super(QtWidgets.QDialog, self).__init__(parent=parent_widget)
        self.setupUi(self)
        self.current_runs = current_runs
        self.current_workspaces = current_workspaces
        self.instrument = instrument

        self.select_button.clicked.connect(self.accept)
        self.cancel_button.clicked.connect(self.reject)

        self.list_selector_widget = ListSelectorView(self)
        self.list_selector_layout.addWidget(self.list_selector_widget, 0, 1, 4, 1)
        self.list_selector_layout.setContentsMargins(0, 0, 0, 0)
        self.list_selector_presenter = ListSelectorPresenter(self.list_selector_widget, self.get_workspace_list())
        self.list_selector_presenter.update_view_from_model()

        self.group_pair_line_edit.editingFinished.connect(self.handle_group_pair_selection_changed)

    def get_workspace_list(self):
        filter_types = ['Asymmetry', 'Pair', 'PhaseQuad']
        filtered_list = AnalysisDataService.getObjectNames()

        current_run_strings = [run_utils.run_list_to_string(run) for run in self.current_runs]

        filtered_list = [item for item in filtered_list if any([self.instrument + run + ';' in item for run in current_run_strings])]

        filtered_list = [item for item in filtered_list if any([filter in item for filter in filter_types])]

        filtered_list = [item for item in filtered_list if item not in self.current_workspaces]

        filtered_list = self.current_workspaces + filtered_list

        model_dict = {}
        for index, item in enumerate(filtered_list):
            model_dict.update({item: [index, item in self.current_workspaces, True]})

        return model_dict

    def get_selected_list(self):
        return self.list_selector_presenter.get_selected_items()

    @staticmethod
    def get_selected_data(current_runs, instrument, current_workspaces, context, parent):
        dialog = WorkspaceSelectorView(current_runs, instrument, current_workspaces, context, parent)

        result = dialog.exec_()

        selected_list = dialog.get_selected_list()

        return(selected_list, result == QtWidgets.QDialog.Accepted)
