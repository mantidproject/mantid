"""*WIKI* 

Clones the input [[MatrixWorkspace|Matrix Workspaces]] and orders the x-axis in an ascending fashion. Ensures that the y-axis and error data is sorted in a consistent way with the x-axis.
All x-values of the input workspace MUST be in either a descending or ascending fashion before passing to this algorithm.

This algorithm is for use with small workspaces loaded. It is particularly suitable for reformatting workspaces loaded via [[LoadASCII]]. Input workspaces must be a distribution.

*WIKI*"""
import mantid.simpleapi as api

from mantid.api import *
from mantid.kernel import *
import numpy as np
import operator

class SortXAxis(PythonAlgorithm):

    def category(self):
        return "Transforms\\Axes"

    def name(self):
        return "SortXAxis"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", defaultValue="",  direction=Direction.Input), doc="Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output), doc="Sorted Output Workspace")
        
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
            if inputws.isHistogramData():
               max_index = np.argmax(indexes)
               indexes = np.delete(indexes, max_index)
            yordered = y[indexes]
            eordered = e[indexes]
            
            outws.setX(i, xordered)
            outws.setY(i, yordered)
            outws.setE(i, eordered)
        self.setProperty('OutputWorkspace', outws)

#############################################################################################

AlgorithmFactory.subscribe(SortXAxis())