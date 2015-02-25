#pylint: disable=no-init,invalid-name
import mantid.simpleapi as api

from mantid.api import *
from mantid.kernel import *
import numpy as np

class SortXAxis(PythonAlgorithm):

    def category(self):
        return "Transforms\\Axes"


    def name(self):
        return "SortXAxis"


    def summary(self):
        return "Clones the input MatrixWorkspace(s) and orders the x-axis in an ascending fashion."


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", defaultValue="",
                             direction=Direction.Input),
                             doc="Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", defaultValue="",
                             direction=Direction.Output),
                             doc="Sorted Output Workspace")


    def PyExec(self):
        input_ws = self.getProperty('InputWorkspace').value
        output_ws = self.getPropertyValue('OutputWorkspace')

        num_specs = input_ws.getNumberHistograms()
        api.CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=output_ws)

        for i in range(0, num_specs):
            x_data = input_ws.readX(i)
            y_data = input_ws.readY(i)
            e_data = input_ws.readE(i)

            indexes =  x_data.argsort()

            x_ordered = x_data[indexes]
            if input_ws.isHistogramData():
                max_index = np.argmax(indexes)
                indexes = np.delete(indexes, max_index)

            y_ordered = y_data[indexes]
            e_ordered = e_data[indexes]

            mtd[output_ws].setX(i, x_ordered)
            mtd[output_ws].setY(i, y_ordered)
            mtd[output_ws].setE(i, e_ordered)

        self.setProperty('OutputWorkspace', output_ws)


AlgorithmFactory.subscribe(SortXAxis())
