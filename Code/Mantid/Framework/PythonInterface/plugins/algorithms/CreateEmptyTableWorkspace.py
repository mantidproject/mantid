#pylint: disable=no-init
from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, WorkspaceFactory
from mantid.kernel import Direction

# Create an empty table workspace to be populated by a python script.
class CreateEmptyTableWorkspace(PythonAlgorithm):

    def summary(self):
        return "Creates an empty TableWorkspace which can be populated with various types of information."

    def PyInit(self):
        # Declare properties
        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output), "The name of the table workspace that will be created.")

    def PyExec(self):
        tableWS = WorkspaceFactory.createTable()

        self.setProperty("OutputWorkspace", tableWS)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateEmptyTableWorkspace)
