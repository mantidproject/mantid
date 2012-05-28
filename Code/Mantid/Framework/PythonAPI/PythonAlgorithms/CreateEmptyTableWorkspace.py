from mantid.api import PythonAlgorithm, registerAlgorithm, ITableWorkspaceProperty, WorkspaceFactory
from mantid.kernel import Direction

# Create an empty table workspace to be populated by a python script.
class CreateEmptyTableWorkspace(PythonAlgorithm):
 
    def PyInit(self):
        # Declare properties
	self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Name of Calibration Table Workspace")
 
    def PyExec(self):
	tableWS = WorkspaceFactory.createTable()
	tableWS.addColumn(type="int",name="Detector ID")
	tableWS.addColumn(type="V3D",name="Detector Position")
	
	self.setProperty("OutputWorkspace", tableWS)
 
# Register algorithm with Mantid
registerAlgorithm(CreateEmptyTableWorkspace)

