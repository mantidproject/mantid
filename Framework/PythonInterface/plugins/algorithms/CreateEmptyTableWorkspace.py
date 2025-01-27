# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, WorkspaceFactory
from mantid.kernel import Direction

# Create an empty table workspace to be populated by a python script.


class CreateEmptyTableWorkspace(PythonAlgorithm):
    def summary(self):
        return "Creates an empty TableWorkspace which can be populated with various types of information."

    def category(self):
        return "Utility\\Workspaces"

    def seeAlso(self):
        return ["DeleteTableRows", "SortTableWorkspace"]

    def PyInit(self):
        # Declare properties
        self.declareProperty(
            ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output), "The name of the table workspace that will be created."
        )

    def PyExec(self):
        tableWS = WorkspaceFactory.createTable()

        self.setProperty("OutputWorkspace", tableWS)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateEmptyTableWorkspace)
