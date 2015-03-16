#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *


class EnginXCalibrate(PythonAlgorithm):
    def category(self):
        return "Diffraction\Engineering;PythonAlgorithms"

    def name(self):
        return "EnginXCalibrate"

    def summary(self):
        return "Calibrates a detector bank by performing a single peak fitting."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", FileAction.Load),\
    		"Calibration run to use")

        self.declareProperty(FloatArrayProperty("ExpectedPeaks", ""),\
    		"A list of dSpacing values where peaks are expected.")

        self.declareProperty("Bank", 1, "Which bank to calibrate")

        self.declareProperty(ITableWorkspaceProperty("DetectorPositions", "", Direction.Input, PropertyMode.Optional),\
    		"Calibrated detector positions. If not specified, default ones are used.")

        self.declareProperty("Difc", 0.0, direction = Direction.Output,\
    		doc = "Calibrated Difc value for the bank")

        self.declareProperty("Zero", 0.0, direction = Direction.Output,\
    		doc = "Calibrated Zero value for the bank")

    def PyExec(self):

        ws = self._focusRun()

        fitPeaksAlg = self.createChildAlgorithm('EnginXFitPeaks')
        fitPeaksAlg.setProperty('InputWorkspace', ws)
        fitPeaksAlg.setProperty('WorkspaceIndex', 0) # There should be only one index anyway
        fitPeaksAlg.setProperty('ExpectedPeaks', self.getProperty('ExpectedPeaks').value)
        fitPeaksAlg.execute()

        self.setProperty('Difc', fitPeaksAlg.getProperty('Difc').value)
        self.setProperty('Zero', fitPeaksAlg.getProperty('Zero').value)

    def _focusRun(self):
        alg = self.createChildAlgorithm('EnginXFocus')
        alg.setProperty('Filename', self.getProperty('Filename').value)
        alg.setProperty('Bank', self.getProperty('Bank').value)

        detPos = self.getProperty('DetectorPositions').value
        if detPos:
            alg.setProperty('DetectorPositions', detPos)

        alg.execute()

        return alg.getProperty('OutputWorkspace').value


AlgorithmFactory.subscribe(EnginXCalibrate)
