from __future__ import (absolute_import, division, print_function)
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator
from mantid.simpleapi import Rebin
import numpy
import rebinwrapperhelpers

class RebinToBinWidthAtX(PythonAlgorithm):
    '''
    Averages the bin widths at given x over the histograms of the input
    workspace. The average is then fed to the Rebin algorithm.
    '''
    _PROP_INPUT_WS  = 'InputWorkspace'
    _PROP_OUTPUT_WS = 'OutputWorkspace'
    _PROP_X_VALUE   = 'X'

    def category(self):
        '''
        Return algorithm's category.
        '''
        return 'Transforms\\Rebin'

    def name(self):
        '''
        Return algorithm's name.
        '''
        return 'RebinToBinWidthAtX'

    def summary(self):
        '''
        Return algorithm's summary.
        '''
        return 'Rebins a workspace to the bin width at given X.'

    def PyInit(self):
        '''
        Declares algorithm's properties.
        '''
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue='', direction=Direction.Input), doc='The workspace containing the input data')
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_OUTPUT_WS, defaultValue='', direction=Direction.Output), doc='The output workspace')
        self.declareProperty(name=self._PROP_X_VALUE, defaultValue=0.0, direction=Direction.Input, doc='Where to pick the bin width')
        rebinwrapperhelpers.declare_rounding_property(self)

    def PyExec(self):
        '''
        Averages the bin widths at X and runs the Rebin algorithm.
        '''
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        outputWs = self.getProperty(self._PROP_OUTPUT_WS).value
        x = self.getProperty(self._PROP_X_VALUE).value
        roundingMode = self.getProperty(rebinwrapperhelpers.PROP_NAME_ROUNDING_MODE).value
        inputIsDistribution = inputWs.isDistribution()
        if inputIsDistribution:
            inputWs = ConvertToHistogram(inputWs)
        n = inputWs.getNumberHistograms()
        widths = numpy.empty(n)
        for wsIndex in range(n):
            xs = inputWs.readX(wsIndex)
            if x <= xs[0] or x > xs[-1]:
                raise RuntimeError(self._PROP_X_VALUE + ' = {0} out of range for workspace index {1}'.format(x, wsIndex))
            binIndex = inputWs.binIndexOf(x, wsIndex)
            dx = xs[binIndex + 1] - xs[binIndex]
            widths[wsIndex] = dx
        binWidth = numpy.mean(widths)
        binWidth = rebinwrapperhelpers.round(binWidth, roundingMode)
        self.log().notice('Binning to bin width {0}'.format(binWidth))
        outputWs = Rebin(InputWorkspace=inputWs, OutputWorkspace=outputWs, Params=binWidth)
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_OUTPUT_WS, outputWs)

AlgorithmFactory.subscribe(RebinToBinWidthAtX)
