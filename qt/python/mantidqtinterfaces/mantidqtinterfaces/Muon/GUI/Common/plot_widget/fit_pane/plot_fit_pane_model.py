# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import get_fit_function_name_from_workspace
from copy import copy


class PlotFitPaneModel(BasePaneModel):
    def __init__(self, context, name):
        super().__init__(context, name)
        end_x = self.context.default_end_x
        self.context.plot_panes_context[self.name].set_defaults([0.0, end_x], [-1.0, 1.0])

    @staticmethod
    def get_fit_workspace_and_indices(fit, with_diff=True):
        if fit is None:
            return [], []
        workspaces = []
        indices = []
        for workspace_name in fit.output_workspace_names():
            first_fit_index = 1  # calc
            second_fit_index = 2  # Diff
            workspaces.append(workspace_name)
            indices.append(first_fit_index)
            if with_diff:
                workspaces.append(workspace_name)
                indices.append(second_fit_index)

        return workspaces, indices

    def _create_workspace_label(self, workspace_name, index):
        return ""

    @staticmethod
    def _get_fit_label(workspace_name, index):
        label = ""
        fit_function_name = get_fit_function_name_from_workspace(workspace_name)
        if fit_function_name:
            if index == 1:
                workspace_type = "Calc"
            elif index == 2:
                workspace_type = "Diff"
            label = f";{fit_function_name};{workspace_type}"
        return label

    def _is_guess_workspace(self, workspace_name):
        return self.context.guess_workspace_prefix in workspace_name

    @staticmethod
    def get_shade_lines(ws, index):
        # need to copy the x data because if switch from points to hist data it causes errors
        x_data = copy(ws.readX(index))
        y_data = ws.readY(index)
        e_data = ws.readE(index)
        return x_data, y_data + e_data, y_data - e_data
