from __future__ import (absolute_import, division, print_function)

from six import iteritems

import mantid.simpleapi as mantid


class FFTWrapper(object):
    """
    A class to wrap the different parts
    of the FFT and its preprocessing.
    This keeps the main FFT class simple.
    """
    def __init__(self,FFT):
        self.name = "FFT"
        self.model = FFT

    def loadData(self,inputs):
        """
        store the data in the wrapper for later
        """
        if "phaseTable" in inputs:
            self.phaseTable = inputs["phaseTable"]
        else:
            self.phaseTable = None
        if "preRe" in inputs:
            self.preRe = inputs["preRe"]
        else:
            self.preRe = None
        if "preIm" in inputs:
            self.preIm = inputs["preIm"]
        else:
            self.preIm = None
        if "FFT" in inputs:
            self.FFT = inputs["FFT"]
        else:
            self.FFT = None
        self.model.setRun(inputs["Run"])

    def execute(self):
        """
        runs the relevant parts of the FFT and the preprocessing
        """
        try:
            if self.phaseTable is not None:
                if self.phaseTable["newTable"]:
                    self.model.makePhaseQuadTable(self.phaseTable)
                self.model.PhaseQuad()

            if self.preRe is not None:
                self.model.preAlg(self.preRe)

            if self.preIm is not None:
                self.model.preAlg(self.preIm)

            if self.FFT is not None:
                self.model.FFTAlg(self.FFT)
        except:
            pass

    def output(self):
        return


class FFTModel(object):
    """
    A simple class which executes
    the relevant algorithms for
    the analysis.
    """
    def __init__(self):
        self.name = "FFT"

    def setRun(self,run):
        self.runName=run

    def preAlg(self,preInputs):
        """
        PaddingAndApodization alg on the data
        """
        preAlg = mantid.AlgorithmManager.create("PaddingAndApodization")
        preAlg.initialize()
        preAlg.setChild(True)
        for name,value in iteritems(preInputs):
            preAlg.setProperty(name,value)
        preAlg.execute()
        mantid.AnalysisDataService.addOrReplace(preInputs["OutputWorkspace"], preAlg.getProperty("OutputWorkspace").value)

    def FFTAlg(self,FFTInputs):
        """
        Use the FFT alg
        """
        alg = mantid.AlgorithmManager.create("FFT")
        alg.initialize()
        alg.setChild(True)
        for name,value in iteritems(FFTInputs):
            alg.setProperty(name,value)
        alg.execute()
        mantid.AnalysisDataService.addOrReplace(FFTInputs["OutputWorkspace"],alg.getProperty("OutputWorkspace").value)

        ws = alg.getPropertyValue("OutputWorkspace")
        group = mantid.AnalysisDataService.retrieve(self.runName)
        group.add(ws)

    def makePhaseQuadTable(self,inputs):
        """
        generates a phase table from CalMuonDetectorPhases
        """
        alg = mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        alg.initialize()
        alg.setChild(True)

        alg.setProperty("FirstGoodData",inputs["FirstGoodData"])
        alg.setProperty("LastGoodData",inputs["LastGoodData"])

        alg.setProperty("InputWorkspace","MuonAnalysis")
        alg.setProperty("DetectorTable","PhaseTable")
        alg.setProperty("DataFitted","fits")
        alg.execute()
        mantid.AnalysisDataService.addOrReplace("PhaseTable",alg.getProperty("DetectorTable").value)

    def PhaseQuad(self):
        """
        do the phaseQuad algorithm
        groups data into a single set
        """
        phaseQuad = mantid.AlgorithmManager.create("PhaseQuad")
        phaseQuad.initialize()
        phaseQuad.setChild(False)
        print (self.runName)
        phaseQuad.setProperty("InputWorkspace","MuonAnalysis")
        phaseQuad.setProperty("PhaseTable","PhaseTable")
        phaseQuad.setProperty("OutputWorkspace","__phaseQuad__")
        phaseQuad.execute()

    def getName(self):
        return self.name
