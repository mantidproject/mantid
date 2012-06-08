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
        self.declareWorkspaceProperty("InputWorkspace","", Direction=Direction.Input, Type=Workspace,
                Optional=True,
                Description=
                    "An input workspace that the python code will modify.\n"
                    "The workspace will be in the python variable named 'input'.")
        
        self.declareProperty("Code", "", Direction=Direction.Input,
                             Description="Python code (can be on multiple lines)." )
        
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output, Type=Workspace,
                Optional=True,
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
        
        # Did the code use an operator
        # like "output = input * 2.0"
        if isinstance(output, WorkspaceProxy):
            wsOut = output
        else:
            # Output is (probably) still a string. Use the name
            if mtd.workspaceExists(wsOutputName):
                # The script did create the workspace; use it
                wsOut = mtd[wsOutputName]
            elif len(wsOutputName)>0:
                # The script did NOT create it
                # So we take care of cloning it so that the output is valid
                CloneWorkspace(InputWorkspace=wsInputName, OutputWorkspace=wsOutputName)
                wsOut = mtd[wsOutputName]
            
        if len(wsOutputName)>0:
            self.setProperty("OutputWorkspace",wsOut)

        return
        
        
mtd.registerPyAlgorithm(RunPythonScript())
