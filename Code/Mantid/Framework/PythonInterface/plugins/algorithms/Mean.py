#pylint: disable=no-init,invalid-name
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

        self.declareProperty("Workspaces", "", validator=mustHaveWorkspaceNames,
                             direction=Direction.Input,
                             doc="Input workspaces. Comma separated workspace names")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                             Direction.Output),
                             doc="Output mean workspace")


    def _are_workspaces_compatible(self, ws_a, ws_b):
        sizeA = ws_a.blocksize() * ws_a.getNumberHistograms()
        sizeB = ws_b.blocksize() * ws_b.getNumberHistograms()
        return sizeA == sizeB


    def PyExec(self):
        workspaces = self.getProperty("Workspaces").value.split(',')
        out_ws = CloneWorkspace(InputWorkspace=mtd[workspaces[0]],
                                OutputWorkspace=self.getPropertyValue("OutputWorkspace"))
        for index in range(1, len(workspaces)):
            name = workspaces[index].strip()
            workspace = mtd[name]
            if not self._are_workspaces_compatible(out_ws, workspace):
                raise RuntimeError("Input Workspaces are not the same shape.")
            out_ws += workspace
        out_ws /= len(workspaces)
        self.setProperty("OutputWorkspace", out_ws)



AlgorithmFactory.subscribe(Mean())
