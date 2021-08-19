# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.plot_widget.base_pane.base_pane_model import BasePaneModel
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws
from Muon.GUI.Common.ADSHandler.workspace_naming import (MAXENT_STR,
                                                         get_run_numbers_as_string_from_workspace_name,
                                                         RECONSTRUCTED_SPECTRA)
from math import ceil


class DuelPlotMaxentPaneModel(BasePaneModel):

    def __init__(self, context, time_group_model, raw_model):
        super().__init__(context, "Maxent Dual plot")
        self.reconstructed_data = {}
        self.reconstructed_data_name = ""
        self._selection = ""
        self._time_spec_limit = 8
        self._run = None
        self._is_groups = True
        self._period = 1

        self._time_group_model = time_group_model

        self._raw_model = raw_model
        self._raw_model._max_spec = self._time_spec_limit
        self._raw_model._spec_limit = self._time_spec_limit

        # defaults are for time domain only
        self.context.plot_panes_context[self.name].set_defaults([0.,10.], [0.0, 1.e6])

    @property
    def selection(self):
        return self._selection

    def set_selection(self, selection):
        self._selection = selection

    def set_period(self, period):
        self._period = period

    def set_if_groups(self, is_groups):
        self._is_groups = is_groups

    def set_run_from_name(self, name):
        self._run = str(get_run_numbers_as_string_from_workspace_name(name, self.context.data_context.instrument))

    def clear_data(self):
        self.reconstructed_data.clear()
        self.reconstructed_data_name = ""
        self._selection = ""

    def create_options(self):
        if self._is_groups:
            return self.get_group_options()
        else:
            return self._raw_model.gen_detector_options()
        return []

    """
    Handle reconstructed data
    """
    def set_reconstructed_data(self, ws, table_name):
        # get the indices and groups first
        self.reconstructed_data_name = ws
        if table_name:
            table = retrieve_ws(table_name)
            for index in range(table.rowCount()):
                data = table.row(index)
                self.reconstructed_data[index] = data["Group"]

    def add_reconstructed_data(self, workspaces, indices):
        first, last = 0, 0
        if self._is_groups:
            first, last = self.get_first_and_last_group_by_index()
            last += 1 # to include the last group
        else:
            first, last = self.get_first_and_last_detector_by_index()
        #plus 1 to include last
        for key in range(first, last):
            workspaces += [self.reconstructed_data_name]
            indices += [key]
        return workspaces, indices

    """
    Plotting
    """
    @staticmethod
    def _get_freq_label(workspace_name):
        label = ''
        if MAXENT_STR in workspace_name:
            label = f';{MAXENT_STR}'
        return label

    def _create_workspace_label(self, workspace_name, index):
        run = str(get_run_numbers_as_string_from_workspace_name(workspace_name, self.context.data_context.instrument))
        # maxent
        if MAXENT_STR in workspace_name:
            freq_label = self._get_freq_label(workspace_name)
            if RECONSTRUCTED_SPECTRA in workspace_name and self._is_groups:
                group = self.reconstructed_data[index]
                return f"{run} {group} {freq_label}"+RECONSTRUCTED_SPECTRA
            elif RECONSTRUCTED_SPECTRA in workspace_name:
                return self._raw_model._create_workspace_label(workspace_name,index)+RECONSTRUCTED_SPECTRA
            else:
                return f"{run} Frequency {freq_label}"
        #actual data
        if self._is_groups and self.reconstructed_data:
            group = self.reconstructed_data[index]
            return f"{run} {group}"
        elif "raw" in workspace_name:
            return self._raw_model._create_workspace_label(workspace_name,index)
        return ""

    def get_workspace_list_and_indices_to_plot(self):
        workspace_list, indices = [], []
        if self._is_groups:
            group_list = self.get_group_list()
            workspace_list, indices = self._time_group_model.get_workspace_list_and_indices_to_plot(True, "Counts", group_list)
        elif self._run:
            workspace_list, indices = self._raw_model.get_workspace_list_and_indices_to_plot(True, "Counts", self._selection, self._run,
                                                                                              self._period)
        return workspace_list, indices

    def create_tiled_keys(self, tiled_by):
        if self._is_groups:
            return ["Maxent"] + self.get_group_list()
        else:
            def_zero, _ = self.get_first_and_last_detector_by_index()
            return ["Maxent"] + self._raw_model.create_tiled_keys(tiled_by, def_zero + 1)

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index = 0):
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return 0
        # maxent
        if MAXENT_STR in workspace_name:
            if RECONSTRUCTED_SPECTRA in workspace_name and self._is_groups:
                group = self.reconstructed_data[index]
                return axes_workspace_map[group] # index is the group name
            elif RECONSTRUCTED_SPECTRA in workspace_name:
                # add 1 as the first axis is reserved for Maxent
                return self._raw_model.convert_index_to_axis(index) + 1
            else:
                return axes_workspace_map["Maxent"]
        #actual data
        if self._is_groups:
            group_pair_name, run_as_string = self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name(workspace_name)
            if group_pair_name in axes_workspace_map:
                return axes_workspace_map[group_pair_name]
            else:
                return 0
        else:
            return self._raw_model.convert_index_to_axis(index) + 1

    """
    Methods for detectors
    """
    def get_first_and_last_detector_by_index(self):
        return self._raw_model._get_first_and_last_detector_to_plot(self._selection)

    """
    Methods for dealing with group selection
    """
    def _get_index_from_group(self, selected_group):
        for index in self.reconstructed_data.keys():
            group = self.reconstructed_data[index]
            if group == selected_group:
                return index
        return 0

    def get_first_and_last_group_by_index(self):
        if self._selection=="":
            return 0,1
        group_string = self._selection.split(':')[0]
        first = self._get_index_from_group(group_string)

        group_string = self._selection.split(':')[1]
        last = self._get_index_from_group(group_string)

        return first, last

    def get_group_list(self):
        if not self.reconstructed_data:
            return []
        first, last = self.get_first_and_last_group_by_index()
        group_list = []
        # add 1 to include last
        for index in range(first,last+1):
            group_list.append(self.reconstructed_data[index])
        return group_list

    def get_group_options(self):
        if not self.reconstructed_data:
            return []
        options = []
        groups = [self.reconstructed_data[key] for key in self.reconstructed_data.keys()]
        num_selections = ceil(len(groups)/self._time_spec_limit)

        for j in range(num_selections-1):
            lower = (j*self._time_spec_limit)
            upper = (j+1)*self._time_spec_limit-1
            options.append(groups[lower]+":"+groups[upper])
        if num_selections > 1:
            lower = ((num_selections-1)*self._time_spec_limit)
        else:
            lower = 0
        options.append(groups[lower]+":"+groups[len(groups)-1])
        return options
