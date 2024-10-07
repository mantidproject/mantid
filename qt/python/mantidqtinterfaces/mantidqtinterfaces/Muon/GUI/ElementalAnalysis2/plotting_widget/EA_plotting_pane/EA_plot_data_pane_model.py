# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel

SPECTRA_INDICES = {"Delayed": 0, "Prompt": 1, "Total": 2}
INVERSE_SPECTRA_INDICES = {0: "Delayed", 1: "Prompt", 2: "Total"}


class EAPlotDataPaneModel(BasePaneModel):
    def __init__(self, context):
        super(EAPlotDataPaneModel, self).__init__(context, name="Plot Data")
        self.context.plot_panes_context[self.name].set_defaults([-10.0, 1000.0], [-100, 2000])

    @staticmethod
    def _generate_run_indices(workspace_list, plot_type):
        indices = [SPECTRA_INDICES[plot_type]] * len(workspace_list)
        return indices

    def get_count_workspaces_to_plot(self, current_group, is_raw):
        """
        :param current_group:
        :param is_raw: Whether to use raw or rebinned data
        :return: a list of workspace names
        """
        try:
            if is_raw:
                workspace_list = [self.context.group_context[current_group].get_counts_workspace_for_run(False)]
            else:
                workspace_list = [self.context.group_context[current_group].get_rebined_or_unbinned_version_of_workspace_if_it_exists()]

            return workspace_list
        except AttributeError:
            return []

    def get_workspaces_to_remove(self, group_names):
        """
        :param group_names:
        :return: a list of workspace names
        """
        is_raw = self.get_workspace_list(group_names, True)
        is_rebinned = self.get_workspace_list(group_names, False)
        return list(set(is_raw + is_rebinned))

    def get_workspace_list(self, group_names, is_raw):
        """
        :param group_names:
        :param is_raw: Whether to use raw or rebinned data
        :return: a list of workspace names
        """
        workspace_list = []
        for group in group_names:
            workspace_list += self.get_count_workspaces_to_plot(group, is_raw)
        return workspace_list

    def get_workspace_and_indices_for_group(self, group, is_raw, plot_type):
        """
        :return: a list of workspace names and corresponding indices to plot
        """
        workspace_list = self.get_count_workspaces_to_plot(group, is_raw)
        indices = self._generate_run_indices(workspace_list, plot_type)

        return workspace_list, indices

    def get_workspaces_to_plot(self, is_raw):
        """
        :param is_raw: Whether to use raw or rebinned data
        :return: a list of workspace names
        """
        currently_selected = self.context.group_context.selected_groups
        workspace_list = self.get_workspace_list(currently_selected, is_raw)

        return workspace_list

    def get_workspace_list_and_indices_to_plot(self, is_raw, plot_type):
        """
        :return: a list of workspace names to plot
        """
        workspace_list = self.get_workspaces_to_plot(is_raw)
        indices = self._generate_run_indices(workspace_list, plot_type)

        return workspace_list, indices

    def create_tiled_keys(self, tiled_by):
        if tiled_by == "Detector":
            detectors_present = []
            for group_name in self.context.group_context.selected_groups:
                group = self.context.group_context[group_name]
                detectors_present.append(group.detector)
            # Turns into a set to remove duplicates and sorts by detector number
            keys = sorted(list(set(detectors_present)), key=lambda element: int(element[-1]))
        else:
            runs_present = []
            for group_name in self.context.group_context.selected_groups:
                group = self.context.group_context[group_name]
                runs_present.append(group.run_number)
            keys = sorted(list(set(runs_present)), key=lambda element: int(element))
        return keys

    def _create_workspace_label(self, workspace_name, index):
        return workspace_name + "_" + INVERSE_SPECTRA_INDICES[index]

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index=None):
        if not self.context.plot_panes_context[self.name].settings.is_tiled:
            return 0

        run_as_string, detector = self.context.group_context.get_detector_and_run_from_workspace_name(workspace_name)
        if detector in axes_workspace_map:
            return axes_workspace_map[detector]

        if run_as_string in axes_workspace_map:
            return axes_workspace_map[run_as_string]

        return 0
