# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import WorkspaceProperty, DataProcessorAlgorithm, AlgorithmFactory, mtd, Progress
from mantid.kernel import logger, Direction
from mantid.simpleapi import CreateWorkspace
import numpy as np


class SwapWidths(DataProcessorAlgorithm):
    _input_ws = None
    _output_ws = None
    _swap_point = None

    def category(self):
        return "Workflow\\MIDAS;Inelastic"

    def summary(self):
        return "Creates a calibration workspace in energy trnasfer for IN16B."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", direction=Direction.Input), doc="Input workspace")

        self.declareProperty(name="SwapPoint", defaultValue=1, doc="Swap point")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace")

    def PyExec(self):
        prog_workflow = Progress(self, start=0, end=1.0, nreports=6)
        prog_workflow.report("Starting SwapWidths algorithm")
        self._setup()
        prog_workflow.report("Reading data values")
        label_1 = "f2.f1.FWHM"
        index_1 = self._indexOf(mtd[self._input_ws].getAxis(1), label_1)
        width1_x = mtd[self._input_ws].readX(index_1)
        width1_y = mtd[self._input_ws].readY(index_1)
        width1_e = mtd[self._input_ws].readE(index_1)
        label_2 = "f2.f2.FWHM"
        index_2 = self._indexOf(mtd[self._input_ws].getAxis(1), label_2)
        width2_x = mtd[self._input_ws].readX(index_2)
        width2_y = mtd[self._input_ws].readY(index_2)
        width2_e = mtd[self._input_ws].readE(index_2)
        number_points = len(width1_y)
        logger.information("Number of points is %i" % number_points)
        logger.information("Swap point is %i" % self._swap_point)

        prog_workflow.report("Calculating swap points")
        x_axis_1 = width1_x
        y_axis_1 = width1_y[: self._swap_point]
        y_axis_1 = np.append(y_axis_1, width2_y[self._swap_point :])
        error_1 = width1_e[: self._swap_point]
        error_1 = np.append(error_1, width2_e[self._swap_point :])
        x_axis_2 = width2_x
        y_axis_2 = width2_y[: self._swap_point]
        y_axis_2 = np.append(y_axis_2, width1_y[self._swap_point :])
        error_2 = width2_e[: self._swap_point]
        error_2 = np.append(error_2, width1_e[self._swap_point :])

        prog_workflow.report("Appending new points after swap")
        dataX = x_axis_1  # create data for WS
        dataY = y_axis_1
        dataE = error_1
        dataX = np.append(dataX, x_axis_2)
        dataY = np.append(dataY, y_axis_2)
        dataE = np.append(dataE, error_2)
        names = label_1 + ", " + label_2  # names for WS
        prog_workflow.report("Create new workspace with correct values")
        CreateWorkspace(
            OutputWorkspace=self._output_ws,
            DataX=dataX,
            DataY=dataY,
            DataE=dataE,
            Nspec=2,
            UnitX="MomentumTransfer",
            VerticalAxisUnit="Text",
            VerticalAxisValues=names,
        )

        prog_workflow.report("Algorithm finished")
        self.setProperty("OutputWorkspace", self._output_ws)

    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws = self.getPropertyValue("InputWorkspace")
        self._output_ws = self.getPropertyValue("OutputWorkspace")
        self._swap_point = self.getProperty("SwapPoint").value
        self._swap_point = int(self._swap_point) + 1

    def _indexOf(self, text_axis, label):
        nvals = text_axis.length()
        for idx in range(nvals):
            if label == text_axis.label(idx):
                return idx
        # If we reach here we didn't find it
        raise LookupError("Label '%s' not found on text axis" % label)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SwapWidths)
