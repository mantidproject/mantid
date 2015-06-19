#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *
import math

class EnginXCalibrateFull(PythonAlgorithm):

    def category(self):
        return "Diffraction\\Engineering;PythonAlgorithms"

    def name(self):
        return "EnginXCalibrateFull"

    def summary(self):
        return "Calibrates every pixel position by performing single peak fitting."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", FileAction.Load),\
    		"Calibration run to use")

        self.declareProperty(ITableWorkspaceProperty("DetectorPositions", "", Direction.Output),\
    		"A table with the detector IDs and calibrated detector positions in V3P format.")

        self.declareProperty(FloatArrayProperty("ExpectedPeaks", ""),\
    		"A list of dSpacing values where peaks are expected.")

        self.declareProperty(FileProperty(name="ExpectedPeaksFromFile",defaultValue="",
                                          action=FileAction.OptionalLoad,extensions = [".csv"]),
                             "Load from file a list of dSpacing values to be translated into TOF to "
                             "find expected peaks. This takes precedence over 'ExpectedPeaks' if both "
                             "options are given.")

        self.declareProperty("Bank", 1, "Which bank to calibrate")

    def PyExec(self):

        import EnginXUtils

        # Get peaks in dSpacing from file, and check we have what we need, before doing anything
        expectedPeaksD = EnginXUtils.readInExpectedPeaks(self.getPropertyValue("ExpectedPeaksFromFile"),
                                                         self.getProperty('ExpectedPeaks').value)

        if len(expectedPeaksD) < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        ws = self._loadCalibrationRun()

        ws = self._prepareWsForFitting(ws)

        positionTable = self._createPositionsTable()

        indices = EnginXUtils.getWsIndicesForBank(self.getProperty('Bank').value, ws)

        prog = Progress(self, 0, 1, len(indices))

        for i in indices:

            _, difc = self._fitPeaks(ws, i, expectedPeaksD)

            det = ws.getDetector(i)

            newPos = self._getCalibratedDetPos(difc, det, ws)

            positionTable.addRow([det.getID(), newPos])

            prog.report()

        self.setProperty("DetectorPositions", positionTable)

    def _loadCalibrationRun(self):
        """ Loads specified calibration run
    	"""
        alg = self.createChildAlgorithm('Load')
        alg.setProperty('Filename', self.getProperty('Filename').value)
        alg.execute()
        return alg.getProperty('OutputWorkspace').value

    def _prepareWsForFitting(self, ws):
        """ Rebins the workspace and converts it to distribution
    	"""
        rebinAlg = self.createChildAlgorithm('Rebin')
        rebinAlg.setProperty('InputWorkspace', ws)
        rebinAlg.setProperty('Params', '-0.0005') # The value is borrowed from OG routines
        rebinAlg.execute()
        result = rebinAlg.getProperty('OutputWorkspace').value

        if result.isDistribution()==False:
            convertAlg = self.createChildAlgorithm('ConvertToDistribution')
            convertAlg.setProperty('Workspace', result)
            convertAlg.execute()

        return result

    def _createPositionsTable(self):
        """ Creates an empty table for storing detector positions
    	"""
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        table.addColumn('int', 'Detector ID')
        table.addColumn('V3D', 'Detector Position')

        return table

    def _fitPeaks(self, ws, wsIndex, expectedPeaksD):
        """
        Fits expected peaks to the spectrum, and returns calibrated zero and difc values.

        @param wsIndex workspace index of the spectrum to fit
        @param  expectedPeaksD :: expected peaks for the fitting, in d-spacing units

        @returns a pair of parameters: difc and zero
    	"""
        alg = self.createChildAlgorithm('EnginXFitPeaks')
        alg.setProperty('InputWorkspace', ws)
        alg.setProperty('WorkspaceIndex', wsIndex) # There should be only one index anyway
        alg.setProperty('ExpectedPeaks', self.getProperty('ExpectedPeaks').value)
        alg.execute()

        difc = alg.getProperty('Difc').value
        zero = alg.getProperty('Zero').value

        return (zero, difc)

    def _getCalibratedDetPos(self, newDifc, det, ws):
        """
        Returns a detector position which corresponds to the newDifc value.

        The two_theta and phi of the detector are preserved, L2 is changed.
    	"""
        # This is how detL2 would be calculated
        # detL2 = det.getDistance(ws.getInstrument().getSample())
        detTwoTheta = ws.detectorTwoTheta(det)
        detPhi = det.getPhi()

        newL2 = (newDifc / (252.816 * 2 * math.sin(detTwoTheta / 2.0))) - 50

        newPos = self._V3DFromSpherical(newL2, detTwoTheta, detPhi)

        return newPos

    def _V3DFromSpherical(self, R, polar, azimuth):
        """
        Returns a cartesian 3D vector for the given spherical coordinates.

        Borrowed from V3D::spherical (C++).
    	"""
        z = R*math.cos(polar)
        ct=R*math.sin(polar)
        x=ct*math.cos(azimuth)
        y=ct*math.sin(azimuth)

        return V3D(x,y,z)


AlgorithmFactory.subscribe(EnginXCalibrateFull)
