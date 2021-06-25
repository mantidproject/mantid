# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from Muon.GUI.Common.plot_widget.data_pane.plot_group_pair_model import PlotGroupPairModel
from Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_model import PlotFitPaneModel

from Muon.GUI.Common.ADSHandler.workspace_naming import (get_fit_function_name_from_workspace,
                                                         get_group_or_pair_from_name,
                                                         get_run_numbers_as_string_from_workspace_name)


class PlotTimeFitPaneModel(PlotGroupPairModel, PlotFitPaneModel):

    def __init__(self, context):
        super().__init__(context, "Fit Data")
        end_x = self.context.default_end_x
        self.context.plot_panes_context[self.name].set_defaults([0.,end_x], [-0.3, 0.3])

    @staticmethod
    def get_fit_workspace_and_indices(fit, with_diff=True):
        if fit is None:
            return [], []
        workspaces = []
        indices = []
        for workspace_name in fit.output_workspace_names():
            first_fit_index = 1  # calc
            if fit.tf_asymmetry_fit:
                first_fit_index = 3
            second_fit_index = 2  # Diff
            workspaces.append(workspace_name)
            indices.append(first_fit_index)
            if with_diff:
                workspaces.append(workspace_name)
                indices.append(second_fit_index)

        return workspaces, indices

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_numbers_as_string_from_workspace_name(workspace_name, self.context.data_context.instrument))
        instrument = self.context.data_context.instrument
        fit_label = self._get_fit_label(workspace_name, index)
        rebin_label = self._get_rebin_label(workspace_name)
        if not self.context.plot_panes_context[self.name]._is_tiled:
            return "".join([instrument, run, ';', group, fit_label, rebin_label])
        if self.context.plot_panes_context[self.name].is_tiled_by == "Group/Pair":
            return "".join([run, fit_label, rebin_label])
        else:
            return "".join([group, fit_label, rebin_label])

    @staticmethod
    def _get_fit_label(workspace_name, index):
        label = ''
        fit_function_name = get_fit_function_name_from_workspace(workspace_name)
        if fit_function_name:
            if index in [1, 3]:
                workspace_type = 'Calc'
            elif index == 2:
                workspace_type = 'Diff'
            label = f";{fit_function_name};{workspace_type}"
        return label
