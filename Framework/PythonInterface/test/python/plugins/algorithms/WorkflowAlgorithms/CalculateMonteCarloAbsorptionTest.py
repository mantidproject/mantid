from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (Load, DeleteWorkspace)

import unittest

class SimpleShapeMonteCarloAbsorptionTest(unittest.TestCase):

    def setUp(self):
        self._red_ws = Load('irs26176_graphite002_red.nxs')

        self._arguments = {'SampleWorkspace': self._red_ws,
                           'SampleChemicalFormula': 'H2-O',
                           'SampleDensityType': 'Mass Density',
                           'SampleDensity': 1.0,
                           'EventsPerPoint': 200,
                           'BeamHeight': 3.5,
                           'BeamWidth': 4.0,
                           'Height': 2.0, }

    def tearDown(self):
        DeleteWorkspace(self._red_ws)
