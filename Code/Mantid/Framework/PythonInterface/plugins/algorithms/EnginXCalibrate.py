#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *

class EnginXCalibrate(PythonAlgorithm):
    INDICES_PROP_NAME = 'SpectrumNumbers'

    def category(self):
        return "Diffraction\\Engineering;PythonAlgorithms"

    def name(self):
        return "EnginXCalibrate"

    def summary(self):
        return "Calibrates a detector bank by performing a single peak fitting."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),\
                             "Workspace with the calibration run to use.")

        self.declareProperty(FloatArrayProperty("ExpectedPeaks", ""),\
    		"A list of dSpacing values where peaks are expected.")

        self.declareProperty(FileProperty(name="ExpectedPeaksFromFile",defaultValue="",
                                          action=FileAction.OptionalLoad,extensions = [".csv"]),
                             "Load from file a list of dSpacing values to be translated into TOF to "
                             "find expected peaks. This takes precedence over 'ExpectedPeaks' if both "
                             "options are given.")

        import EnginXUtils
        self.declareProperty("Bank", '', StringListValidator(EnginXUtils.ENGINX_BANKS),
                             direction=Direction.Input,
                             doc = "Which bank to calibrate. It can be specified as 1 or 2, or "
                             "equivalently, North or South. See also " + self.INDICES_PROP_NAME + " "
                             "for a more flexible alternative to select specific detectors")

        self.declareProperty(self.INDICES_PROP_NAME, '', direction=Direction.Input,
                             doc = 'Sets the spectrum numbers for the detectors '
                             'that should be considered in the calibration (all others will be '
                             'ignored). This options cannot be used together with Bank, as they overlap. '
                             'You can give multiple ranges, for example: "0-99", or "0-9, 50-59, 100-109".')

        self.declareProperty(ITableWorkspaceProperty("DetectorPositions", "",\
                Direction.Input, PropertyMode.Optional),\
    		"Calibrated detector positions. If not specified, default ones are used.")

        self.declareProperty('OutputParametersTableName', '', direction=Direction.Input,
                             doc = 'Name for a table workspace with the calibration parameters calculated '
                             'from this algorithm: difc and zero parameters for GSAS. these two parameters '
                             'are added as two columns in a single row. If not given, no table is '
                             'generated.')

        self.declareProperty("Difc", 0.0, direction = Direction.Output,\
                             doc = "Calibrated Difc value for the bank or range of pixels/detectors given")

        self.declareProperty("Zero", 0.0, direction = Direction.Output,\
                             doc = "Calibrated Zero value for the bank or range of pixels/detectors given")

    def PyExec(self):

        import EnginXUtils

        focussed_ws = self._focusRun(self.getProperty('InputWorkspace').value,
                                     self.getProperty('Bank').value,
                                     self.getProperty(self.INDICES_PROP_NAME).value)

        # Get peaks in dSpacing from file
        expectedPeaksD = EnginXUtils.readInExpectedPeaks(self.getPropertyValue("ExpectedPeaksFromFile"),
                                                         self.getProperty('ExpectedPeaks').value)

        if len(expectedPeaksD) < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        difc, zero = self._fitParams(focussed_ws, expectedPeaksD)

        self._produceOutputs(difc, zero)

    def _fitParams(self, focusedWS, expectedPeaksD):
        """
        Fit the GSAS parameters that this algorithm produces: difc and zero

        @param focusedWS :: focused workspace to do the fitting on
        @param expectedPeaksD :: expected peaks for the fitting, in d-spacing units

        @returns a pair of parameters: difc and zero
        """

        fitPeaksAlg = self.createChildAlgorithm('EnginXFitPeaks')
        fitPeaksAlg.setProperty('InputWorkspace', focusedWS)
        fitPeaksAlg.setProperty('WorkspaceIndex', 0) # There should be only one index anyway
        fitPeaksAlg.setProperty('ExpectedPeaks', expectedPeaksD)
        # we could also pass raw 'ExpectedPeaks' and 'ExpectedPeaksFromFile' to
        # EnginXFitPEaks, but better to check inputs early, before this
        fitPeaksAlg.execute()

        difc = fitPeaksAlg.getProperty('Difc').value
        zero = fitPeaksAlg.getProperty('Zero').value

        return difc, zero

    def _focusRun(self, ws, bank, indices):
        """
        Focuses the input workspace by running EnginXFocus which will produce a single spectrum workspace.

        @param ws :: workspace to focus
        @param bank :: the focussing will be applied on the detectors of this bank
        @param indices :: list of indices to consider, as an alternative to bank (bank and indices are
        mutually exclusive)

        @return focussed (summed) workspace
        """
        alg = self.createChildAlgorithm('EnginXFocus')
        alg.setProperty('InputWorkspace', ws)
        alg.setProperty('Bank', bank)
        alg.setProperty(self.INDICES_PROP_NAME, indices)

        detPos = self.getProperty('DetectorPositions').value
        if detPos:
            alg.setProperty('DetectorPositions', detPos)

        alg.execute()

        return alg.getProperty('OutputWorkspace').value

    def _produceOutputs(self, difc, zero):
        """
        Just fills in the output properties as requested

        @param difc :: the difc GSAS parameter as fitted here
        @param zero :: the zero GSAS parameter as fitted here
        """

        import EnginXUtils

        self.setProperty('Difc', difc)
        self.setProperty('Zero', zero)

        # make output table if requested
        tblName = self.getPropertyValue("OutputParametersTableName")
        if '' != tblName:
            EnginXUtils.generateOutputParTable(tblName, difc, zero)
            self.log().information("Output parameters added into a table workspace: %s" % tblName)


AlgorithmFactory.subscribe(EnginXCalibrate)
