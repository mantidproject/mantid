# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty
from mantid.kernel import Direction


class CheckForSampleLogs(PythonAlgorithm):
    """Check if certain sample logs exists on a workspace"""

    def category(self):
        """Return category"""
        return "Utility\\Workspaces"

    def seeAlso(self):
        return ["CompareSampleLogs", "CopyLogs"]

    def name(self):
        """Return name"""
        return "CheckForSampleLogs"

    def summary(self):
        return "Check if the workspace has some given sample logs"

    def PyInit(self):
        """Declare properties"""
        self.declareProperty(WorkspaceProperty("Workspace", "", Direction.Input), "The workspace to check.")
        self.declareProperty("LogNames", "", "Names of the logs to look for")
        self.declareProperty(
            "Result", "A string that will be empty if all the logs are found, otherwise will contain an error message", Direction.Output
        )
        return

    def PyExec(self):
        """Main execution body"""
        # get parameters
        w = self.getProperty("Workspace").value
        logNames = self.getProperty("LogNames").value
        resultString = ""
        # check for parameters and build the result string
        for value in logNames.split(","):
            value = value.strip()
            if len(value) > 0:
                if not w.run().hasProperty(value):
                    resultString += "Property " + value + " not found\n"

        self.getLogger().notice(resultString)
        self.setProperty("Result", resultString)


AlgorithmFactory.subscribe(CheckForSampleLogs)
