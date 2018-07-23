from __future__ import (absolute_import, division, print_function)

import unittest
import argparse
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.kernel import Quat
from mantid.simpleapi import *

class ComponentInfoTest(unittest.TestCase):

    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False) # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    """
    ----------------------------------------------------------------------------
    Normal Tests
    ----------------------------------------------------------------------------

    The following test cases test normal usage of the exposed methods.
    """

    def test_len(self):
        """ Check that there are only 6 components """
        info = self._ws.componentInfo()
        self.assertEquals(len(info), 6)

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

    def test_detectorsInSubtree(self):
        """ Test that a list of detectors is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.detectorsInSubtree(0)), list)

    def test_componentsInSubtree(self):
        """ Test that a list of components is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.componentsInSubtree(0)), list)

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

    def test_hasSource(self):
        """ Check if there is a source """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasSource(), True)

    def test_hasSample(self):
        """ Check if there is a sample """
        info = self._ws.componentInfo()
        self.assertEquals(info.hasSample(), True)

    def test_source(self):
        """ Check if a source component is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.source()) , int)

    def test_sample(self):
        """ Check if a sample component is returned """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.sample()) , int)

    def test_sourcePosition(self):
        """ Check that the source postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.sourcePosition()), V3D)

    def test_samplePosition(self):
        """ Check that the sample postition is a V3D object """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.samplePosition()), V3D)

    def test_hasParent(self):
        """ Check if a component has a parent component """
        info = self._ws.componentInfo()
        self.assertTrue(info.hasParent(0))

    def test_parent(self):
        """ Check that for a component that has a parent, the parent
            component is retrieved. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.parent(0)), int)

    def test_children(self):
        """ Check that for a component that has children, the children
            components can be retrieved. """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.children(0)), list)

    def test_name(self):
        """ Get the name of a component as a string """
        info = self._ws.componentInfo()
        self.assertEquals(type(info.name(0)), str)

    def test_createWorkspaceAndComponentInfo(self):
        """ Try to create a workspace and see if ComponentInfo object is accessable """
        dataX = [1,2,3,4,5]
        dataY = [1,2,3,4,5]
        workspace = CreateWorkspace(DataX=dataX, DataY=dataY)
        info = workspace.componentInfo()
        self.assertEquals(info.size(), 1)


    """
    ----------------------------------------------------------------------------
    Extreme Tests
    ----------------------------------------------------------------------------

    The following test cases test around boundary cases for the exposed methods.
    """

    def test_isDetector_extreme(self):
    	info = self._ws.componentInfo()
    	with self.assertRaises(OverflowError):
    		info.isDetector(-1)

    def test_detectorsInSubtree_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
        	info.detectorsInSubtree(-1)
        self.assertEquals(type(info.detectorsInSubtree(5)), list)

    def test_componentsInSubtree_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
        	info.componentsInSubtree(-1)
        self.assertEquals(type(info.componentsInSubtree(5)), list)

    def test_position_extreme(self):
    	info = self._ws.componentInfo()
    	with self.assertRaises(OverflowError):
    		info.position(-1)
    	self.assertEquals(type(info.position(0)), V3D)
    	self.assertEquals(type(info.position(5)), V3D)

    def test_rotation_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.rotation(-1)
        self.assertEquals(type(info.rotation(0)), Quat)
        self.assertEquals(type(info.rotation(5)), Quat)

    def test_relativePosition_extreme(self):
    	info = self._ws.componentInfo()
    	with self.assertRaises(OverflowError):
    		info.relativePosition(-1)
    	self.assertEquals(type(info.relativePosition(0)), V3D)
    	self.assertEquals(type(info.relativePosition(5)), V3D)

    def test_relativeRotation_extreme(self):
    	info = self._ws.componentInfo()
    	with self.assertRaises(OverflowError):
    		info.relativeRotation(-1)
    	self.assertEquals(type(info.relativeRotation(0)), Quat)
    	self.assertEquals(type(info.relativeRotation(5)), Quat)

    def test_setPosition_extreme(self):
        info = self._ws.componentInfo()
        pos = V3D(0,0,0)
        with self.assertRaises(OverflowError):
            info.setPosition(-1, pos)

    def test_setRotation_extreme(self):
        info = self._ws.componentInfo()
        quat = Quat(0,0,0,0)
        with self.assertRaises(OverflowError):
            info.setRotation(-1, quat)

    def test_hasParent_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.hasParent(-1)
        self.assertTrue(info.hasParent(0))
        self.assertFalse(info.hasParent(5))

    def test_parent_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.parent(-1)
        self.assertEquals(type(info.parent(0)), int)
        self.assertEquals(type(info.parent(5)), int)

    def test_children_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.children(-1)
        self.assertEquals(type(info.children(0)), list)
        self.assertEquals(type(info.children(5)), list)

    def test_name_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.name(-1)
        self.assertEquals(type(info.name(0)), str)
        self.assertEquals(type(info.name(5)), str)


    """
    ----------------------------------------------------------------------------
    Exceptional Tests
    ----------------------------------------------------------------------------

    Each of the tests below tries to pass invalid parameters to the exposed
    methods and expect an error to be thrown.
    """

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

    def test_hasSource_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.hasSource(0)

    def test_hasSample_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.hasSample(0)

    def test_source_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.source(0)

    def test_sample_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.sample(0)

    def test_sourcePosition_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.sourcePosition(0)

    def test_samplePosition_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.samplePosition(0)

    def test_hasParent_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.hasParent("Zero")
        with self.assertRaises(TypeError):
            info.hasParent(0.0)

    def test_parent_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.parent("Zero")
        with self.assertRaises(TypeError):
            info.parent(0.0)

    def test_children_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.children("Zero")
        with self.assertRaises(TypeError):
            info.children(0.0)

    def test_name_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
        	info.name("Name")
        with self.assertRaises(TypeError):
    		info.name(0.12)

if __name__ == '__main__':
    unittest.main()
