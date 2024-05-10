# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import itertools

from IndirectReductionCommon import calibrate, group_spectra, load_files, rebin_logarithmic

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (
    AddSampleLog,
    CloneWorkspace,
    CropWorkspace,
    DeleteWorkspace,
    Divide,
    MergeRuns,
    NormaliseByCurrent,
    RebinToWorkspace,
    ReplaceSpecialValues,
    ConjoinSpectra,
    CreateWorkspace,
)
import numpy as np

# pylint: disable=too-few-public-methods


def _str_or_none(string):
    return string if string != "" else None


class DRange(object):
    """
    A class to represent a dRange.
    """

    def __init__(self, lower, upper):
        self._range = [lower, upper]

    def __getitem__(self, idx):
        return self._range[idx]

    def __str__(self):
        return "%.3f - %.3f Angstrom" % (self._range[0], self._range[1])

    def __lt__(self, drange_map):
        return self._range[0] < drange_map[0]

    def __eq__(self, drange_map):
        return self._range[0] == drange_map[0] and self._range[1] == drange_map[1]

    def __hash__(self):
        return hash((self._range[0], self._range[1]))


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
    20.63e4: DRange(12.2, 13.8),
}


class DRangeToWorkspaceMap(object):
    """
    A "wrapper" class for a map, which maps workspaces from their corresponding
    time regimes.
    """

    def __init__(self):
        self._map = {}

    def add_workspaces(self, workspaces, combinator=None):
        """
        Takes in the given workspace ands lists them alongside their time regime
        value.  If the time regime has yet to be created, it will create it,
        and if there is already a workspace listed beside the time regime, then
        the new workspace will be appended to that list. If a combinator is specified,
        combines each list.

        @param workspaces   The workspaces to add
        @param combinator   The combinator to use in combining workspaces in the
                            same d-range
        """
        for workspace in workspaces:
            self.add_workspace(workspace)

        if combinator is not None:
            self._map = {
                d_range: CloneWorkspace(InputWorkspace=combinator(ws_list), OutputWorkspace="combined", StoreInADS=False)
                for d_range, ws_list in self._map.items()
            }

    def add_workspace(self, workspace):
        """
        Takes in the given workspace and lists it alongside its time regime
        value.  If the time regime has yet to be created, it will create it,
        and if there is already a workspace listed beside the time regime, then
        the new ws will be appended to that list.

        @param workspace The workspace to add
        """

        # Get the time regime of the workspace, and use it to find the DRange.
        d_range = self._d_range_of(workspace)

        logger.information("dRange for workspace %s is %s" % (workspace.name(), str(d_range)))

        # Add the workspace to the map, alongside its DRange.
        if d_range not in self._map:
            self._map[d_range] = [workspace]
        else:
            self._map[d_range].append(workspace)

    def transform(self, morphism):
        """
        Transforms this DRange Map, by applying the specified morphism to
        each key-value pair in the map.

        :param morphism:    The morphism to apply.
        """
        self._map = {d_range: morphism(d_range, workspace) for d_range, workspace in self._map.items()}

    def combine(self, d_range_map, combinator):
        """
        Creates a new DRangeToWorkspaceMap by combining this map with the
        specified map, using the specified combinator.

        :param d_range_map: The map to combine with this.
        :param combinator:  The combinator specifying how to combine.
        :return:            The combined map.
        """
        combined_map = DRangeToWorkspaceMap()

        for d_range in self.keys():
            if d_range in d_range_map:
                combined_map[d_range] = combinator(self._map[d_range], d_range_map[d_range])
        return combined_map

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

    def __setitem__(self, d_range, workspace):
        """
        Set a dRange and corresponding *single* ws.
        """
        self._map[d_range] = workspace

    def __getitem__(self, d_range):
        """
        Returns the workspace/list of workspaces corresponding to the specified d-range.
        :param d_range: The d-range.
        :return:        The workspace/list of workspaces corresponding to the specified d-range.
        """
        return self._map[d_range]

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

    def __len__(self):
        return len(self._map)

    def __str__(self):
        str_output = "{\n"

        for d_range, workspaces in self._map.items():
            str_output += str(d_range) + ": " + str(workspaces) + "\n"
        return str_output + "}"

    def _d_range_of(self, workspace):
        return TIME_REGIME_TO_DRANGE[self._time_regime_of(workspace)]

    def _time_regime_of(self, workspace):
        time_regime = workspace.dataX(0)[0]
        time_regimes = sorted(TIME_REGIME_TO_DRANGE.keys())
        num_regimes = len(time_regimes)

        for idx in range(num_regimes):
            if idx == num_regimes - 1:
                if time_regimes[idx] < time_regime:
                    return time_regimes[idx]
            else:
                if time_regimes[idx] < time_regime < time_regimes[idx + 1]:
                    return time_regimes[idx]
        return time_regime


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
    intersections = (find_intersection_of_ranges(range_pair[0], range_pair[1]) for range_pair in range_combos)

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

    smallest_idx, smallest_ws = min(enumerate(workspaces), key=lambda x: x[1].blocksize())

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
            rebinned_workspaces.append(
                RebinToWorkspace(
                    WorkspaceToRebin=workspace,
                    WorkspaceToMatch=smallest_ws,
                    OutputWorkspace="rebinned",
                    StoreInADS=False,
                    EnableLogging=False,
                )
            )

    return rebinned_workspaces


def rebin_and_subtract(minuend_workspace, subtrahend_workspace):
    """
    Rebins the subtrahend workspace to match the minuend workspace and
    then subtracts the subtrahend workspace from the minuend workspace.

    :param minuend_workspace:       The workspace to subtract from.
    :param subtrahend_workspace:    The workspace to be subtracted.
    :return:                        The minuend workspace - the subtrahend workspace.
    """
    return minuend_workspace - RebinToWorkspace(
        WorkspaceToRebin=subtrahend_workspace,
        WorkspaceToMatch=minuend_workspace,
        OutputWorkspace="rebinned_container",
        StoreInADS=False,
        EnableLogging=False,
    )


def divide_workspace(dividend_workspace, divisor_workspace):
    """
    Divides the specified dividend workspace by the specified divisor workspace.
    Replaces Infinity and NaNValues with 0.

    :param dividend_workspace: The workspace to be divided.
    :param divisor_workspace:  The workspace to divide by.
    :return:                   The dividend workspace / the divisor workspace.
    """
    dividend_ws, divisor_ws = rebin_to_smallest(dividend_workspace, divisor_workspace)
    divided_ws = Divide(LHSWorkspace=dividend_ws, RHSWorkspace=divisor_ws, OutputWorkspace="divided", StoreInADS=False, EnableLogging=False)
    return ReplaceSpecialValues(
        InputWorkspace=divided_ws, NaNValue=0.0, InfinityValue=0.0, OutputWorkspace="removed_special", StoreInADS=False, EnableLogging=False
    )


def rebin_and_sum(workspaces):
    """
    Rebins the specified list to the workspace with the smallest
    x-range in the list then sums all the workspaces together.

    :param workspaces: The list of workspaces to rebin to the smallest.
    :return:           The summed and rebinned workspace
    """
    # sum will add together the proton charge in the sample logs
    # so when the NormalizeByCurrent is called we get an average of the workspaces
    return sum(rebin_to_smallest(*workspaces))


def delete_workspaces(workspace_names):
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


def create_loader(ipf_filename, minimum_spectrum, maximum_spectrum, load_logs, load_opts):
    """
    Creates a loader which loads a supplied string containing the runs to load.

    :param ipf_filename:        The name of the instrument parameter file.
    :param minimum_spectrum:    The minimum spectrum to load.
    :param maximum_spectrum:    The maximum spectrum to load.
    :param load_logs:           True if logs are to be loaded, false otherwise.
    :param load_opts:           The additional load options.
    :return:                    A loader.
    """

    def loader(runs):
        return load_files(runs, ipf_filename, minimum_spectrum, maximum_spectrum, load_logs=load_logs, load_opts=load_opts)

    return loader


def create_drange_map_generator(runs_loader, combinator, delete_runs=True):
    """
    Creates a drange map generator, which creates a drange map from a supplied string of runs.

    :param runs_loader: The loader to use in loading the runs.
    :param combinator:  The combinator used to combine the runs.
    :param delete_runs: If true, deletes the loaded runs after addition to the drange map.
    :return:            A drange map generator.
    """

    def generator(runs, transform):
        drange_map = DRangeToWorkspaceMap()
        workspace_names, _, _ = runs_loader(runs)
        drange_map.add_workspaces([transform(workspace_name) for workspace_name in workspace_names], combinator)
        if delete_runs:
            delete_workspaces(workspace_names)
        return drange_map

    return generator


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
    _con_ws_map = None
    _van_ws_map = None
    _load_logs = None
    _spec_min = None
    _spec_max = None

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["ISISIndirectDiffractionReduction"]

    def summary(self):
        return (
            "This Python algorithm performs the operations necessary for the reduction of diffraction data "
            + "from the Osiris instrument at ISIS "
            + "into dSpacing, by correcting for the monitor and linking the various d-ranges together."
        )

    def PyInit(self):
        runs_desc = (
            "The list of run numbers that are part of the sample run. "
            + "There should be five of these in most cases. Enter them as comma separated values."
        )

        self.declareProperty(StringArrayProperty("Sample"), doc=runs_desc)

        self.declareProperty(StringArrayProperty("Vanadium"), doc=runs_desc)

        self.declareProperty(StringArrayProperty("Container"), doc=runs_desc)

        self.declareProperty("ContainerScaleFactor", 1.0, doc="Factor by which to scale the container")

        self.declareProperty(
            FileProperty("CalFile", "", action=FileAction.Load),
            doc="Filename of the .cal file to use in the [[ApplyDiffCal]] and [[ConvertUnits]] child algorithms.",
        )

        self.declareProperty("SpectraMin", 3, doc="Minimum Spectrum to Load from (Must be more than 3)")

        self.declareProperty("SpectraMax", 962, doc="Maximum Spectrum to Load from file (Must be less than 962)")

        self.declareProperty(
            name="GroupingMethod",
            defaultValue="All",
            validator=StringListValidator(["All", "File", "Custom", "Groups"]),
            doc="The method used to group detectors.",
        )
        self.declareProperty(name="GroupingString", defaultValue="", direction=Direction.Input, doc="Detectors to group as a string")
        self.declareProperty(
            FileProperty("GroupingFile", "", action=FileAction.OptionalLoad, extensions=[".map"]),
            doc="A file containing a detector grouping.",
        )
        self.declareProperty(
            name="NGroups",
            defaultValue=1,
            validator=IntBoundedValidator(lower=1),
            direction=Direction.Input,
            doc="The number of groups for grouping the detectors.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output),
            doc="Name to give the output workspace. If no name is provided, " + "one will be generated based on the run numbers.",
        )

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Load log files when loading runs")

        self._cal = None
        self._output_ws_name = None

    def _get_properties(self):
        self._load_logs = self.getProperty("LoadLogFiles").value
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

        self._grouping_method = self.getPropertyValue("GroupingMethod")
        self._grouping_string = _str_or_none(self.getPropertyValue("GroupingString"))
        self._grouping_file = _str_or_none(self.getPropertyValue("GroupingFile"))
        self._number_of_groups = self.getProperty("NGroups").value

    def validateInputs(self):
        issues = dict()

        try:
            self._get_properties()
        except ValueError as exc:
            issues["DRange"] = str(exc)
        except RuntimeError as exc:
            issues["Sample"] = str(exc)

        return issues

    # pylint: disable=too-many-branches
    def PyExec(self):
        """
        Execute the algorithm in diffraction-only mode
        """

        # Load all sample, vanadium files
        ipf_file_name = "OSIRIS_diffraction_diffonly_Parameters.xml"
        load_opts = {"DeleteMonitors": True}
        load_runs = create_loader(ipf_file_name, self._spec_min, self._spec_max, self._load_logs, load_opts)
        drange_map_generator = create_drange_map_generator(load_runs, rebin_and_sum)
        run_numbers = self._parse_run_numbers(self._sample_runs)
        # Create a sample drange map from the sample runs
        self._sam_ws_map = drange_map_generator(self._sample_runs, lambda name: mtd[name])

        # Create a vanadium drange map from the sample runs
        self._van_ws_map = drange_map_generator(self._vanadium_runs, lambda name: mtd[name])

        # Load the container run
        if self._container_files:
            # Scale the container run if required
            if self._container_scale_factor != 1.0:
                self._con_ws_map = drange_map_generator(self._container_files, lambda name: mtd[name] * self._container_scale_factor)
            else:
                self._con_ws_map = drange_map_generator(self._container_files, lambda name: mtd[name])

            result_map = self._sam_ws_map.combine(self._con_ws_map, rebin_and_subtract)
        else:
            result_map = self._sam_ws_map

        calibrator = self._diffraction_calibrator()

        # Calibrate the Sample workspaces.
        result_map.transform(calibrator)

        # Calibrate the Vanadium workspaces.
        self._van_ws_map.transform(calibrator)

        # Divide all sample files by the corresponding vanadium files.
        result_map = result_map.combine(self._van_ws_map, divide_workspace)

        if len(result_map) > 1:
            extracted_spectra = [self._extract_ws_spectra(ws, str(drange)) for drange, ws in result_map.items()]

            have_same_spectra_count = len({len(spectra) for spectra in extracted_spectra}) == 1
            if not have_same_spectra_count:
                logger.warning("Cannot merge focussed workspaces with different number of spectra")
                output_ws = [ws for ws in result_map.values()][0]
            else:
                # Group workspaces located at the same index
                matched_spectra = [list(spectra) for spectra in zip(*extracted_spectra)]
                # Merge workspaces located at the same index
                merged_spectra = [
                    MergeRuns(InputWorkspaces=spectra, OutputWorkspace=self._output_ws_name + f"_merged_{idx}")
                    for idx, spectra in enumerate(matched_spectra)
                ]

                max_x_size = max([spectra.dataX(0).size for spectra in merged_spectra])
                max_y_size = max([spectra.dataY(0).size for spectra in merged_spectra])
                max_e_size = max([spectra.dataE(0).size for spectra in merged_spectra])

                for i in range(len(merged_spectra)):
                    if (
                        merged_spectra[i].dataX(0).size != max_x_size
                        or merged_spectra[i].dataY(0).size != max_y_size
                        or merged_spectra[i].dataE(0).size != max_e_size
                    ):
                        dataX = merged_spectra[i].dataX(0)
                        dataY = merged_spectra[i].dataY(0)
                        dataE = merged_spectra[i].dataE(0)

                        dataX = np.append(dataX, [dataX[-1]] * (max_x_size - dataX.size))
                        dataY = np.append(dataY, [0] * (max_y_size - dataY.size))
                        dataE = np.append(dataE, [0] * (max_e_size - dataE.size))

                        merged_spectra[i] = CreateWorkspace(
                            NSpec=1,
                            DataX=dataX,
                            DataY=dataY,
                            DataE=dataE,
                            UnitX=merged_spectra[i].getAxis(0).getUnit().unitID(),
                            Distribution=merged_spectra[i].isDistribution(),
                            ParentWorkspace=merged_spectra[i].name(),
                            OutputWorkspace=merged_spectra[i].name(),
                        )

                input_workspaces_str = ",".join([ws.name() for ws in merged_spectra])
                ConjoinSpectra(InputWorkspaces=input_workspaces_str, OutputWorkspace=self._output_ws_name)
                output_ws = mtd[self._output_ws_name]

                delete_workspaces([ws.name() for group in matched_spectra for ws in group])
                delete_workspaces([ws.name() for ws in merged_spectra])

        elif len(result_map) == 1:
            output_ws = list(result_map.values())[0]
        else:
            error_msg = (
                "D-Ranges found in runs have no overlap:\n"
                + "Found Sample D-Ranges: "
                + ", ".join(map(str, self._sam_ws_map.keys()))
                + "\n"
                + "Found Vanadium D-Ranges: "
                + ", ".join(map(str, self._van_ws_map.keys()))
            )

            if self._container_files:
                error_msg += "\nFound Container D-Ranges: " + ", ".join(map(str, self._con_ws_map.keys()))
            raise RuntimeError(error_msg)

        if self._output_ws_name:
            mtd.addOrReplace(self._output_ws_name, output_ws)

        d_ranges = sorted(result_map.keys())
        AddSampleLog(Workspace=output_ws, LogName="D-Ranges", LogText=", ".join(map(str, d_ranges)))
        if run_numbers:
            AddSampleLog(Workspace=output_ws, LogName="run_number", LogText=run_numbers)

        # Create scalar data to cope with where merge has combined overlapping data.
        intersections = get_intersection_of_ranges(d_ranges)

        # apply scalar data to result workspace
        for i in range(0, output_ws.getNumberHistograms()):
            data_x = output_ws.dataX(i)
            data_y = []
            data_e = []
            for j in range(0, len(data_x) - 1):
                x_val = (data_x[j] + data_x[j + 1]) / 2.0

                if is_in_ranges(intersections, x_val):
                    data_y.append(2)
                    data_e.append(2)
                else:
                    data_y.append(1)
                    data_e.append(1)

            result_y = output_ws.dataY(i)
            result_e = output_ws.dataE(i)

            result_y = result_y / data_y
            result_e = result_e / data_e

            output_ws.setY(i, result_y)
            output_ws.setE(i, result_e)

        self.setProperty("OutputWorkspace", output_ws)

    def _diffraction_calibrator(self):
        """
        Creates a diffraction calibrator, which takes a DRange and a workspace
        and returns a calibrated workspace.

        :return: A diffraction calibrator.
        """

        def calibrator(d_range, workspace):
            normalised = NormaliseByCurrent(InputWorkspace=workspace, StoreInADS=False, EnableLogging=False)

            calibrated = calibrate(normalised, self._cal)

            rebinned = rebin_logarithmic(calibrated, self._cal)

            grouped = group_spectra(
                rebinned,
                method=self._grouping_method,
                group_file=self._grouping_file,
                group_string=self._grouping_string,
                number_of_groups=self._number_of_groups,
                spectra_range=[int(self._spec_min), int(self._spec_max)],
            )

            return CropWorkspace(
                InputWorkspace=grouped,
                XMin=d_range[0],
                XMax=d_range[1],
                OutputWorkspace="calibrated_sample",
                StoreInADS=False,
                EnableLogging=False,
            )

        return calibrator

    def _extract_ws_spectra(self, ws_to_split, drange):
        """
        Extracts individual spectra from the workspace into a list of workspaces. Each workspace will contain
        one of the spectra from the workspace and will have the form of "<ws_name>_<spectrum number>".
        :param ws_to_split: The workspace to split into individual workspaces
        :return: A list of extracted workspaces - one per spectra in the original workspace
        """

        num_spectra = ws_to_split.getNumberHistograms()
        spectra_bank_list = []
        for i in range(0, num_spectra):
            output_name = drange + f"_{i + 1}"
            # Have to use crop workspace as extract single spectrum struggles with the variable bin widths
            spectra_bank_list.append(
                CropWorkspace(InputWorkspace=ws_to_split, OutputWorkspace=output_name, StartWorkspaceIndex=i, EndWorkspaceIndex=i)
            )
        return spectra_bank_list

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
            raise ValueError('Provided list, "' + string + '", was incorrectly formatted\n')

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

    def _parse_run_numbers(self, sample_runs):
        """
        Matches run numbers used in the reduction from the filenames

        @param sample_runs A string of run numbers to find
        @returns A list of run_numbers
        """
        import re

        run_numbers = ""
        for sample in sample_runs:
            match = re.search(r"OSIRIS(\d+).", sample)
            if match:
                number = match.groups()[0][2:] if match.groups()[0].startswith("00") else match.groups()[0]
                run_numbers = run_numbers + "," + number
            else:
                return ""
        return run_numbers[1:]


AlgorithmFactory.subscribe(OSIRISDiffractionReduction)
