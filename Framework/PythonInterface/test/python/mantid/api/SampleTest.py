# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CreateWorkspace
from mantid.simpleapi import SetSampleMaterial
from mantid.geometry import CrystalStructure
from mantid.geometry import CSGObject
from numpy import pi

class SampleTest(unittest.TestCase):

    def setUp(self):
        self._ws = CreateWorkspace(DataX=[1,2,3,4,5], DataY=[1,2,3,4,5], OutputWorkspace="dummy")

    def test_geometry_getters_and_setters(self):
        sample = self._ws.sample()

        sample.setThickness(12.5)
        self.assertEquals(sample.getThickness(), 12.5)
        sample.setHeight(10.2)
        self.assertEquals(sample.getHeight(), 10.2)
        sample.setWidth(5.9)
        self.assertEquals(sample.getWidth(), 5.9)

    def test_crystal_structure_handling(self):
        sample = self._ws.sample()

        self.assertEquals(sample.hasCrystalStructure(), False)
        self.assertRaises(RuntimeError, sample.getCrystalStructure)

        cs = CrystalStructure('5.43 5.43 5.43',
                              'F d -3 m',
                              'Si 0 0 0 1.0 0.01')

        sample.setCrystalStructure(cs)

        self.assertEquals(sample.hasCrystalStructure(), True)

        cs_from_sample = sample.getCrystalStructure()

        self.assertEquals(cs.getSpaceGroup().getHMSymbol(), cs_from_sample.getSpaceGroup().getHMSymbol())
        self.assertEquals(cs.getUnitCell().a(), cs_from_sample.getUnitCell().a())
        self.assertEquals(len(cs.getScatterers()), len(cs_from_sample.getScatterers()))
        self.assertEquals(cs.getScatterers()[0], cs_from_sample.getScatterers()[0])


        sample.clearCrystalStructure()

        self.assertEquals(sample.hasCrystalStructure(), False)
        self.assertRaises(RuntimeError, sample.getCrystalStructure)

    def test_material(self):
        SetSampleMaterial(self._ws,"Al2 O3",SampleMassDensity=4)
        material = self._ws.sample().getMaterial()

        self.assertAlmostEqual(material.numberDensity, 0.1181, places=4)
        self.assertAlmostEqual(material.relativeMolecularMass(), 101.961, places=3)

        atoms, numatoms = material.chemicalFormula()

        self.assertEquals(len(atoms), len(numatoms))
        self.assertEquals(len(atoms), 2)
        self.assertEquals(numatoms[0], 2)
        self.assertEquals(numatoms[1], 3)

        xs0 = atoms[0].neutron()
        xs1 = atoms[1].neutron()
        # the correct way to calculate for coherent cross section
        # is to average the scattering lengths then convert to a cross section
        b_real = (xs0['coh_scatt_length_real']*2 + xs1['coh_scatt_length_real']*3) / 5
        b_imag = (xs0['coh_scatt_length_img']*2 + xs1['coh_scatt_length_img']*3) / 5
        xs = .04 * pi * (b_real * b_real + b_imag * b_imag)
        self.assertAlmostEquals(material.cohScatterXSection(), xs, places=4)

    def test_get_shape(self):
        sample = self._ws.sample()
        self.assertEquals(type(sample.getShape()), CSGObject)

    def test_get_shape_xml(self):
        sample = self._ws.sample()
        shape = sample.getShape()
        xml = shape.getShapeXML()
        self.assertEquals(type(xml), str)




if __name__ == '__main__':
    unittest.main()
