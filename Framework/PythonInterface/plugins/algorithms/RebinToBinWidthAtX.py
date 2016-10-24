from __future__ import (absolute_import, division, print_function)
from mantid.api import AlgorithmFactory, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import Direction, StringListValidator
import numpy

class RebinToBinWidthAtX(PythonAlgorithm):
    _PROP_INPUT_WS  = 'InputWorkspace'
    _PROP_OUTPUT_WS = 'OutputWorkspace'
    _PROP_ROUNDING  = 'Rounding'
    _PROP_X_VALUE   = 'X'

    _ROUNDING_NONE = 'None'
    _ROUNDING_TEN_TO_INT = '10^n'

    def category(self):
        return 'Transforms\\Rebin'

    def name(self):
        return 'RebinToBinWidthAtX'

    def summary(self):
        return 'Rebins a workspace to the bin width at given X.'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue='', direction=Direction.Input), doc='The workspace containing the input data')
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_OUTPUT_WS, defaultValue='', direction=Direction.Output), doc='The output workspace')
        self.declareProperty(name=self._PROP_X_VALUE, defaultValue=0.0, direction=Direction.Input, doc='Where to pick the bin width')
        rounding = StringListValidator()
        rounding.addAllowedValue(self._ROUNDING_NONE)
        rounding.addAllowedValue(self._ROUNDING_TEN_TO_INT)
        self.declareProperty(name=self._PROP_ROUNDING, defaultValue=self._ROUNDING_NONE, validator=rounding, direction=Direction.Input, doc='Bin width rounding')

    def PyExec(self):
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        outputWs = self.getProperty(self._PROP_OUTPUT_WS).value
        x = self.getProperty(self._PROP_X_VALUE).value
        roundingMode = self.getProperty(self._PROP_ROUNDING).value
        inputIsDistribution = inputWs.isDistribution()
        if inputIsDistribution:
            inputWs = ConvertToHistogram(inputWs)
        n = inputWs.getNumberHistograms()
        widths = numpy.empty(n)
        for wsIndex in range(n):
            xs = inputWs.readX(wsIndex)
            if x <= xs[0] or x > xs[-1]:
                raise RuntimeError(self._PROP_X_VALUE + ' out of range for workspace index {0}'.format(wsIndex))
            binIndex = inputWs.binIndexOf(x, wsIndex)
            dx = xs[binIndex + 1] - xs[binIndex]
            widths[wsIndex] = dx
        binWidth = numpy.mean(widths)
        if roundingMode == self._ROUNDING_TEN_TO_INT:
            binWidth = 10.0**numpy.floor(numpy.log10(binWidth))
        self.log().notice('Binning to bin width {0}'.format(binWidth))
        outputWs = Rebin(InputWorkspace=inputWs, OutputWorkspace=outputWs, Params=binWidth)
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_OUTPUT_WS, outputWs)

AlgorithmFactory.subscribe(RebinToBinWidthAtX)
