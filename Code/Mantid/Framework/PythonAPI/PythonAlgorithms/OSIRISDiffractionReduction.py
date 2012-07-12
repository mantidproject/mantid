"""*WIKI* 
== Usage ==

== Source Code ==
The source code for the Python Algorithm may be viewed at: [http://trac.mantidproject.org/mantid/browser/trunk/Code/Mantid/Framework/PythonAPI/PythonAlgorithms/OSIRISDiffractionReduction.py OSIRISDiffractionReduction.py]

The source code for the reducer class which is used may be viewed at: [http://trac.mantidproject.org/mantid/browser/trunk/Code/Mantid/scripts/Inelastic/osiris_diffraction_reducer.py osiris_diffraction_reducer.py]

*WIKI*"""

from MantidFramework import *
from mantidsimple import *

import re
import itertools
from types import NoneType

timeRegimeToDRange = {
     1.17e4: tuple([ 0.7,  2.5]),
     2.94e4: tuple([ 2.1,  3.3]),
     4.71e4: tuple([ 3.1,  4.3]),
     6.48e4: tuple([ 4.1,  5.3]),
     8.25e4: tuple([ 5.2,  6.2]),
    10.02e4: tuple([ 6.2,  7.3]),
    11.79e4: tuple([ 7.3,  8.3]),
    13.55e4: tuple([ 8.3,  9.5]),
    15.32e4: tuple([ 9.4, 10.6]),
    17.09e4: tuple([10.4, 11.6]),
    18.86e4: tuple([11.0, 12.5])}

""" A "wrapper" class for a map, which maps workspaces from their corresponding
time regimes.
"""
class DRangeToWsMap(object):
	
	def __init__(self):
		self._map = {}
	
	def addWs(self, ws):
		""" Takes in the given workspace and lists it alongside its time regime value.
		If the time regime has yet to be created, it will create it, and if there is already
		a workspace listed beside the time regime, then the new ws will be appended
		to that list.
		"""
		# Get the time regime of the workspace, and use it to find the DRange.
		timeRegime = ws.dataX(0)[0]
		try:
			dRange = timeRegimeToDRange[timeRegime]
		except KeyError:
			mtd.sendErrorMessage("Unable to identify the DRange of " + ws.getName() + 
				", which has a time regime of " + str(timeRegime))
			sys.exit()
		
		# Add the workspace to the map, alongside its DRange.
		if dRange in self._map:
			self._map[dRange].append(ws)
		else:
			self._map[dRange] = [ws]
			
	def setItem(self, dRange, ws):
		""" Set a dRange and corresponding *single* ws.
		"""
		self._map[dRange] = ws
		
	def getMap(self):
		""" Get access to wrapped map.
		"""
		return self._map
		
def averageWsList(wsList):
	""" Returns the average of a list of workspaces.
	"""
	# Assert we have some ws in the list, and if there is only one then return it.
	assert len(wsList) > 0, "getAverageWs: Trying to take an average of nothing."
	if len(wsList) == 1:
		return wsList[0]
		
	# Generate the final name of the averaged workspace.
	avName = "avg"
	for ws in wsList:
		avName += "_" + ws.getName()
	
	# Compute the average and put into "__temp_avg".
	__temp_avg = wsList[0] + wsList[1]
	for i in range(2, len(wsList) ):
		__temp_avg += wsList[i]
	__temp_avg/= len(wsList)
	
	# Delete the old workspaces that are now included in the average.
	for ws in wsList:
		DeleteWorkspace(Workspace=ws.getName())
		
	# Rename the average ws and return it.
	RenameWorkspace(InputWorkspace=__temp_avg, OutputWorkspace=avName)
	return mtd[avName]

def findIntersectionOfTwoRanges(rangeA, rangeB):
	
	assert rangeA[0] < rangeA[1], "Malformed range."
	assert rangeB[0] < rangeB[1], "Malformed range."
	
	if( rangeA[0] <= rangeA[1] <= rangeB[0] <= rangeB[1]):
		return
	if( rangeB[0] <= rangeB[1] <= rangeA[0] <= rangeA[1]):
		return
	if( rangeA[0] <= rangeB[0] <= rangeB[1] <= rangeA[1] ):
		return rangeB
	if( rangeB[0] <= rangeA[0] <= rangeA[1] <= rangeB[1] ):
		return rangeA
	if( rangeA[0] <= rangeB[0] <= rangeA[1] <= rangeB[1] ):
		return [rangeB[0], rangeA[1]]
	if( rangeB[0] <= rangeA[0] <= rangeB[1] <= rangeA[1] ):
		return [rangeA[0], rangeB[1]]
	
	assert False, "We should never reach here ..."

def getIntersectionsOfRanges(rangeList):
	""" Get the intersections of a list of ranges.  For example, given the ranges:
	[1, 3], [3, 5] and [4, 6], the intersections would be a single range of [4, 5].
	
	NOTE: Assumes that no more than a maximum of two ranges will ever cross 
	at the same point.  Also, all ranges should obey range[0] <= range[1].
	"""
	# Sanity check.
	for range in rangeList:
		assert len(range) == 2, "Unable to find the intersection of a malformed range."
	
	# Find all combinations of ranges, and see where they intersect.
	rangeCombos = list(itertools.combinations(rangeList, 2))
	intersections = []
	for rangePair in rangeCombos:
		intersection = findIntersectionOfTwoRanges(rangePair[0], rangePair[1])
		if type(intersection) is not types.NoneType:
			intersections.append(intersection)
	
	# Return the sorted intersections.
	intersections.sort()
	return intersections
	
def isInRanges(rangeList, n):
	for range in rangeList:
		if range[0] < n < range[1]:
			return True
	return False

class OSIRISDiffractionReduction(PythonAlgorithm):
	""" Handles the reduction of OSIRIS Diffraction Data.
	"""
	
	def PyInit(self):
		self.declareProperty('Sample', '')
		self.declareProperty('Vanadium', '')
		self.declareProperty('CalFile', '')
		self.declareWorkspaceProperty('OutputWorkspace', '', Direction.Output)
		
		self._sams = []
		self._vans = []
		self._cal = None
		self._outputWsName = None
		
		self._samMap = DRangeToWsMap()
		self._vanMap = DRangeToWsMap()
	
	def PyExec(self):
		# Set OSIRIS as default instrument.
		mtd.settings["default.instrument"] = 'OSIRIS'
		
		# Set all algo inputs to local vars.  Some validation/parsing via FileFinder,
        # which is helpful since this is an algorithm that could be called outside of
        # of the Indirect Diffraction interface.
		self._outputWsName = self.getPropertyValue("OutputWorkspace")
		for sam in re.compile(r',').split(self.getProperty("Sample")):
			try:
				val = FileFinder.findRuns(sam)[0]
			except RuntimeError:
				mtd.sendErrorMessage("Could not locate sample file: " + sam)
				sys.exit()
			self._sams.append(val)
		for van in re.compile(r',').split(self.getProperty("Vanadium")):
			try:
				val = FileFinder.findRuns(van)[0]
			except RuntimeError:
				mtd.sendErrorMessage("Could not locate vanadium file: " + van)
				sys.exit()
			self._vans.append(val)
		self._cal = self.getProperty("CalFile")
		
		# Load all sample and vanadium files, and add the resulting workspaces to the DRangeToWsMaps.
		for file in self._sams + self._vans:
			Load(Filename=file, OutputWorkspace=file, SpectrumMin=3, SpectrumMax=962)
		for sam in self._sams:
			self._samMap.addWs(mtd[sam])	
		for van in self._vans:
			self._vanMap.addWs(mtd[van])
		
		# Check to make sure that there are corresponding vanadium files with the same DRange for each sample file.
		for dRange in self._samMap.getMap().iterkeys():
			if dRange not in self._vanMap.getMap():
				mtd.sendErrorMessage("There is no van file that covers the " + str(dRange) + " DRange.")
				sys.exit()
		
		# Average together any sample workspaces with the same DRange.  This will mean our map of DRanges
		# to list of workspaces becomes a map of DRanges, each to a *single* workspace.
		tempSamMap = DRangeToWsMap()
		for dRange, wsList in self._samMap.getMap().iteritems():
			tempSamMap.setItem(dRange, averageWsList(wsList))
		self._samMap = tempSamMap
		
		# Now do the same to the vanadium workspaces.
		tempVanMap = DRangeToWsMap()
		for dRange, wsList in self._vanMap.getMap().iteritems():
			tempVanMap.setItem(dRange, averageWsList(wsList))
		self._vanMap = tempVanMap
		
		# Run necessary algorithms on BOTH the Vanadium and Sample workspaces.
		for dRange, ws in self._samMap.getMap().items() + self._vanMap.getMap().items():
			NormaliseByCurrent(ws, ws)
			AlignDetectors(ws, ws, self._cal)
			DiffractionFocussing(ws, ws, self._cal)
			CropWorkspace(ws, ws, XMin=dRange[0], XMax=dRange[1])
		
		# Divide all sample files by the corresponding vanadium files.
		for dRange in self._samMap.getMap().iterkeys():
			samWs = self._samMap.getMap()[dRange]
			vanWs = self._vanMap.getMap()[dRange]
			Divide(samWs, vanWs, samWs)
			ReplaceSpecialValues(samWs, samWs, NaNValue=0.0, InfinityValue=0.0)
			
		# Create a list of sample workspace NAMES, since we need this for MergeRuns.
		samWsNamesList = []
		for sam in self._samMap.getMap().itervalues():
			samWsNamesList.append(sam.getName())
		
		# Merge the sample files into one.
		MergeRuns(','.join(samWsNamesList), self._outputWsName)
		result = mtd[self._outputWsName]
		
		# Create scalar data to cope with where merge has combined overlapping data.
		intersections = getIntersectionsOfRanges(self._samMap.getMap().keys())
		dataX = result.dataX(0)
		dataY = []; dataE = []
		for i in range(0, len(dataX)-1):
			x = ( dataX[i] + dataX[i+1] ) / 2.0
			if isInRanges(intersections, x):
				dataY.append(2); dataE.append(2)
			else:
				dataY.append(1); dataE.append(1)
				
		# Create scalar from data and use to scale result.
		CreateWorkspace(OutputWorkspace="scaling", DataX=dataX, DataY=dataY, DataE=dataE, UnitX="dSpacing")
		scalar = mtd["scaling"]
		Divide(result, scalar, result)
		
		# Delete all workspaces we've created, except the result.
		for ws in self._vanMap.getMap().values() + self._samMap.getMap().values() + [scalar]:
			DeleteWorkspace(ws)
		
		self.setProperty("OutputWorkspace", result)
		
	def category(self):
		return 'Diffraction;PythonAlgorithms'

mtd.registerPyAlgorithm(OSIRISDiffractionReduction())
