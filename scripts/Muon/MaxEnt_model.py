from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid
from PyQt4.QtCore import QThread


class MaxEntThread(QThread):
    # a wrapper to allow threading with
    # the MaxEnt algorithm.

    def __init__(self,alg):
        QThread.__init__(self)
        self.alg=alg

    def __del__(self):
         self.wait()
		  
    def run(self):
        self.alg.execute()
        self.alg.output()
        return

    def setInputs(self,inputs):
        self.alg.setInputs(inputs)

class MaxEntModel(object):
    # A simple class to hold the MaxEnt algorithm

    def __init__(self):
        self.alg=mantid.AlgorithmManager.create("MaxEnt")
        self.alg.initialize()
        self.alg.setChild(True)

    def setInputs(self,inputs):
        self.inputs=inputs
        for name,value in iteritems(self.inputs):
            self.alg.setProperty(name,value)

    def execute(self):
        self.alg.execute()

    def output(self):
        mantid.AnalysisDataService.addOrReplace( self.inputs["EvolChi"],self.alg.getProperty("EvolChi").value)
        mantid.AnalysisDataService.addOrReplace( self.inputs["EvolAngle"],self.alg.getProperty("EvolAngle").value)
        mantid.AnalysisDataService.addOrReplace( self.inputs["ReconstructedImage"],self.alg.getProperty("ReconstructedImage").value)
        mantid.AnalysisDataService.addOrReplace( self.inputs["ReconstructedData"],self.alg.getProperty("ReconstructedData").value)

        if mantid.AnalysisDataService.doesExist("EvolChiMuon"):
            EvolChiMuon=mantid.AnalysisDataService.retrieve("EvolChiMuon")
            EvolChiMuon.add(self.inputs["EvolChi"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=self.inputs["EvolChi"],OutputWorkspace="EvolChiMuon")

        if mantid.AnalysisDataService.doesExist("EvolAngleMuon"):
            EvolAngleMuon=mantid.AnalysisDataService.retrieve("EvolAngleMuon")
            EvolAngleMuon.add(self.inputs["EvolAngle"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=self.inputs["EvolAngle"],OutputWorkspace="EvolAngleMuon")

        if mantid.AnalysisDataService.doesExist("ReconstructedImageMuon"):
            ReconstructedImageMuon=mantid.AnalysisDataService.retrieve("ReconstructedImageMuon")
            ReconstructedImageMuon.add(self.inputs["ReconstructedImage"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=self.inputs["ReconstructedImage"],OutputWorkspace="ReconstructedImageMuon")

        if mantid.AnalysisDataService.doesExist("ReconstructedDataMuon"):
            ReconstructedMuon=mantid.AnalysisDataService.retrieve("ReconstructedDataMuon")
            ReconstructedMuon.add(self.inputs["ReconstructedData"])
        else:
            mantid.GroupWorkspaces(InputWorkspaces=self.inputs["ReconstructedData"],OutputWorkspace="ReconstructedDataMuon")
