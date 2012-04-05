"""*WIKI* 

This algorithm will invert a masking workspace.  It is just a 'NOT' operation on SpecialWorkspace2D.

*WIKI*"""


from MantidFramework import *
from mantidsimple import *
import os

class InvertMask(PythonAlgorithm):
    """ Invert a masking workspace.
    """
   
    def category(self):
        return "Diagnostics;PythonAlgorithms"

    def name(self):
        return "InvertMask"

    def PyInit(self):
        self.declareWorkspaceProperty("InputWorkspace","", Direction=Direction.Input,
                Description="Masking Workspace (SpecialWorkspace2D) to be inverted")
        self.declareWorkspaceProperty("OutputWorkspace", "", Direction=Direction.Output, 
                Description="Output Masking Workspace as the inverted one of the input workspace.")
      
      
    def PyExec(self):
        """ Main body of execution
        """
        # 1. get parameter values
        wsInputName = self.getPropertyValue("InputWorkspace")
        wsOutputName = self.getPropertyValue("OutputWorkspace")

        wsIn = mtd[wsInputName]
        if wsIn is None:
            raise NotImplementedError("Input Workspace does not exist")

        # 2. Invert
        BinaryOperateMasks(InputWorkspace1=wsInputName, OperationType="NOT",
                OutputWorkspace=wsOutputName)

        # 3. Set result
        wsOut = mtd[wsOutputName]
        self.setProperty("OutputWorkspace",wsOut)

        return
        
        
mtd.registerPyAlgorithm(InvertMask())
