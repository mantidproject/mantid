from __future__ import (absolute_import, division, print_function)
from six import iteritems, iterkeys, itervalues

import itertools

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

#pylint: disable=too-few-public-methods


class DRange(object):
    """
    A class to represent a dRange.
    """

    def __init__(self, lower, upper):
        self._range = [lower, upper]

    def __getitem__(self, idx):
        return self._range[idx]

    def __str__(self):
        return '%.3f - %.3f Angstrom' % (self._range[0], self._range[1])


TIME_REGIME_TO_DRANGE = {
    1.17e4: DRange( 0.7,  2.5),
    2.94e4: DRange( 2.1,  3.3),
    4.71e4: DRange( 3.1,  4.3),
    6.48e4: DRange( 4.1,  5.3),
    8.25e4: DRange( 5.2,  6.2),
    10.02e4: DRange( 6.2,  7.3),
    11.79e4: DRange( 7.3,  8.3),
    13.55e4: DRange( 8.3,  9.5),
    15.32e4: DRange( 9.4, 10.6),
    17.09e4: DRange(10.4, 11.6),
    18.86e4: DRange(11.0, 12.5),
    20.63e4: DRange(12.2, 13.8)
}


class DRangeToWorkspaceMap(object):
    """
    A "wrapper" class for a map, which maps workspaces from their corresponding
    time regimes.
    """

    def __init__(self):
        self._map = {}

    def addWs(self, ws_name, d_range=None):
        """
        Takes in the given workspace and lists it alongside its time regime
        value.  If the time regime has yet to be created, it will create it,
        and if there is already a workspace listed beside the time regime, then
        the new ws will be appended to that list.

        @param ws_name Name of workspace to add
        @param d_range Optionally override the dRange
        """
        wrksp = mtd[ws_name]

        # Get the time regime of the workspace, and use it to find the DRange.
        time_regime = wrksp.dataX(0)[0]
        time_regimes = sorted(TIME_REGIME_TO_DRANGE.keys())

        for idx in range(len(time_regimes)):
            if idx == len(time_regimes) - 1:
                if time_regimes[idx] < time_regime:
                    time_regime = time_regimes[idx]
                    break
            else:
                if time_regimes[idx] < time_regime < time_regimes[idx + 1]:
                    time_regime = time_regimes[idx]
                    break

        if d_range is None:
            d_range = TIME_REGIME_TO_DRANGE[time_regime]
        else:
            d_range = TIME_REGIME_TO_DRANGE[time_regimes[d_range]]

        logger.information('dRange for workspace %s is %s' % (ws_name, str(d_range)))

        # Add the workspace to the map, alongside its DRange.
        if d_range not in self._map:
            self._map[d_range] = [ws_name]
        else:
            #check if x ranges matchs and existing run
            for ws_name in self._map[d_range]:
                map_lastx = mtd[ws_name].readX(0)[-1]
                ws_lastx = wrksp.readX(0)[-1]

                #if it matches ignore it
                if map_lastx == ws_lastx:
                    DeleteWorkspace(wrksp)
                    return

            self._map[d_range].append(ws_name)

    def setItem(self, d_range, ws_name):
        """
        Set a dRange and corresponding *single* ws.
        """
        self._map[d_range] = ws_name

    def getMap(self):
        """
        Get access to wrapped map.
        """
        return self._map


def average_ws_list(ws_list):
    """
    Returns the average of a list of workspaces.
    """
    # Assert we have some ws in the list, and if there is only one then return it.
    if len(ws_list) == 0:
        raise RuntimeError("getAverageWs: Trying to take an average of nothing")

    if len(ws_list) == 1:
        return ws_list[0]

    # Generate the final name of the averaged workspace.
    avName = "avg"
    for name in ws_list:
        avName += "_" + name

    numWorkspaces = len(ws_list)

    # Compute the average and put into "__temp_avg".
    __temp_avg = mtd[ws_list[0]]
    for i in range(1, numWorkspaces):
        __temp_avg += mtd[ws_list[i]]

    __temp_avg /= numWorkspaces

    # Rename the average ws and return it.
    RenameWorkspace(InputWorkspace=__temp_avg, OutputWorkspace=avName)
    return avName


def find_intersection_of_ranges(rangeA, rangeB):
    if rangeA[0] >= rangeA[1] or rangeB[0] >= rangeB[1]:
        raise RuntimeError("Malformed range")

    if rangeA[0] <= rangeA[1] <= rangeB[0] <= rangeB[1]:
        return
    if rangeB[0] <= rangeB[1] <= rangeA[0] <= rangeA[1]:
        return
    if rangeA[0] <= rangeB[0] <= rangeB[1] <= rangeA[1]:
        return rangeB
    if rangeB[0] <= rangeA[0] <= rangeA[1] <= rangeB[1]:
        return rangeA
    if rangeA[0] <= rangeB[0] <= rangeA[1] <= rangeB[1]:
        return [rangeB[0], rangeA[1]]
    if rangeB[0] <= rangeA[0] <= rangeB[1] <= rangeA[1]:
        return [rangeA[0], rangeB[1]]

    # Should never reach here
    raise RuntimeError()


def get_intersetcion_of_ranges(range_list):
    """
    Get the intersections of a list of ranges.  For example, given the ranges:
    [1, 3], [3, 5] and [4, 6], the intersections would be a single range of [4,
    5].

    NOTE: Assumes that no more than a maximum of two ranges will ever cross at
    the same point.  Also, all ranges should obey range[0] <= range[1].
    """
    # Find all combinations of ranges, and see where they intersect.
    rangeCombos = list(itertools.combinations(range_list, 2))
    intersections = []
    for rangePair in rangeCombos:
        intersection = find_intersection_of_ranges(rangePair[0], rangePair[1])
        if intersection is not None:
            intersections.append(intersection)

    # Return the sorted intersections.
    intersections.sort()
    return intersections


def is_in_ranges(range_list, val):
    for myrange in range_list:
        if myrange[0] < val < myrange[1]:
            return True
    return False


#pylint: disable=no-init,too-many-instance-attributes
class OSIRISDiffractionReduction(PythonAlgorithm):
    """
    Handles the reduction of OSIRIS Diffraction Data.
    """

    _cal = None
    _output_ws_name = None
    _sample_runs = None
    _vanadium_runs = None
    _container_files = None
    _container_scale_factor = None
    _sam_ws_map = None
    _van_ws_map = None
    _man_d_range = None
    _load_logs = None
    _spec_min = None
    _spec_max = None

    def category(self):
        return 'Diffraction\\Reduction'

    def summary(self):
        return "This Python algorithm performs the operations necessary for the reduction of diffraction data "+\
               "from the Osiris instrument at ISIS "+\
               "into dSpacing, by correcting for the monitor and linking the various d-ranges together."

    def PyInit(self):
        runs_desc='The list of run numbers that are part of the sample run. '+\
                  'There should be five of these in most cases. Enter them as comma separated values.'

        self.declareProperty(StringArrayProperty('Sample'),
                             doc=runs_desc)

        self.declareProperty(StringArrayProperty('Vanadium'),
                             doc=runs_desc)

        self.declareProperty(StringArrayProperty('Container'),
                             doc=runs_desc)

        self.declareProperty('ContainerScaleFactor', 1.0,
                             doc='Factor by which to scale the container')

        self.declareProperty(FileProperty('CalFile', '', action=FileAction.Load),
                             doc='Filename of the .cal file to use in the [[AlignDetectors]] and '+
                             '[[DiffractionFocussing]] child algorithms.')

        self.declareProperty('SpectraMin', 3, doc='Minimum Spectrum to Load from (Must be more than 3)')

        self.declareProperty('SpectraMax', 962, doc='Maximum Spectrum to Load from file (Must be less than 962)')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace. If no name is provided, "+
                             "one will be generated based on the run numbers.")

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

        self.declareProperty('DetectDRange', True,
                             doc='Disable to override automatic dRange detection')

        # Note that dRange numbers are offset to match the numbering in the OSIRIS manual
        # http://www.isis.stfc.ac.uk/instruments/osiris/documents/osiris-user-guide6672.pdf
        self.declareProperty('DRange', 1, validator=IntBoundedValidator(1, len(TIME_REGIME_TO_DRANGE) + 1),
                             doc='Drange to use when DetectDRange is disabled')

        self._cal = None
        self._output_ws_name = None

        self._sam_ws_map = DRangeToWorkspaceMap()
        self._van_ws_map = DRangeToWorkspaceMap()

    def _get_properties(self):
        self._load_logs = self.getProperty('LoadLogFiles').value
        self._cal = self.getProperty("CalFile").value
        self._output_ws_name = self.getPropertyValue("OutputWorkspace")

        self._sample_runs = self._find_runs(self.getProperty("Sample").value)
        self._vanadium_runs = self._find_runs(self.getProperty("Vanadium").value)

        self._container_files = self.getProperty("Container").value
        self._container_scale_factor = self.getProperty("ContainerScaleFactor").value

        if self._container_files:
            self._container_files = self._find_runs(self._container_files)

        self._spec_min = self.getPropertyValue("SpectraMin")
        self._spec_max = self.getPropertyValue("SpectraMax")

        self._man_d_range = None
        if not self.getProperty("DetectDRange").value:
            self._man_d_range = self.getProperty("DRange").value - 1

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        num_samples = len(self._sample_runs)
        num_vanadium = len(self._vanadium_runs)
        if num_samples != num_vanadium:
            run_num_mismatch = 'You must input the same number of sample and vanadium runs'
            issues['Sample'] = run_num_mismatch
            issues['Vanadium'] = run_num_mismatch
        if self._container_files:
            num_containers = len(self._container_files)
            if num_samples != num_containers:
                issues['Container'] = 'You must input the same number of sample and container runs'

        return issues

    #pylint: disable=too-many-branches
    def PyExec(self):
        """
        Execute the algorithm in diffraction-only mode
        """
        # Load all sample, vanadium files
        for fileName in self._sample_runs + self._vanadium_runs:
            Load(Filename=fileName,
                 OutputWorkspace=fileName,
                 SpectrumMin=self._spec_min,
                 SpectrumMax=self._spec_max,
                 LoadLogFiles=self._load_logs)

        # Load the container run
        if self._container_files:
            for container in self._container_files:
                Load(Filename=container,
                     OutputWorkspace=container,
                     SpectrumMin=self._spec_min,
                     SpectrumMax=self._spec_max,
                     LoadLogFiles=self._load_logs)

                # Scale the container run if required
                if self._container_scale_factor != 1.0:
                    Scale(InputWorkspace=container,
                          OutputWorkspace=container,
                          Factor=self._container_scale_factor,
                          Operation='Multiply')

        # Add the sample workspaces to the dRange to sample map
        for idx in range(len(self._sample_runs)):
            if self._container_files:

                RebinToWorkspace(WorkspaceToRebin=self._container_files[idx],
                                 WorkspaceToMatch=self._sample_runs[idx],
                                 OutputWorkspace=self._container_files[idx])

                Minus(LHSWorkspace=self._sample_runs[idx],
                      RHSWorkspace=self._container_files[idx],
                      OutputWorkspace=self._sample_runs[idx])

            self._sam_ws_map.addWs(self._sample_runs[idx])

        # Add the vanadium workspaces to the dRange to vanadium map
        for van in self._vanadium_runs:
            self._van_ws_map.addWs(van)

        # Finished with container now so delete it
        if self._container_files:
            for container in self._container_files:
                DeleteWorkspace(container)

        # Check to make sure that there are corresponding vanadium files with the same DRange for each sample file.
        for d_range in iterkeys(self._sam_ws_map.getMap()):
            if d_range not in self._van_ws_map.getMap():
                raise RuntimeError("There is no van file that covers the " + str(d_range) + " DRange.")

        # Average together any sample workspaces with the same DRange.
        # This will mean our map of DRanges to list of workspaces becomes a map
        # of DRanges, each to a *single* workspace.
        temp_sam_map = DRangeToWorkspaceMap()
        for d_range, ws_list in iteritems(self._sam_ws_map.getMap()):
            temp_sam_map.setItem(d_range, average_ws_list(ws_list))
        self._sam_ws_map = temp_sam_map

        # Now do the same to the vanadium workspaces.
        temp_van_map = DRangeToWorkspaceMap()
        for d_range, ws_list in iteritems(self._van_ws_map.getMap()):
            temp_van_map.setItem(d_range, average_ws_list(ws_list))
        self._van_ws_map = temp_van_map

        # Run necessary algorithms on BOTH the Vanadium and Sample workspaces.
        for d_range, wrksp in list(self._sam_ws_map.getMap().items()) + list(self._van_ws_map.getMap().items()):
            self.log().information('Wrksp:' + str(wrksp) + ' Cal:' + str(self._cal))
            NormaliseByCurrent(InputWorkspace=wrksp,
                               OutputWorkspace=wrksp)
            AlignDetectors(InputWorkspace=wrksp,
                           OutputWorkspace=wrksp,
                           CalibrationFile=self._cal)
            DiffractionFocussing(InputWorkspace=wrksp,
                                 OutputWorkspace=wrksp,
                                 GroupingFileName=self._cal)
            CropWorkspace(InputWorkspace=wrksp,
                          OutputWorkspace=wrksp,
                          XMin=d_range[0],
                          XMax=d_range[1])

        # Divide all sample files by the corresponding vanadium files.
        for d_range in iterkeys(self._sam_ws_map.getMap()):
            sam_ws = self._sam_ws_map.getMap()[d_range]
            van_ws = self._van_ws_map.getMap()[d_range]
            sam_ws, van_ws = self._rebin_to_smallest(sam_ws, van_ws)
            Divide(LHSWorkspace=sam_ws,
                   RHSWorkspace=van_ws,
                   OutputWorkspace=sam_ws)
            ReplaceSpecialValues(InputWorkspace=sam_ws,
                                 OutputWorkspace=sam_ws,
                                 NaNValue=0.0,
                                 InfinityValue=0.0)

        # Create a list of sample workspace NAMES, since we need this for MergeRuns.
        samWsNamesList = []
        for sam in itervalues(self._sam_ws_map.getMap()):
            samWsNamesList.append(sam)

        if len(samWsNamesList) > 1:
            # Merge the sample files into one.
            MergeRuns(InputWorkspaces=samWsNamesList,
                      OutputWorkspace=self._output_ws_name)
            for name in samWsNamesList:
                DeleteWorkspace(Workspace=name)
        else:
            RenameWorkspace(InputWorkspace=samWsNamesList[0],
                            OutputWorkspace=self._output_ws_name)

        result = mtd[self._output_ws_name]

        # Create scalar data to cope with where merge has combined overlapping data.
        intersections = get_intersetcion_of_ranges(list(self._sam_ws_map.getMap().keys()))

        dataX = result.dataX(0)
        dataY = []
        dataE = []
        for i in range(0, len(dataX)-1):
            x_val = (dataX[i] + dataX[i+1]) / 2.0
            if is_in_ranges(intersections, x_val):
                dataY.append(2)
                dataE.append(2)
            else:
                dataY.append(1)
                dataE.append(1)

        # apply scalar data to result workspace
        for i in range(0, result.getNumberHistograms()):
            resultY = result.dataY(i)
            resultE = result.dataE(i)

            resultY = resultY / dataY
            resultE = resultE / dataE

            result.setY(i,resultY)
            result.setE(i,resultE)

        # Delete all workspaces we've created, except the result.
        for wrksp in self._van_ws_map.getMap().values():
            DeleteWorkspace(Workspace=wrksp)

        self.setProperty("OutputWorkspace", result)

    def _find_runs(self, runs):
        """
        Use the FileFinder to find search for the runs given by the string of
        comma-separated run numbers.

        @param run_str A string of run numbers to find
        @returns A list of filepaths
        """
        run_files = []
        for run in runs:
            try:
                run_files.append(FileFinder.findRuns(run)[0])
            except IndexError:
                raise RuntimeError("Could not locate sample file: " + run)

        return run_files

    def _rebin_to_smallest(self, samWS, vanWS):
        """
        At some point a change to the control program meant that the raw data
        got an extra bin. This prevents runs past this point being normalised
        with a vanadium from an earlier point.  Here we simply rebin to the
        smallest workspace if the sizes don't match

        @param samWS A workspace object containing the sample run
        @param vanWS A workspace object containing the vanadium run
        @returns samWS, vanWS rebinned  to the smallest if necessary
        """
        sample_size, van_size = mtd[samWS].blocksize(), mtd[vanWS].blocksize()
        if sample_size == van_size:
            return samWS, vanWS

        if sample_size < van_size:
            # Rebin vanadium to match sample
            RebinToWorkspace(WorkspaceToRebin=vanWS,
                             WorkspaceToMatch=samWS,
                             OutputWorkspace=vanWS)
        else:
            # Rebin sample to match vanadium
            RebinToWorkspace(WorkspaceToRebin=samWS,
                             WorkspaceToMatch=vanWS,
                             OutputWorkspace=samWS)

        return samWS, vanWS


AlgorithmFactory.subscribe(OSIRISDiffractionReduction)
