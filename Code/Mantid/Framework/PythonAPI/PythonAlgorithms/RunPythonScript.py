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
        
        # Output workspace properties of sub-algorithms are given the name 'ChildAlgOutput'
        # by default. In the case of python algorithms, it means that such a workspace
        # will be put in the ADS if another name is not given. Catch that case here:
        if wsOutputName=="ChildAlgOutput":
            wsOutputName = "__RunPythonScriptOutput"
            self.setPropertyValue("OutputWorkspace", wsOutputName)
        
        # Get the code to be run
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
            elif len(wsOutputName)>0 and len(wsInputName)>0:
                # The script did NOT create it
                # So we take care of cloning it so that the output is valid
                CloneWorkspace(InputWorkspace=wsInputName, OutputWorkspace=wsOutputName)
                wsOut = mtd[wsOutputName]                
            else:
                # We don't have an input workspace to work with
                # Create a dummy workspace so that the output is valid
                #TODO: would be best if output workspace properties
                # could be optional
                CreateSingleValuedWorkspace(OutputWorkspace=wsOutputName,
                                            DataValue=1.0, ErrorValue=0.0)
                wsOut = mtd[wsOutputName]
        self.setProperty("OutputWorkspace",wsOut)

        return
        
        
mtd.registerPyAlgorithm(RunPythonScript())
