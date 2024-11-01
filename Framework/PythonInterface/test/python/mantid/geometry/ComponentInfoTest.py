# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import numpy as np
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.kernel import Quat
from mantid.geometry import CSGObject
from mantid.simpleapi import CloneWorkspace, CreateWorkspace


class ComponentInfoTest(unittest.TestCase):
    _ws = None

    def setUp(self):
        if self.__class__._ws is None:
            self.__class__._ws = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(2, 1, False)  # no monitors
            self.__class__._ws.getSpectrum(0).clearDetectorIDs()

    """
    ----------------------------------------------------------------------------
    Normal Tests
    ----------------------------------------------------------------------------

    The following test cases test normal usage of the exposed methods.
    """

    def test_len(self):
        """Check that there are only 6 components"""
        info = self._ws.componentInfo()
        self.assertEqual(len(info), 6)

    def test_size(self):
        """Check that there are only 6 components"""
        info = self._ws.componentInfo()
        self.assertEqual(info.size(), 6)

    def test_isDetector(self):
        """Check which components are detectors"""
        info = self._ws.componentInfo()
        self.assertEqual(info.isDetector(0), True)
        self.assertEqual(info.isDetector(1), True)
        self.assertEqual(info.isDetector(2), False)
        self.assertEqual(info.isDetector(3), False)
        self.assertEqual(info.isDetector(4), False)
        self.assertEqual(info.isDetector(5), False)

    def test_detectorsInSubtree(self):
        """Test that a list of detectors is returned"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.detectorsInSubtree(0)), np.ndarray)

    def test_componentsInSubtree(self):
        """Test that a list of components is returned"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.componentsInSubtree(0)), np.ndarray)

    def test_position(self):
        """Test that the component's position is returned."""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.position(0)), V3D)

    def test_rotation(self):
        """Test that the component's rotation is returned."""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.rotation(0)), Quat)

    def test_relativePosition(self):
        """Test that the component's relative position is returned."""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.relativePosition(0)), V3D)

    def test_relativeRotation(self):
        """Test that the component's relative rotation is returned."""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.relativeRotation(0)), Quat)

    def test_setPosition(self):
        """Test that the component's position can be set correctly."""
        info = self._ws.componentInfo()
        pos = V3D(0, 0, 0)
        info.setPosition(0, pos)
        retPos = info.position(0)
        self.assertEqual(pos, retPos)

    def test_setRotation(self):
        """Test that the component's rotation can be set correctly."""
        info = self._ws.componentInfo()
        quat = Quat(0, 0, 0, 0)
        info.setRotation(0, quat)
        retQuat = info.rotation(0)
        self.assertEqual(quat, retQuat)

    def test_hasSource(self):
        """Check if there is a source"""
        info = self._ws.componentInfo()
        self.assertEqual(info.hasSource(), True)

    def test_hasEquivalentSource(self):
        """Check if the sources are equivalent"""
        info = self._ws.componentInfo()
        ws_other = CloneWorkspace(self._ws)
        info_other = ws_other.componentInfo()
        self.assertEqual(info.hasEquivalentSource(info_other), True)
        info_other.setPosition(info.source(), info.sourcePosition() + V3D(1.0 - 6, 0, 0))
        self.assertEqual(info.hasEquivalentSource(info_other), False)

    def test_hasSample(self):
        """Check if there is a sample"""
        info = self._ws.componentInfo()
        self.assertEqual(info.hasSample(), True)

    def test_hasEquivalentSample(self):
        """Check if the samples are equivalent"""
        info = self._ws.componentInfo()
        ws_other = CloneWorkspace(self._ws)
        info_other = ws_other.componentInfo()
        self.assertEqual(info.hasEquivalentSample(info_other), True)
        info_other.setPosition(info.sample(), info.samplePosition() + V3D(1.0 - 6, 0, 0))
        self.assertEqual(info.hasEquivalentSample(info_other), False)

    def test_source(self):
        """Check if a source component is returned"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.source()), int)

    def test_sample(self):
        """Check if a sample component is returned"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.sample()), int)

    def test_sourcePosition(self):
        """Check that the source postition is a V3D object"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.sourcePosition()), V3D)

    def test_samplePosition(self):
        """Check that the sample postition is a V3D object"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.samplePosition()), V3D)

    def test_hasParent(self):
        """Check if a component has a parent component"""
        info = self._ws.componentInfo()
        self.assertTrue(info.hasParent(0))

    def test_parent(self):
        """Check that for a component that has a parent, the parent
        component is retrieved."""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.parent(0)), int)

    def test_children(self):
        """Check that for a component that has children, the children
        components can be retrieved."""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.children(0)), np.ndarray)

    def test_name(self):
        """Get the name of a component as a string"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.name(0)), str)

    def test_l1(self):
        """Get the l1 value"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.l1()), float)

    def test_scaleFactor(self):
        """Get the scale factor"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.scaleFactor(0)), V3D)

    def test_setScaleFactor(self):
        """Set the scale factor"""
        info = self._ws.componentInfo()
        sf = V3D(0, 0, 0)
        info.setScaleFactor(0, sf)
        self.assertEqual(info.scaleFactor(0), sf)

    def test_hasValidShape(self):
        """Check for a valid shape"""
        info = self._ws.componentInfo()
        self.assertEqual(info.hasValidShape(0), True)

    def test_shape(self):
        """Check a shape is returned"""
        info = self._ws.componentInfo()
        self.assertEqual(type(info.shape(0)), CSGObject)

    def test_createWorkspaceAndComponentInfo(self):
        """Try to create a workspace and see if ComponentInfo object is accessable"""
        dataX = [1, 2, 3, 4, 5]
        dataY = [1, 2, 3, 4, 5]
        workspace = CreateWorkspace(DataX=dataX, DataY=dataY)
        info = workspace.componentInfo()
        self.assertEqual(info.size(), 1)

    def test_indexOfAny(self):
        info = self._ws.componentInfo()
        index = info.indexOfAny(info.name(info.root()))
        # Root index and the discovered index should be the same
        self.assertEqual(index, info.root())

    def test_uniqueName(self):
        info = self._ws.componentInfo()
        self.assertTrue(info.uniqueName(info.name(info.root())))
        self.assertFalse(info.uniqueName("fictional-name"))

    def test_indexOfAny_throws(self):
        info = self._ws.componentInfo()
        with self.assertRaises(ValueError):
            info.indexOfAny("fictitious")

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
        self.assertEqual(type(info.detectorsInSubtree(5)), np.ndarray)

    def test_componentsInSubtree_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.componentsInSubtree(-1)
        self.assertEqual(type(info.componentsInSubtree(5)), np.ndarray)

    def test_position_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.position(-1)
        self.assertEqual(type(info.position(0)), V3D)
        self.assertEqual(type(info.position(5)), V3D)

    def test_rotation_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.rotation(-1)
        self.assertEqual(type(info.rotation(0)), Quat)
        self.assertEqual(type(info.rotation(5)), Quat)

    def test_relativePosition_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.relativePosition(-1)
        self.assertEqual(type(info.relativePosition(0)), V3D)
        self.assertEqual(type(info.relativePosition(5)), V3D)

    def test_relativeRotation_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.relativeRotation(-1)
        self.assertEqual(type(info.relativeRotation(0)), Quat)
        self.assertEqual(type(info.relativeRotation(5)), Quat)

    def test_setPosition_extreme(self):
        info = self._ws.componentInfo()
        pos = V3D(0, 0, 0)
        with self.assertRaises(OverflowError):
            info.setPosition(-1, pos)

    def test_setRotation_extreme(self):
        info = self._ws.componentInfo()
        quat = Quat(0, 0, 0, 0)
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
        self.assertEqual(type(info.parent(0)), int)
        self.assertEqual(type(info.parent(5)), int)

    def test_children_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.children(-1)
        self.assertEqual(type(info.children(0)), np.ndarray)
        self.assertEqual(type(info.children(5)), np.ndarray)

    def test_name_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.name(-1)
        self.assertEqual(type(info.name(0)), str)
        self.assertEqual(type(info.name(5)), str)

    def test_scaleFactor_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.scaleFactor(-1)
        self.assertEqual(type(info.scaleFactor(0)), V3D)
        self.assertEqual(type(info.scaleFactor(5)), V3D)

    def test_setScaleFactor_extreme(self):
        info = self._ws.componentInfo()
        sf = V3D(0, 0, 0)
        with self.assertRaises(OverflowError):
            info.setScaleFactor(-1, sf)

    def test_hasValidShape_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.hasValidShape(-1)
        self.assertEqual(type(info.hasValidShape(0)), bool)
        self.assertEqual(type(info.hasValidShape(5)), bool)

    def test_shape_extreme(self):
        info = self._ws.componentInfo()
        with self.assertRaises(OverflowError):
            info.shape(-1)
        self.assertEqual(type(info.shape(0)), CSGObject)
        self.assertEqual(type(info.shape(5)), CSGObject)

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
        pos = [0, 0, 0]
        with self.assertRaises(TypeError):
            info.setPosition(0, pos)

    def test_setRotation_exceptional(self):
        info = self._ws.componentInfo()
        rot = [0, 0, 0, 0]
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

    def test_l1_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.l1(0)

    def test_scaleFactor_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.scaleFactor("Scale factor")
        with self.assertRaises(TypeError):
            info.scaleFactor(0.12)

    def test_setScaleFactor_exceptional(self):
        info = self._ws.componentInfo()
        sf = V3D(0, 0, 0)
        with self.assertRaises(TypeError):
            info.setScaleFactor("1", sf)
        with self.assertRaises(TypeError):
            info.setScaleFactor(1.0, sf)

    def test_hasValidShape_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.hasValidShape("ValidShape")
        with self.assertRaises(TypeError):
            info.hasValidShape(1010.00)

    def test_shape_exceptional(self):
        info = self._ws.componentInfo()
        with self.assertRaises(TypeError):
            info.shape("Shape")
        with self.assertRaises(TypeError):
            info.shape(11.32)

    def test_basic_iterator(self):
        info = self._ws.componentInfo()
        expected_iterations = len(info)
        actual_iterations = len(list(iter(info)))
        self.assertEqual(expected_iterations, actual_iterations)
        it = iter(info)
        self.assertEqual(next(it).index, 0)
        self.assertEqual(next(it).index, 1)

    def test_isDetector_via_iterator(self):
        comp_info = self._ws.componentInfo()
        n_detectors = len(self._ws.detectorInfo())
        it = iter(comp_info)
        self.assertEqual(next(it).isDetector, True)
        self.assertEqual(next(it).isDetector, True)
        self.assertEqual(next(it).isDetector, False)
        self.assertEqual(next(it).isDetector, False)

    def test_position_via_iterator(self):
        comp_info = self._ws.componentInfo()
        source_pos = comp_info.sourcePosition()
        it = iter(comp_info)
        # basic check on first detector position
        self.assertGreater(next(it).position.distance(source_pos), 0)

    def test_children_via_iterator(self):
        info = self._ws.componentInfo()
        it = iter(info)
        first_det = next(it)
        self.assertEqual(type(first_det.children), np.ndarray)
        self.assertEqual(len(first_det.children), 0)
        root = next(it)
        for root in it:
            continue
        self.assertEqual(root.index, info.root())  # sanity check
        self.assertTrue(np.array_equal(root.children, np.array([0, 1, 2, 3, 4], dtype="uint64")))

    def test_detectorsInSubtree_via_iterator(self):
        info = self._ws.componentInfo()
        it = iter(info)
        first_det = next(it)
        self.assertEqual(type(first_det.detectorsInSubtree), np.ndarray)
        # For detectors, only contain own index
        self.assertTrue(np.array_equal(first_det.detectorsInSubtree, np.array([0], dtype="uint64")))
        root = next(it)
        for root in it:
            continue
        self.assertTrue(np.array_equal(root.detectorsInSubtree, np.array([0, 1], dtype="uint64")))

    def test_componentsInSubtree_via_iterator(self):
        info = self._ws.componentInfo()
        it = iter(info)
        first_det = next(it)
        self.assertEqual(type(first_det.detectorsInSubtree), np.ndarray)
        # For detectors, only contain own index
        self.assertTrue(np.array_equal(first_det.componentsInSubtree, np.array([0], dtype="uint64")))
        root = next(it)
        for root in it:
            continue
        # All component indices expected including self
        self.assertTrue(np.array_equal(root.componentsInSubtree, np.array([0, 1, 2, 3, 4, 5], dtype="uint64")))


if __name__ == "__main__":
    unittest.main()
