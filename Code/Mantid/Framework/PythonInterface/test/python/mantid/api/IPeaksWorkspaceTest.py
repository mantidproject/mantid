import unittest
from testhelpers import run_algorithm, WorkspaceCreationHelper
from mantid.kernel import V3D
from mantid.api import IPeaksWorkspace

class IPeaksWorkspaceTest(unittest.TestCase):
    """
    Test the python interface to PeaksWorkspace's
    """

    def test_interface(self):
        """ Rudimentary test to get peak and get/set some values """
        pws = WorkspaceCreationHelper.createPeaksWorkspace(1)
        self.assertTrue(isinstance(pws, IPeaksWorkspace))
        self.assertEqual(pws.getNumberPeaks(), 1)
        p = pws.getPeak(0)

        # Try a few IPeak get/setters. Not everything.
        p.setH(234)
        self.assertEqual(p.getH(), 234)
        p.setHKL(5,6,7)
        self.assertEqual(p.getH(), 5)
        self.assertEqual(p.getK(), 6)
        self.assertEqual(p.getL(), 7)

        hkl = p.getHKL()
        self.assertEquals(hkl, V3D(5,6,7))

        p.setIntensity(456)
        p.setSigmaIntensity(789)
        self.assertEqual(p.getIntensity(), 456)
        self.assertEqual(p.getSigmaIntensity(), 789)

        # Finally try to remove a peak
        pws.removePeak(0)
        self.assertEqual(pws.getNumberPeaks(), 0)

        # Create a new peak at some Q in the lab frame
        qlab = V3D(1,2,3)
        p = pws.createPeak(qlab, 1.54)
        self.assertAlmostEquals( p.getQLabFrame().X(), 1.0, 3)

        # Now try to add the peak back
        pws.addPeak(p)
        self.assertEqual(pws.getNumberPeaks(), 1)

        # Check that it is what we added to it
        p = pws.getPeak(0)
        self.assertAlmostEquals( p.getQLabFrame().X(), 1.0, 3)

        # Peaks workspace will not be integrated by default.
        self.assertTrue(not pws.hasIntegratedPeaks())

if __name__ == '__main__':
    unittest.main()


