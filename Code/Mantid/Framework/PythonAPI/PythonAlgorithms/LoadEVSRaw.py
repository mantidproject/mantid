from MantidFramework import *
from mantid.api import FileFinder

import itertools

# This is a helper "functor" for use with the "groupby" itertools function.
# When fed a SORTED list of UNIQUE values one by one, it will return the same "group key"
# for contiguous values.  I.e. passing in 2, 3, 5, 6 and 7 will return 0, 0, 1, 1 and 1. 
class _ContiguousGrouper():
	def __init__(self):
		self._key = int()
		self._currentVal = None
	
	def __call__(self, val):
		if self._currentVal is not None and self._currentVal + 1 is not val:
			self._key += 1
		self._currentVal = val
		return self._key

# Turns [2, 9, 3, 6, 5, 3, 9, 3, 3, 7] into ...
def _listToIntervals(values):
	# ... [2, 3, 5, 6, 7, 9] ...
	sortedUniqueVals = sorted(list(set(values)))
	
	# ... [[2, 3], [5, 6, 7], [9]] ...
	contiguousLists = [list(g) for k, g in itertools.groupby(sortedUniqueVals, _ContiguousGrouper())]
	
	# ... and then returns [[2, 3], [5, 7], [9, 9]].
	return [[cL[0], cL[len(cL) - 1]] for cL in contiguousLists]
	
# Turns [[2, 3], [5, 7], [9, 9]] into [[2, 3], [5, 6, 7], [9]]
def _intervalsToList(intervals):
	result = [range(interval[0], interval[1] + 1) for interval in intervals]
	return list(set(itertools.chain.from_iterable(result)))
	
# Will parse a string of the form "8-10" into the list of numbers [8, 9, 10].
def _parseIntervalToken(intervalToken):
	# Tokenise again on "-"
	bounds = intervalToken.split("-")
	# Strip whitespace.
	bounds = [bound.strip() for bound in bounds]
	
	for bound in bounds:
		# Validate.
		if len(bounds) == 0:
			raise RuntimeError("Could not parse an empty value: " + str(_range))
		if len(bounds) > 2:
			raise RuntimeError("Unexpected range specified: " + str(_range))	
		if not bound.isdigit():
			raise RuntimeError("Non-numeric charater detected: " + bound)
	
	if len(bounds) == 1:
		# Return a list with a single integer.
		return [int(bounds[0])]
	else:
		# Return a list with the range of integers from lower to upper.
		return [number for number in range(int(bounds[0]), int(bounds[1]) + 1)]

class Intervals:
	# Having "*intervals" as a parameter instead of "intervals" allows us
	# to type "Intervals( (0,3), (6, 8) )" instead of "Intervals( ( (0,3), (6, 8) ) )"
	def __init__(self, *intervals):
		# Convert into a list, then back into intervals, to make
		# sure we have no overlapping intervals (which would result in
		# duplicate values.
		values = _intervalsToList(intervals)
		self._intervals = _listToIntervals(values)
	
	#Factory.
	@classmethod
	def fromString(cls, string):
		# Tokenise on commas.
		intervalTokens = string.split(",")
		
		# Call parseRange on each tokenised range.
		numbers = [_parseIntervalToken(intervalToken) for intervalToken in intervalTokens]
		
		# Chain the result (a list of lists) together to make one single list of unique values.
		result = list(set(itertools.chain.from_iterable(numbers)))
		
		# Construct a new Intervals object, populate its intervals, and return.
		newObj = cls()
		newObj._intervals = _listToIntervals(result)
		return newObj
	
	#Factory.
	@classmethod
	def fromList(cls, values):
		result = list(set(values))
		
		# Construct a new Intervals object, populate its intervals, and return.
		newObj = cls()
		newObj._intervals = _listToIntervals(result)
		return newObj
	
	# Returns an array of all the values represented by this "Intervals" instance.
	def getValues(self):
		return [value for interval in self._intervals for value in range(interval[0], interval[1] + 1)]
	
	# Returns the raw intervals.
	def getIntervals(self):
		return self._intervals
	
	# So that "2 in Intervals( (0, 3) )" returns True.
	def __contains__(self, id):
		for interval in self._intervals:
			if interval[0] <= id <= interval[1]:
				return True
		return False
	
	# So that we can type "groups = Intervals( (0, 3) ) + Intervals( (6, 10) )"
	def __add__(self, other):
		newObj = Intervals()
		newObj._intervals = self._intervals + other._intervals
		return newObj
	
	""" TODO: At the moment this is just a generator.  Implement a proper iterator. """
	# So that we can type "for i in Intervals( (0, 2), (4, 5) ):"
	def __iter__(self):
		for interval in self._intervals:
			for value in range(interval[0], interval[1] + 1):
				yield value
	
	# So we can type "interval = Intervals( (3, 5), (10, 12) )" and then "interval[3]" returns 10.
	def __getitem__(self, index):
		return self.getValues()[index]

"""=================================================================================="""
"""=================================================================================="""
"""=================================================================================="""

# Essentially, just a convenience "wrapper" for a map which maps DetectorGroupings to
# period numbers.  Allows us to call map[spectrumID], which returns the periods mapped
# from the Grouping that spectrumID belongs to.
class GroupingMap():
	def __init__(self, map):
		self._map = map
	
	def __getitem__(self, spectrumID):
		for grouping in self._map:
			if spectrumID in grouping:
				return self._map[grouping]
		raise RuntimeError("Unable to find spectrum ID in map.")
		
# Enumerating the different logical groupings of detectors, by their spectra IDs.
class DetectorGrouping:
	# Monitors
	MONITORS = Intervals( (1, 2) )
	
	# Backward detectors are grouped into three modules.
	BACKWARD_1 = Intervals( (3, 46) )
	BACKWARD_2 = Intervals( (47, 90) )
	BACKWARD_3 = Intervals( (91, 134) )
	
	# Forward detectors are put into two groups A and B.  The foils are cycled repeatedly
	# through a run so that when group A is foiled group B is not, and vice versa.
	FORWARD_A = Intervals( (135, 142), (151, 158), (167, 174), (183, 190) )
	FORWARD_B = Intervals( (143, 150), (159, 166), (175, 182), (191, 198) )
	
	# Further logical groupings.
	BACKWARD = BACKWARD_1 + BACKWARD_2 + BACKWARD_3
	BACKWARD_OR_MONITOR = BACKWARD + MONITORS
	FORWARD = FORWARD_A + FORWARD_B
	ANY = MONITORS + BACKWARD + FORWARD

# Enumerate the different foil states.
class Foil:
	OUT = "Out"
	THIN = "Thin"
	THICK = "Thick"
	
	IN = THIN, THICK
	
	ANY = OUT, THIN, THICK

# A "functor" to map spectra IDs to workspace indices.  This is just temporary until
# further development of the C++ MatrixWorkspace class means a good (quick) enough
# function can be exported to the Python API.
class SpectrumID2WsIndexMapper:
	
	# Constructor. Takes in a single workspace.
	def __init__(self, ws):
		
		# Save for later.
		self._ws = ws
		self._map = {}
		
		# Populate map.
		for wsIndex in range(0, ws.getNumberHistograms()):
			spectrumID = ws.getSpectrum(wsIndex).getSpectrumNo()
			self._map[spectrumID] = wsIndex
	
	# Make our mapper a callable object, or "functor".  Returns the workspace index mapped
	# from the spectrum ID provided.
	def __call__(self, spectrumID):
		if spectrumID in self._map:
			return self._map[spectrumID]
		raise RuntimeError("Unable to find spectrum ID of " + str(spectrumID) +
			" in workspace " + self._ws.getName() + ".")

# Take in a ws group (a run or sum of runs with 6 child workspaces representing each period), as well as the spectra IDs
# that we are insterested in.
class PeriodManager:
	def __init__(self, wsGroup, spectraIDs):
		
		""" TODO: Do the backward foil periods change depending on run?  We can easily adjust
		__init__ later to accept any parameters we like, (filenames, or actual workspaces), so
		that the lists can be changed at run time, but for now, assume these lists are fine for
		all cases.  Keep them here. """
		
		""" TODO: Should the periods for spectra and the periods for monitors not ALWAYS be the 
		same as each other?"""
		
		# Map groups of detectors to lists of periods.  Each map value has 2 lists: periods for spectra,
		# and periods for monitors that are used with those spectra.
		self._spectraIDtoFoilOutPeriods = GroupingMap({
			DetectorGrouping.BACKWARD_1 :	((3, 4),	(3, 4)),
			DetectorGrouping.BACKWARD_2 :	((5, 6),	(5, 6)),
			DetectorGrouping.BACKWARD_3 :	((1, 2),	(1, 2)),
			DetectorGrouping.FORWARD_A :	((2, 4, 6),	(4, 6)),
			DetectorGrouping.FORWARD_B :	((1, 3, 5),	(3, 5))
		})
		
		self._spectraIDtoThinFoilPeriods = GroupingMap({
			DetectorGrouping.BACKWARD_1 :	((1, 2),	(1, 2)),
			DetectorGrouping.BACKWARD_2 :	((3, 4),	(3, 4)),
			DetectorGrouping.BACKWARD_3 :	((5, 6),	(5, 6)),
			DetectorGrouping.FORWARD_A :	((1, 3, 5),	(5, 2)),
			DetectorGrouping.FORWARD_B :	((2, 4, 6),	(6, 1))
		})
		
		self._spectraIDtoThickFoilPeriods = GroupingMap({
			DetectorGrouping.BACKWARD_1 :	((5, 6),	(5, 6)),
			DetectorGrouping.BACKWARD_2 :	((1, 2),	(1, 2)),
			DetectorGrouping.BACKWARD_3 :	((3, 4),	(3, 4)),
			DetectorGrouping.FORWARD_A :	((1, 3),	(1, 3)),
			DetectorGrouping.FORWARD_B :	((2, 4),	(2, 4))
		})
		
		# Arbitrarily take the first period of the workspace group, since all the information we need from it is
		# assumed to be the same in all periods.
		period = mtd[wsGroup.getNames()[0]]
		# Instantiate a spectrum ID to ws index mapper, with the period.
		toWsIndex = SpectrumID2WsIndexMapper(period)
		
		self._spectraIDs = spectraIDs
		self._mappings = {}
		self._monMappings = {}
		
		# For each spectrum ID, construct a mapping of foil states to periods, and map
		# each mapping to that spectrums ws index.
		for spectrumID in self._spectraIDs:
			# Convert spectrumID to ws index.
			wsIndex = toWsIndex(spectrumID)
			mapping = {
				Foil.OUT : self._spectraIDtoFoilOutPeriods[spectrumID][0],
				Foil.THIN : self._spectraIDtoThinFoilPeriods[spectrumID][0],
				Foil.THICK : self._spectraIDtoThickFoilPeriods[spectrumID][0]
			}
			self._mappings[wsIndex] = mapping
			
			monMapping = {
				Foil.OUT : self._spectraIDtoFoilOutPeriods[spectrumID][1],
				Foil.THIN : self._spectraIDtoThinFoilPeriods[spectrumID][1],
				Foil.THICK : self._spectraIDtoThickFoilPeriods[spectrumID][1]
			}
			
			self._monMappings[wsIndex] = monMapping
	
	# Return the mappings of foil states to periods for each ws index.
	def getMappings(self):
		return self._mappings
	
	# Return the monitor mappings of foil states to periods for each ws index.	
	def getMonMappings(self):
		return self._monMappings

"""=================================================================================="""
"""=================================================================================="""
"""=================================================================================="""

# Given a list of workspaces, will sum them together into a single new workspace, with the given name.
# If no name is given, then one is constructed from the names of the given workspaces.
def sumWsList(wsList, summedWsName = None):
	if len(wsList) == 1:
		if summedWsName is not None:
			CloneWorkspace(InputWorkspace=wsList[0].getName(), OutputWorkspace=summedWsName)
			return mtd[summedWsName]
		return wsList[0]
	
	sum = wsList[0] + wsList[1]
	
	if len(wsList) > 2:
		for i in range(2, len(wsList) - 1):
			sum += wsList[i]
	
	if summedWsName is None:
		summedWsName = "_PLUS_".join([ws.getName() for ws in wsList])
	
	RenameWorkspace(InputWorkspace=sum.getName(), OutputWorkspace=summedWsName)
	
	return mtd[summedWsName]

# Given a spectrum and a range of times, returns the sum of the counts between those two times.
def sumCountsBetweenTimes(spectrum, fromTime, toTime, wsIndex=0):
	dataX = spectrum.dataX(wsIndex)
	dataY = spectrum.dataY(wsIndex)
	
	return sum( [y for x, y in zip(dataX, dataY) if fromTime <= x <= toTime] )

"""=================================================================================="""
"""=================================================================================="""
"""=================================================================================="""

class LoadEVSRaw(PythonAlgorithm):
	def category(self):
		return 'Inelastic;PythonAlgorithms'

	def PyInit(self):
		# Declare algorithm properties.
		self.declareProperty('Runs', '' , MandatoryValidator(), Description='Runs, e.g. \"2012-2020\"')
		self.declareWorkspaceProperty('OutputWorkspace', '' , Direction.Output, Description='Name of workspace into which the result will be put.')
		self.declareProperty('Spectra', '' , MandatoryValidator(), Description='Spectra IDs, e.g. \"5-10\".  Can accept a full range of both backward and forward scattering spectra.')
		""" TODO: What is this ... ? """
		self.declareProperty('Beta', 0.0, MandatoryValidator(), Description='')

	def PyExec(self):
		# Parse / store algorithm property values for later use.
		runNumbers		= Intervals.fromString(self.getProperty('Runs'))
		self._spectraIDs	= Intervals.fromString(self.getProperty('Spectra'))
		outputWsName	= self.getPropertyValue('OutputWorkspace')
		beta	 		= self.getProperty('Beta')
		
		# SpectraID of first monitor.
		monitorID	= DetectorGrouping.MONITORS[0]
		
		# Min and max X values, used to crop loaded files.
		cropLower	= 0
		cropUpper	= 700
		
		# Find all files before loading.
		filenames = [FileFinder.Instance().getFullPath("EVS" + str(runNumber) + ".raw") for runNumber in runNumbers]
		for runNumber, filename in zip(runNumbers, filenames):
			if filename == '':
				raise RuntimeError("Unable to find file for run " + str(runNumber) + ".")
		
		# Load files, each into two group workspaces: monitors kept separate from other spectra.
		# Workspaces are cropped between cropLower and cropUpper.
		for filename in filenames:
			LoadRaw(Filename=filename, OutputWorkspace=filename, LoadLogFiles=False,
				SpectrumList=[monitorID] + self._spectraIDs.getValues(), LoadMonitors="Separate")
			CropWorkspace(InputWorkspace=filename, OutputWorkspace=filename,XMin=cropLower,XMax=cropUpper)
			CropWorkspace(InputWorkspace=filename+"_Monitors", OutputWorkspace=filename+"_Monitors",XMin=cropLower,XMax=cropUpper)
		
		# Get handles to workspaces.
		runs = [mtd[filename] for filename in filenames]
		runsMon = [mtd[filename + "_Monitors"] for filename in filenames]
		
		# Sum the runs together, and delete the individual run workspaces.
		name = "EVS" + self.getProperty('Runs') + "_sp" + self.getProperty('Spectra')
		summedRuns = sumWsList(runs, name)
		summedRunsMon = sumWsList(runsMon, name + "_Monitor")
		for ws in runs + runsMon:
			DeleteWorkspace(Workspace=ws.getName())
		
		# Clone some new workspaces, into which we will place our calculated foil data, and place in a map so we can
		# access the workspaces by foil state.
		clonedWsName = summedRuns.getNames()[0]
		foilWsMap = {}
		monFoilWsMap = {}
		for foilState in Foil.ANY:
			# Generate names.
			wsName = summedRuns.getName() + "_" + foilState
			monWsName = wsName + "_Mon"
			# Clone new workspaces.
			CloneWorkspace(InputWorkspace=clonedWsName, OutputWorkspace=wsName)
			CloneWorkspace(InputWorkspace=clonedWsName, OutputWorkspace=monWsName)
			# Add to maps.
			foilWsMap[foilState] = mtd[wsName]
			monFoilWsMap[foilState] = mtd[monWsName]
		
		# Query an instance of a PeriodManager for maps of workspaceIndex to foil out / thin foil / thick foil period mappings.
		periodMan = PeriodManager(summedRuns, self._spectraIDs)
		periodMappings = periodMan.getMappings()
		monPeriodMappings = periodMan.getMonMappings()
		
		# Get handles to each of the period and period monitor workspaces.
		periodWsList = [ mtd[periodWsName] for periodWsName in summedRuns.getNames() ]
		monPeriodWsList = [ mtd[monPeriodWsName] for monPeriodWsName in summedRunsMon.getNames() ]
		
		""" TODO: Is this just an equivalent to a "rebinning" step? """
		# Create a "time" workspace by arbitrarily picking the first spectra of the first period, and using its X axis data.
		# This workspace will have Y axis data that corresponds to the differences in time between each X axis time boundary.
		# It also has units of "TOF", so that other workspaces with TOF units may be divided by it.
		dataX = periodWsList[0].dataX(0)
		dataY = [dataX[i] - dataX[i - 1] for i in range(1, len(dataX))]
		timeWs = CreateWorkspace(DataX=list(dataX), DataY=list(dataY), DataE=list(dataY), UnitX="TOF")
		
		# Divide period and monitors through by the timeWs, and then delete it.
		for periodWs in periodWsList + monPeriodWsList:
			Divide(LHSWorkspace=periodWs.getName(),RHSWorkspace=timeWs.getName(),OutputWorkspace=periodWs.getName())
		DeleteWorkspace(Workspace=timeWs.getName())
		
		sumMap = {}
		monSumMap = {}
		
		# Iterate through each workspace index / foil state pair, and sum together the spectra data for the relavant periods,
		# and normalise to monitor.
		for wsIndex, foilState in itertools.product(periodMappings.iterkeys(), Foil.ANY):
			
			# Get the period numbers which correspond to this ws index and foil state ...
			foilPeriods = periodMappings[wsIndex][foilState]
			monFoilPeriods = monPeriodMappings[wsIndex][foilState]
			# ... then use those period numbers to get handles to the periods themselves ...
			foilPeriodWsList = [ periodWsList[foilPeriod - 1] for foilPeriod in foilPeriods ]
			monFoilPeriodWsList = [ monPeriodWsList[foilPeriod - 1] for monFoilPeriod in monFoilPeriods ]
			# .. then use those handles to get wrappers to the actual data in those periods.
			foilPeriodWsDataList = [ foilPeriodWs.dataY(wsIndex) for foilPeriodWs in foilPeriodWsList ]
			monFoilPeriodWsDataList = [ monFoilPeriodWs.dataY(0) for monFoilPeriodWs in monFoilPeriodWsList ]
			
			# Now zip up the period data.  Zipping does the following:
			# A = 0, 1, 2, ... 
			# B = 0, 1, 2, ...
			# zip(A, B) = (0, 0), (1, 1), (2, 2), ...
			zipped = itertools.izip(*foilPeriodWsDataList)
			monZipped = itertools.izip(*monFoilPeriodWsDataList)
			
			# Sum up the period data for each spectra.
			for i, z in itertools.izip(itertools.count(), zipped):
				foilWsMap[foilState].dataY(wsIndex)[i] = sum(z)
			
			# Sum up the period data for each spectra's monitors.
			for i, monZ in itertools.izip(itertools.count(), monZipped):
				monFoilWsMap[foilState].dataY(wsIndex)[i] = sum(monZ)
		
		# Iterate through each workspace index / foil state pair, and sum counts over spectra and monitor foil states.
		for wsIndex, foilState in itertools.product(periodMappings.iterkeys(), Foil.ANY):
			# Sum monitor counts between 600 and 700 usec, and add to monitor normalising map.
			monNorm = sumCountsBetweenTimes(monFoilWsMap[foilState], 600, 700)
			monSumMap[wsIndex, foilState] = monNorm
			
			# Convert back to spectra ID so we can decide which values to sum between.
			spectrumID = foilWsMap[foilState].getSpectrum(wsIndex).getSpectrumNo()
			# Sum between values, and add to sum normalising map.
			if spectrumID in DetectorGrouping.BACKWARD:
				sumMap[ wsIndex, foilState ] = sumCountsBetweenTimes(foilWsMap[foilState], 400, 450, wsIndex)
			elif spectrumID in DetectorGrouping.FORWARD:
				sumMap[ wsIndex, foilState ] = sumCountsBetweenTimes(foilWsMap[foilState], 410, 430, wsIndex)
			else:
				# We should never reach here.
				assert False, "Programming error in algorithm"
		
		# For each combination of ws index and foil state, normalise on monitor counts and spectra area sum.
		for wsIndex, foilState in itertools.product(periodMappings.iterkeys(), (Foil.ANY)):
			
			data = foilWsMap[foilState].dataY(wsIndex)
			
			monSum = monSumMap[wsIndex, foilState]
			if monSum is 0:
				monSum = 0.0000001
			for i in range(0,len(data)):
				data[i] *= ( 1000 /  monSum)
		
		# For each combination of ws index and foil state, normalise on monitor counts and spectra area sum.
		for wsIndex, foilState in itertools.product(periodMappings.iterkeys(), (Foil.IN)):
			
			data = foilWsMap[foilState].dataY(wsIndex)
			
			for i in range(0,len(data)):
				data[i] *= ( sumMap[wsIndex,Foil.OUT] / sumMap[wsIndex,foilState] )
		
		CloneWorkspace(InputWorkspace=clonedWsName, OutputWorkspace=outputWsName)
		outputWs = mtd[outputWsName]
		
		for wsIndex in periodMappings:
			# Get spectra ID from wsIndex.
			spectrumID = foilWsMap[foilState].getSpectrum(wsIndex).getSpectrumNo()
			
			# 
			cResult = outputWs.dataY(wsIndex)
			cOut = foilWsMap[Foil.OUT].dataY(wsIndex)
			cThin = foilWsMap[Foil.THIN].dataY(wsIndex)
			cThick = foilWsMap[Foil.THICK].dataY(wsIndex)
			
			for i in range(0, len(cResult)):
				if spectrumID in DetectorGrouping.BACKWARD:
					""" TODO: Backward difference calcs not having desired effect at the moment.
					Is the double difference below really the one used in RawB? """
					
					#THICK DIFFERENCE
					#cResult[i] = cThick[i] - cOut[i]
					
					#DOUBLE DIFFERENCE
					#cResult[i] = cThick[i] * (1 - beta) - cOut[i] + beta * cThin[i]
					cResult[i] = cOut[i] * (1 - beta) - cThin[i] + beta * cThick[i]
				
				elif spectrumID in DetectorGrouping.FORWARD:
					# SINGLE DIFFERENCE
					cResult[i] = cThin[i] - cOut[i]
		
		# Set as the output workspace.
		self.setProperty('OutputWorkspace', outputWs)

# Register algorthm with Mantid.
mantid.registerPyAlgorithm(LoadEVSRaw())
