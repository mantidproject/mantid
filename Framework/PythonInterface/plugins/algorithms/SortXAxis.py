#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator
import numpy as np


class SortXAxis(PythonAlgorithm):

    def category(self):
        return "Transforms\\Axes;Utility\\Sorting"

    def seeAlso(self):
        return [ "SortDetectors" ]

    def name(self):
        return "SortXAxis"

    def summary(self):
        return "Clones the input MatrixWorkspace(s) and orders the x-axis in an ascending fashion."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input),
                             doc="Input workspace")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output),
                             doc="Sorted Output Workspace")
        self.declareProperty("Ordering",
                             defaultValue="Ascending",
                             validator=StringListValidator(["Ascending", "Descending"]),
                             direction=Direction.Input,
                             doc="Ascending or descending sorting")

    def PyExec(self):
        input_ws = self.getProperty('InputWorkspace').value
        output_ws = self.getPropertyValue('OutputWorkspace')

        num_specs = input_ws.getNumberHistograms()

        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        clone_alg.setProperty("InputWorkspace", input_ws)
        clone_alg.setProperty("OutputWorkspace", output_ws)
        clone_alg.execute()
        output_ws = clone_alg.getProperty("OutputWorkspace").value

        for i in range(0, num_specs):
            x_data = input_ws.readX(i)
            y_data = input_ws.readY(i)
            e_data = input_ws.readE(i)

            indexes = x_data.argsort()

            if self.getPropertyValue("Ordering") == "Descending":
                self.log().information("Sort descending")
                indexes = indexes[::-1]

            x_ordered = x_data[indexes]

            if input_ws.isHistogramData():
                max_index = np.argmax(indexes)
                indexes = np.delete(indexes, max_index)

            y_ordered = y_data[indexes]
            e_ordered = e_data[indexes]

            output_ws.setX(i, x_ordered)
            output_ws.setY(i, y_ordered)
            output_ws.setE(i, e_ordered)

            if input_ws.hasDx(i):
                dx = input_ws.readDx(i)
                dx_ordered = dx[indexes]
                output_ws.setDx(i, dx_ordered)

        self.setProperty('OutputWorkspace', output_ws)


#AlgorithmFactory.subscribe(SortXAxis)
