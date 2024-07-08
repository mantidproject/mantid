# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid


class FFTWrapper(object):
    """
    A class to wrap the different parts
    of the FFT and its preprocessing.
    This keeps the main FFT class simple.
    """

    def __init__(self, FFT):
        self.name = "FFT"
        self.model = FFT
        self.phaseTable = None
        self.preRe = None
        self.preIm = None
        self.FFT = None

    def cancel(self):
        self.model.cancel()

    def loadData(self, inputs):
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
        if self.phaseTable is not None:
            if self.phaseTable["newTable"]:
                self.model.makePhaseQuadTable(self.phaseTable)
            self.model.PhaseQuad(self.phaseTable)
        if self.preRe is not None:
            self.model.preAlg(self.preRe)
        if self.preIm is not None:
            self.model.preAlg(self.preIm)
        if self.FFT is not None:
            self.model.FFTAlg(self.FFT)


class FFTModel(object):
    """
    A simple class which executes
    the relevant algorithms for
    the analysis.
    """

    def __init__(self):
        self.name = "FFT"
        self.alg = None

    def cancel(self):
        if self.alg is not None:
            self.alg.cancel()

    def setRun(self, run):
        self.runName = run

    def preAlg(self, preInputs):
        """
        PaddingAndApodization alg on the data
        """
        self.alg = mantid.AlgorithmManager.create("PaddingAndApodization")
        self.alg.initialize()
        self.alg.setRethrows(True)

        for name, value in preInputs.items():
            self.alg.setProperty(name, value)
        self.alg.execute()
        self.alg = None

    def FFTAlg(self, FFTInputs):
        """
        Use the FFT alg
        """
        self.alg = mantid.AlgorithmManager.create("FFT")
        self.alg.initialize()
        self.alg.setRethrows(True)

        for name, value in FFTInputs.items():
            self.alg.setProperty(name, value)
        self.alg.execute()

        group = mantid.AnalysisDataService.retrieve(self.runName)
        group.add(FFTInputs["OutputWorkspace"])
        self.alg = None

    def makePhaseQuadTable(self, inputs):
        """
        generates a phase table from CalMuonDetectorPhases
        """
        mantid.CloneWorkspace(InputWorkspace=inputs["InputWorkspace"], OutputWorkspace="__tmp__")
        mantid.MaskDetectors(Workspace="__tmp__", DetectorList=inputs["MaskedDetectors"], StoreInADS=False)

        self.alg = mantid.AlgorithmManager.create("CalMuonDetectorPhases")
        self.alg.initialize()
        self.alg.setRethrows(True)

        self.alg.setProperty("FirstGoodData", inputs["FirstGoodData"])
        self.alg.setProperty("LastGoodData", inputs["LastGoodData"])

        self.alg.setProperty("InputWorkspace", "__tmp__")
        self.alg.setProperty("DetectorTable", "PhaseTable")
        self.alg.setProperty("DataFitted", "fits")

        self.alg.execute()
        mantid.DeleteWorkspace("__tmp__")
        mantid.DeleteWorkspace("fits")
        self.alg = None

    def PhaseQuad(self, inputs):
        """
        do the phaseQuad algorithm
        groups data into a single set
        """
        cloned_workspace = mantid.CloneWorkspace(InputWorkspace=inputs["InputWorkspace"], StoreInADS=False)
        mantid.MaskDetectors(Workspace=cloned_workspace, DetectorList=inputs["MaskedDetectors"], StoreInADS=False)
        mantid.CropWorkspace(
            InputWorkspace=cloned_workspace,
            XMin=inputs["FirstGoodData"],
            XMax=inputs["LastGoodData"],
            OutputWorkspace="cropped_workspace_pre_phasequad",
        )

        self.alg = mantid.AlgorithmManager.create("PhaseQuad")
        self.alg.initialize()
        self.alg.setRethrows(True)

        self.alg.setProperty("InputWorkspace", "cropped_workspace_pre_phasequad")
        self.alg.setProperty("PhaseTable", "PhaseTable")
        self.alg.setProperty("OutputWorkspace", "__phaseQuad__")
        self.alg.execute()
        mantid.DeleteWorkspace("cropped_workspace_pre_phasequad")
        self.alg = None

    def getName(self):
        return self.name
