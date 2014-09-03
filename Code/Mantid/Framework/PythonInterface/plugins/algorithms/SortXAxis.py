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

    def summary(self):
        return "Clones the input MatrixWorkspace(s) and orders the x-axis in an ascending fashion."

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
