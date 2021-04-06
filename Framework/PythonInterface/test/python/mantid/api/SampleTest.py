# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateSampleWorkspace, CreatePeaksWorkspace, CreateWorkspace, SetSampleMaterial, SetUB
from mantid.geometry import CrystalStructure, CSGObject, OrientedLattice
from mantid.api import Sample
from numpy import pi
import copy


class SampleTest(unittest.TestCase):
    def test_lattice_accessors(self):
        instrument_ws = CreateSampleWorkspace()
        peaks = CreatePeaksWorkspace(instrument_ws, 0)
        SetUB(peaks, 1, 1, 1, 90, 90, 90)
        sample = peaks.sample()

        self.assertTrue(sample.hasOrientedLattice())
        self.assertTrue(isinstance(sample.getOrientedLattice(), OrientedLattice))
        sample.clearOrientedLattice()
        self.assertFalse(sample.hasOrientedLattice())

    def test_geometry_getters_and_setters(self):
        sample = Sample()
        sample.setThickness(12.5)
        self.assertEqual(sample.getThickness(), 12.5)
        sample.setHeight(10.2)
        self.assertEqual(sample.getHeight(), 10.2)
        sample.setWidth(5.9)
        self.assertEqual(sample.getWidth(), 5.9)

    def test_crystal_structure_handling(self):
        sample = Sample()

        self.assertEqual(sample.hasCrystalStructure(), False)
        self.assertRaises(RuntimeError, sample.getCrystalStructure)

        cs = CrystalStructure('5.43 5.43 5.43',
                              'F d -3 m',
                              'Si 0 0 0 1.0 0.01')

        sample.setCrystalStructure(cs)

        self.assertEqual(sample.hasCrystalStructure(), True)

        cs_from_sample = sample.getCrystalStructure()

        self.assertEqual(cs.getSpaceGroup().getHMSymbol(), cs_from_sample.getSpaceGroup().getHMSymbol())
        self.assertEqual(cs.getUnitCell().a(), cs_from_sample.getUnitCell().a())
        self.assertEqual(len(cs.getScatterers()), len(cs_from_sample.getScatterers()))
        self.assertEqual(cs.getScatterers()[0], cs_from_sample.getScatterers()[0])


        sample.clearCrystalStructure()

        self.assertEqual(sample.hasCrystalStructure(), False)
        self.assertRaises(RuntimeError, sample.getCrystalStructure)

    def test_material(self):
        ws = CreateWorkspace(DataX=[1], DataY=[1], StoreInADS=False)
        sample = ws.sample()
        SetSampleMaterial(ws,"Al2 O3",SampleMassDensity=4, StoreInADS=False)
        material = sample.getMaterial()

        self.assertAlmostEqual(material.numberDensity, 0.1181, places=4)
        self.assertAlmostEqual(material.relativeMolecularMass(), 101.961, places=3)

        atoms, numatoms = material.chemicalFormula()

        self.assertEqual(len(atoms), len(numatoms))
        self.assertEqual(len(atoms), 2)
        self.assertEqual(numatoms[0], 2)
        self.assertEqual(numatoms[1], 3)

        xs0 = atoms[0].neutron()
        xs1 = atoms[1].neutron()
        # the correct way to calculate for coherent cross section
        # is to average the scattering lengths then convert to a cross section
        b_real = (xs0['coh_scatt_length_real']*2 + xs1['coh_scatt_length_real']*3) / 5
        b_imag = (xs0['coh_scatt_length_img']*2 + xs1['coh_scatt_length_img']*3) / 5
        xs = .04 * pi * (b_real * b_real + b_imag * b_imag)
        self.assertAlmostEquals(material.cohScatterXSection(), xs, places=4)

    def test_get_shape(self):
        sample = Sample()
        self.assertEqual(type(sample.getShape()), CSGObject)

    def test_get_shape_xml(self):
        sample = Sample()
        shape = sample.getShape()
        xml = shape.getShapeXML()
        self.assertEqual(type(xml), str)

    def do_test_copyable(self, copy_op):
        original = Sample()
        width = 1.0
        height = 2.0
        thickness = 3.0
        original.setThickness(thickness)
        original.setHeight(height)
        original.setWidth(width)
        # make copy
        cp = copy_op(original)
        # Check identity different
        self.assertNotEqual(id(original), id(cp))
        # Simple tests that cp is equal to original
        self.assertEqual(original.getHeight(), cp.getHeight())
        self.assertEqual(original.getWidth(), cp.getWidth())
        self.assertEqual(original.getThickness(), cp.getThickness())
        # Check really did succeed and is not tied in any way to original
        del original
        self.assertTrue(id(cp) > 0)
        self.assertEqual(height, cp.getHeight())
        self.assertEqual(width, cp.getWidth())
        self.assertEqual(thickness, cp.getThickness())

    def test_shallow_copyable(self):
        self.do_test_copyable(copy.copy)

    def test_deep_copyable(self):
        self.do_test_copyable(copy.deepcopy)

    def test_equals(self):
        a = Sample()
        b = Sample()
        self.assertEqual(a, b)
        b.setThickness(10)
        self.assertNotEqual(a, b)

if __name__ == '__main__':
    unittest.main()
