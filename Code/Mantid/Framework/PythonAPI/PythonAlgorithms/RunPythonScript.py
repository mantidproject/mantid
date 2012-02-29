"""*WIKI* 

Algorithm that will run a snippet of python code.
This is meant to be used by [[LoadLiveData]] to perform some processing.

*WIKI*"""


from MantidFramework import *
from mantidsimple import *
import os

class RunPythonScript(PythonAlgorithm):
    """ Invert a masking workspace.
    """
   
    def category(self):
        return "DataHandling\\LiveData"

    def name(self):
        return "RunPythonScript"

    def PyInit(self):
        self.declareWorkspaceProperty("InputWorkspace","", Direction=Direction.Input,
                Description=
                    "An input workspace that the python code will modify.\n"
                    "The workspace will be in the python variable named 'input'.")
        
        self.declareProperty("Code", "", Direction=Direction.Input,
                             Description="Python code (can be on multiple lines)." )
        
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output, 
                Description=
                "An output workspace to be produced by the python code.\n"
                "The python code should create the workspace named by the python variable 'output'.")
      
      
    def PyExec(self):
        """ Main body of execution
        """
        # 1. get parameter values
        wsInputName = self.getPropertyValue("InputWorkspace")
        wsOutputName = self.getPropertyValue("OutputWorkspace")
        code = self.getPropertyValue("Code")

        # Prepare variables expected in the script code
        input = mtd[wsInputName]
        output = wsOutputName
        
        # Run the script code passed
        exec(code)

        if mtd.workspaceExists(output):
            # The script did create the workspace; use it
            wsOut = mtd[output]
        else:
            # The script did NOT create it
            # So we take care of cloning it so that the output is valid
            CloneWorkspace(InputWorkspace=wsInputName, OutputWorkspace=wsOutputName)
            wsOut = mtd[wsOutputName]
            
        self.setProperty("OutputWorkspace",wsOut)

        return
        
        
mtd.registerPyAlgorithm(RunPythonScript())
