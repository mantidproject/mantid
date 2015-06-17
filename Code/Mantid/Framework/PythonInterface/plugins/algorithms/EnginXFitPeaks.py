#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *
import mantid.simpleapi as sapi

import math

class EnginXFitPeaks(PythonAlgorithm):
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
    	# Get expected peaks in TOF for the detector
        expectedPeaksTof = self._expectedPeaksInTOF()
        # FindPeaks will returned a list of peaks sorted by the centre found. Sort the peaks as well,
        # so we can match them with fitted centres later.
        expectedPeaksTof = sorted(expectedPeaksTof)
        expectedPeaksD = self._readInExpectedPeaks()

        # Find approximate peak positions, asumming Gaussian shapes
        findPeaksAlg = self.createChildAlgorithm('FindPeaks')
        findPeaksAlg.setProperty('InputWorkspace', self.getProperty("InputWorkspace").value)
        findPeaksAlg.setProperty('PeakPositions', expectedPeaksTof)
        findPeaksAlg.setProperty('PeakFunction', 'Gaussian')
        findPeaksAlg.setProperty('WorkspaceIndex', self.getProperty("WorkspaceIndex").value)
        findPeaksAlg.execute()
        foundPeaks = findPeaksAlg.getProperty('PeaksList').value

        if foundPeaks.rowCount() < len(expectedPeaksTof):
            txt = "Peaks effectively found: " + str(foundPeaks)[1:-1]
            raise RuntimeError("Some peaks from the list of expected peaks were not found by the algorithm "
                               "FindPeaks. " + txt)

        difc, zero = self._fitAllPeaks(foundPeaks, expectedPeaksD)

        self._produceOutputs(difc, zero)

    def _readInExpectedPeaks(self):
        """ Reads in expected peaks from the .csv file """
        readInArray = []
        exPeakArray = []
        updateFileName = self.getPropertyValue("ExpectedPeaksFromFile")
        if updateFileName != "":
            with open(updateFileName) as f:
                for line in f:
                    readInArray.append([float(x) for x in line.split(',')])
            for a in readInArray:
                for b in a:
                    exPeakArray.append(b)
            if exPeakArray == []:
                print "File could not be read. Defaults being used."
                expectedPeaksD = sorted(self.getProperty('ExpectedPeaks').value)
            else:
                print "using file"
                expectedPeaksD = sorted(exPeakArray)
        else:
            expectedPeaksD = sorted(self.getProperty('ExpectedPeaks').value)
        return expectedPeaksD

    def _getDefaultPeaks(self):
        """ Gets default peaks for EnginX algorithm. Values from CeO2 """
        defaultPeak = [3.1243, 2.7057, 1.9132, 1.6316, 1.5621, 1.3529, 1.2415,
                       1.2100, 1.1046, 1.0414, 0.9566, 0.9147, 0.9019, 0.8556,
                       0.8252, 0.8158, 0.7811]
        return defaultPeak

    def _produceOutputs(self, difc, zero):
        """
        Fills in the output properties as requested. NOTE: this method and the methods that this calls
        might/should be merged with similar methods in other EnginX algorithms (like EnginXCalibrate)
        and possibly moved into EnginXUtils.py. That depends on how the EnginX algorithms evolve overall.
        In principle, when EnginXCalibrate is updated with a similar 'OutputParametersTableName' optional
        output property, it should use the 'OutputParametersTableName' of this (EnginXFitPeaks) algorithm.

        @param difc :: the difc GSAS parameter as fitted here
        @param zero :: the zero GSAS parameter as fitted here
        """
        # mandatory outputs
        self.setProperty('Difc', difc)
        self.setProperty('Zero', zero)

        # optional outputs
        tblName = self.getPropertyValue("OutputParametersTableName")
        if '' != tblName:
            self._generateOutputParFitTable(tblName)

    def _generateOutputParFitTable(self, name):
        """
        Produces a table workspace with the two fitted parameters
        @param name :: the name to use for the table workspace that is created here
        """
        tbl = sapi.CreateEmptyTableWorkspace(OutputWorkspace=name)
        tbl.addColumn('double', 'difc')
        tbl.addColumn('double', 'zero')
        tbl.addRow([float(self.getPropertyValue('difc')), float(self.getPropertyValue('zero'))])

        self.log().information("Output parameters added into a table workspace: %s" % name)

    def _fitAllPeaks(self, foundPeaks, expectedPeaksD):
        """
        This tries to fit as many peaks as there are in the list of expected peaks passed to the algorithm.
        The parameters from the (Gaussian) peaks fitted by FindPeaks elsewhere (before calling this method)
        are used as initial guesses.

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
            fitAlg.setProperty('InputWorkspace', self.getProperty("InputWorkspace").value)
            fitAlg.setProperty('WorkspaceIndex', self.getProperty("WorkspaceIndex").value)
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


    def _expectedPeaksInTOF(self):
        """ Converts expected peak dSpacing values to TOF values for the detector
    	"""
        ws = self.getProperty("InputWorkspace").value
        wsIndex = self.getProperty("WorkspaceIndex").value

    	# Detector for specified spectrum
        det = ws.getDetector(wsIndex)

    	# Current detector parameters
        detL2 = det.getDistance(ws.getInstrument().getSample())
        detTwoTheta = ws.detectorTwoTheta(det)

    	# Function for converting dSpacing -> TOF for the detector
        dSpacingToTof = lambda d: 252.816 * 2 * (50 + detL2) * math.sin(detTwoTheta / 2.0) * d
        expectedPeaks = self._readInExpectedPeaks()

    	# Expected peak positions in TOF for the detector
        expectedPeaksTof = [dSpacingToTof(ep) for ep in expectedPeaks]

        return expectedPeaksTof


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
