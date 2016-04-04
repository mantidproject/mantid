from mantid.kernel import Direction
from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, Progress)

import numpy as np
import math


class VesuvioThickness(PythonAlgorithm):

    _amplitude = None

    def summary(self):
        return "Produces the sample density for vesuvio " \
               + "based on sample transmission and composite masses"

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        self.declareProperty("Amplitude", 0.0,
                             doc="The amplitude of the peak") 


    def _get_properties(self):
        self._amplitude = self.getPropertyValue("Amplitude")
                             
    def validateInputs(self):

    def PyExec(self):
        d = d / 200.0
        b = math.sqrt((self._amplitude / (4 * math.pi)))


    def _free_xst(self, mass, b):
        g = 1.00867/mass
        xs = 4 * math.pi * math.pow(b,2) / pow((g + 1), 2)
        xs_total = np.sum(xs)

AlgorithmFactory.subscribe(VesuvioThickness)
