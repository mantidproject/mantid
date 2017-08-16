from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid


class MaxEntModel(object):

    def __init__(self,inputs):
        self.inputs=inputs
        self.alg=mantid.AlgorithmManager.create("MaxEnt")
        self.alg.initialize()
        self.alg.setChild(True)
        for name,value in iteritems(self.inputs):
            mantid.logger.warning(name+"  "+str(value))
            self.alg.setProperty(name,value)
    def execute(self):
        self.alg.execute() 
    def output(self):
        wsChi=  mantid.AnalysisDataService.addOrReplace( self.inputs["EvolChi"],self.alg.getProperty("EvolChi").value)
        wsAngle=mantid.AnalysisDataService.addOrReplace( self.inputs["EvolAngle"],self.alg.getProperty("EvolAngle").value)
        wsImage=mantid.AnalysisDataService.addOrReplace( self.inputs["ReconstructedImage"],self.alg.getProperty("ReconstructedImage").value)
        wsData= mantid.AnalysisDataService.addOrReplace( self.inputs["ReconstructedData"],self.alg.getProperty("ReconstructedData").value)

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

