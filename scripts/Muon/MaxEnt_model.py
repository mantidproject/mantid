from __future__ import (absolute_import, division, print_function)
from six import iteritems
import mantid.simpleapi as mantid
import threading

# wrapper class for calling threaded 
# MaxEnt algorithm.
# This is needed to enusre threads exit. 
# Without this the GUI will hang for 
# large data sets, at the cost of 
# the algorithm being slower
class MaxEntThread(object):
    def __init__(self,alg):
        self.alg=alg

    def execute(self):
        print ("MaxEnt calculation started (this may take a while)")
        thread = MaxEntThreadWrapper(self.alg)   
        thread.start()  

    def setInputs(self,inputs):
        self.alg.setInputs(inputs)   


# a wrapper to allow threading with
# the MaxEnt algorithm. 
class MaxEntThreadWrapper(threading.Thread):
    def __init__(self,alg):
        threading.Thread.__init__(self)
        self.alg=alg

    def run(self):
        self.alg.execute()
        self.alg.output()
        return
 
# A simple class to hold the MaxEnt algorithm
class MaxEntModel(object):

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

