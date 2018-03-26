from __future__ import (absolute_import, division, print_function)

from six import iteritems

import mantid.simpleapi as mantid


class MaxEntWrapper(object):
    """
    A class to wrap the different parts
    of the MaxEnt and its preprocessing.
    This keeps the main MaxEnt class simple.
    """
    def __init__(self,maxent):
        self.name = "MaxEnt"
        self.model = maxent

    def loadData(self,inputs):
        """
        store the data in the wrapper for later
        """
        if "phaseTable" in inputs:
            self.phaseTable = inputs["phaseTable"]
        else:
            self.phaseTable = None
        if "maxent" in inputs:
            self.maxent = inputs["maxent"]
        else:
            self.maxent = None
        self.model.setRun(inputs["Run"])

    def execute(self):
        """
        runs the relevant parts of the MaxEnt and the preprocessing
        """
        try:
            if self.phaseTable is not None:
                self.model.makePhaseTable(self.phaseTable)

            if self.maxent is not None:
                self.model.MaxEntAlg(self.maxent)

        except:
            pass

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

    def setRun(self,run):
        self.run=run

    def MaxEntAlg(self,inputs):
        """
        Use the MaxEnt alg
        """
        alg = mantid.AlgorithmManager.create("MuonMaxent")
        alg.initialize()
        alg.setChild(True)
        for name,value in iteritems(inputs):
            alg.setProperty(name,value)
        alg.execute()
        print(inputs)
        self.addOutput(inputs,alg,"OutputWorkspace")
        self.addOutput(inputs,alg,"OutputPhaseTable")
        self.addOutput(inputs,alg,"OutputDeadTimeTable")
        self.addOutput(inputs,alg,"ReconstructedSpectra")
        self.addOutput(inputs,alg,"PhaseConvergenceTable")

    def makePhaseTable(self,inputs):
        """
        generates a phase table from CalMuonDetectorPhases
        """
        calcAlg=mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        calcAlg.initialize()
        calcAlg.setChild(True)
        
        for name,value in iteritems(inputs):
            calcAlg.setProperty(name,value)
        calcAlg.execute()
        name="DetectorTable"
        mantid.AnalysisDataService.addOrReplace( inputs[name],calcAlg.getProperty(name).value)

    def addOutput(self,inputs,alg,name):
        if name in inputs:
            mantid.AnalysisDataService.addOrReplace(inputs[name],alg.getProperty(name).value)
        else:
            return
        if mantid.AnalysisDataService.doesExist(self.run):
            group=mantid.AnalysisDataService.retrieve(self.run)
        else:
            mantid.GroupWorkspaces(OutputWorkspace=self.run)

        group.add(inputs[name])



    def getName(self):
        return self.name
