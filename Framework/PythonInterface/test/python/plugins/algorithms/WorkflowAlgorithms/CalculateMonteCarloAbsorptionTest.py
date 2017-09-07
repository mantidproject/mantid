from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import (Load, DeleteWorkspace, CalculateMonteCarloAbsorption)

import unittest

class CalculateMonteCarloAbsorptionTest(unittest.TestCase):

    _red_ws = None
    _container_ws = None
    _indirect_elastic_ws = None

    def setUp(self):
        red_ws = Load('irs26176_graphite002_red.nxs')
        self._red_ws = red_ws
        container_ws = Load('irs26173_graphite002_red.nxs')
        self._container_ws = container_ws

        self._arguments = {'SampleWorkspace': self._red_ws,
                           'SampleChemicalFormula': 'H2-O',
                           'SampleDensityType': 'Mass Density',
                           'SampleDensity': 1.0,
                           'EventsPerPoint': 200,
                           'BeamHeight': 3.5,
                           'BeamWidth': 4.0,
                           'Height': 2.0 }
        self._test_arguments = dict()

    def tearDown(self):
        DeleteWorkspace(self._red_ws)

    def _setup_container(self):
        container_args = {'ContainerWorkspace':self._container_ws,
                          'ContainerChemicalFormula':'Al',
                          'ContainerDensityType':'Mass Density',
                          'ContainerDensity':1.0,
                          'ContainerFrontThickness':1.5,
                          'ContainerBackThickness':1.5 }
        self._arguments.update(container_args)

    def _setup_indirect_elastic(self):
        pass

    def _test_corrections_workspace(self, corr_ws):
        x_unit = corr_ws.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit, self._sample_unit)

        y_unit = corr_ws.YUnitLabel()
        self.assertEquals(y_unit, 'Attenuation factor')

        num_hists = corr_ws.getNumberHistograms()
        self.assertEquals(num_hists, 10)

        blocksize = corr_ws.blocksize()
        self.assertEquals(blocksize, 1905)

    def _test_corrections_workspaces(self, workspaces):
        self.assertNotEquals(workspaces, None)

        for workspace in workspaces:
            self._test_corrections_workspace(workspace)

    def _run_correction_and_test(self, shape):
        arguments = self._arguments.copy()
        arguments.update(self._test_arguments)
        self._sample_unit = arguments['SampleWorkspace'].getAxis(0).getUnit().unitID()
        corrected = CalculateMonteCarloAbsorption(Shape=shape,
                                                  **arguments)
        self._test_corrections_workspaces(corrected)

    def _run_correction_with_container_test(self, shape):
        self._setup_container()
        self._run_correction_and_test(shape)

    def _run_indirect_elastic_test(self, shape):
        self._setup_indirect_elastic()
        self._run_correction_and_test(shape)

    def _flat_plate_test(self, test_func):
        self._test_arguments['SampleWidth'] = 2.0
        self._test_arguments['SampleThickness'] = 2.0
        test_func('FlatPlate')

    def _annulus_test(self, test_func, shape='Annulus'):
        self._test_arguments.clear()
        self._test_arguments['SampleRadius'] = 0.2
        self._test_arguments['SampleInnerRadius'] = 1.0
        self._test_arguments['SampleOuterRadius'] = 1.5
        self._test_arguments['ContainerInnerRadius'] = 0.5
        self._test_arguments['ContainerOuterRadius'] = 2.0
        test_func(shape)

    def _cylinder_test(self, test_func):
        self._annulus_test(test_func, shape='Cylinder')

    def test_flat_plate_no_container(self):
        self._flat_plate_test(self._run_correction_and_test)

    def test_cylinder_no_container(self):
        self._cylinder_test(self._run_correction_and_test)

    def test_annulus_no_container(self):
        self._annulus_test(self._run_correction_and_test)

    def test_flat_plate_with_container(self):
        self._flat_plate_test(self._run_correction_with_container_test)

    def test_cylinder_with_container(self):
        self._cylinder_test(self._run_correction_with_container_test)

    def test_annulus_with_container(self):
        self._annulus_test(self._run_correction_with_container_test)

    def test_flat_plate_indirect_elastic(self):
        self._flat_plate_test(self._run_indirect_elastic_test)

    def test_cylinder_indirect_elastic(self):
        self._cylinder_test(self._run_indirect_elastic_test)

    def test_annulus_indirect_elastic(self):
        self._annulus_test(self._run_indirect_elastic_test)

if __name__ == "__main__":
    unittest.main()
