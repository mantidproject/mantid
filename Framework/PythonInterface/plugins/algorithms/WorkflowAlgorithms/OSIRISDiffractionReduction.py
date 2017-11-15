from __future__ import (absolute_import, division, print_function)

import itertools

from IndirectReductionCommon import load_files

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (AddSampleLog, AlignDetectors, CropWorkspace, DeleteWorkspace, DiffractionFocussing,
                              MergeRuns, NormalizeByCurrent, RebinToWorkspace, ReplaceSpecialValues)


# pylint: disable=too-few-public-methods

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
    1.17e4: DRange(0.7, 2.5),
    2.94e4: DRange(2.1, 3.3),
    4.71e4: DRange(3.1, 4.3),
    6.48e4: DRange(4.1, 5.3),
    8.25e4: DRange(5.2, 6.2),
    10.02e4: DRange(6.2, 7.3),
    11.79e4: DRange(7.3, 8.3),
    13.55e4: DRange(8.3, 9.5),
    15.32e4: DRange(9.4, 10.6),
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

    def add_ws(self, workspace, d_range=None):
        """
        Takes in the given workspace and lists it alongside its time regime
        value.  If the time regime has yet to be created, it will create it,
        and if there is already a workspace listed beside the time regime, then
        the new ws will be appended to that list.

        @param workspace The workspace to add
        @param d_range Optionally override the dRange
        """

        # Get the time regime of the workspace, and use it to find the DRange.
        time_regime = workspace.dataX(0)[0]
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

            try:
                d_range = TIME_REGIME_TO_DRANGE[time_regimes[d_range]]
            except RuntimeError:
                raise RuntimeError("Supplied d-range, " + str(d_range) + ", is out of bounds.")

        logger.information('dRange for workspace %s is %s' % (workspace.getName(), str(d_range)))

        # Add the workspace to the map, alongside its DRange.
        if d_range not in self._map:
            self._map[d_range] = [workspace]
        else:

            # Check if x ranges match an existing run
            for ws in self._map[d_range]:
                map_lastx = ws.readX(0)[-1]
                ws_lastx = workspace.readX(0)[-1]

                # if it matches ignore it
                if map_lastx == ws_lastx:
                    return

            self._map[d_range].append(workspace)

    def average_across_dranges(self):
        """
        Averages workspaces which are mapped to the same drange,
        removing them from the map and mapping the averaged workspace
        to that drange.
        """
        temp_map = {}

        for d_range, ws_list in self._map.items():
            ws_list = rebin_to_smallest(*ws_list)
            temp_map[d_range] = average_ws_list(ws_list)

        self._map = temp_map

    def items(self):
        """
        :return: An iterator over key, value tuples in this d-range map.
        """
        return self._map.items()

    def keys(self):
        """
        :return: An iterator over the keys in this d-range map.
        """
        return self._map.keys()

    def values(self):
        """
        :return: An iterator over values in this d-range map.
        """
        return self._map.values()

    def __setitem__(self, d_range, ws_name):
        """
        Set a dRange and corresponding *single* ws.
        """
        self._map[d_range] = ws_name

    def __iter__(self):
        """
        :return: An iterator for this d-range map.
        """
        return self._map.__iter__()

    def __contains__(self, d_range):
        """
        Checks whether this d-range map contains the specified
        d-range.

        :param d_range: The d-range to check for.
        :return:        True if this d-range map contains the
                        specified d-range. False otherwise.
        """
        return d_range in self._map

    def __str__(self):
        str_output = "{\n"

        for d_range, workspaces in self._map.items():
            str_output += str(d_range) + ": " + str(workspaces) + "\n"
        return str_output + "}"


def average_ws_list(ws_list):
    """
    Calculates the average of a list of workspaces (not workspace names)
    - stores the result in a new workspace.

    :param ws_list: The list of workspaces to average.
    :return:        The name of the workspace containing the average.
    """
    # Assert we have some ws in the list, and if there is only one then return it.
    num_workspaces = len(ws_list)

    if num_workspaces == 0:
        raise RuntimeError("getAverageWs: Trying to take an average of nothing")

    if num_workspaces == 1:
        return ws_list[0]

    return sum(ws_list) / num_workspaces


def find_intersection_of_ranges(range_a, range_b):
    if range_a[0] >= range_a[1] or range_b[0] >= range_b[1]:
        raise RuntimeError("Malformed range")

    if range_a[0] <= range_a[1] <= range_b[0] <= range_b[1]:
        return
    if range_b[0] <= range_b[1] <= range_a[0] <= range_a[1]:
        return
    if range_a[0] <= range_b[0] <= range_b[1] <= range_a[1]:
        return range_b
    if range_b[0] <= range_a[0] <= range_a[1] <= range_b[1]:
        return range_a
    if range_a[0] <= range_b[0] <= range_a[1] <= range_b[1]:
        return [range_b[0], range_a[1]]
    if range_b[0] <= range_a[0] <= range_b[1] <= range_a[1]:
        return [range_a[0], range_b[1]]

    # Should never reach here
    raise RuntimeError()


def get_intersection_of_ranges(range_list):
    """
    Get the intersections of a list of ranges.  For example, given the ranges:
    [1, 3], [3, 5] and [4, 6], the intersections would be a single range of [4,
    5].

    NOTE: Assumes that no more than a maximum of two ranges will ever cross at
    the same point.  Also, all ranges should obey range[0] <= range[1].
    """
    # Find all combinations of ranges, and see where they intersect.
    range_combos = itertools.combinations(range_list, 2)

    # Retrieve all intersections
    intersections = (find_intersection_of_ranges(range_pair[0], range_pair[1])
                     for range_pair in range_combos)

    # Filter out None type intersections
    intersections = filter(lambda intersection: intersection is not None, intersections)

    # Return the sorted intersections.
    intersections = sorted(intersections)
    return intersections


def is_in_ranges(range_list, val):
    return any(arange[0] < val < arange[1] for arange in range_list)


def list_to_range(array_to_convert):
    """
    Converts a specified array to a range representation, based on the
    following specification -

    Array with one element          : Array is returned immediately
    Array with two or more elements : A range from the first to second element
                                      (inclusive) is returned.

    :param array_to_convert:  The array to convert to a range.
    :return:                The generated range.
    """
    if len(array_to_convert) == 1:
        return array_to_convert
    else:
        return range(array_to_convert[0], array_to_convert[1] + 1)


def rebin_to_smallest(*workspaces):
    """
    Rebins the specified list to the workspace with the smallest
    x-range in the list.

    :param workspaces: The list of workspaces to rebin to the smallest.
    :return:           The rebinned list of workspaces.
    """
    if len(workspaces) == 1:
        return workspaces

    smallest_idx, smallest_ws = \
        min(enumerate(workspaces), key=lambda x: x[1].blocksize())

    rebinned_workspaces = []
    for idx, workspace in enumerate(workspaces):

        # Check whether this is the workspace with the smallest x-range.
        # No reason to rebin workspace to match itself.
        # NOTE: In the future this may append workspace.clone() - this will
        # occur in the circumstance that the input files do not want to be
        # removed from the ADS.
        if idx == smallest_idx:
            rebinned_workspaces.append(workspace)
        else:
            rebinned_workspaces.append(RebinToWorkspace(WorkspaceToRebin=workspace,
                                                        WorkspaceToMatch=smallest_ws,
                                                        OutputWorkspace="rebinned",
                                                        StoreInADS=False, enableLogging=False))

    return rebinned_workspaces


# pylint: disable=no-init,too-many-instance-attributes
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
        return "This Python algorithm performs the operations necessary for the reduction of diffraction data " + \
               "from the Osiris instrument at ISIS " + \
               "into dSpacing, by correcting for the monitor and linking the various d-ranges together."

    def PyInit(self):
        runs_desc = 'The list of run numbers that are part of the sample run. ' + \
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
                             doc='Filename of the .cal file to use in the [[AlignDetectors]] and ' +
                                 '[[DiffractionFocussing]] child algorithms.')

        self.declareProperty('SpectraMin', 3, doc='Minimum Spectrum to Load from (Must be more than 3)')

        self.declareProperty('SpectraMax', 962, doc='Maximum Spectrum to Load from file (Must be less than 962)')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace. If no name is provided, " +
                                 "one will be generated based on the run numbers.")

        self.declareProperty(name='LoadLogFiles', defaultValue=True,
                             doc='Load log files when loading runs')

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
            self._man_d_range = self._parse_string_array(self.getProperty("DRange").value)
            self._man_d_range = [x - 1 for x in self._man_d_range]
            num_ranges = len(self._man_d_range)
            num_runs = len(self._sample_runs)

            if num_runs % num_ranges == 0:
                self._man_d_range = list(self._man_d_range * int(num_runs / num_ranges))
            elif num_runs != num_ranges:
                raise ValueError("Less D-Ranges supplied than Sample Runs. Expected " + str(num_runs)
                                 + ", Received " + str(num_ranges) + ".")

    def validateInputs(self):
        issues = dict()

        try:
            self._get_properties()
        except ValueError as exc:
            issues['DRange'] = str(exc)
        except RuntimeError as exc:
            issues['Sample'] = str(exc)

        num_samples = len(self._sample_runs)
        num_vanadium = len(self._vanadium_runs)

        if num_samples > num_vanadium:
            run_num_mismatch = 'You must input at least as many vanadium files as sample files'
            issues['Sample'] = run_num_mismatch
            issues['Vanadium'] = run_num_mismatch
        if self._container_files:
            num_containers = len(self._container_files)
            if num_samples != num_containers:
                issues['Container'] = 'You must input the same number of sample and container runs'

        return issues

    # pylint: disable=too-many-branches
    def PyExec(self):
        """
        Execute the algorithm in diffraction-only mode
        """

        # Load all sample, vanadium files
        ipf_file_name = 'OSIRIS_diffraction_diffonly_Parameters.xml'
        load_opts = {"DeleteMonitors": True}

        sample_ws_names, _ = load_files(self._sample_runs,
                                        ipf_file_name,
                                        self._spec_min,
                                        self._spec_max,
                                        load_logs=self._load_logs,
                                        load_opts=load_opts)

        vanadium_ws_names, _ = load_files(self._vanadium_runs,
                                          ipf_file_name,
                                          self._spec_min,
                                          self._spec_max,
                                          load_logs=self._load_logs,
                                          load_opts=load_opts)

        container_ws_names = []
        container_workspaces = []

        # Load the container run
        if self._container_files:
            container_ws_names, _ = load_files(self._container_files,
                                               ipf_file_name,
                                               self._spec_min,
                                               self._spec_max,
                                               load_logs=self._load_logs,
                                               load_opts=load_opts)

            # Scale the container run if required
            if self._container_scale_factor != 1.0:

                # Scale every container workspace
                for container_ws_name in container_ws_names:
                    container_workspaces.append(mtd[container_ws_name] * self._container_scale_factor)
            else:
                container_workspaces = container_ws_names

        # Add the sample workspaces to the dRange to sample map
        for idx, sample_ws_name in enumerate(sample_ws_names):

            if container_workspaces:
                rebinned_container = RebinToWorkspace(WorkspaceToRebin=container_workspaces[idx],
                                                      WorkspaceToMatch=sample_ws_name,
                                                      OutputWorkspace="rebinned_container",
                                                      StoreInADS=False, enableLogging=False)
                sample_ws = mtd[sample_ws_name] - rebinned_container
            else:
                sample_ws = mtd[sample_ws_name]

            if self._man_d_range is not None and idx < len(self._man_d_range):
                self._sam_ws_map.add_ws(sample_ws,
                                        self._man_d_range[idx])
            else:
                self._sam_ws_map.add_ws(sample_ws)

        self._delete_workspaces(container_ws_names)

        # Add the vanadium workspaces to the vanadium drange map
        self._add_to_drange_map(vanadium_ws_names, self._van_ws_map)

        # Check to make sure that there are corresponding vanadium files with the same DRange for each sample file.
        for d_range in self._sam_ws_map:
            if d_range not in self._van_ws_map:
                raise RuntimeError("There is no van file that covers the " + str(d_range) + " DRange.")

        # Average together any sample workspaces with the same DRange.
        # This will mean our map of DRanges to list of workspaces becomes a map
        # of DRanges, each to a *single* workspace.
        self._sam_ws_map.average_across_dranges()

        # Now do the same to the vanadium workspaces
        self._van_ws_map.average_across_dranges()

        # Run necessary algorithms on the Sample workspaces.
        self._calibrate_runs_in_map(self._sam_ws_map)

        # Run necessary algorithms on the Vanadium workspaces.
        self._calibrate_runs_in_map(self._van_ws_map)

        # Workspaces in vanadium map are no longer in the ADS - can safely delete
        # vanadium workspaces in ADS.
        self._delete_workspaces(vanadium_ws_names)

        # Divide all sample files by the corresponding vanadium files.
        divided = self._divide_all_by(self._sam_ws_map.values(), self._van_ws_map.values())

        # Workspaces must be added to the ADS, as there does not yet exist
        # a workspace list property (must be passed to merge runs by name).
        for sample_ws_name, sample_ws in zip(sample_ws_names,
                                             divided):
            mtd.addOrReplace(sample_ws_name, sample_ws)

        if len(divided) > 1:
            # Merge the sample files into one.
            output_ws = MergeRuns(InputWorkspaces=sample_ws_names,
                                  OutputWorkspace="merged_sample_runs",
                                  StoreInADS=False, enableLogging=False)
        else:
            output_ws = divided[0]

        # Sample workspaces are now finished with and can be deleted
        # safely from the ADS.
        self._delete_workspaces(sample_ws_names)

        mtd.addOrReplace(self._output_ws_name, output_ws)

        AddSampleLog(Workspace=output_ws, LogName="D-Ranges",
                     LogText="D-Ranges used for reduction: " + self.getPropertyValue("Drange"))

        result = mtd[self._output_ws_name]

        # Create scalar data to cope with where merge has combined overlapping data.
        intersections = get_intersection_of_ranges(self._sam_ws_map.keys())

        data_x = result.dataX(0)
        data_y = []
        data_e = []
        for i in range(0, len(data_x) - 1):
            x_val = (data_x[i] + data_x[i + 1]) / 2.0

            if is_in_ranges(intersections, x_val):
                data_y.append(2)
                data_e.append(2)
            else:
                data_y.append(1)
                data_e.append(1)

        # apply scalar data to result workspace
        for i in range(0, result.getNumberHistograms()):
            result_y = result.dataY(i)
            result_e = result.dataE(i)

            result_y = result_y / data_y
            result_e = result_e / data_e

            result.setY(i, result_y)
            result.setE(i, result_e)

        self.setProperty("OutputWorkspace", result)

    def _delete_workspaces(self, workspace_names):
        """
        Deletes the workspaces with the specified names, using the specified
        delete_set_property and delete_exec methods of a deleting algorithm.

        :param workspace_names:     The names of the workspaces to delete.
        :param delete_set_property: The setProperty method of the delete algorithm.
        :param delete_exec:         The execute method of the delete algorithm.
        """
        for workspace_name in workspace_names:

            if mtd.doesExist(workspace_name):
                DeleteWorkspace(workspace_name)

    def _add_to_drange_map(self, workspace_names, drange_map):
        """
        Adds the specified workspaces to the specified drange map.
        Attempts to add using the manually entered dranges where
        possible - if not, automatically finds the drange.

        :param workspaces:      The names of the workspaces to add to the drange map.
        :param drange_map:      The drange map to add the workspaces to.
        :param do_log_found:    If True print the found workspaces to log.
                                Else don't print to log.
        """
        if self._man_d_range is not None:
            drange_len = len(self._man_d_range)
        else:
            drange_len = None

        for idx, ws_name in enumerate(workspace_names):
            if drange_len is not None and idx < drange_len:
                drange_map.add_ws(mtd[ws_name], self._man_d_range[idx])
            else:
                drange_map.add_ws(mtd[ws_name])

    def _parse_string_array(self, string):
        """
        Parse a specified string into an array based on the following specification:

        a   : Add a to the array where a is an integer.
        a-b : Add the range 'a to b' to the array where a and b are integers.
        a,b : Add elements of a and b to the array where a and b are integers or ranges.

        Ignores whitespace.

        :param string:  The string to parse to an array.
        :return:        The array parsed from the string using the specification.
        """
        str_ranges = string.replace(" ", "").split(",")
        str_ranges = [x.split("-") for x in str_ranges]

        # Convert string ranges to integer ranges.
        try:
            int_ranges = [[int(x) for x in str_range] for str_range in str_ranges]
        except BaseException:
            raise ValueError('Provided list, "' + string + '", was incorrectly formatted\n'
                                                           '')

        # Expand integer ranges formed from a string 'a-b', to a range from a to b
        # Single provided integers remain the same
        int_ranges = map(list_to_range, int_ranges)

        # Return flattened list of range values
        return [range_value for int_range in int_ranges for range_value in int_range]

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

    def _calibrate_runs_in_map(self, drange_map):

        for d_range, wrksp in drange_map.items():
            normalised = NormalizeByCurrent(InputWorkspace=wrksp,
                                            OutputWorkspace="normalised_sample",
                                            StoreInADS=False, enableLogging=False)

            aligned = AlignDetectors(InputWorkspace=normalised,
                                     CalibrationFile=self._cal,
                                     OutputWorkspace="aligned_sample",
                                     StoreInADS=False, enableLogging=False)

            focussed = DiffractionFocussing(InputWorkspace=aligned,
                                            GroupingFileName=self._cal,
                                            OutputWorkspace="focussed_sample",
                                            StoreInADS=False, enableLogging=False)

            drange_map[d_range] = CropWorkspace(InputWorkspace=focussed,
                                                XMin=d_range[0], XMax=d_range[1],
                                                OutputWorkspace="calibrated_sample",
                                                StoreInADS=False, enableLogging=False)

    def _init_child_algorithm(self, algorithm_name, enable_logging=False):
        """
        Initializes the algorithm with the specified name as a child algorithm.
        The getProperty, setProperty and execute methods for this child algorithm
        are returned.

        :param algorithm_name:   The name of the algorithm to initialize.
        :param enable_logging:   If True, enables algorithm logging.
        :return:                getProperty, setProperty, execute methods of
                                the created child algorithm.
        """
        algorithm = self.createChildAlgorithm(algorithm_name, enable_logging)
        return algorithm.getProperty, algorithm.setProperty, algorithm.execute

    def _divide_all_by(self, dividend_workspaces, divisor_workspaces):
        """
        Divides the workspaces with the specified divided workspace names by
        the workspaces with the specified divisor workspace names.

        :param dividend_workspaces:   The workspaces to be divided.
        :param divisor_workspaces:    The workspaces to divide by.
        :return:                      A list of the resultant divided workspaces.
        """
        divided = []

        # Divide all dividend workspaces by the corresponding divisor workspaces.
        for dividend_ws, divisor_ws in zip(dividend_workspaces, divisor_workspaces):
            dividend_ws, divisor_ws = rebin_to_smallest(dividend_ws, divisor_ws)
            divided_ws = dividend_ws / divisor_ws
            divided.append(ReplaceSpecialValues(InputWorkspace=divided_ws,
                                                NaNValue=0.0, InfinityValue=0.0,
                                                OutputWorkspace="removed_special",
                                                StoreInADS=False, enableLogging=False))

        return divided


AlgorithmFactory.subscribe(OSIRISDiffractionReduction)
