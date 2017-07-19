from __future__ import (absolute_import, division, print_function)

from mantid.simpleapi import SimpleShapeMonteCarloAbsorption, Load, ConvertUnits, CompareWorkspaces, SetSampleMaterial
from mantid.kernel import *
from mantid.api import *

import unittest


class SimpleShapeMonteCarloAbsorptionTest(unittest.TestCase):
    def setUp(self):
        """

        """
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

    def test_flat_plate(self):
        """
        Test flat plate shape
        """
        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='FlatPlate',
                                                    Width=2.0,
                                                    Thickness=2.0,
                                                    **kwargs)

        self._corrected_flat_plate = corrected

        self._test_corrections_workspace(corrected)

    def test_cylinder(self):
        """
        Test cylinder shape
        """
        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='Cylinder',
                                                    Radius=2.0,
                                                    **kwargs)

        self._test_corrections_workspace(corrected)

    def test_annulus(self):
        """
        Test annulus shape
        """
        kwargs = self._arguments
        corrected = SimpleShapeMonteCarloAbsorption(InputWorkspace=self._red_ws,
                                                    Shape='Annulus',
                                                    InnerRadius=1.0,
                                                    OuterRadius=2.0,
                                                    **kwargs)

        self._test_corrections_workspace(corrected)

    def test_number_density(self):
        """
        Mass Density for water is 1.0
        Number Density for water is 0.033428
        These should give similar results
        """

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
        CompareWorkspaces(self._corrected_flat_plate, corrected_num, Tolerance=1e-6)

if __name__ == "__main__":
    unittest.main()
