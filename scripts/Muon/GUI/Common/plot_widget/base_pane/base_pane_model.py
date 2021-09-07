# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class BasePaneModel(object):

    def __init__(self, context, name="Plot"):
        self.context = context
        self.name = name
        self.context.plot_panes_context.add_pane(self.name)

    @staticmethod
    def _generate_run_indices(workspace_list):
        indices = [0]*len(workspace_list)
        return indices

    def get_workspaces_to_plot(self, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        return []

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
         :return: a list of workspace names to plot
         """
        workspace_list = self.get_workspaces_to_plot(is_raw, plot_type)
        indices = self._generate_run_indices(workspace_list)

        return workspace_list, indices

    def get_workspaces_to_remove(self, group_pair_names, is_raw, plot_type):
        """
        :param is_raw: Whether to use raw or rebinned data
        :param plot_type: plotting type, e.g Counts, Frequency Re
        :return: a list of workspace names
        """
        workspace_list = []
        return workspace_list

    def create_tiled_keys(self, tiled_by):
        return []

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, indicies = None):
        return 0

    def _create_workspace_label(self, workspace_name, index):
        return workspace_name+"_"+str(index)

    def _is_guess_workspace(self, workspace_name):
        return False
