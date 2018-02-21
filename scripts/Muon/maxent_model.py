from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid


class MaxEntModel(object):
    # A simple class to hold the MaxEnt algorithm

    def __init__(self):
        self.alg=mantid.AlgorithmManager.create("MuonMaxent")
        self.alg.initialize()
        self.alg.setChild(True)

    def setInputs(self,inputs,runName):
        self.inputs=inputs
        for name,value in iteritems(self.inputs):
            self.alg.setProperty(name,value)
        self.run=runName

    def execute(self):
        self.alg.execute()

    def cancel(self):
        self.alg.cancel()

    def output(self):
        self.addOutput("OutputWorkspace")
        self.addOutput("OutputPhaseTable")
        self.addOutput("OutputDeadTimeTable")
        self.addOutput("ReconstructedSpectra")
        self.addOutput("PhaseConvergenceTable")

    def addOutput(self,name):
        if name in self.inputs:
            mantid.AnalysisDataService.addOrReplace( self.inputs[name],self.alg.getProperty(name).value)
        else:
            return
        if mantid.AnalysisDataService.doesExist(self.run):
            group=mantid.AnalysisDataService.retrieve(self.run)
        else:
            mantid.GroupWorkspaces(OutputWorkspace=self.run)

        group.add(self.inputs[name])


class PhaseModel(object):
    # A simple class to hold the CalcMuonDetectorPhases algorithm

    def __init__(self):
        self.calcAlg=mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        self.calcAlg.initialize()
        self.calcAlg.setChild(True)

    def loadData(self,inputs_phases):
        self.inputs_phases = inputs_phases
        if self.inputs_phases is not None:
            for name,value in iteritems(self.inputs_phases):
                self.calcAlg.setProperty(name,value)

    def execute(self):
        if self.inputs_phases is not None:
            self.calcAlg.execute()
            name="DetectorTable"
            mantid.AnalysisDataService.addOrReplace( self.inputs_phases[name],self.calcAlg.getProperty(name).value)

    def cancel(self):
        self.calcAlg.cancel()

    def output(self):
        return
