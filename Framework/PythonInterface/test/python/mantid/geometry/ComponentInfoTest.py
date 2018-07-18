from __future__ import (absolute_import, division, print_function)

import unittest
import argparse
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.kernel import Quat

class ComponentInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    def test_detectorsInSubtree(self):
        """ Test that a list of detectors is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.detectorsInSubtree(0)), list)

    def test_componentsInSubtree(self):
        """ Test that a list of components is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.componentsInSubtree(0)), list)

    def test_size(self):
        """ Check that there are only 6 components """
        info = self._ws.componentInfo()
        self.assertEquals(info.size(), 6)

    def test_isDetector(self):
        """Check which components are detectors """
        info = self._ws.componentInfo()
        self.assertEquals(info.isDetector(0), True)
        self.assertEquals(info.isDetector(1), True)
        self.assertEquals(info.isDetector(2), False)
        self.assertEquals(info.isDetector(3), False)
        self.assertEquals(info.isDetector(4), False)
        self.assertEquals(info.isDetector(5), False)

    def test_hasDetectorInfo(self):
        """ Check if detector information is available """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasDetectorInfo(), True)

    def test_position(self):
        """ Test that the component's position is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.position(0)), V3D)

    def test_rotation(self):
        """ Test that the component's rotation is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.rotation(0)), Quat)

    def test_relativePosition(self):
        """ Test that the component's relative position is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.relativePosition(0)), V3D)

    def test_relativeRotation(self):
        """ Test that the component's relative rotation is returned. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.relativeRotation(0)), Quat)

    def test_setPosition(self):
        """ Test that the component's position can be set correctly. """
        info = self._ws.componentInfo()
        pos = V3D(0,0,0)
        info.setPosition(0, pos)
        retPos = info.position(0)
        self.assertEquals(pos, retPos)

    def test_setRotation(self):
        """ Test that the component's rotation can be set correctly. """
        info = self._ws.componentInfo()
        quat = Quat(0,0,0,0)
        info.setRotation(0, quat)
        retQuat = info.rotation(0)
        self.assertEquals(quat, retQuat)

    def test_sourcePosition(self):
        """ Check that the source postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.sourcePosition()), V3D)

    def test_samplePosition(self):
        """ Check that the sample postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.samplePosition()), V3D)

    def test_hasSource(self):
        """ Check if there is a source """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasSource(), True)

    def test_hasSample(self):
        """ Check if there is a sample """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasSample(), True)

    def test_name(self):
        """ Get the name of a component as a string """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.name(0)), str)


    """ 
    The section below is for destructive tests - i.e extreme
    and exceptional test cases. 
    """

    def test_detectorsInSubtree_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.detectorsInSubtree("Error")
        with self.assertRaises(TypeError):
        	info.detectorsInSubtree(10.0)

    def test_componentsInSubtree_exceptional(self):
	    info = self._ws.componentInfo()
	    with self.assertRaises(TypeError):
	    	info.componentsInSubtree("Error")
	    with self.assertRaises(TypeError):
	    	info.componentsInSubtree(10.0)

    def test_size_exceptional(self):
    	info = self._ws.componentInfo()
    	with self.assertRaises(TypeError):
    		info.size(0)

    def test_isDetector_exceptional(self):
    	info = self._ws.componentInfo()
    	with self.assertRaises(TypeError):
    		info.isDetector("Error")
    	with self.assertRaises(TypeError):
    		info.isDetector(10.0)

    def test_hasDetectorInfo_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.hasDetectorInfo(0)

    def test_position_exceptional(self):
		info = self._ws.componentInfo()
		with self.assertRaises(TypeError):
			info.position("Zero")
		with self.assertRaises(TypeError):
			info.position(0.0)

    def test_rotation_exceptional(self):
		info = self._ws.componentInfo()
		with self.assertRaises(TypeError):
			info.rotation("Zero")
		with self.assertRaises(TypeError):
			info.rotation(0.0)

    def test_relativePosition_exceptional(self):
		info = self._ws.componentInfo()
		with self.assertRaises(TypeError):
			info.relativePosition("Zero")
		with self.assertRaises(TypeError):
			info.relativePosition(0.0)

    def test_relativeRotation_exceptional(self):
		info = self._ws.componentInfo()
		with self.assertRaises(TypeError):
			info.relativeRotation("Zero")
		with self.assertRaises(TypeError):
			info.relativeRotation(0.0)

    def test_setPosition_exceptional(self):
        info = self._ws.componentInfo()
        pos = [0,0,0]
        with self.assertRaises(TypeError):
        	info.setPosition(0, pos)

    def test_setRotation_exceptional(self):
        info = self._ws.componentInfo()
        rot = [0,0,0,0]
        with self.assertRaises(TypeError):
        	info.setRotation(0, rot)

    def test_sourcePosition_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.sourcePosition(0)

    def test_samplePosition_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.samplePosition(0)

    def test_hasSource_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.hasSource(0)

    def test_hasSample_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.hasSample(0)

    def test_name_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.name("Name")
        with self.assertRaises(TypeError):
    		info.name(0.12)

if __name__ == '__main__':
    unittest.main()
