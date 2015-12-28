#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *
import math
import EnggUtils

class EnggCalibrateFull(PythonAlgorithm):
    INDICES_PROP_NAME = 'SpectrumNumbers'

    def category(self):
        return "Diffraction\\Engineering"

    def name(self):
        return "EnggCalibrateFull"

    def summary(self):
        return "Calibrates every detector/pixel position by performing single peak fitting."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("Workspace", "", Direction.InOut),
                             "Workspace with the calibration run to use. The calibration will be applied on it.")


        self.declareProperty(MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input,
                                                     PropertyMode.Optional),
                             "Workspace with the Vanadium (correction and calibration) run.")

        self.declareProperty(ITableWorkspaceProperty('VanIntegrationWorkspace', '',
                                                     Direction.Input, PropertyMode.Optional),
                             doc = 'Results of integrating the spectra of a Vanadium run, with one column '
                             '(integration result) and one row per spectrum. This can be used in '
                             'combination with OutVanadiumCurveFits from a previous execution and '
                             'VanadiumWorkspace to provide pre-calculated values for Vanadium correction.')

        self.declareProperty(MatrixWorkspaceProperty('VanCurvesWorkspace', '', Direction.Input,
                                                     PropertyMode.Optional),
                             doc = 'A workspace2D with the fitting workspaces corresponding to '
                             'the instrument banks. This workspace has three spectra per bank, as produced '
                             'by the algorithm Fit. This is meant to be used as an alternative input '
                             'VanadiumWorkspace for testing and performance reasons. If not given, no '
                             'workspace is generated.')

        vana_grp = 'Vanadium (open beam) properties'
        self.setPropertyGroup('VanadiumWorkspace', vana_grp)
        self.setPropertyGroup('VanIntegrationWorkspace', vana_grp)
        self.setPropertyGroup('VanCurvesWorkspace', vana_grp)

        self.declareProperty(ITableWorkspaceProperty("OutDetPosTable", "", Direction.Output),\
                             doc="A table with the detector IDs and calibrated detector positions and "
                             "additional calibration information. The table includes: the old positions "
                             "in V3D format (3D vector with x, y, z values), the new positions in V3D, the "
                             "new positions in spherical coordinates, the change in L2, and the DIFC and "
                             "ZERO parameters.")

        self.declareProperty("Bank", '', StringListValidator(EnggUtils.ENGINX_BANKS),
                             direction=Direction.Input,
                             doc = "Which bank to calibrate: It can be specified as 1 or 2, or "
                             "equivalently, North or South. See also " + self.INDICES_PROP_NAME + " "
                             "for a more flexible alternative to select specific detectors")

        self.declareProperty(self.INDICES_PROP_NAME, '', direction=Direction.Input,
                             doc = 'Sets the spectrum numbers for the detectors '
                             'that should be considered in the calibration (all others will be '
                             'ignored). This option cannot be used together with Bank, as they overlap. '
                             'You can give multiple ranges, for example: "0-99", or "0-9, 50-59, 100-109".')

        banks_grp = 'Banks / spectra'
        self.setPropertyGroup('Bank', banks_grp)
        self.setPropertyGroup(self.INDICES_PROP_NAME, banks_grp)

        self.declareProperty(FileProperty("OutDetPosFilename", "", FileAction.OptionalSave, [".csv"]),
                             doc="Name of the file to save the pre-/post-calibrated detector positions - this "
                             "saves the same information that is provided in the output table workspace "
                             "(OutDetPosTable).")

        opt_outs_grp = 'Optional outputs'
        self.setPropertyGroup('OutDetPosFilename', opt_outs_grp)

        self.declareProperty(FloatArrayProperty("ExpectedPeaks",
                                                values=EnggUtils.default_ceria_expected_peaks(),
                                                direction=Direction.Input),
                             doc="A list of dSpacing values where peaks are expected.")

        self.declareProperty(FileProperty(name="ExpectedPeaksFromFile",defaultValue="",
                                          action=FileAction.OptionalLoad,extensions = [".csv"]),
                             doc="Load from file a list of dSpacing values to be translated into TOF to "
                             "find expected peaks. This takes precedence over 'ExpectedPeaks' if both "
                             "options are given.")

        peaks_grp = 'Peaks to fit'
        self.setPropertyGroup('ExpectedPeaks', peaks_grp)
        self.setPropertyGroup('ExpectedPeaksFromFile', peaks_grp)


    def PyExec(self):

        # Get peaks in dSpacing from file, and check we have what we need, before doing anything
        expectedPeaksD = EnggUtils.read_in_expected_peaks(self.getPropertyValue("ExpectedPeaksFromFile"),
                                                          self.getProperty('ExpectedPeaks').value)

        if len(expectedPeaksD) < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        inWS = self.getProperty('Workspace').value
        WSIndices = EnggUtils.getWsIndicesFromInProperties(inWS, self.getProperty('Bank').value,
                                                           self.getProperty(self.INDICES_PROP_NAME).value)

        vanWS = self.getProperty("VanadiumWorkspace").value
        vanIntegWS = self.getProperty('VanIntegrationWorkspace').value
        vanCurvesWS = self.getProperty('VanCurvesWorkspace').value
        # These corrections rely on ToF<->Dspacing conversions, so ideally they'd be done after the
        # calibration step, which creates a cycle / chicken-and-egg issue.
        EnggUtils.applyVanadiumCorrections(self, inWS, WSIndices, vanWS, vanIntegWS, vanCurvesWS)

        rebinnedWS = self._prepareWsForFitting(inWS)
        posTbl = self._calculateCalibPositionsTbl(rebinnedWS, WSIndices, expectedPeaksD)

        # Produce 2 results: 'output table' and 'apply calibration' + (optional) calibration file
        self.setProperty("OutDetPosTable", posTbl)
        self._applyCalibrationTable(inWS, posTbl)
        self._outputDetPosFile(self.getPropertyValue('OutDetPosFilename'), posTbl)

    def _prepareWsForFitting(self, ws):
        """
        Rebins the workspace and converts it to distribution
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

    def _calculateCalibPositionsTbl(self, ws, indices, expectedPeaksD):
        """
        Makes a table of calibrated positions (and additional parameters). It goes through
        the detectors of the workspace and calculates the difc and zero parameters by fitting
        peaks, then calculates the coordinates and adds them in the returned table.

        @param ws :: input workspace with an instrument definition
        @param indices :: workspace indices to consider for calibration (for example all the
        indices of a bank)
        @param expectedPeaksD :: expected peaks in d-spacing

        @return table with the detector positions, one row per detector

        """
        posTbl = self._createPositionsTable()

        prog = Progress(self, start=0, end=1, nreports=len(indices))

        for i in indices:

            try:
                zero, difc = self._fitPeaks(ws, i, expectedPeaksD)
            except RuntimeError as re:
                raise RuntimeError("Severe issue found when trying to fit peaks for the detector with ID %d. "
                                   "This calibration algorithm cannot continue. Please check the expected "
                                   "peaks provided. Details from "
                                   "FindPeaks: %s"%(i, str(re)))

            det = ws.getDetector(i)
            newPos, newL2 = self._getCalibratedDetPos(difc, det, ws)

            # get old (pre-calibration) detector coordinates
            oldL2 = det.getDistance(ws.getInstrument().getSample())
            det2Theta = ws.detectorTwoTheta(det)
            detPhi = det.getPhi()
            oldPos = det.getPos()

            posTbl.addRow([det.getID(), oldPos, newPos, newL2, det2Theta, detPhi, newL2-oldL2,
                           difc, zero])
            prog.report()

        return posTbl

    def _fitPeaks(self, ws, wsIndex, expectedPeaksD):
        """
        Fits expected peaks to the spectrum, and returns calibrated zero and difc values.

        @param wsIndex workspace index of the spectrum to fit
        @param expectedPeaksD :: expected peaks for the fitting, in d-spacing units

        @returns a pair of parameters: difc and zero
    	"""
        alg = self.createChildAlgorithm('EnggFitPeaks')
        alg.setProperty('InputWorkspace', ws)
        alg.setProperty('WorkspaceIndex', wsIndex) # There should be only one index anyway
        alg.setProperty('ExpectedPeaks', expectedPeaksD)
        alg.execute()

        difc = alg.getProperty('Difc').value
        zero = alg.getProperty('Zero').value

        return (zero, difc)

    def _createPositionsTable(self):
        """
        Helper method to create an empty table for storing detector positions

        @return table with the expected output columns
        """
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        # Note: the colums 'Detector ID' and 'Detector Position' must have those
        # exact names (expected by child alg. ApplyCalibration in EnggFocus)
        table.addColumn('int', 'Detector ID')
        table.addColumn('V3D', 'Old Detector Position')
        table.addColumn('V3D', 'Detector Position')
        table.addColumn('float', 'L2')
        table.addColumn('float', '2 \\theta')
        table.addColumn('float', '\\phi')
        table.addColumn('float', '\\delta L2 (calibrated - old)')
        table.addColumn('float', 'difc')
        table.addColumn('float', 'zero')

        return table

    def _outputDetPosFile(self, filename, tbl):
        """
        Writes a text (csv) file with the detector positions information that also goes into the
        output DetectorPostions table workspace.

        @param filename :: name of the file to write. If it is empty nothing is saved/written.
        @param tbl :: detector positions table workspace
        """
        if not filename:
            return

        if 9 > tbl.columnCount():
            return

        self.log().information("Saving detector positions in file: %s" % filename)
        with open(filename, "w") as f:
            f.write('# detector ID, old position (x), old position (y), old position (z), '
                    'calibrated position (x), calibrated position (y), calibrated position (z), '
                    'calib. L2, calib. 2\\theta, calib. \\phi, delta L2 (calibrated - old), difc, zero\n')
            for r in range(0, tbl.rowCount()):
                f.write("%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s\n" %
                        (tbl.cell(r,0),
                         tbl.cell(r,1).getX(), tbl.cell(r,1).getY(), tbl.cell(r,1).getZ(),
                         tbl.cell(r,2).getX(), tbl.cell(r,2).getY(), tbl.cell(r,2).getZ(),
                         tbl.cell(r,3), tbl.cell(r,4), tbl.cell(r,5),
                         tbl.cell(r,6), tbl.cell(r,7), tbl.cell(r,8)))

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

        return (newPos, newL2)

    def _applyCalibrationTable(self, ws, detPos):
        """
        Corrects the detector positions using the result of calibration (if one is specified).
        The parameters of the instrument definition of the workspace are updated in place using the
        geometry parameters given in the input table.

        @param ws :: workspace to calibrate which should be consistent with the detector positions
        table passed

        @param detPos :: detector positions table to apply the calibration. It must have columns
        'Detector ID' and 'Detector Position'
        """
        self.log().notice("Applying calibration on the input workspace")
        alg = self.createChildAlgorithm('ApplyCalibration')
        alg.setProperty('Workspace', ws)
        alg.setProperty('PositionTable', detPos)
        alg.execute()

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


AlgorithmFactory.subscribe(EnggCalibrateFull)
