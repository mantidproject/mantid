# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List
from Muon.GUI.Common.ADSHandler.workspace_naming import *
from Muon.GUI.Common.contexts.muon_context import MuonContext

DEFAULT_X_LIMITS = [0, 15]


class WorkspacePlotInformation(NamedTuple):
    workspace_name: str
    index: int
    axis: int
    normalised: bool
    errors: bool
    label: str

    # equal only checks for workspace, axis, and spec num, as the user could have changed the other states
    def __eq__(self, other):
        if self.workspace_name == other.workspace_name and \
                self.index == other.index and \
                self.axis == other.axis:
            return True
        else:
            return False


class PlottingCanvasModel(object):

    def __init__(self, context: MuonContext):
        self._user_axis_limits = None
        self._axes_workspace_map = {}
        self._context = context
        # Options from the view
        self._tiled_by = "Group/Pair"
        self._is_tiled = False
        self._errors = False
        self._normalised = False

    @property
    def is_tiled(self):
        return self._is_tiled

    @is_tiled.setter
    def is_tiled(self, state: bool):
        self._is_tiled = state

    def create_workspace_plot_information(self, input_workspace_names, input_indices, errors,
                                          existing_plot_information):
        workspace_plot_information = []
        for workspace_name, index in zip(input_workspace_names, input_indices):
            axis = self._get_workspace_plot_axis(workspace_name)

            workspace_plot_information += [self.create_plot_information(workspace_name, index, axis, errors)]
        workspaces_plot_information_to_add = [plot_info for plot_info in workspace_plot_information if plot_info not in
                                              existing_plot_information]

        return workspaces_plot_information_to_add

    def _get_workspace_plot_axis(self, workspace_name):
        if not self._is_tiled:
            return 0
        for axis, key in enumerate(self._axes_workspace_map):
            if key in workspace_name:
                return axis
        else:
            return 0

    def update_tiled_axis_map(self, tiled_keys: List[str], tiled_by_type: str):
        self._is_tiled = True
        self._axes_workspace_map = {}
        self._tiled_by = tiled_by_type
        for axis_number, key in enumerate(tiled_keys):
            self._axes_workspace_map[key] = axis_number

    def create_plot_information(self, workspace_name, index, axis, errors):
        label = self._create_workspace_label(workspace_name, index)
        return WorkspacePlotInformation(workspace_name=workspace_name, index=index, axis=axis,
                                        normalised=self._normalised,
                                        errors=errors, label=label)

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_number_from_workspace_name(workspace_name, self._context.data_context.instrument))
        instrument = self._context.data_context.instrument
        fft_label = self._get_freq_label()
        fit_label = self._get_fit_label(workspace_name, index)
        rebin_label = self._get_rebin_label(workspace_name)
        if not self._is_tiled:
            return "".join([instrument, run, ';', group, rebin_label])
        if self._tiled_by == "Group/Pair":
            return "".join([run, ';', self._get_rebin_label(workspace_name)])
        else:
            return "".join([group, ';', self._get_rebin_label(workspace_name)])

    def _get_freq_label(self):
        return ''

    def _get_rebin_label(self, workspace_name):
        if REBIN_STR in workspace_name:
            return ''.join([';', REBIN_STR])
        else:
            return ''

    def _get_fit_label(self, workspace_name, index):
        return ''

    def create_axes_titles(self):
        if not self._is_tiled:
            return ''
        else:
            return list(self._axes_workspace_map.keys())
