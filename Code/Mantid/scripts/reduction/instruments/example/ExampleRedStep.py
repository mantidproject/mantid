from MantidFramework import *
from mantidsimple import *

class ExampleRedStep(PythonAlgorithm):
    
    def name(self):
        return "ExampleRedStep"

    def PyInit(self):
        self.declareWorkspaceProperty("InputWorkspace", "", Direction.Input)
        self.declareProperty("OutputWorkspace", "")
        self.declareAlgorithmProperty("Algorithm")

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace")
        output_ws = self.getProperty("OutputWorkspace")
        
        alg = self.getProperty("Algorithm")
        alg.setPropertyValue("InputWorkspace", str(input_ws))
        alg.setPropertyValue("OutputWorkspace", output_ws)
        alg.execute()

#mtd.registerPyAlgorithm(ExampleRedStep())

class ExampleLoader(PythonAlgorithm):
    
    def name(self):
        return "ExampleLoader"

    def PyInit(self):
        self.declareProperty("Filename", "")
        self.declareProperty("OutputWorkspace", "")

    def PyExec(self):
        filename = self.getProperty("Filename")
        output_ws = self.getProperty("OutputWorkspace")
        LoadAscii(filename, output_ws)
        
        print filename, output_ws

