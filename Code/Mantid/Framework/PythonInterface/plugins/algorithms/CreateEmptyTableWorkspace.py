"""*WIKI* 

This algorithm creates an empty table workspace and puts it in the data service to make it available to python.

*WIKI*"""

from mantid.api import PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty, WorkspaceFactory
from mantid.kernel import Direction

# Create an empty table workspace to be populated by a python script.
class CreateEmptyTableWorkspace(PythonAlgorithm):
 
    def PyInit(self):
        # Declare properties
        self.setWikiSummary("""Creates an empty table workspace that can be populated by python code.""")
        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output), "The name of the table workspace that will be created.")
 
    def PyExec(self):
        tableWS = WorkspaceFactory.createTable()

        self.setProperty("OutputWorkspace", tableWS)
 
# Register algorithm with Mantid
AlgorithmFactory.subscribe(CreateEmptyTableWorkspace)

