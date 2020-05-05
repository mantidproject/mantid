# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import NamedTuple, List
from Muon.GUI.Common.ADSHandler.workspace_naming import *
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.fitting_tab_widget.fitting_tab_model import MUON_ANALYSIS_GUESS_WS, \
    FREQUENCY_DOMAIN_ANALYSIS_GUESS_WS

FIT_FUNCTION_GUESS_LABEL = "Fit function guess"


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

    def create_workspace_plot_information(self, input_workspace_names: List[str], input_indices: List[int],
                                          errors: bool) -> List[WorkspacePlotInformation]:
        """
        Creates a list of workspace plot information (workspace name, index, axis..) from a input list
        of indices and workspace names
        :param input_workspace_names: List of workspace names
        :param input_indices: List of workspace indices
        :param errors: Boolean stating whether errors are to be plotted
        :return: A list of WorkspacePlotInformation
        """
        workspace_plot_information = []
        for workspace_name, index in zip(input_workspace_names, input_indices):
            axis = self._get_workspace_plot_axis(workspace_name)
            if not self._is_guess_workspace(workspace_name):
                workspace_plot_information += [self.create_plot_information(workspace_name, index, axis, errors)]
            else:
                workspace_plot_information += [self.create_plot_information_for_guess_ws(workspace_name)]

        return workspace_plot_information

    def update_tiled_axis_map(self, tiled_keys: List[str], tiled_by_type: str):
        """
        Updates the map containing the {tiled_key: axis}. This map is used to assign
        each WorkspacePlotInformation to the correct tiled.
        :param tiled_keys: A list of strings which are used as the keys for the tiles.
        :param tiled_by_type: A string describing the tiling mechanism.
        """
        self._is_tiled = True
        self._axes_workspace_map = {}
        self._tiled_by = tiled_by_type
        for axis_number, key in enumerate(tiled_keys):
            self._axes_workspace_map[key] = axis_number

    def create_plot_information(self, workspace_name: str, index: int, axis: int,
                                errors: bool) -> WorkspacePlotInformation:
        """
        Creates a workspace plot information instance (workspace name, index, axis..) from an input
        workspace name, index, axis and errors flag.
        :param workspace_name: List of workspace names
        :param index: List of workspace indices
        :param axis: An integer describing the axis which the workspace and index will be plotted on.
        :param errors: Boolean stating whether errors are to be plotted
        :return: A WorkspacePlotInformation instance desciribng the data to be plotted
        """
        label = self._create_workspace_label(workspace_name, index)
        return WorkspacePlotInformation(workspace_name=workspace_name, index=index, axis=axis,
                                        normalised=self._normalised,
                                        errors=errors, label=label)

    def create_plot_information_for_guess_ws(self, guess_ws_name: str) -> WorkspacePlotInformation:
        """
        Creates a workspacePlotInformation instance for the fit_guess
        By default this is plotted on the zero'th axis.
        :param guess_ws_name: The workspace name for the fit function guess
        :return: A WorkspacePlotInformation instance describing the data to be plotted
        """
        return WorkspacePlotInformation(workspace_name=guess_ws_name, index=1, axis=0,
                                        normalised=self._normalised,
                                        errors=False, label=FIT_FUNCTION_GUESS_LABEL)

    def create_axes_titles(self):
        if not self._is_tiled:
            return ''
        else:
            return list(self._axes_workspace_map.keys())

    def _create_workspace_label(self, workspace_name, index):
        group = str(get_group_or_pair_from_name(workspace_name))
        run = str(get_run_number_from_workspace_name(workspace_name, self._context.data_context.instrument))
        instrument = self._context.data_context.instrument
        fit_label = self._get_fit_label(workspace_name, index)
        rebin_label = self._get_rebin_label(workspace_name)
        freq_label = self._get_freq_lebel(workspace_name)
        if not self._is_tiled:
            return "".join([instrument, run, ';', group, fit_label, rebin_label, freq_label])
        if self._tiled_by == "Group/Pair":
            return "".join([run, fit_label, rebin_label, freq_label])
        else:
            return "".join([group, fit_label, rebin_label, freq_label])

    def _get_workspace_plot_axis(self, workspace_name: str):
        if not self._is_tiled:
            return 0
        for key, axis in self._axes_workspace_map.items():
            if key in workspace_name:
                return axis
        else:
            return 0

    @staticmethod
    def _get_rebin_label(workspace_name):
        if REBIN_STR in workspace_name:
            return ''.join([';', REBIN_STR])
        else:
            return ''

    @staticmethod
    def _get_freq_lebel(workspace_name):
        label = ''
        if FFT_STR in workspace_name:
            label = ''.join([';', get_fft_component_from_workspace_name(workspace_name)])
        elif MAXENT_STR in workspace_name:
            label = ''.join([';', MAXENT_STR])
        return label

    @staticmethod
    def _get_fit_label(workspace_name, index):
        label = ''
        fit_function_name = get_fit_function_name_from_workspace(workspace_name)
        if fit_function_name:
            if index in [1, 3]:
                workspace_type = 'Calc'
            elif index == 2:
                workspace_type = 'Diff'
            label = ''.join([';', fit_function_name, ';', workspace_type])
        return label

    def _is_guess_workspace(self, workspace_name):
        if MUON_ANALYSIS_GUESS_WS in workspace_name or FREQUENCY_DOMAIN_ANALYSIS_GUESS_WS in workspace_name:
            return True
        else:
            return False
