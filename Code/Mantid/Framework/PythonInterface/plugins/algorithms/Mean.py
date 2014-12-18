from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *

class Mean(PythonAlgorithm):

    def category(self):
        return "Arithmetic"

    def name(self):
        return "Mean"

    def summary(self):
        return "Calculates the arithemetic mean of the workspaces provided."

    def PyInit(self):
        mustHaveWorkspaceNames = StringMandatoryValidator()
        self.declareProperty("Workspaces", "", validator=mustHaveWorkspaceNames, direction=Direction.Input, doc="Input workspaces. Comma separated workspace names")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output mean workspace")

    def areWorkspacesCompatible(self, a, b):
        sizeA = a.blocksize() * a.getNumberHistograms()
        sizeB = b.blocksize() * b.getNumberHistograms()
        return sizeA == sizeB

    def PyExec(self):
        workspaces = self.getProperty("Workspaces").value.split(',')
        out_ws = CloneWorkspace(InputWorkspace=mtd[workspaces[0]], OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        for index in range(1, len(workspaces)):
            name = workspaces[index].strip()
            ws = mtd[name]
            if not self.areWorkspacesCompatible(out_ws, ws):
                raise RuntimeError("Input Workspaces are not the same shape.")
            out_ws += ws
        out_ws /= len(workspaces)
        self.setProperty("OutputWorkspace", out_ws)



#############################################################################################

AlgorithmFactory.subscribe(Mean())
