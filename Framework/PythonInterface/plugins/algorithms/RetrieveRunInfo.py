# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
from mantid.api import mtd, PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty
from mantid.simpleapi import (
    CloneWorkspace,
    CreateEmptyTableWorkspace,
    CreateLogPropertyTable,
    DeleteWorkspace,
    Load,
    LoadRaw,
    RenameWorkspace,
)
from mantid.kernel import StringMandatoryValidator, Direction
from mantid import config, FileFinder
import os
from itertools import filterfalse


class Intervals(object):
    # Having "*intervals" as a parameter instead of "intervals" allows us
    # to type "Intervals( (0,3), (6, 8) )" instead of "Intervals( ( (0,3), (6, 8) ) )"
    def __init__(self, *intervals):
        # Convert into a list, then back into intervals, to make
        # sure we have no overlapping intervals (which would result in
        # duplicate values.
        values = _intervalsToList(intervals)
        self._intervals = _listToIntervals(values)

    # Factory.
    @classmethod
    def fromString(cls, mystring):
        # Tokenise on commas.
        intervalTokens = mystring.split(",")

        # Call parseRange on each tokenised range.
        numbers = [_parseIntervalToken(intervalToken) for intervalToken in intervalTokens]

        # Chain the result (a list of lists) together to make one single list of unique values.
        result = list(set(itertools.chain.from_iterable(numbers)))

        # Construct a new Intervals object, populate its intervals, and return.
        newObj = cls()
        newObj._intervals = _listToIntervals(result)
        return newObj

    # Factory.
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
    def __contains__(self, ids):
        for interval in self._intervals:
            if interval[0] <= ids <= interval[1]:
                return True
        return False

    # So that we can type "groups = Intervals( (0, 3) ) + Intervals( (6, 10) )"
    def __add__(self, other):
        newObj = Intervals()
        newObj._intervals = self._intervals + other._intervals
        return newObj

    # TODO: At the moment this is just a generator.  Implement a proper iterator.
    # So that we can type "for i in Intervals( (0, 2), (4, 5) ):"
    def __iter__(self):
        for interval in self._intervals:
            for value in range(interval[0], interval[1] + 1):
                yield value

    # So we can type "interval = Intervals( (3, 5), (10, 12) )" and then "interval[3]" returns 10.
    def __getitem__(self, index):
        return self.getValues()[index]

    # Mainly for debugging.
    def __str__(self):
        strings = ["(" + str(interval[0]) + ", " + str(interval[1]) + ")" for interval in self._intervals]
        return ", ".join(strings)

    def __len__(self):
        return len(self.getValues())


# Given a list of workspaces, will sum them together into a single new workspace, with the given name.
# If no name is given, then one is constructed from the names of the given workspaces.


def sumWsList(wsList, summedWsName=None):
    if len(wsList) == 1:
        if summedWsName is not None:
            CloneWorkspace(InputWorkspace=wsList[0].name(), OutputWorkspace=summedWsName)
            return mtd[summedWsName]
        return wsList[0]

    sumws = wsList[0] + wsList[1]

    if len(wsList) > 2:
        for i in range(2, len(wsList) - 1):
            sumws += wsList[i]

    if summedWsName is None:
        summedWsName = "_PLUS_".join([ws.name() for ws in wsList])

    RenameWorkspace(InputWorkspace=sumws.name(), OutputWorkspace=summedWsName)

    return mtd[summedWsName]


# pylint: disable=too-few-public-methods


class FileBackedWsIterator(object):
    """An iterator to iterate over workspaces.  Each filename in the list
    provided is loaded into a workspace, validated by the given ws_validator,
    yielded, and then deleted from memory."""

    def __init__(self, filenames):
        """Constructor, takes in the list of filenames to load, who's
        workspaces will be iterated over."""
        # Validate.
        if not isinstance(filenames, list):
            raise TypeError("Expected a list.")
        if not all([self._is_string(s) for s in filenames]):
            raise TypeError("Expected a list of strings.")
        if len(filenames) < 1:
            raise ValueError("Expected at least one filename.")

        # In the general case, we may or may not have checked for the existance
        # of the files previously, so before we even start iterating throw if
        # any are missing.
        missing_files = list(filterfalse(os.path.exists, filenames))
        if len(missing_files) > 0:
            raise ValueError("One or more files are missing: " + str(missing_files))

        self._filenames = filenames
        self._loaded_ws = None

    def __iter__(self):
        """Method that allows this object to treated as an iterator."""
        for filename in self._filenames:
            # Delete the previously loaded ws, if one exists, then load the
            # new ws.
            self._delete_loaded_ws()
            try:
                self._load_into_ws(filename)
            except RuntimeError:
                raise RuntimeError('Problem loading file "' + filename + '"')

            # Yield the newly loaded ws.
            yield self._loaded_ws

        # Final tidy-up.
        self._delete_loaded_ws()

    def _is_string(self, obj):
        """Convenience method to test if an object is a string or not."""
        return isinstance(obj, str)

    def _load_into_ws(self, filename):
        """Load the given filename and return it.  Use LoadRaw for raw files,
        so we can turn LoadLogFiles off."""
        wsName = "__temp_" + filename

        dummy_base, ext = os.path.splitext(filename)
        if ext == ".raw":
            # Loading log files is extremely slow on archive
            LoadRaw(Filename=filename, OutputWorkspace=wsName, LoadLogFiles=False)
        else:
            Load(Filename=filename, OutputWorkspace=wsName)

        self._loaded_ws = mtd[wsName]

    def _delete_loaded_ws(self):
        """If there has been a file loaded into a ws, delete it."""
        if self._loaded_ws:
            DeleteWorkspace(Workspace=self._loaded_ws)


class RetrieveRunInfo(PythonAlgorithm):
    def category(self):
        return "DataHandling\\Catalog"

    def summary(self):
        return (
            "Given a range of run numbers and an output workspace name, will compile a table of info for "
            + "each run of the instrument you have set as default."
        )

    def seeAlso(self):
        return ["CreateLogPropertyTable"]

    def PyInit(self):
        # Declare algorithm properties.
        self.declareProperty("Runs", "", StringMandatoryValidator(), doc='The range of runs to retrieve the run info for. E.g. "100-105".')
        self.declareProperty(
            ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output),
            doc="""The name of the TableWorkspace that will be created.
                                     '''You must specify a name that does not already exist.''' """,
        )

    def PyExec(self):
        PROP_NAMES = ["inst_abrv", "run_number", "user_name", "run_title", "hd_dur"]

        # Not all ISIS run files have the relevant prop_names, but we may as
        # well limit to ISIS only runs at this stage.
        if config["default.facility"] != "ISIS":
            raise ValueError("Only ISIS runs are supported by this alg.")

        # Ensure workspace does not already exist.
        output_ws_name = self.getPropertyValue("OutputWorkspace")
        if mtd.doesExist(output_ws_name):
            raise ValueError('Workspace "' + output_ws_name + '" already ' "exists. Either delete it, or choose another workspace name.")

        # Check that all run files are available.
        run_string = self.getPropertyValue("Runs")
        try:
            filenames = list(FileFinder.findRuns(run_string))
        except RuntimeError as re:
            raise ValueError(str(re))

        # Set up the output ws table.
        CreateEmptyTableWorkspace(OutputWorkspace=output_ws_name)
        output_ws = mtd[output_ws_name]
        for prop_name in PROP_NAMES:
            output_ws.addColumn(name=prop_name, type="str")

        # Set up (and iterate over) a "file backed" iterator, which takes care
        # of loading files and then deleting the resulting workspaces in turn
        # when we are finished with them.
        ws_iter = FileBackedWsIterator(filenames)
        for ws in ws_iter:
            # Create a single row table for each file.
            temp_table_name = ws.name() + "_INFO"
            CreateLogPropertyTable(
                InputWorkspaces=ws.name(),
                LogPropertyNames=", ".join(PROP_NAMES),
                GroupPolicy="First",  # Include only the 1st child of any groups.
                OutputWorkspace=temp_table_name,
            )
            # Add its contents to the output before deleting it.
            temp_table = mtd[temp_table_name]
            output_ws.addRow(temp_table.row(0))
            DeleteWorkspace(Workspace=temp_table_name)

        self.setPropertyValue("OutputWorkspace", output_ws_name)


# Register algorthm with Mantid.
AlgorithmFactory.subscribe(RetrieveRunInfo)
