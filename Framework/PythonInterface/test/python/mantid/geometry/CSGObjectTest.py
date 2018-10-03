# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

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
        inst = self._testws.getInstrument()
        pixel = inst.getComponentByName("pixel")
        shape = pixel.shape()
        self.assertTrue(isinstance(shape, CSGObject))
        xml = pixel.shape().getShapeXML()
        self.assertTrue('radius val="0.0127"' in xml)

    def test_boundingBox_retrieval(self):
        inst = self._testws.getInstrument()
        pixel = inst.getComponentByName("pixel")
        shape = pixel.shape()
        box = shape.getBoundingBox()
        self.assertTrue(isinstance(box, BoundingBox))

if __name__ == '__main__':
    unittest.main()
