from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid


class MaxEntModel(object):
    # A simple class to hold the MaxEnt algorithm

    def __init__(self):
        self.alg=mantid.AlgorithmManager.create("MaxEnt")
        self.alg.initialize()
        self.alg.setChild(True)

    def setInputs(self,inputs,runName):
        self.inputs=inputs
        for name,value in iteritems(self.inputs):
            self.alg.setProperty(name,value)
        self.run=runName

    def execute(self):
        self.alg.execute()

    def output(self):
        mantid.AnalysisDataService.addOrReplace( self.inputs["EvolChi"],self.alg.getProperty("EvolChi").value)
        mantid.AnalysisDataService.addOrReplace( self.inputs["EvolAngle"],self.alg.getProperty("EvolAngle").value)
        mantid.AnalysisDataService.addOrReplace( self.inputs["ReconstructedImage"],self.alg.getProperty("ReconstructedImage").value)
        mantid.AnalysisDataService.addOrReplace( self.inputs["ReconstructedData"],self.alg.getProperty("ReconstructedData").value)

        if mantid.AnalysisDataService.doesExist(self.run):
            group=mantid.AnalysisDataService.retrieve(self.run)
        else:
            mantid.GroupWorkspaces(OutputWorkspace=self.run)

        group.add(self.inputs["EvolChi"])
        group.add(self.inputs["EvolAngle"])
        group.add(self.inputs["ReconstructedImage"])
        group.add(self.inputs["ReconstructedData"])
