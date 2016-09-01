# -*- coding: utf-8 -*-

from mantid.api import DataProcessorAlgorithm, AlgorithmFactory

class DGSReductionILL(DataProcessorAlgorithm):
    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        return 'Workflow\\Inelastic'

    def name(self):
        return 'DGSReductionILL'

    def summary(self):
        return 'Data reduction workflow for the direct geometry time-of-flight spectrometers at ILL'

    def version(self):
        return 1

    def PyExec(self):
        pass

    def PyInit(self):
        pass

AlgorithmFactory.subscribe(DGSReductionILL)
