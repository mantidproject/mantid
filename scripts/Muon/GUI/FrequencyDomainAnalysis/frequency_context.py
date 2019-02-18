# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

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
        # get everything from the ADS
        return
        self.options = mantid.AnalysisDataService.getObjectNames()
        self.options = [item.replace(" ", "") for item in self.options]
        self.N_points = len(tmpWS.readX(0))
        self.instrument = tmpWS.getInstrument().getName()

        self.runName = self.instrument + str(tmpWS.getRunNumber()).zfill(8)

    # get methods
    def getNPoints(self):
        print(self.context.current_runs)
        run_numbers = self.context.current_runs
        instrument = str(self.context.instrument)
        ws = [instrument + str(run_number[0]) +
              "_raw_data" for run_number in run_numbers]
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
    def getWorkspaceNames(self):
        pair_names = list(self.context.pair_names)
        run_numbers = self.context.current_runs
        final_options = []
        for run in run_numbers:
            final_options.append(
                self.context.instrument + str(run[0]) + " (PhaseQuad)")
            for name in pair_names:
                final_options.append(
                    wsName.get_pair_data_workspace_name(self.context,
                                                        str(name),
                                                        str(run[0])))
        return final_options

    # Get the groups/pairs for active WS
    def getGroupedWorkspaceNames(self):
        run_numbers = self.context.current_runs
        instrument = str(self.context.instrument)
        runs = [instrument + str(run_number[0]) for run_number in run_numbers]
        return runs
