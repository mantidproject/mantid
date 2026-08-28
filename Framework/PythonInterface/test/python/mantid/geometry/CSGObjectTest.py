# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.geometry import BoundingBox, CSGObject


class CSGObjectTest(unittest.TestCase):
    _testws = None

    def setUp(self):
        import testhelpers

        if not self.__class__._testws:
            alg = testhelpers.run_algorithm("LoadEmptyInstrument", Filename="ALF_Definition.xml", child=True)
            self.__class__._testws = alg.getProperty("OutputWorkspace").value

    def test_objects_XML_can_be_retrieved(self):
        component_info = self._testws.componentInfo()
        shape = component_info.shape(component_info.indexOfAny("pixel"))
        self.assertTrue(isinstance(shape, CSGObject))
        xml = shape.getShapeXML()
        self.assertTrue('radius val="0.0127"' in xml)

    def test_boundingBox_retrieval(self):
        component_info = self._testws.componentInfo()
        shape = component_info.shape(component_info.indexOfAny("pixel"))
        box = shape.getBoundingBox()
        self.assertTrue(isinstance(box, BoundingBox))


if __name__ == "__main__":
    unittest.main()
