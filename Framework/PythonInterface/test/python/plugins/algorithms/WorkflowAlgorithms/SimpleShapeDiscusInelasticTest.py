# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (SimpleShapeDiscusInelastic, Load, ConvertUnits,
                              DeleteWorkspace)
import unittest


class SimpleShapeDiscusInelasticTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        red_ws = Load('irs26176_graphite002_red.nxs')
        red_ws.run().addProperty("deltaE-mode", "Indirect", True);
        red_ws.run().addProperty("Ei", 1.845, True);

        self._red_ws = red_ws

        sqw_ws = Load('iris26176_graphite002_sqw.nxs')
        self._sqw_ws = sqw_ws

        self._arguments = {'SampleChemicalFormula': 'H2-O',
                           'SampleMassDensity': 1.0,
                           'NeutronPathsSingle': 50,
                           'NeutronPathsMultiple': 50,
                           'Height': 2.0,
                           'NumberScatterings': 2}

        self._annulus_arguments = self._arguments.copy()
        self._annulus_arguments.update({
            'Shape': 'Annulus',
            'SampleOuterRadius': 2.0
        })

    @classmethod
    def tearDownClass(self):
        DeleteWorkspace(self._red_ws)
        DeleteWorkspace(self._sqw_ws)

    def _test_corrections_workspace(self, corr_ws_grp):
        number_ws = corr_ws_grp.getNumberOfEntries()
        # Scatter_1, Scatter_1_NoAbs, Scatter_2, Scatter_2_2_Summed
        self.assertEqual(number_ws,4)

        for i in range(number_ws):
            x_unit = corr_ws_grp[i].getAxis(0).getUnit().unitID()
            self.assertEqual(x_unit, 'DeltaE')

            y_unit = corr_ws_grp[i].YUnitLabel()
            self.assertEqual(y_unit, 'Scattered Weight')

            num_hists = corr_ws_grp[i].getNumberHistograms()
            self.assertEqual(num_hists, 10)

            blocksize = corr_ws_grp[i].blocksize()
            self.assertEqual(blocksize, 1905)

    def test_flat_plate(self):
        # Test flat plate shape

        kwargs = self._arguments
        results = SimpleShapeDiscusInelastic(ReducedWorkspace=self._red_ws,
                                             SqwWorkspace=self._sqw_ws,
                                             Shape='FlatPlate',
                                             Width=2.0,
                                             Thickness=2.0,
                                             **kwargs)

        self._test_corrections_workspace(results)

    def test_cylinder(self):
        # Test cylinder shape

        kwargs = self._arguments
        results = SimpleShapeDiscusInelastic(ReducedWorkspace=self._red_ws,
                                             SqwWorkspace=self._sqw_ws,
                                             Shape='Cylinder',
                                             SampleRadius=2.0,
                                             **kwargs)

        self._test_corrections_workspace(results)

    def test_annulus(self):
        # Test annulus shape

        kwargs = self._annulus_arguments
        results = SimpleShapeDiscusInelastic(ReducedWorkspace=self._red_ws,
                                             SqwWorkspace=self._sqw_ws,
                                             SampleInnerRadius=1.0,
                                             **kwargs)

        self._test_corrections_workspace(results)


    # ------------------------------------- Failure Cases --------------------

    def test_no_chemical_formula_or_cross_sections_causes_an_error(self):
        kwargs = {'ReducedWorkspace': self._red_ws,
                  'SqwWorkspace' :self._sqw_ws,
                  'SampleMassDensity': 1.0,
                  'NeutronPathsSingle': 50,
                  'NeutronPathsMultiple': 50,
                  'Height': 2.0,
                  'Shape': 'FlatPlate',
                  'Width': 1.4,
                  'Thickness': 2.1}

        with self.assertRaises(RuntimeError):
            SimpleShapeDiscusInelastic(**kwargs)

    def test_flat_plate_no_params(self):
        # If the shape is flat plate but the relevant parameters haven't been entered this should throw
        # relevant params are Height, Width, Thickness

        kwargs = {'ReducedWorkspace': self._red_ws,
                  'SqwWorkspace': self._sqw_ws,
                  'ChemicalFormula': 'H2-O',
                  'SampleMassDensity': 1.0,
                  'NeutronPathsSingle': 50,
                  'NeutronPathsMultiple': 50,
                  'Shape': 'FlatPlate'}

        with self.assertRaises(RuntimeError):
            SimpleShapeDiscusInelastic(**kwargs)

    def test_not_in_deltaE(self):
        red_ws_not_deltaE = Load('irs26176_graphite002_red.nxs')

        red_ws_not_deltaE = ConvertUnits(
            InputWorkspace=self._red_ws,
            Target='Wavelength',
            EMode='Indirect',
            EFixed=1.845)

        kwargs = {'ReducedWorkspace': red_ws_not_deltaE,
                  'SqwWorkspace': self._sqw_ws,
                  'ChemicalFormula': 'H2-O',
                  'SampleMassDensity': 1.0,
                  'NeutronPathsSingle': 50,
                  'NeutronPathsMultiple': 50,
                  'Height': 2.0,
                  'Shape': 'FlatPlate',
                  'Width': 1.4,
                  'Thickness': 2.1}

        with self.assertRaises(RuntimeError):
            SimpleShapeDiscusInelastic(**kwargs)

        DeleteWorkspace(red_ws_not_deltaE)


if __name__ == "__main__":
    unittest.main()
