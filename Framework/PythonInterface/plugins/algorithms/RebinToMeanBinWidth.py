from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import numpy
import sys

class RebinToMeanBinWidth(PythonAlgorithm):
    _PROP_END_X     = 'AveragingEndX'
    _PROP_INPUT_WS  = 'InputWorkspace'
    _PROP_OUTPUT_WS = 'OutputWorkspace'
    _PROP_START_X   = 'AveragingStartX'

    def category(self):
        return 'Transforms\\Rebin'

    def name(self):
        return 'RebinToMeanBinWidth'

    def summary(self):
        return 'Rebins a workspace to the average bin width.'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_INPUT_WS, defaultValue='', direction=Direction.Input), doc='The workspace containing the input data')
        self.declareProperty(MatrixWorkspaceProperty(name=self._PROP_OUTPUT_WS, defaultValue='', direction=Direction.Output), doc='The rebinned workspace')
        self.declareProperty(name=self._PROP_START_X, defaultValue=-sys.float_info.max, direction=Direction.Input, doc='Start X for the average bin width search')
        self.declareProperty(name=self._PROP_END_X, defaultValue=sys.float_info.max, direction=Direction.Input, doc='End X for the averge bin width search')

    def PyExec(self):
        inputWs = self.getProperty(self._PROP_INPUT_WS).value
        outputWs = self.getProperty(self._PROP_OUTPUT_WS).value
        startX = self.getProperty(self._PROP_START_X).value
        endX = self.getProperty(self._PROP_END_X).value
        inputIsDistribution = inputWs.isDistribution()
        if inputIsDistribution:
            inputWs = ConvertToHistogram(inputWs)
        n = inputWs.getNumberHistograms()
        means = numpy.empty(n)
        for wsIndex in range(n):
            xs = inputWs.readX(wsIndex)
            means[wsIndex] = numpy.mean(xs[xs >= startX and xs < endX])
        binWidth = numpy.mean(means)
        # TODO raise exception if binWidth <= 0
        outputWs = Rebin(InputWorkspace=inputWs, OutputWorkspace=outputWs, Params=str(binWidth))
        if inputIsDistribution:
            inputWs = ConvertToDistribution(inputWs)
        self.setProperty(self._PROP_OUTPUT_WS, outputWs)

AlgorithmFactory.subscribe(RebinToMeanBinWidth)
