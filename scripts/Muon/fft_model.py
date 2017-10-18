from __future__ import (absolute_import, division, print_function)

from six import iteritems
import math

import mantid.simpleapi as mantid


class FFTWrapper(object):
    """
    A class to wrap the different parts
    of the FFT and its preprocessing.
    This keeps the main FFT class simple.
    """
    def __init__(self,FFT):
        self.name="FFT"
        self.model=FFT

    def loadData(self,inputs):
        """
        store the data in the wrapper for later
        """
        if "phaseTable" in inputs:
            self.phaseTable=inputs["phaseTable"]
        else:
            self.phaseTable=None
        if "preRe" in inputs:
            self.preRe=inputs["preRe"]
        else:
            self.preRe=None
        if "preIm" in inputs:
            self.preIm=inputs["preIm"]
        else:
            self.preIm=None
        if "FFT" in inputs:
            self.FFT=inputs["FFT"]
        else:
            self.FFT=None
        self.model.setRun(inputs["Run"])

    def execute(self):
        """
        runs the relevant parts of the FFT and the preprocessing
        """
        if self.phaseTable is not None:
            if self.phaseTable["newTable"]:
                self.model.makePhaseQuadTable(self.phaseTable["axis"],self.phaseTable["Instrument"])
            self.model.PhaseQuad()

        if self.preRe is not None:
            self.model.preAlg(self.preRe)

        if self.preIm is not None:
            self.model.preAlg(self.preIm)

        if self.FFT is not None:
            self.model.FFTAlg(self.FFT)

    def output(self):
        return


class FFTModel(object):
    """
    A simple class which executes
    the relevant algorithms for
    the analysis.
    """
    def __init__(self):
        self.name="FFT"

    def setRun(self,run):
        self.runName=run

    def preAlg(self,preInputs):
        """
        PaddingAndApodization alg on the data
        """
        preAlg=mantid.AlgorithmManager.create("PaddingAndApodization")
        preAlg.initialize()
        preAlg.setChild(True)
        for name,value in iteritems(preInputs):
            preAlg.setProperty(name,value)
        preAlg.execute()
        mantid.AnalysisDataService.addOrReplace(preInputs["OutputWorkspace"],preAlg.getProperty("OutputWorkspace").value)

    def FFTAlg(self,FFTInputs):
        """
        Use the FFT alg
        """
        alg=mantid.AlgorithmManager.create("FFT")
        alg.initialize()
        alg.setChild(True)
        for name,value in iteritems(FFTInputs):
            alg.setProperty(name,value)
        alg.execute()
        mantid.AnalysisDataService.addOrReplace(FFTInputs["OutputWorkspace"],alg.getProperty("OutputWorkspace").value)

        ws=alg.getPropertyValue("OutputWorkspace")
        group = mantid.AnalysisDataService.retrieve(self.runName)
        group.add(ws)

    def makePhaseQuadTable(self,axis,instrument):
        """
        generates a phase table based on the detector setup
        need to become an algorithm
        """
        wsAlg=mantid.AlgorithmManager.create("CreateSimulationWorkspace")
        wsAlg.initialize()
        wsAlg.setChild(True)
        wsAlg.setProperty("Instrument",instrument)
        wsAlg.setProperty("BinParams","0,1,32")
        wsAlg.setProperty("OutputWorkspace","__tmp__")
        wsAlg.execute()
        output=wsAlg.getProperty("OutputWorkspace").value

        tableAlg=mantid.AlgorithmManager.create("CreateEmptyTableWorkspace")
        tableAlg.initialize()
        tableAlg.setChild(False)
        tableAlg.setProperty("OutputWorkspace","PhaseTable")
        tableAlg.execute()

        phaseTable=mantid.AnalysisDataService.retrieve("PhaseTable")
        phaseTable.addColumn("int","DetectorID")
        phaseTable.addColumn("double","Phase")
        phaseTable.addColumn("double","Asym")

        for j in range(output.getNumberHistograms()):
            det = output.getDetector(j).getPos()-output.getInstrument().getSample().getPos()
            r=math.sqrt(det.X()**2+det.Y()**2+det.Z()**2)
            if(axis=="x"):
                phi=math.atan2(det.Z(),det.Y())
                asym=math.sqrt(det.Z()**2+det.Y()**2)/r
            elif(axis=="y"):
                phi=math.atan2(det.X(),det.Z())
                asym=math.sqrt(det.X()**2+det.Z()**2)/r
            else: # z
                phi=math.atan2(det.Y(),det.X())
                asym=math.sqrt(det.Y()**2+det.X()**2)/r
            phaseTable.addRow([j,asym,phi])

    def PhaseQuad(self):
        """
        do the phaseQuad algorithm
        groups data into a single set
         """
        phaseQuad=mantid.AlgorithmManager.create("PhaseQuad")
        phaseQuad.initialize()
        phaseQuad.setChild(False)
        print (self.runName)
        phaseQuad.setProperty("InputWorkspace","MuonAnalysis")
        phaseQuad.setProperty("PhaseTable","PhaseTable")
        phaseQuad.setProperty("OutputWorkspace","__phaseQuad__")
        phaseQuad.execute()

    def getName(self):
        return self.name
