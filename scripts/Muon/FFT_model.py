from __future__ import (absolute_import, division, print_function)
import mantid.simpleapi as mantid
from six import iteritems


class FFTModel(object):

    def __init__(self):
        self.name="FFT"

    def preAlg(self,preInputs):
        preAlg=mantid.AlgorithmManager.create("FFTPreProcessing")
        preAlg.initialize()
        preAlg.setChild(True)
        for name,value in iteritems(preInputs):
            preAlg.setProperty(name,value)
        preAlg.execute()
        mantid.AnalysisDataService.addOrReplace(preInputs["OutputWorkspace"],preAlg.getProperty("OutputWorkspace").value)

    def FFTAlg(self,FFTInputs):
        alg=mantid.AlgorithmManager.create("FFT")
        alg.initialize()
        alg.setChild(True)
        for name,value in iteritems(FFTInputs):
            alg.setProperty(name,value)
        alg.execute()
        mantid.AnalysisDataService.addOrReplace(FFTInputs["OutputWorkspace"],alg.getProperty("OutputWorkspace").value)
 
        ws=alg.getPropertyValue("OutputWorkspace")
        if mantid.AnalysisDataService.doesExist("FFTMuon"):
            FFTMuon=mantid.AnalysisDataService.retrieve("FFTMuon")
            FFTMuon.add(ws)
        else:
            FFTMuon=mantid.GroupWorkspaces(InputWorkspaces=ws)

    def getName(self):
        return self.name
