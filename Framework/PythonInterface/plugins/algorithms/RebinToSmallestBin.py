from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import sys

class RebinToSmallestBin(PythonAlgorithm):
    _PROP_INPUT_WS  = 'InputWorkspace'
    _PROP_OUTPUT_WS = 'OutputWorkspace'

    def category(self):
        return 'Transforms\\Rebin'

    def name(self):
        return 'RebinToSmallestBin'

    def summary(self):
        return 'Rebins a workspace to the minimum bin width.'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue='', direction=Direction.Input), doc='The workspace containing the input data')
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_OUTPUT_WS, defaultValue='', direction=Direction.Output), doc='The output workspace')

    def PyExec(self):
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        outputWs = self.getProperty(self._PROP_OUTPUT_WS).value
        inputIsDistribution = inputWs.isDistribution()
        if inputIsDistribution:
            inputWs = ConvertToHistogram(inputWs)
        minBinWidth = sys.float_info.max
        for wsIndex in range(inputWs.getNumberHistograms()):
            xs = inputWs.readX(wsIndex)
            for binIndex in range(len(xs) - 1):
                binWidth = xs[binIndex + 1] - xs[binIndex]
                minBinWidth = min(minBinWidth, binWidth)
        # TODO check if Rebin works with inverted x axis; in that case use abs(minBinWidth). Otherwise, raise exception if minBinWidth <= 0
        outputWs = Rebin(InputWorkspace=inputWs, OutputWorkspace=outputWs, Params=str(minBinWidth))
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_OUTPUT_WS, outputWs)

AlgorithmFactory.subscribe(RebinToSmallestBin)
