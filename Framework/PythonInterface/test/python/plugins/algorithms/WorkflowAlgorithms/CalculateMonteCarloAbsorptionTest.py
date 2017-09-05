from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (Load, DeleteWorkspace, CalculateMonteCarloAbsorption)

import unittest

class SimpleShapeMonteCarloAbsorptionTest(unittest.TestCase):

    def setUp(self):
        self._red_ws = Load('irs26176_graphite002_red')

        self._arguments = {'SampleChemicalFormula': 'H2-O',
                           'SampleDensityType': 'Mass Density',
                           'SampleDensity': 1.0,
                           'EventsPerPoint': 200,
                           'BeamHeight': 3.5,
                           'BeamWidth': 4.0,
                           'Height': 2.0 }

    def tearDown(self):
        DeleteWorkspace(self._red_ws)

    def _setup_container(self):
        self._container_ws = Load('ALF26176')

        container_args = {'ContainerWorkspace':'irs26173_graphite002_red',
                          'ContainerChemicalFormula':'Al',
                          'ContainerDensityType':'MassDensity',
                          'ContainerDensity':1.0 }
        self._arguments.update(container_args)

    def test_flat_plate_no_container(self):
        corrected = CalculateMonteCarloAbsorption(SampleWorkspace=self._red_ws,
                                                  Shape='FlatPlate',
                                                  SampleWidth=2.0,
                                                  SampleThickness=2.0,
                                                  **self._arguments)
        self._test_corrections_workspace(corrected)


    def _test_corrections_workspace(self, corr_ws):
        x_unit = corr_ws.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit, 'Wavelength')

        y_unit = corr_ws.YUnitLabel()
        self.assertEquals(y_unit, 'Attenuation factor')

        num_hists = corr_ws.getNumberHistograms()
        self.assertEquals(num_hists, 10)

        blocksize = corr_ws.blocksize()
        self.assertEquals(blocksize, 1905)