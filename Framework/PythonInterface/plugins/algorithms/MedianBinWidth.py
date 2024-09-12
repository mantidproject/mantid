# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, HistogramValidator, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction
import numpy
import roundinghelper


class MedianBinWidth(PythonAlgorithm):
    _PROP_BIN_WIDTH = "BinWidth"
    _PROP_INPUT_WS = "InputWorkspace"

    def category(self):
        """
        Return algorithm's category.
        """
        return "Utility\\Calculation"

    def name(self):
        """
        Return algorithm's name.
        """
        return "MedianBinWidth"

    def summary(self):
        """
        Return algorithm's summary.
        """
        return "Calculates the average of workspace's histograms'" " median bin widths."

    def version(self):
        """
        Return algorithm's version.
        """
        return 1

    def PyInit(self):
        """
        Declares algorithm's properties.
        """
        self.declareProperty(
            MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue="", validator=HistogramValidator(), direction=Direction.Input),
            doc="The workspace containing the input data",
        )
        roundinghelper.declare_rounding_property(self)
        self.declareProperty(self._PROP_BIN_WIDTH, defaultValue=0.0, direction=Direction.Output, doc="The averaged median bin width")

    def PyExec(self):
        """
        Averages the median bin widths of the input workspace.
        """
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        roundingMode = self.getProperty(roundinghelper.PROP_NAME_ROUNDING_MODE).value
        xs = inputWs.extractX()
        dxs = numpy.diff(xs)
        medians = numpy.median(dxs, axis=1)
        binWidth = numpy.mean(medians)
        binWidth = roundinghelper.round(binWidth, roundingMode)
        self.setProperty(self._PROP_BIN_WIDTH, numpy.abs(binWidth))


AlgorithmFactory.subscribe(MedianBinWidth)
