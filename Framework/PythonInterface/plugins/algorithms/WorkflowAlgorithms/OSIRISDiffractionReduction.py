from __future__ import (absolute_import, division, print_function)

import itertools

from IndirectReductionCommon import load_files

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


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

    def add_ws(self, ws_name, d_range=None):
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

            # Check if x ranges match an existing run
            for ws_name in self._map[d_range]:
                map_lastx = mtd[ws_name].readX(0)[-1]
                ws_lastx = wrksp.readX(0)[-1]

                # if it matches ignore it
                if map_lastx == ws_lastx:
                    DeleteWorkspace(wrksp)
                    return

            self._map[d_range].append(ws_name)

    def average_across_dranges(self):
        """
        Averages workspaces which are mapped to the same drange,
        removing them from the map and mapping the averaged workspace
        to that drange.
        """
        temp_map = {}

        for d_range, ws_list in self._map.items():
            ws_list = rebin_to_smallest(ws_list)
            temp_map[d_range] = average_ws_list(ws_list)

        self._map = temp_map

    def set_item(self, d_range, ws_name):
        """
        Set a dRange and corresponding *single* ws.
        """
        self._map[d_range] = ws_name

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


def average_ws_list(ws_list):
    """
    Calculates the average of a list of workspaces - stores the result in a new workspace.

    :param ws_list: The list of workspaces to average.
    :return:        The name of the workspace containing the average.
    """
    # Assert we have some ws in the list, and if there is only one then return it.
    if len(ws_list) == 0:
        raise RuntimeError("getAverageWs: Trying to take an average of nothing")

    if len(ws_list) == 1:
        return ws_list[0]

    # Generate the final name of the averaged workspace.
    av_name = "avg" + "_" + "_".join(ws_list)

    num_workspaces = len(ws_list)

    # Compute the average and put into "__temp_avg".
    temperatures = map(lambda index: mtd[ws_list[index]], range(1, num_workspaces))
    __temp_avg = sum(temperatures)
    __temp_avg /= num_workspaces

    # Rename the average ws and return it.
    RenameWorkspace(InputWorkspace=__temp_avg, OutputWorkspace=av_name)
    return av_name.getName()


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


def get_intersection_of_ranges(range_list):
    """
    Get the intersections of a list of ranges.  For example, given the ranges:
    [1, 3], [3, 5] and [4, 6], the intersections would be a single range of [4,
    5].

    NOTE: Assumes that no more than a maximum of two ranges will ever cross at
    the same point.  Also, all ranges should obey range[0] <= range[1].
    """
    # Find all combinations of ranges, and see where they intersect.
    range_combos = list(itertools.combinations(range_list, 2))

    # Retrieve all intersections
    intersections = map(lambda range_pair: find_intersection_of_ranges(range_pair[0], range_pair[1]),
                        range_combos)

    # Filter out None type intersections
    intersections = filter(lambda intersection: intersection is not None, intersections)

    # Return the sorted intersections.
    intersections.sort()
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


def rebin_to_smallest(workspaces):
    """
    Rebins the specified list to the workspace with the smallest
    x-range in the list.

    :param workspaces: The list of workspaces to rebin to the smallest.
    :return:           The rebinned list of workspaces.
    """
    smallest_ws = min(workspaces, key=lambda ws: mtd[ws].blocksize())

    return [RebinToWorkspace(
        WorkspaceToRebin=ws,
        WorkspaceToMatch=smallest_ws,
        OutputWorkspace=ws
    ).getName() for ws in workspaces]


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

        self.declareProperty('DetectDRange', True,
                             doc='Disable to override automatic dRange detection')

        # Note that dRange numbers are offset to match the numbering in the OSIRIS manual
        # http://www.isis.stfc.ac.uk/instruments/osiris/documents/osiris-user-guide6672.pdf
        self.declareProperty('DRange', defaultValue="", doc='Dranges to use when DetectDRange is disabled')

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

        # Check whether to manually override d-ranges.
        try:
            if not self.getProperty("DetectDRange").value:
                self._man_d_range = self._parse_string_array(self.getProperty("DRange").value)
                self._man_d_range = [x - 1 for x in self._man_d_range]
        except BaseException as exc:
            self.log().error(str(exc))

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

    # pylint: disable=too-many-branches
    def PyExec(self):
        """
        Execute the algorithm in diffraction-only mode
        """

        # Load all sample, vanadium files
        ipf_file_name = 'OSIRIS_diffraction_diffonly_Parameters.xml'
        sample_ws_names, _ = load_files(self._sample_runs,
                                        ipf_file_name,
                                        self._spec_min,
                                        self._spec_max,
                                        load_logs=self._load_logs)
        vanadium_ws_names, _ = load_files(self._vanadium_runs,
                                          ipf_file_name,
                                          self._spec_min,
                                          self._spec_max,
                                          load_logs=self._load_logs)
        container_ws_names = []

        # Load the container run
        if self._container_files:
            container_ws_names, _ = load_files(self._container_files,
                                               ipf_file_name,
                                               self._spec_min,
                                               self._spec_max,
                                               load_logs=self._load_logs)

            # Scale the container run if required
            if self._container_scale_factor != 1.0:

                # Retrieve function pointers in advance to avoid expensive hash
                # function on each loop iteration (improves performance)
                scale_get_property, scale_set_property, scale_exec = self._init_child_algorithm("Scale")
                scale_set_property("Operator", "Multiply")

                # Scale every container workspace
                for container_ws_name in container_ws_names:
                    scale_set_property("InputWorkspace", container_ws_name)
                    scale_set_property("OutputWorkspace", container_ws_name)
                    scale_set_property("Factor", self._container_scale_factor)
                    scale_exec()
                    mtd.addOrReplace(container_ws_name, scale_get_property("OutputWorkspace").value)

        # Initialize rebin algorithm and retrieve function pointers to improve performance
        rebin_get_property, rebin_set_property, rebin_exec \
            = self._init_child_algorithm("RebinToWorkspace")

        # Intialize minus algorithm and retrieve function pointers to improve performance
        minus_get_property, minus_set_property, minus_exec \
            = self._init_child_algorithm("Minus")

        # Add the sample workspaces to the dRange to sample map
        for idx, sample_ws_name in enumerate(sample_ws_names):
            sample_ws = sample_ws_name

            if container_ws_names:
                rebin_set_property("WorkspaceToRebin", container_ws_names[idx])
                rebin_set_property("WorkspaceToMatch", sample_ws)
                rebin_set_property("OutputWorkspace".container_ws_names[idx])
                rebin_exec()

                minus_set_property("LHSWorkspace", sample_ws)
                minus_set_property("RHSWorkspace", rebin_get_property("OutputWorkspace").value)
                minus_set_property("OutputWorkspace", sample_ws)
                minus_exec()
                mtd.addOrReplace(sample_ws_name, minus_get_property("OutputWorkspace").value)

            if self._man_d_range is not None and idx < len(self._man_d_range):
                self._sam_ws_map.add_ws(sample_ws_name, self._man_d_range[idx])
            else:
                self._sam_ws_map.add_ws(sample_ws_name)

        # Add the vanadium workspaces to the dRange to vanadium map
        self._add_to_drange_map(vanadium_ws_names, self._van_ws_map)

        # Create delete workspace algorithm and retrieve function pointers to improve performance
        _, delete_set_property, delete_exec \
            = self._init_child_algorithm("DeleteWorkspace")

        # Finished with container workspaces - delete them
        for container in container_ws_names:
            delete_set_property("Workspace", container)
            delete_exec()
            # Delete monitors
            delete_set_property("Workspace", container + "_mon")
            delete_exec()

        # Check to make sure that there are corresponding vanadium files with the same DRange for each sample file.
        for d_range in self._sam_ws_map:
            if d_range not in self._van_ws_map:
                raise RuntimeError("There is no van file that covers the " + str(d_range) + " DRange.")

        # Average together any sample workspaces with the same DRange.
        # This will mean our map of DRanges to list of workspaces becomes a map
        # of DRanges, each to a *single* workspace.
        self._sam_ws_map.average_across_dranges()

        # Now do the same to the vanadium workspaces.
        self._van_ws_map.average_across_dranges()

        # Create NormaliseByCurrent algorithm and retrieve function pointers to improve performance
        normalise_get_property, normalise_set_property, normalise_exec \
            = self._init_child_algorithm("NormaliseByCurrent")

        # Create AlignDetectors algorithm and retrieve function pointers to improve performance
        align_get_property, align_set_property, align_exec \
            = self._init_child_algorithm("AlignDetectors")

        # Create DiffractionFocussing algorithm and retrieve function pointers to improve performance
        diff_focus_get_property, diff_focus_set_property, diff_focus_exec \
            = self._init_child_algorithm("DiffractionFocussing")

        # Create CropWorkspace algorithm and retrieve function pointers to improve performance
        crop_get_property, crop_set_property, crop_exec \
            = self._init_child_algorithm("CropWorkspace")

        # Run necessary algorithms on BOTH the Vanadium and Sample workspaces.
        for d_range, wrksp in itertools.chain(self._sam_ws_map.items(), self._van_ws_map.items()):
            output_ws = wrksp
            normalise_set_property("InputWorkspace", wrksp)
            normalise_set_property("OutputWorkspace", output_ws)
            normalise_exec()

            align_set_property("InputWorkspace", normalise_get_property("OutputWorkspace").value)
            align_set_property("OutputWorkspace", output_ws)
            align_set_property("CalibrationFile", self._cal)
            align_exec()

            diff_focus_set_property("InputWorkspace", align_get_property("OutputWorkspace").value)
            diff_focus_set_property("OutputWorkspace", output_ws)
            diff_focus_set_property("GroupingFileName", self._cal)
            diff_focus_exec()

            crop_set_property("InputWorkspace", diff_focus_get_property("OutputWorkspace").value)
            crop_set_property("OutputWorkspace", output_ws)
            crop_set_property("XMin", d_range[0])
            crop_set_property("XMax", d_range[1])
            crop_exec()

            mtd.addOrReplace(wrksp, crop_get_property("OutputWorkspace").value)

        # Divide all sample files by the corresponding vanadium files.
        self._divide_all_by(self._sam_ws_map.values(), self._van_ws_map.values())

        # Create a list of sample workspace NAMES, since we need this for MergeRuns.
        sam_ws_names_list = list(self._sam_ws_map.values())

        if len(sam_ws_names_list) > 1:

            # Merge the sample files into one.
            merge_runs_alg = self.createChildAlgorithm("MergeRuns", enableLogging=False)
            merge_runs_alg.setProperty("InputWorkspaces", sam_ws_names_list)
            merge_runs_alg.setProperty("OutputWorkspace", self._output_ws_name)
            merge_runs_alg.execute()
            self._output_ws_name = merge_runs_alg.getPropertyValue("OutputWorkspace")
            mtd.addOrReplace(output_ws, merge_runs_alg.getProperty("OutputWorkspace").value)

            for name in sam_ws_names_list:
                delete_set_property("Workspace", name)
                delete_exec()

                # Delete monitor workspaces
                delete_set_property("Workspace", name + "_mon")
                delete_exec()
        else:
            rename_alg = self.createChildAlgorithm("RenameWorkspace", enableLogging=False)
            rename_alg.setProperty("InputWorkspace", sam_ws_names_list[0])
            rename_alg.setProperty("OutputWorkspace", self._output_ws_name)

        result = mtd[self._output_ws_name]

        # Create scalar data to cope with where merge has combined overlapping data.
        intersections = get_intersection_of_ranges(list(self._sam_ws_map.keys()))

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

        # Delete all workspaces we've created, except the result.
        for wrksp in self._van_ws_map.values():
            delete_set_property("Workspace", wrksp)
            delete_exec()

            # Delete monitor workspaces
            delete_set_property("Workspace", wrksp + "_mon")
            delete_exec()

        self.setProperty("OutputWorkspace", result)

    def _add_to_drange_map(self, workspaces, drange_map):
        """
        Adds the specified workspaces to the specified drange map.
        Attempts to add using the manually entered dranges where
        possible - if not, automatically finds the drange.

        :param workspaces:      The workspaces to add to the drange map.
        :param drange_map:      The drange map to add the workspaces to.
        :param do_log_found:    If True print the found workspaces to log.
                                Else don't print to log.
        """

        for idx, ws in enumerate(workspaces):
            if self._man_d_range is not None and \
                            idx < len(self._man_d_range):
                drange_map.add_ws(ws, self._man_d_range[idx])
            else:
                drange_map.add_ws(ws)

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
            int_ranges = map(lambda str_range: [int(x) for x in str_range], str_ranges)
        except RuntimeError:
            raise ValueError("Provided d-range was incorrectly formatted.")

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

    def _divide_all_by(self, dividend_ws_names, divisor_ws_names):
        """
        Divides the workspaces with the specified divided workspace names by
        the workspaces with the specified divisor workspace names.

        :param dividend_ws_names:   The names of the workspaces to be divided.
        :param divisor_ws_names:    The names of the workspaces to divide by.
        """

        divide_get_property, divide_set_property, divide_exec \
            = self._init_child_algorithm("Divide")

        replace_special_get_property, replace_special_set_property, replace_special_exec \
            = self._init_child_algorithm("ReplaceSpecialValues")

        # Divide all dividend workspaces by the corresponding divisor workspaces.
        for dividend_ws_name, divisor_ws_name in zip(dividend_ws_names, divisor_ws_names):
            ws_list = rebin_to_smallest([dividend_ws_name, divisor_ws_name])
            dividend_ws_name, divisor_ws_name = ws_list[0], ws_list[1]

            divide_set_property("LHSWorkspace", dividend_ws_name)
            divide_set_property("RHSWorkspace", divisor_ws_name)
            divide_set_property("OutputWorkspace", dividend_ws_name)
            divide_exec()

            replace_special_set_property("InputWorkspace", divide_get_property("OutputWorkspace").value)
            replace_special_set_property("OutputWorkspace", dividend_ws_name)
            replace_special_set_property("NaNValue", 0.0)
            replace_special_set_property("InfinityValue", 0.0)
            replace_special_exec()
            mtd.addOrReplace(dividend_ws_name, replace_special_get_property("OutputWorkspace").value)


AlgorithmFactory.subscribe(OSIRISDiffractionReduction)
