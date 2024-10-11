# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import config, Direction

import os

WKSP_PROP = "OutputWorkspace"
INST_PAR_PROP = "InstrumentParFile"

# Instrument parameter headers. Key in the dictionary is the number of columns in the file
IP_HEADERS = {5: "spectrum,theta,t0,-,R", 6: "spectrum,-,theta,t0,-,R"}

# Child Algorithm logging
_LOGGING_ = False

# ----------------------------------------------------------------------------------------


class LoadEmptyVesuvio(PythonAlgorithm):
    def summary(self):
        return "Loads an empty workspace containing the Vesuvio instrument at ISIS."

    # ----------------------------------------------------------------------------------------

    def category(self):
        """Defines the category the algorithm will be put in the algorithm browser"""
        return "DataHandling\\Raw"

    # ----------------------------------------------------------------------------------------

    def seeAlso(self):
        return ["LoadVesuvio"]

    # ----------------------------------------------------------------------------------------
    def PyInit(self):
        self.declareProperty(
            FileProperty(INST_PAR_PROP, "", action=FileAction.OptionalLoad, extensions=["dat"]),
            doc="An optional IP file. If provided the values are used to correct "
            "the default instrument values and attach the t0 values to each "
            "detector",
        )

        self.declareProperty(WorkspaceProperty(WKSP_PROP, "", Direction.Output), doc="The name of the output workspace.")

    # ----------------------------------------------------------------------------------------

    def PyExec(self):
        vesuvio_ws = self._load_empty_evs()

        # Load the IP file if it is provided
        ip_filename = self.getPropertyValue(INST_PAR_PROP)
        if ip_filename != "":
            vesuvio_ws = self._load_ip_file(vesuvio_ws, ip_filename)

        self.setProperty(WKSP_PROP, vesuvio_ws)

    # ----------------------------------------------------------------------------------------

    def _load_empty_evs(self):
        """
        Load en empty VESUVIO workspace with default IDF detector psotiions.
        @return VESUVIO workspace
        """
        # Get IDF
        inst_dir = config.getInstrumentDirectory()
        inst_file = os.path.join(inst_dir, "VESUVIO_Definition.xml")

        # Load the instrument
        load_empty = self.createChildAlgorithm("LoadEmptyInstrument")
        load_empty.setLogging(_LOGGING_)
        load_empty.setProperty("Filename", inst_file)
        load_empty.setProperty("OutputWorkspace", self.getPropertyValue(WKSP_PROP))
        load_empty.execute()

        return load_empty.getProperty("OutputWorkspace").value

    # ----------------------------------------------------------------------------------------

    def _load_ip_file(self, workspace, ip_file):
        """
        If provided, load the instrument parameter file into the result workspace.
        @param ip_file A string containing the full path to an IP file
        @return Updated workspace
        """
        ip_header = self._get_header_format(ip_file)

        # More verbose until the child algorithm stuff is sorted
        update_inst = self.createChildAlgorithm("UpdateInstrumentFromFile")
        update_inst.setLogging(_LOGGING_)
        update_inst.setProperty("Workspace", workspace)
        update_inst.setProperty("Filename", ip_file)
        update_inst.setProperty("MoveMonitors", False)
        update_inst.setProperty("IgnorePhi", True)
        update_inst.setProperty("AsciiHeader", ip_header)
        update_inst.execute()

        return update_inst.getProperty("Workspace").value

    # ----------------------------------------------------------------------------------------

    def _get_header_format(self, ip_filename):
        """
        Returns the header format to be used for the given IP file.
        Currently supports 5/6 column files.
        Raises ValueError if anything other than a 5/6 column file is found.
        @filename ip_filename :: Full path to the IP file.
        @return The header format string for use with UpdateInstrumentFromFile
        """
        ipfile = open(ip_filename, "r")
        first_line = ipfile.readline()
        columns = first_line.split()  # splits on whitespace characters
        try:
            return IP_HEADERS[len(columns)]
        except KeyError:
            raise ValueError("Unknown format for IP file. Currently support 5/6 column " "variants. ncols=%d" % (len(columns)))


# ----------------------------------------------------------------------------------------


AlgorithmFactory.subscribe(LoadEmptyVesuvio)
