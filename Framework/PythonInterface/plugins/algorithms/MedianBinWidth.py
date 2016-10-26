from __future__ import (absolute_import, division, print_function)

from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator
import numpy
import roundinghelper

class MedianBinWidth(PythonAlgorithm):

    _PROP_BIN_WIDTH = 'BinWidth'
    _PROP_INPUT_WS  = 'InputWorkspace'

    def category(self):
        '''
        Return algorithm's category.
        '''
        return 'Utility\\Calculation'

    def name(self):
        '''
        Return algorithm's name.
        '''
        return 'MedianBinWidth'

    def summary(self):
        '''
        Return algorithm's summary.
        '''
        return "Calculates the average of workspace's histograms' median bin widths."

    def version(self):
        '''
        Return algorithm's version.
        '''
        return 1

    def PyInit(self):
        '''
        Declares algorithm's properties.
        '''
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue='', direction=Direction.Input), doc='The workspace containing the input data')
        roundinghelper.declare_rounding_property(self)
        self.declareProperty(self._PROP_BIN_WIDTH, defaultValue=0.0, direction=Direction.Output, doc='The averaged median bin width')

    def PyExec(self):
        '''
        Averages the median bin widths of the input workspace.
        '''
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        roundingMode = self.getProperty(roundinghelper.PROP_NAME_ROUNDING_MODE).value
        inputIsDistribution = inputWs.isDistribution()
        if inputIsDistribution:
            inputWs = ConvertToHistogram(inputWs)
        n = inputWs.getNumberHistograms()
        medians = numpy.empty(n)
        for wsIndex in range(n):
            xs = inputWs.readX(wsIndex)
            dxs = xs[1:] - xs[:-1]
            medians[wsIndex] = numpy.median(dxs)
        binWidth = numpy.mean(medians)
        binWidth = roundinghelper.round(binWidth, roundingMode)
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_BIN_WIDTH, binWidth)

AlgorithmFactory.subscribe(MedianBinWidth)
