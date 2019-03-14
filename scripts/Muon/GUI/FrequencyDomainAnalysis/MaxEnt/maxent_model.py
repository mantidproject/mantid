# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from six import iteritems

import mantid.simpleapi as mantid
from Muon.GUI.Common.validate_errors import validateToErrors


class MaxEntWrapper(object):

    """
    A class to wrap the different parts
    of the MaxEnt and its preprocessing.
    This keeps the main MaxEnt class simple.
    """

    def __init__(self, maxent):
        self.name = "MaxEnt"
        self.model = maxent

    def loadData(self, inputs):
        """
        store the data in the wrapper for later
        """
        self.phaseTable = inputs.get("phaseTable", None)
        self.maxent = inputs.get("maxent", None)
        self.model.setRun(inputs["Run"])

    def execute(self):
        """
        runs the relevant parts of the MaxEnt and the preprocessing
        """
        if self.phaseTable is not None:
            self.model.makePhaseTable(self.phaseTable)

        if self.maxent is not None:
            self.model.MaxEntAlg(self.maxent)

    def cancel(self):
        self.model.cancel()

    def output(self):
        return


class MaxEntModel(object):

    """
    A simple class which executes
    the relevant algorithms for
    the analysis.
    """

    def __init__(self):
        self.name = "MaxEnt"
        self.alg = None

    def setRun(self, run):
        self.run = run

    def MaxEntAlg(self, inputs):
        """
        Use the MaxEnt alg
        """
        self.alg = mantid.AlgorithmManager.create("MuonMaxent")
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        for name, value in iteritems(inputs):
            self.alg.setProperty(name, value)
        self.alg.setRethrows(True)
        validateToErrors(self.alg)
        self.alg.execute()
        self.addOutput(inputs, self.alg, "OutputWorkspace")
        self.addOutput(inputs, self.alg, "OutputPhaseTable")
        self.addOutput(inputs, self.alg, "OutputDeadTimeTable")
        self.addOutput(inputs, self.alg, "ReconstructedSpectra")
        self.addOutput(inputs, self.alg, "PhaseConvergenceTable")
        self.alg = None

    def makePhaseTable(self, inputs):
        """
        generates a phase table from CalMuonDetectorPhases
        """
        self.alg = mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        self.alg.initialize()
        self.alg.setAlwaysStoreInADS(False)
        self.alg.setRethrows(True)

        for name, value in iteritems(inputs):
            self.alg.setProperty(name, value)
        # check for version 1
        if inputs["InputWorkspace"] != "MuonAnalysis" and "MuonAnalysisGrouped" in inputs["InputWorkspace"]:
            raise ValueError(
                "Cannot currently generate phase table from this data using CalMuonDetectorPhases")
        self.alg.execute()
        name = "DetectorTable"
        mantid.AnalysisDataService.addOrReplace(
            inputs[name],
            self.alg.getProperty(name).value)
        self.alg = None

    def addOutput(self, inputs, alg, name):

        if name in inputs:
            mantid.AnalysisDataService.addOrReplace(
                inputs[name],
                alg.getProperty(name).value)
        else:
            return
        if mantid.AnalysisDataService.doesExist(self.run):
            group = mantid.AnalysisDataService.retrieve(self.run)
        else:
            mantid.GroupWorkspaces(OutputWorkspace=self.run)
        group.add(inputs[name])

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def getName(self):
        return self.name
