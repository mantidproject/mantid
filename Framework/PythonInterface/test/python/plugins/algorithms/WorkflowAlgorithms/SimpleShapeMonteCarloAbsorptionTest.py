from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import (SimpleShapeMonteCarloAbsorption, Load, ConvertUnits,
                              CompareWorkspaces, SetSampleMaterial, CreateSampleWorkspace,
                              DeleteWorkspace)
from mantid.kernel import *
from mantid.api import *

import unittest


class SimpleShapeMonteCarloAbsorptionTest(unittest.TestCase):
    def setUp(self):
        red_ws = Load('irs26176_graphite002_red.nxs')
        red_ws = ConvertUnits(InputWorkspace=red_ws, Target='Wavelength', EMode='Indirect', EFixed=1.845)

        self._arguments = {'ChemicalFormula': 'H2-O',
                           'DensityType': 'Mass Density',
                           'Density': 1.0,
                           'EventsPerPoint': 200,
                           'BeamHeight': 3.5,
                           'BeamWidth': 4.0,
                           'Height': 2.0}

        self._red_ws = red_ws

        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='FlatPlate',
                                                    Width=2.0,
                                                    Thickness=2.0,
                                                    **self._arguments)

        # store the basic flat plate workspace so it can be compared with others
        self._corrected_flat_plate = corrected

    def _test_corrections_workspace(self, corr_ws):
        x_unit = corr_ws.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit, 'Wavelength')

        y_unit = corr_ws.YUnitLabel()
        self.assertEquals(y_unit, 'Attenuation factor')

        num_hists = corr_ws.getNumberHistograms()
        self.assertEquals(num_hists, 10)

        blocksize = corr_ws.blocksize()
        self.assertEquals(blocksize, 1905)

    def tearDown(self):
        DeleteWorkspace(self._red_ws)
        DeleteWorkspace(self._corrected_flat_plate)

    def test_flat_plate(self):
        # Test flat plate shape

        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='FlatPlate',
                                                    Width=2.0,
                                                    Thickness=2.0,
                                                    **kwargs)

        self._test_corrections_workspace(corrected)

    def test_cylinder(self):
        # Test cylinder shape

        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='Cylinder',
                                                    Radius=2.0,
                                                    **kwargs)

        self._test_corrections_workspace(corrected)

    def test_annulus(self):
        # Test annulus shape

        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='Annulus',
                                                    InnerRadius=1.0,
                                                    OuterRadius=2.0,
                                                    **kwargs)

        self._test_corrections_workspace(corrected)

    def test_number_density(self):
        # Mass Density for water is 1.0
        # Number Density for water is 0.033428
        # These should give similar results

        kwargs = self._arguments
        kwargs['DensityType'] = 'Number Density'
        kwargs['Density'] = 0.033428

        corrected_num = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                        Shape='FlatPlate',
                                                        Width=2.0,
                                                        Thickness=2.0,
                                                        **kwargs)

        # _corrected_flat_plate is with mass density 1.0
        CompareWorkspaces(self._corrected_flat_plate, corrected_num, Tolerance=1e-6)
        DeleteWorkspace(corrected_num)

    def test_material_already_defined(self):
        SetSampleMaterial(InputWorkspace=self._red_ws,
                          ChemicalFormula='H2-O',
                          SampleMassDensity=1.0)

        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    MaterialAlreadyDefined=True,
                                                    EventsPerPoint=200,
                                                    BeamHeight=3.5,
                                                    BeamWidth=4.0,
                                                    Height=2.0,
                                                    Shape='FlatPlate',
                                                    Width=2.0,
                                                    Thickness=2.0)

        self._test_corrections_workspace(corrected)
        CompareWorkspaces(self._corrected_flat_plate, corrected, Tolerance=1e-6)

    def test_ILL_reduced(self):

        ill_red_ws = Load('ILL/IN16B/091515_red.nxs')

        ill_red_ws = ConvertUnits(ill_red_ws, Target='Wavelength', EMode='Indirect', EFixed=1.845)

        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=ill_red_ws,
                                                    Shape='FlatPlate',
                                                    Width=2.0,
                                                    Thickness=2.0,
                                                    **kwargs)

        x_unit = corrected.getAxis(0).getUnit().unitID()
        self.assertEquals(x_unit, 'Wavelength')

        y_unit = corrected.YUnitLabel()
        self.assertEquals(y_unit, 'Attenuation factor')

        num_hists = corrected.getNumberHistograms()
        self.assertEquals(num_hists, 18)

        blocksize = corrected.blocksize()
        self.assertEquals(blocksize, 1024)

        DeleteWorkspace(ill_red_ws)

    # TODO: add test for powder diffraction data

    # ------------------------------------- Failure Cases ----------------------------------------

    def test_no_chemical_formula(self):
        kwargs = {'InputWorkspace': self._red_ws,
                  'MaterialAlreadyDefined': False,
                  'DensityType': 'Mass Density',
                  'Density': 1.0,
                  'EventsPerPoint': 200,
                  'BeamHeight': 3.5,
                  'BeamWidth': 4.0,
                  'Height': 2.0,
                  'Shape': 'FlatPlate',
                  'Width': 1.4,
                  'Thickness': 2.1}

        self.assertRaises(RuntimeError, SimpleShapeMonteCarloAbsorption, **kwargs)

    def test_flat_plate_no_params(self):
        # If the shape is flat plate but the relevant parameters haven't been entered this should throw
        # relevant params are Height, Width, Thickness

        kwargs = {'InputWorkspace': self._red_ws,
                  'ChemicalFormula': 'H2-O',
                  'DensityType': 'Mass Density',
                  'Density': 1.0,
                  'EventsPerPoint': 200,
                  'BeamHeight': 3.5,
                  'BeamWidth': 4.0,
                  'Shape': 'FlatPlate'}

        self.assertRaises(RuntimeError, SimpleShapeMonteCarloAbsorption, **kwargs)

    def test_not_in_wavelength(self):
        red_ws_not_wavelength = Load('irs26176_graphite002_red.nxs')

        kwargs = {'InputWorkspace': red_ws_not_wavelength,
                  'ChemicalFormula': 'H2-O',
                  'DensityType': 'Mass Density',
                  'Density': 1.0,
                  'EventsPerPoint': 200,
                  'BeamHeight': 3.5,
                  'BeamWidth': 4.0,
                  'Height': 2.0,
                  'Shape': 'FlatPlate',
                  'Width': 1.4,
                  'Thickness': 2.1}

        self.assertRaises(RuntimeError, SimpleShapeMonteCarloAbsorption, **kwargs)
        DeleteWorkspace(red_ws_not_wavelength)


if __name__ == "__main__":
    unittest.main()
