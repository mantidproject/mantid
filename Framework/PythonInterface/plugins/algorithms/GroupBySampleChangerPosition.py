# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,too-many-instance-attributes,too-many-branches,no-init,deprecated-module
from mantid.simpleapi import GroupWorkspaces
from mantid.api import (Algorithm, AlgorithmFactory, WorkspaceGroupProperty)
from mantid.kernel import (Direction, StringArrayProperty)

SAMP_POSITION = {6.0: "btm",
                 85.5: "mid",
                 165.0: "top",
                 "None": "None"}


class GroupBySampleChangerPosition(Algorithm):
    _input_workspace = None
    _output_prefix = ''
    _output_suffix = ''

    def category(self):
        return 'Workflow\\Inelastic;Inelastic\\Indirect'

    def summary(self):
        return 'Separates a Group workspace into separate groups based on sample changer position.'

    def PyInit(self):
        # Input properties
        self.declareProperty(WorkspaceGroupProperty('InputWorkspace', '',
                                                    direction=Direction.Input),
                             doc='Workspace group for the workspaces to be sorted.')

        self.declareProperty('OutputGroupPrefix', '',
                             doc='String to prefix the output groups.')
        self.declareProperty('OutputGroupSuffix', '',
                             doc='String to suffix the output groups.')

        self.declareProperty(StringArrayProperty('OutputWorkspaceList', [], direction=Direction.Output),
                             doc='List of Workspace group for the resulting workspaces.')

    #pylint: disable=too-many-locals
    def PyExec(self):
        self._input_workspace = self.getProperty('InputWorkspace').value
        self._output_prefix = self.getProperty('OutputGroupPrefix').value
        self._output_suffix = self.getProperty('OutputGroupSuffix').value
        sample_sorted_lists = self._sort_runs_by_sample()
        output_ws_name = []
        for sample in sample_sorted_lists:
            group_ws_name = self._output_prefix + "_" + SAMP_POSITION[sample] + "_" + self._output_suffix
            GroupWorkspaces(InputWorkspaces=sample_sorted_lists[sample], OutputWorkspace=group_ws_name)
            output_ws_name.append(group_ws_name)
        self.setProperty('OutputWorkspaceList', output_ws_name)

    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate the instrument configuration by checking if a parameter file exists
        return issues

    def _sort_runs_by_sample(self):
        sample_sorted_lists = {}
        for ws in self._input_workspace:
            if ws.getRun().hasProperty("SAMP_POSN"):
                samp_posn = ws.getRun().getProperty("SAMP_POSN").value[1]
                if samp_posn in sample_sorted_lists.keys():
                    sample_sorted_lists[samp_posn].append(ws.name())
                else:
                    sample_sorted_lists[samp_posn] = [ws.name()]
            else:
                if "None" in sample_sorted_lists.keys():
                    sample_sorted_lists["None"].append(ws.name())
                else:
                    sample_sorted_lists["None"] = [ws.name()]
        return sample_sorted_lists


# Register algorithm with Mantid
AlgorithmFactory.subscribe(GroupBySampleChangerPosition)
