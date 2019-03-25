# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid
from Muon.GUI.Common.utilities.run_string_utils import run_list_to_string
from Muon.GUI.Common.muon_data_context import get_default_grouping

import Muon.GUI.Common.ADSHandler.workspace_naming as wsName


class FrequencyContext(object):

    """
    A simple class for identifing the current run
    and it can return the name, run and instrument.
    The current run is the same as the one in MonAnalysis
    """

    def __init__(self, context):
        self.context = context
        self.options = "None"
        self.options = [item.replace(" ", "") for item in self.options]
        self.N_points = 1
        self.instrument = "None"

        self.runName = "None"

    @property
    def version(self):
        return 2

    def setUp(self, tmpWS):
        pass

    # get methods
    def getNPoints(self):
        run_numbers = self.context.current_runs
        ws = [wsName.get_raw_data_workspace_name(self.context, run_list_to_string(run_number)) for run_number in
              run_numbers]
        self.N_points = 1
        for name in ws:
            data = mantid.AnalysisDataService.retrieve(name)
            if self.N_points < len(data.readX(0)):
                self.N_points = len(data.readX(0))

        return self.N_points

    def getCurrentWS(self):
        return self.runName, self.options

    def getRunName(self):
        return self.runName

    def getInstrument(self):
        return self.context.instrument

    # check if data matches current
    def digit(self, x):
        return int(filter(str.isdigit, x) or 0)

    def hasDataChanged(self):
        return False

    # check if muon analysis exists
    def MuonAnalysisExists(self):
        # if period data look for the first period
        if mantid.AnalysisDataService.doesExist("MuonAnalysis_1"):
            tmpWS = mantid.AnalysisDataService.retrieve("MuonAnalysis_1")
            return True, tmpWS
            # if its not period data
        elif mantid.AnalysisDataService.doesExist("MuonAnalysis"):
            tmpWS = mantid.AnalysisDataService.retrieve("MuonAnalysis")
            return True, tmpWS
        else:
            return False, None

    # Get the groups/pairs for active WS
    # ignore raw files
    def getWorkspaceNames(self, use_raw):
        pair_names = list(self.context.pair_names)
        group_names = list(self.context.group_names)
        run_numbers = self.context.current_runs
        final_options = []
        for run in run_numbers:
            final_options += [wsName.get_raw_data_workspace_name(self.context, run_list_to_string(run), period=str(period + 1)) +
                              " (PhaseQuad)" for period in range(self.context.num_periods(run))]
            for name in pair_names:
                final_options.append(
                    wsName.get_pair_data_workspace_name(self.context,
                                                        str(name),
                                                        run_list_to_string(run), not use_raw))
            for group_name in group_names:
                final_options.append(
                    wsName.get_group_asymmetry_name(self.context, str(group_name), run_list_to_string(run),
                                                    not use_raw))
        return final_options

    # Get the groups/pairs for active WS
    def getGroupedWorkspaceNames(self):
        run_numbers = self.context.current_runs
        runs = [wsName.get_raw_data_workspace_name(self.context, run_list_to_string(run_number), period=str(period + 1))
                for run_number in run_numbers for period in range(self.context.num_periods(run_number))]
        return runs

    def get_detectors_excluded_from_default_grouping_tables(self):
        groups, _ = get_default_grouping(
            self.context.current_workspace, self.context.instrument, self.context.main_field_direction)
        detectors_in_group = []
        for group in groups:
            detectors_in_group += group.detectors
        detectors_in_group = set(detectors_in_group)

        return [det for det in range(1, self.context.num_detectors) if det not in detectors_in_group]
