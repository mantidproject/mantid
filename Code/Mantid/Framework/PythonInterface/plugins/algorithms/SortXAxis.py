"""*WIKI* 

Clones the input [[MatrixWorkspace|Matrix Workspaces]] and orders the x-axis in an ascending fashion. Ensures that the y-axis and error data is sorted in a consistent way with the x-axis.

This algorithm is for use with small workspaces loaded. It is particularly suitable for reformatting workspaces loaded via [[LoadASCII]]. Input workspaces must be a distribution.

*WIKI*"""
import mantid.simpleapi as api

from mantid.api import *
from mantid.kernel import *
import numpy as np

class SortXAxis(PythonAlgorithm):

    def category(self):
        return "Transforms\\Axes"

    def name(self):
        return "SortXAxis"

    def PyInit(self):
        distributionValidator = HistogramValidator(False)
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", validator=distributionValidator, direction=Direction.Input), doc="Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Sorted Output Workspace")
        
    def PyExec(self):
        inputws = self.getProperty("InputWorkspace").value
        specs = inputws.getNumberHistograms()
        outws = api.CloneWorkspace(InputWorkspace=inputws)
        for i in range(0, specs):
            x = inputws.readX(i)
            y = inputws.readY(i)
            e = inputws.readE(i)
            indexes =  x.argsort()
            xordered = x[indexes]
            yordered = y[indexes]
            eordered = e[indexes]
            outws.setX(i, xordered)
            outws.setY(i, yordered)
            outws.setE(i, eordered)
        self.setProperty('OutputWorkspace', outws)

#############################################################################################

AlgorithmFactory.subscribe(SortXAxis())