# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
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
        container_ws = Load('irs26173_graphite002_red.nxs')
        indirect_elastic_ws = Load('osi104367_elf.nxs')
        self._red_ws = red_ws
        self._container_ws = container_ws
        self._indirect_elastic_ws = indirect_elastic_ws
        self._expected_unit = self._red_ws.getAxis(0).getUnit().unitID()
        self._expected_hist = 10
        self._expected_blocksize = 1905

        self._arguments = {'SampleChemicalFormula': 'H2-O',
                           'SampleDensityType': 'Mass Density',
                           'SampleDensity': 1.0,
                           'EventsPerPoint': 200,
                           'BeamHeight': 3.5,
                           'BeamWidth': 4.0,
                           'Height': 2.0 }

        self._container_args = {'ContainerWorkspace':self._container_ws,
                                'ContainerChemicalFormula':'Al',
                                'ContainerDensityType':'Mass Density',
                                'ContainerDensity':1.0 }
        self._test_arguments = dict()

    def tearDown(self):
        DeleteWorkspace(self._red_ws)

        if self._container_ws is not None:
            DeleteWorkspace(self._container_ws)
        if self._indirect_elastic_ws is not None:
            DeleteWorkspace(self._indirect_elastic_ws)

    def _setup_flat_plate_container(self):
        self._test_arguments['ContainerFrontThickness'] = 1.5
        self._test_arguments['ContainerBackThickness'] = 1.5

    def _setup_annulus_container(self):
        self._test_arguments['ContainerInnerRadius'] = 1.0
        self._test_arguments['ContainerOuterRadius'] = 2.0

    def _test_corrections_workspace(self, corr_ws):
        x_unit = corr_ws.getAxis(0).getUnit().unitID()
        self.assertEqual(x_unit, self._expected_unit)

        y_unit = corr_ws.YUnitLabel()
        self.assertEqual(y_unit, 'Attenuation factor')

        num_hists = corr_ws.getNumberHistograms()
        self.assertEqual(num_hists, self._expected_hist)

        blocksize = corr_ws.blocksize()
        self.assertEqual(blocksize, self._expected_blocksize)

    def _test_corrections_workspaces(self, workspaces):
        self.assertNotEquals(workspaces, None)

        for workspace in workspaces:
            self._test_corrections_workspace(workspace)

    def _run_correction_and_test(self, shape, sample_ws=None):

        if sample_ws is None:
            sample_ws = self._red_ws

        arguments = self._arguments.copy()
        arguments.update(self._test_arguments)
        corrected = CalculateMonteCarloAbsorption(SampleWorkspace=sample_ws,
                                                  Shape=shape,
                                                  **arguments)
        self._test_corrections_workspaces(corrected)

    def _run_correction_with_container_test(self, shape):
        self._test_arguments.update(self._container_args)

        if shape == 'FlatPlate':
            self._setup_flat_plate_container()
        else:
            self._setup_annulus_container()

        self._run_correction_and_test(shape)

    def _run_indirect_elastic_test(self, shape):
        self._expected_unit = "MomentumTransfer"
        self._expected_hist = 17
        self._expected_blocksize = 1
        self._run_correction_and_test(shape, self._indirect_elastic_ws)

    def _flat_plate_test(self, test_func):
        self._test_arguments['SampleWidth'] = 2.0
        self._test_arguments['SampleThickness'] = 2.0
        test_func('FlatPlate')

    def _annulus_test(self, test_func):
        self._test_arguments['SampleInnerRadius'] = 1.2
        self._test_arguments['SampleOuterRadius'] = 1.8
        test_func('Annulus')

    def _cylinder_test(self, test_func):
        self._test_arguments['SampleRadius'] = 0.5
        test_func('Cylinder')

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
