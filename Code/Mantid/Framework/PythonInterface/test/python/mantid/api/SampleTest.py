import unittest
from mantid.api import Sample
from mantid.simpleapi import CreateWorkspace

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

if __name__ == '__main__':
    unittest.main()
