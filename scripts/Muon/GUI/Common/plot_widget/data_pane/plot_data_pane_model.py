# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.data_pane.plot_group_pair_model import PlotGroupPairModel


class PlotDataPaneModel(PlotGroupPairModel):

    def __init__(self, context):
        super().__init__(context,"Plot Data")
        self.context.plot_panes_context[self.name].set_defaults([0.,15.], [-0.3, 0.3])

    def get_time_workspaces_to_plot(self, current_group_pair, is_raw, plot_type):
        """
        :param current_group_pair: The group/pair currently selected
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: Whether to plot counts or asymmetry
        :return: a list of workspace names
        """
        try:
            if is_raw:
                workspace_list = self.context.group_pair_context[current_group_pair].get_asymmetry_workspace_names(
                    self.context.data_context.current_runs)
            else:
                workspace_list = self.context.group_pair_context[
                    current_group_pair].get_asymmetry_workspace_names_rebinned(
                    self.context.data_context.current_runs)

            if plot_type == "Counts":
                workspace_list = [item.replace("Asymmetry", "Counts")
                                  for item in workspace_list if "Asymmetry" in item]

            return workspace_list
        except AttributeError:
            return []
