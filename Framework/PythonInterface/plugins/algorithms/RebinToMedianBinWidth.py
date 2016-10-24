from __future__ import (absolute_import, division, print_function)
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator
import numpy

class RebinToMedianBinWidth(PythonAlgorithm):
    '''
    Averages the median bin widths over the histograms of the input
    workspace. The average is then fed to the Rebin algorithm.
    '''
    _PROP_INPUT_WS  = 'InputWorkspace'
    _PROP_OUTPUT_WS = 'OutputWorkspace'
    _PROP_ROUNDING  = 'Rounding'

    _ROUNDING_NONE = 'None'
    _ROUNDING_TEN_TO_INT = '10^n'

    def category(self):
        '''
        Return algorithm's category.
        '''
        return 'Transforms\\Rebin'

    def name(self):
        '''
        Return algorithm's name.
        '''
        return 'RebinToMedianBinWidth'

    def summary(self):
        '''
        Return algorithm's summary.
        '''
        return 'Rebins a workspace to the median bin width.'

    def PyInit(self):
        '''
        Declares algorithm's properties.
        '''
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue='', direction=Direction.Input), doc='The workspace containing the input data')
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_OUTPUT_WS, defaultValue='', direction=Direction.Output), doc='The output workspace')
        rounding = StringListValidator()
        rounding.addAllowedValue(self._ROUNDING_NONE)
        rounding.addAllowedValue(self._ROUNDING_TEN_TO_INT)
        self.declareProperty(name=self._PROP_ROUNDING, defaultValue=self._ROUNDING_NONE, validator=rounding, direction=Direction.Input, doc='Bin width rounding')

    def PyExec(self):
        '''
        Averages the median bin widths of the input workspace and
        executes the Rebin algorithm.
        '''
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        outputWs = self.getProperty(self._PROP_OUTPUT_WS).value
        roundingMode = self.getProperty(self._PROP_ROUNDING).value
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
        if roundingMode == self._ROUNDING_TEN_TO_INT:
            binWidth = 10.0**numpy.floor(numpy.log10(binWidth))
        self.log().notice('Binning to bin width {0}'.format(binWidth))
        outputWs = Rebin(InputWorkspace=inputWs, OutputWorkspace=outputWs, Params=binWidth)
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_OUTPUT_WS, outputWs)

AlgorithmFactory.subscribe(RebinToMedianBinWidth)
