import unittest
from mantid.api import Sample
from mantid.simpleapi import CreateWorkspace
from mantid.geometry import CrystalStructure

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




if __name__ == '__main__':
    unittest.main()
