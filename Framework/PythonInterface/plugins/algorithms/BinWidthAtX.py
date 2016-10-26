from __future__ import (absolute_import, division, print_function)

from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty,\
    PythonAlgorithm
from mantid.kernel import Direction
import numpy
import roundinghelper


class BinWidthAtX(PythonAlgorithm):

    _PROP_BIN_WIDTH = 'BinWidth'
    _PROP_INPUT_WS = 'InputWorkspace'
    _PROP_X_VALUE = 'X'

    def category(self):
        '''
        Return algorithm's category.
        '''
        return 'Utility\\Calculation'

    def name(self):
        '''
        Return algorithm's name.
        '''
        return 'BinWidthAtX'

    def summary(self):
        '''
        Return algorithm's summary.
        '''
        return 'Calculates the bin width at X, averaged over all histograms.'

    def version(self):
        '''
        Return algorithm's version.
        '''
        return 1

    def PyInit(self):
        '''
        Declares algorithm's properties.
        '''
        self.declareProperty(
            MatrixWorkspaceProperty(name=self._PROP_INPUT_WS,
                                    defaultValue='',
                                    direction=Direction.Input),
            doc='The workspace containing the input data')
        self.declareProperty(
            name=self._PROP_X_VALUE, defaultValue=0.0,
            direction=Direction.Input, doc='')
        roundinghelper.declare_rounding_property(self)
        self.declareProperty(
            name=self._PROP_BIN_WIDTH, defaultValue=0.0,
            direction=Direction.Output, doc='The averaged bin width')

    def PyExec(self):
        '''
        Averages the bin widths at X.
        '''
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        x = self.getProperty(self._PROP_X_VALUE).value
        roundingMode = self.getProperty(
            roundinghelper.PROP_NAME_ROUNDING_MODE).value
        inputIsDistribution = inputWs.isDistribution()
        if inputIsDistribution:
            inputWs = ConvertToHistogram(inputWs)
        n = inputWs.getNumberHistograms()
        widths = numpy.empty(n)
        for wsIndex in range(n):
            xs = inputWs.readX(wsIndex)
            if x <= xs[0] or x > xs[-1]:
                raise RuntimeError(
                    self._PROP_X_VALUE +
                    ' = {0} out of range for workspace index {1}'
                    .format(x, wsIndex))
            binIndex = inputWs.binIndexOf(x, wsIndex)
            dx = xs[binIndex + 1] - xs[binIndex]
            widths[wsIndex] = dx
        binWidth = numpy.mean(widths)
        binWidth = roundinghelper.round(binWidth, roundingMode)
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_BIN_WIDTH, binWidth)

AlgorithmFactory.subscribe(BinWidthAtX)
