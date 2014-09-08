from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

class ExampleRedStep(PythonAlgorithm):

    def name(self):
        return "ExampleRedStep"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output))
        self.declareProperty(AlgorithmProperty("Algorithm"))

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        output_ws = self.getProperty("OutputWorkspace").value

        alg = self.getProperty("Algorithm").value
        alg.setPropertyValue("InputWorkspace", str(input_ws))
        alg.setPropertyValue("OutputWorkspace", output_ws)
        alg.execute()

#AlgorithmFactory.subscribe(ExampleRedStep)

class ExampleLoader(PythonAlgorithm):

    def name(self):
        return "ExampleLoader"

    def PyInit(self):
        self.declareProperty("Filename", "")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        output_ws = self.getProperty("OutputWorkspace").value
        LoadAscii(filename, output_ws)

        print filename, output_ws

