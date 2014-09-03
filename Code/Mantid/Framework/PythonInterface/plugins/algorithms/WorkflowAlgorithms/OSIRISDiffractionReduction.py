from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import itertools
import os

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

    def addWs(self, wsname):
        """ Takes in the given workspace and lists it alongside its time regime value.
        If the time regime has yet to be created, it will create it, and if there is already
        a workspace listed beside the time regime, then the new ws will be appended
        to that list.
        """
        ws=mtd[wsname]
        # Get the time regime of the workspace, and use it to find the DRange.
        timeRegime = ws.dataX(0)[0]
        try:
            dRange = timeRegimeToDRange[timeRegime]
        except KeyError:
            raise RuntimeError("Unable to identify the DRange of " + wsname +
                ", which has a time regime of " + str(timeRegime))

        # Add the workspace to the map, alongside its DRange.
        if dRange not in self._map:
            self._map[dRange] = [wsname]
        else:
            #check if x ranges matchs and existing run
            for ws_name in self._map[dRange]:
                map_lastx = mtd[ws_name].readX(0)[-1]
                ws_lastx = ws.readX(0)[-1]

                #if it matches ignore it
                if map_lastx == ws_lastx:
                    DeleteWorkspace(ws)
                    return

            self._map[dRange].append(wsname)

    def setItem(self, dRange, wsname):
        """ Set a dRange and corresponding *single* ws.
        """
        self._map[dRange] = wsname

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
    for name in wsList:
        avName += "_" + name

    numWorkspaces = len(wsList)

    # Compute the average and put into "__temp_avg".
    __temp_avg = mtd[wsList[0]]
    for i in range(1, numWorkspaces):
        __temp_avg += mtd[wsList[i]]

    __temp_avg /= numWorkspaces

    # Rename the average ws and return it.
    RenameWorkspace(InputWorkspace=__temp_avg, OutputWorkspace=avName)
    return avName

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
        if intersection is not None:
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
    def category(self):
        return 'Diffraction;PythonAlgorithms'

    def summary(self):
      return "This Python algorithm performs the operations necessary for the reduction of diffraction data from the Osiris instrument at ISIS \
              into dSpacing, by correcting for the monitor and linking the various d-ranges together."

    def PyInit(self):
        runs_desc='The list of run numbers that are part of the sample run. \
                   There should be five of these in most cases. Enter them as comma separated values.'
        self.declareProperty('Sample', '', doc=runs_desc)
        self.declareProperty('Vanadium', '', doc=runs_desc)
        self.declareProperty(FileProperty('CalFile', '', action=FileAction.Load),
                             doc='Filename of the .cal file to use in the [[AlignDetectors]] and [[DiffractionFocussing]] child algorithms.')
        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace. If no name is provided, one will be generated based on the run numbers.")

        self._cal = None
        self._outputWsName = None

        self._samMap = DRangeToWsMap()
        self._vanMap = DRangeToWsMap()

    def PyExec(self):
        # Set OSIRIS as default instrument.
        config["default.instrument"] = 'OSIRIS'

        self._cal = self.getProperty("CalFile").value
        self._outputWsName = self.getPropertyValue("OutputWorkspace")

        sampleRuns = self.findRuns(self.getPropertyValue("Sample"))

        self.execDiffOnly(sampleRuns)

    def execDiffOnly(self, sampleRuns):
        """
            Execute the algorithm in diffraction-only mode
            @param sampleRuns A list of files pointing to the sample runs
        """
        self._sams = sampleRuns
        self._vans = self.findRuns(self.getPropertyValue("Vanadium"))

        # Load all sample and vanadium files, and add the resulting workspaces to the DRangeToWsMaps.
        for file in self._sams + self._vans:
            Load(Filename=file, OutputWorkspace=file, SpectrumMin=3, SpectrumMax=962)
        for sam in self._sams:
            self._samMap.addWs(sam)
        for van in self._vans:
            self._vanMap.addWs(van)

        # Check to make sure that there are corresponding vanadium files with the same DRange for each sample file.
        for dRange in self._samMap.getMap().iterkeys():
            if dRange not in self._vanMap.getMap():
                raise RuntimeError("There is no van file that covers the " + str(dRange) + " DRange.")

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
            NormaliseByCurrent(InputWorkspace=ws,OutputWorkspace=ws)
            AlignDetectors(InputWorkspace=ws, OutputWorkspace=ws, CalibrationFile=self._cal)
            DiffractionFocussing(InputWorkspace=ws, OutputWorkspace=ws, GroupingFileName=self._cal)
            CropWorkspace(InputWorkspace=ws, OutputWorkspace=ws, XMin=dRange[0], XMax=dRange[1])

        # Divide all sample files by the corresponding vanadium files.
        for dRange in self._samMap.getMap().iterkeys():
            samWs = self._samMap.getMap()[dRange]
            vanWs = self._vanMap.getMap()[dRange]
            samWs, vanWs = self.rebinToSmallest(samWs, vanWs)
            Divide(LHSWorkspace=samWs, RHSWorkspace=vanWs, OutputWorkspace=samWs)
            ReplaceSpecialValues(InputWorkspace=samWs, OutputWorkspace=samWs, NaNValue=0.0, InfinityValue=0.0)

        # Create a list of sample workspace NAMES, since we need this for MergeRuns.
        samWsNamesList = []
        for sam in self._samMap.getMap().itervalues():
            samWsNamesList.append(sam)

        if len(samWsNamesList) > 1:
            # Merge the sample files into one.
            MergeRuns(InputWorkspaces=samWsNamesList, OutputWorkspace=self._outputWsName)
            for name in samWsNamesList:
                DeleteWorkspace(Workspace=name)
        else:
            RenameWorkspace(InputWorkspace=samWsNamesList[0],OutputWorkspace=self._outputWsName)

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

        # apply scalar data to result workspace
        for i in range(0, result.getNumberHistograms()):
            resultY = result.dataY(i)
            resultE = result.dataE(i)

            resultY = resultY / dataY
            resultE = resultE / dataE

            result.setY(i,resultY)
            result.setE(i,resultE)

        # Delete all workspaces we've created, except the result.
        for ws in self._vanMap.getMap().values():
            DeleteWorkspace(Workspace=ws)

        self.setProperty("OutputWorkspace", result)

    def findRuns(self, run_str):
        """
           Use the FileFinder to find search for the runs given by the string of comma-separated run numbers
           @param run_str A string of run numbers to find
           @returns A list of filepaths
        """
        runs = run_str.split(",")
        run_files = []
        for run in runs:
            try:
                run_files.append(FileFinder.findRuns(run)[0])
            except IndexError:
                raise RuntimeError("Could not locate sample file: " + run)

        return run_files

    def rebinToSmallest(self, samWS, vanWS):
        """
            At some point a change to the control program
            meant that the raw data got an extra bin. This
            prevents runs past this point being normalised
            with a vanadium from an earlier point.
            Here we simply rebin to the smallest workspace if
            the sizes don't match
            @param samWS A workspace object containing the sample run
            @param vanWS A workspace object containing the vanadium run
            @returns samWS, vanWS rebinned  to the smallest if necessary
        """
        sample_size, van_size = mtd[samWS].blocksize(), mtd[vanWS].blocksize()
        if sample_size == van_size:
            return samWS, vanWS

        if sample_size < van_size:
            RebinToWorkspace(WorkspaceToRebin=vanWS, WorkspaceToMatch=samWS,OutputWorkspace=vanWS)
        else:
            RebinToWorkspace(WorkspaceToRebin=samWS, WorkspaceToMatch=vanWS,OutputWorkspace=samWS)

        return samWS, vanWS


AlgorithmFactory.subscribe(OSIRISDiffractionReduction)
