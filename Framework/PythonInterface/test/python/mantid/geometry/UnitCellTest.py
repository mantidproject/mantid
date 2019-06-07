# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import testhelpers
from mantid.geometry import UnitCell, AngleUnits
from mantid.kernel import V3D
import numpy as np

class UnitCellTest(unittest.TestCase):

    def test_invalid_parameters_throw(self):
        self.assertRaises(RuntimeError, UnitCell, 0, 0, 0, 0, 0, 0)

    def test_simple_constructor(self):
        u1 = UnitCell()
        self.assertEqual(u1.a1(), 1)
        self.assertEqual(u1.alpha(), 90)

        u2 = UnitCell(3,4,5)
        self.assertAlmostEquals(u2.b1(),1./3., 10)
        self.assertAlmostEquals(u2.alphastar(), 90, 10)

        u4 = u2
        self.assertAlmostEquals(u4.volume(),1./u2.recVolume(),10)
        u2.seta(3)
        self.assertAlmostEquals(u2.a(),3,10)

    def test_numpy_array_conversion(self):
        row0 = (0.162546756312, 0.00815256992072, -0.00145274558861)
        row1 = (row0[1], 0.028262965555, 0.00102046431298)
        row2 = (row0[2], row1[2], 0.0156808990098 )
        gstar = np.array( [row0,row1,row2] )

        u = UnitCell()
        testhelpers.assertRaisesNothing(self, u.recalculateFromGstar, gstar)
        self._check_cell(u)

    def test_to_string(self):
        unit = UnitCell(3,3,3)
        expected_str = "UnitCell with lattice parameters: a = 3 b = 3 c = 3 "\
                       "alpha = 90 beta = 90 gamma = 90"
        expected_repr = "UnitCell(3, 3, 3, 90, 90, 90)"

        self.assertEqual(expected_str, str(unit))
        self.assertEqual(expected_repr, unit.__repr__())

        newUnit = eval(unit.__repr__())
        self.assertEqual(unit.a(), newUnit.a())
        self.assertEqual(unit.b(), newUnit.b())
        self.assertEqual(unit.c(), newUnit.c())

        self.assertEqual(unit.alpha(), newUnit.alpha())
        self.assertEqual(unit.beta(), newUnit.beta())
        self.assertEqual(unit.gamma(), newUnit.gamma())

    def _check_cell(self, cell):
        self.assertAlmostEqual(cell.a(),2.5,10)
        self.assertAlmostEqual(cell.b(),6,10)
        self.assertAlmostEqual(cell.c(),8,10)
        self.assertAlmostEqual(cell.alpha(),93,10)
        self.assertAlmostEqual(cell.beta(),88,10)
        self.assertAlmostEqual(cell.gamma(),97,10)

        # get the some elements of the B matrix
        self.assertEqual(type(cell.getB()), np.ndarray)
        self.assertAlmostEqual(cell.getB()[0][0],0.403170877311,10)
        self.assertAlmostEqual(cell.getB()[2][0],0.0,10)
        self.assertAlmostEqual(cell.getB()[0][2],-0.00360329991666,10)
        self.assertAlmostEqual(cell.getB()[2][2],0.125,10)

        # d spacing for direct lattice at (1,1,1) (will automatically check dstar)
        self.assertAlmostEqual(cell.d(1.,1.,1.),2.1227107587,10)
        self.assertAlmostEqual(cell.d(V3D(1.,1.,1.)),2.1227107587,10)
        # angle
        self.assertAlmostEqual(cell.recAngle(1,1,1,1,0,0,AngleUnits.Radians),0.471054990614,10)

        self.assertEqual(type(cell.getG()), np.ndarray)
        self.assertEqual(type(cell.getGstar()), np.ndarray)


if __name__ == '__main__':
    unittest.main()
