#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as sapi

import math

class EnginXFitPeaks(PythonAlgorithm):
    expectedDimType = 'Time-of-flight'

    def category(self):
        return "Diffraction\\Engineering;PythonAlgorithms"

    def name(self):
        return "EnginXFitPeaks"

    def summary(self):
        return ("The algorithm fits an expected diffraction pattern to a workpace spectrum by "
                "performing single peak fits.")

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),\
    		"Workspace to fit peaks in. ToF is expected X unit.")

        self.declareProperty("WorkspaceIndex", 0,\
    		"Index of the spectra to fit peaks in")

        self.declareProperty(FloatArrayProperty("ExpectedPeaks", (self._getDefaultPeaks())),\
    		"A list of dSpacing values to be translated into TOF to find expected peaks.")

        self.declareProperty(FileProperty(name="ExpectedPeaksFromFile",defaultValue="",
                                          action=FileAction.OptionalLoad,extensions = [".csv"]),
                             "Load from file a list of dSpacing values to be translated into TOF to "
                             "find expected peaks.")


        self.declareProperty('OutputParametersTableName', '', direction=Direction.Input,
                             doc = 'Name for a table workspace with the fitted values calculated by '
                             'this algorithm (Difc and Zero parameters) for GSAS. '
                             'These two parameters are added as two columns in a single row. If not given, '
                             'the table workspace is not created.')

        self.declareProperty("Difc", 0.0, direction = Direction.Output,\
    		doc = "Fitted Difc value")

        self.declareProperty("Zero", 0.0, direction = Direction.Output,\
    		doc = "Fitted Zero value")

    def PyExec(self):

        import EnginXUtils

        # Get peaks in dSpacing from file
        expectedPeaksD = EnginXUtils.readInExpectedPeaks(self.getPropertyValue("ExpectedPeaksFromFile"),
                                                         self.getProperty('ExpectedPeaks').value)

        if expectedPeaksD < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        # Get expected peaks in TOF for the detector
        inWS = self.getProperty("InputWorkspace").value
        dimType = inWS.getXDimension().getName()
        if self.expectedDimType != dimType:
            raise ValueError("This algorithm expects a workspace with %s X dimension, but "
                             "the X dimension of the input workspace is: '%s'" % (self.expectedDimType, dimType))

        wsIndex = self.getProperty("WorkspaceIndex").value

        # FindPeaks will return a list of peaks sorted by the centre found. Sort the peaks as well,
        # so we can match them with fitted centres later.
        expectedPeaksToF = sorted(self._expectedPeaksInTOF(expectedPeaksD, inWS, wsIndex))

        foundPeaks = self._peaksFromFindPeaks(inWS, expectedPeaksToF, wsIndex)
        if foundPeaks.rowCount() < len(expectedPeaksToF):
            txt = "Peaks effectively found: " + str(foundPeaks)[1:-1]
            raise RuntimeError("Some peaks from the list of expected peaks were not found by the algorithm "
                               "FindPeaks. " + txt)

        difc, zero = self._fitAllPeaks(inWS, wsIndex, foundPeaks, expectedPeaksD)

        self._produceOutputs(difc, zero)

    def _getDefaultPeaks(self):
        """ Gets default peaks for EnginX algorithm. Values from CeO2 """
        defaultPeaks = [3.1243, 2.7057, 1.9132, 1.6316, 1.5621, 1.3529, 1.2415,
                       1.2100, 1.1046, 1.0414, 0.9566, 0.9147, 0.9019, 0.8556,
                       0.8252, 0.8158, 0.7811]

        return defaultPeaks

    def _produceOutputs(self, difc, zero):
        """
        Fills in the output properties as requested via the input properties.

        @param difc :: the difc GSAS parameter as fitted here
        @param zero :: the zero GSAS parameter as fitted here
        """

        import EnginXUtils

        # mandatory outputs
        self.setProperty('Difc', difc)
        self.setProperty('Zero', zero)

        # optional outputs
        tblName = self.getPropertyValue("OutputParametersTableName")
        if '' != tblName:
            EnginXUtils.generateOutputParTable(tblName, difc, zero)
            self.log().information("Output parameters added into a table workspace: %s" % tblName)

    def _fitAllPeaks(self, inWS, wsIndex, foundPeaks, expectedPeaksD):
        """
        This method is the core of EnginXFitPeaks. Ittries to fit as many peaks as there are in the list of
        expected peaks passed to the algorithm.

        The parameters from the (Gaussian) peaks fitted by FindPeaks elsewhere (before calling this method)
        are used as initial guesses.

        @param inWS :: input workspace with spectra for fitting
        @param wsIndex :: workspace index of the spectrum where the given peaks should be fitted
        @param foundPeaks :: list of peaks found by FindPeaks or similar algorithm
        @param expectedPeaksD :: list of expected peaks provided as input to this algorithm (in dSpacing units)

        @returns difc and zero parameters obtained from fitting a
        linear background (in _fitDSpacingToTOF) to the peaks fitted
        here individually

        """
        fittedPeaks = self._createFittedPeaksTable()

        peakType = 'BackToBackExponential'
        for i in range(foundPeaks.rowCount()):

            row = foundPeaks.row(i)
            # Peak parameters estimated by FindPeaks
            centre = row['centre']
            width = row['width']
            height = row['height']
            # Oh oh, this actually happens sometimes for some spectra of the system test dataset
            # and it should be clarified when the role of FindPeaks etc. is fixed (trac ticket #10907)
            if width <= 0.:
                continue

            # Sigma value of the peak, assuming Gaussian shape
            sigma = width / (2 * math.sqrt(2 * math.log(2)))

            # Approximate peak intensity, assuming Gaussian shape
            intensity = height * sigma * math.sqrt(2 * math.pi)

            peak = FunctionFactory.createFunction(peakType)
            peak.setParameter('X0', centre)
            peak.setParameter('S', sigma)
            peak.setParameter('I', intensity)

            # Magic numbers
            COEF_LEFT = 2
            COEF_RIGHT = 3

            # Try to predict a fit window for the peak
            xMin = centre - (width * COEF_LEFT)
            xMax = centre + (width * COEF_RIGHT)

            # Fit using predicted window and a proper function with approximated initital values
            fitAlg = self.createChildAlgorithm('Fit')
            fitAlg.setProperty('Function', 'name=LinearBackground;' + str(peak))
            fitAlg.setProperty('InputWorkspace', inWS)
            fitAlg.setProperty('WorkspaceIndex', wsIndex)
            fitAlg.setProperty('StartX', xMin)
            fitAlg.setProperty('EndX', xMax)
            fitAlg.setProperty('CreateOutput', True)
            fitAlg.execute()
            paramTable = fitAlg.getProperty('OutputParameters').value

            fittedParams = {}
            fittedParams['dSpacing'] = expectedPeaksD[i]
            fittedParams['Chi'] = fitAlg.getProperty('OutputChi2overDoF').value
            self._addParametersToMap(fittedParams, paramTable)

            fittedPeaks.addRow(fittedParams)

        # Check if we were able to really fit any peak
        if 0 == fittedPeaks.rowCount():
            detailTxt = ("Could find " + str(len(foundPeaks)) + " peaks using the algorithm FindPeaks but " +
                         "then it was not possible to fit any peak starting from these peaks found and using '" +
                         peakType + "' as peak function.")
            raise RuntimeError('Could not fit any peak. Please check the list of expected peaks, as it does not '
                               'seem to be appropriate for the workspace given. More details: ' +
                               detailTxt)

        # Better than failing to fit the linear function
        if 1 == fittedPeaks.rowCount():
            raise RuntimeError('Could find only one peak. This is not enough to fit the output parameters '
                               'difc and zero. Please check the list of expected peaks given and if it is '
                               'appropriate for the workspace')

        difc, zero = self._fitDSpacingToTOF(fittedPeaks)
        return (difc, zero)

    def _peaksFromFindPeaks(self, inWS, expectedPeaksToF, wsIndex):
        """
        Use the algorithm FindPeaks to check that the expected peaks are there.

        @param inWS data workspace
        @param expectedPeaksToF vector/list of expected peak values
        @param wsIndex workspace index

        @return list of peaks found by FindPeaks. If there are no issues, the length
        of this list should be the same as the number of expected peaks received.
        """
        # Find approximate peak positions, asumming Gaussian shapes
        findPeaksAlg = self.createChildAlgorithm('FindPeaks')
        findPeaksAlg.setProperty('InputWorkspace', inWS)
        findPeaksAlg.setProperty('PeakPositions', expectedPeaksToF)
        findPeaksAlg.setProperty('PeakFunction', 'Gaussian')
        findPeaksAlg.setProperty('WorkspaceIndex', wsIndex)
        findPeaksAlg.execute()
        foundPeaks = findPeaksAlg.getProperty('PeaksList').value
        return foundPeaks

    def _fitDSpacingToTOF(self, fittedPeaksTable):
        """ Fits a linear background to the dSpacing <-> TOF relationship and returns fitted difc
    		and zero values.
    	"""
        convertTableAlg = self.createChildAlgorithm('ConvertTableToMatrixWorkspace')
        convertTableAlg.setProperty('InputWorkspace', fittedPeaksTable)
        convertTableAlg.setProperty('ColumnX', 'dSpacing')
        convertTableAlg.setProperty('ColumnY', 'X0')
        convertTableAlg.execute()
        dSpacingVsTof = convertTableAlg.getProperty('OutputWorkspace').value

    	# Fit the curve to get linear coefficients of TOF <-> dSpacing relationship for the detector
        fitAlg = self.createChildAlgorithm('Fit')
        fitAlg.setProperty('Function', 'name=LinearBackground')
        fitAlg.setProperty('InputWorkspace', dSpacingVsTof)
        fitAlg.setProperty('WorkspaceIndex', 0)
        fitAlg.setProperty('CreateOutput', True)
        fitAlg.execute()
        paramTable = fitAlg.getProperty('OutputParameters').value

        zero = paramTable.cell('Value', 0) # A0
        difc = paramTable.cell('Value', 1) # A1

        return (difc, zero)

    def _expectedPeaksInTOF(self, expectedPeaks, inWS, wsIndex):
        """
        Converts expected peak dSpacing values to TOF values for the
        detector. Implemented by using the Mantid algorithm ConvertUnits. A
        simple user script to do what this function does would be
        as follows:

        yVals = [1] * (len(expectedPeaks) - 1)
        wsFrom = sapi.CreateWorkspace(UnitX='dSpacing', DataX=expectedPeaks, DataY=yVals,
                                      ParentWorkspace=inWS)
        targetUnits = 'TOF'
        wsTo = sapi.ConvertUnits(InputWorkspace=wsFrom, Target=targetUnits)
        peaksToF = wsTo.dataX(0)
        values = [peaksToF[i] for i in range(0,len(peaksToF))]

        @param expectedPeaks :: vector of expected peaks, in dSpacing units
        @param inWS :: input workspace with the relevant instrument/geometry
        @param wsIndex spectrum index

        Returns:
            a vector of ToF values converted from the input (dSpacing) vector.

        """

        # When receiving a (for example) focussed workspace we still do not know how
        # to properly deal with it. CreateWorkspace won't copy the instrument sample
        # and source even if given the option ParentWorkspace. Resort to old style
        # hard-coded calculation.
        # The present behavior of 'ConvertUnits' is to show an information log:
        # "Unable to calculate sample-detector distance for 1 spectra. Masking spectrum"
        # and silently produce a wrong output workspace. That might need revisiting.
        if 1 == inWS.getNumberHistograms():
            return self._doApproxHardCodedConvertUnitsToToF(expectedPeaks, inWS, wsIndex)

        # Create workspace just to convert dSpacing -> ToF, yVals are irrelevant
        # this used to be calculated with:
        # lambda d: 252.816 * 2 * (50 + detL2) * math.sin(detTwoTheta / 2.0) * d
        # which is approximately what ConverUnits will do
        # remember the -1, we must produce a histogram data workspace, which is what
        # for example EnginXCalibrate expects
        yVals = [1] * (len(expectedPeaks) - 1)
        # Do like: wsFrom = sapi.CreateWorkspace(UnitX='dSpacing', DataX=expectedPeaks, DataY=yVals,
        #                                        ParentWorkspace=self.getProperty("InputWorkspace").value)
        createAlg = self.createChildAlgorithm("CreateWorkspace")
        createAlg.setProperty("UnitX", 'dSpacing')
        createAlg.setProperty("DataX", expectedPeaks)
        createAlg.setProperty("DataY", yVals)
        createAlg.setProperty("ParentWorkspace", inWS)
        createAlg.execute()

        wsFrom = createAlg.getProperty("OutputWorkspace").value

        # finally convert units, like: sapi.ConvertUnits(InputWorkspace=wsFrom, Target=targetUnits)
        convAlg = self.createChildAlgorithm("ConvertUnits")
        convAlg.setProperty("InputWorkspace", wsFrom)
        targetUnits = 'TOF'
        convAlg.setProperty("Target", targetUnits)
        # note: this implicitly uses default property "EMode" value 'Elastic'
        goodExec = convAlg.execute()

        if not goodExec:
            raise RuntimeError("Conversion of units went wrong. Failed to run ConvertUnits for %d peaks: %s"
                               % (len(expectedPeaks), expectedPeaks))

        wsTo = convAlg.getProperty('OutputWorkspace').value
        peaksToF = wsTo.readX(0)
        if len(peaksToF) != len(expectedPeaks):
            raise RuntimeError("Conversion of units went wrong. Converted %d peaks from the original "
                               "list of %d peaks. The instrument definition might be incomplete for the "
                               "original workspace / file."% (len(peaksToF), len(expectedPeaks)))

        ToFvalues = [peaksToF[i] for i in range(0,len(peaksToF))]
        # catch potential failures because of geometry issues, etc.
        if ToFvalues == expectedPeaks:
            vals = self._doApproxHardCodedConvertUnitsToToF(expectedPeaks, inWS, wsIndex)
            return vals

        return ToFvalues

    def _doApproxHardCodedConvertUnitsToToF(self, dspValues, ws, wsIndex):
        """
        Converts from dSpacing to Time-of-flight, for one spectrum/detector. This method
        is here for exceptional cases that presently need clarification / further work,
        here and elsewhere in Mantid, and should ideally be removed in favor of the more
        general method that uses the algorithm ConvertUnits.

        @param dspValues to convert from dSpacing
        @param ws workspace with the appropriate instrument / geometry definition
        @param wsIndex index of the spectrum

        Return:
        input values converted from dSpacing to ToF
        """
        det = ws.getDetector(wsIndex)

        # Current detector parameters
        detL2 = det.getDistance(ws.getInstrument().getSample())
        detTwoTheta = ws.detectorTwoTheta(det)

        # hard coded equation to convert dSpacing -> TOF for the single detector
        dSpacingToTof = lambda d: 252.816 * 2 * (50 + detL2) * math.sin(detTwoTheta / 2.0) * d

        # Values (in principle, expected peak positions) in TOF for the detector
        ToFvalues = [dSpacingToTof(ep) for ep in dspValues]
        return ToFvalues

    def _createFittedPeaksTable(self):
        """ Creates a table where to put peak fitting results to
    	"""
        alg = self.createChildAlgorithm('CreateEmptyTableWorkspace')
        alg.execute()
        table = alg.getProperty('OutputWorkspace').value

        table.addColumn('double', 'dSpacing')

        for name in ['A0', 'A1', 'X0', 'A', 'B', 'S', 'I']:
            table.addColumn('double', name)
            table.addColumn('double', name + '_Err')

        table.addColumn('double', 'Chi')

        return table

    def _addParametersToMap(self, paramMap, paramTable):
        """ Reads parameters from the Fit Parameter table, and add their values and errors to the map
        """
        for i in range(paramTable.rowCount() - 1): # Skip the last (fit goodness) row
            row = paramTable.row(i)

            # Get local func. param name. E.g., not f1.A0, but just A0
            name = (row['Name'].rpartition('.'))[2]

            paramMap[name] = row['Value']
            paramMap[name + '_Err'] = row['Error']


AlgorithmFactory.subscribe(EnginXFitPeaks)
