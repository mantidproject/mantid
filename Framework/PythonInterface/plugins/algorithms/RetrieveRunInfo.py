# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-few-public-methods
from mantid.api import mtd, PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty
from mantid.simpleapi import (
    CreateEmptyTableWorkspace,
    CreateLogPropertyTable,
    DeleteWorkspace,
    Load,
    LoadRaw,
)
from mantid.kernel import StringMandatoryValidator, Direction
from mantid import config, FileFinder
import os
from itertools import filterfalse


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
