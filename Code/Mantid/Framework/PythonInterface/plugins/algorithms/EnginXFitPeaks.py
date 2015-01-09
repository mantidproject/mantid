from mantid.kernel import *
from mantid.api import *

import math
import numpy as np

class EnginXFitPeaks(PythonAlgorithm):
    def category(self):
    	return "Diffraction\Engineering;PythonAlgorithms"

    def name(self):
    	return "EnginXFitPeaks"

    def summary(self):
    	return "The algorithm fits an expected diffraction pattern to a workpace spectrum by performing single peak fits."

    def PyInit(self):
    	self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input),
    		"Workspace to fit peaks in. ToF is expected X unit.")

    	self.declareProperty("WorkspaceIndex", 0,
    		"Index of the spectra to fit peaks in")

    	self.declareProperty(FloatArrayProperty("ExpectedPeaks", [0.6, 1.9]),
    		"A list of dSpacing values to be translated into TOF to find expected peaks.")
    	
    	self.declareProperty(FileProperty(name="ExpectedPeaksFromFile",defaultValue="",action=FileAction.OptionalLoad,extensions = [".csv"]),"Load from file a list of dSpacing values to be translated into TOF to find expected peaks.")

    	self.declareProperty("Difc", 0.0, direction = Direction.Output,
    		doc = "Fitted Difc value")

    	self.declareProperty("Zero", 0.0, direction = Direction.Output,
    		doc = "Fitted Zero value")

    def PyExec(self):
    	# Get expected peaks in TOF for the detector
    	expectedPeaksTof = self._expectedPeaksInTOF()
      
    	# FindPeaks will returned a list of peaks sorted by the centre found. Sort the peaks as well,
    	# so we can match them with fitted centres later.
    	expectedPeaksTof = sorted(expectedPeaksTof)
    	expectedPeaksD = sorted(self.getProperty('ExpectedPeaks').value)

    	# Find approximate peak positions, asumming Gaussian shapes
    	findPeaksAlg = self.createChildAlgorithm('FindPeaks')
    	findPeaksAlg.setProperty('InputWorkspace', self.getProperty("InputWorkspace").value)
    	findPeaksAlg.setProperty('PeakPositions', expectedPeaksTof)
    	findPeaksAlg.setProperty('PeakFunction', 'Gaussian')
    	findPeaksAlg.setProperty('WorkspaceIndex', self.getProperty("WorkspaceIndex").value)
    	findPeaksAlg.execute()
    	foundPeaks = findPeaksAlg.getProperty('PeaksList').value

    	if (foundPeaks.rowCount() < len(expectedPeaksTof)):
    		raise Exception("Some peaks were not found")

    	fittedPeaks = self._createFittedPeaksTable()

    	for i in range(foundPeaks.rowCount()):

    		row = foundPeaks.row(i)

    		# Peak parameters estimated by FindPeaks
    		centre = row['centre']
    		width = row['width']
    		height = row['height']

    		# Sigma value of the peak, assuming Gaussian shape
    		sigma = width / (2 * math.sqrt(2 * math.log(2)))

    		# Approximate peak intensity, assuming Gaussian shape
    		intensity = height * sigma * math.sqrt(2 * math.pi)

    		peak = FunctionFactory.createFunction("BackToBackExponential")
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

    	(difc, zero) = self._fitDSpacingToTOF(fittedPeaks)

    	self.setProperty('Difc', difc)
    	self.setProperty('Zero', zero)

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

    	expectedPeaks = self.getProperty("ExpectedPeaks").value

    	# Expected peak positions in TOF for the detector
    	expectedPeaksTof = map(dSpacingToTof, expectedPeaks)

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