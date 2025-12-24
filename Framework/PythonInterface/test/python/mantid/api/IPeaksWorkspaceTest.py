# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from testhelpers import WorkspaceCreationHelper
from mantid.kernel import SpecialCoordinateSystem, V3D
from mantid.api import IPeaksWorkspace, IPeak

import math


class IPeaksWorkspaceTest(unittest.TestCase):
    """
    Test the python interface to PeaksWorkspace's
    """

    def test_interface(self):
        """Rudimentary test to get peak and get/set some values"""
        pws = WorkspaceCreationHelper.createPeaksWorkspace(1)
        self.assertTrue(isinstance(pws, IPeaksWorkspace))
        self.assertEqual(pws.getNumberPeaks(), 1)
        p = pws.getPeak(0)

        # Try a few IPeak get/setters. Not everything.
        p.setH(234)
        self.assertEqual(p.getH(), 234)
        p.setHKL(5, 6, 7)
        self.assertEqual(p.getH(), 5)
        self.assertEqual(p.getK(), 6)
        self.assertEqual(p.getL(), 7)

        hkl = p.getHKL()
        self.assertEqual(hkl, V3D(5, 6, 7))

        p.setIntensity(456)
        p.setSigmaIntensity(789)
        self.assertEqual(p.getIntensity(), 456)
        self.assertEqual(p.getSigmaIntensity(), 789)

        # Finally try to remove a peak
        pws.removePeak(0)
        self.assertEqual(pws.getNumberPeaks(), 0)

        # Create a new peak at some Q in the lab frame
        qlab = V3D(1, 2, 3)
        p = pws.createPeak(qlab, 1.54)
        p.getQLabFrame()
        self.assertAlmostEqual(p.getQLabFrame().X(), 1.0, 3)

        # Now try to add the peak back
        pws.addPeak(p)
        self.assertEqual(pws.getNumberPeaks(), 1)

        # Check that it is what we added to it
        p = pws.getPeak(0)
        self.assertAlmostEqual(p.getQLabFrame().X(), 1.0, 3)

        # Peaks workspace will not be integrated by default.
        self.assertTrue(not pws.hasIntegratedPeaks())

    def test_addPeak_SpecialCoordinateSystem(self):
        r"""Verify we can add peaks in different coordinate systems"""
        pws = WorkspaceCreationHelper.createPeaksWorkspace(numPeaks=0, createOrientedLattice=True)
        pws.addPeak([1, 2, 3], SpecialCoordinateSystem.QLab)
        pws.addPeak([1, 2, 3], SpecialCoordinateSystem.QSample)
        pws.addPeak([1, 2, 3], SpecialCoordinateSystem.HKL)

    def test_createPeakHKL(self):
        r"""Simple test that the creational method is exposed"""
        pws = WorkspaceCreationHelper.createPeaksWorkspace(0, True)
        p = pws.createPeakHKL([1, 1, 1])
        self.assertFalse(p is None)

    def test_peak_setQLabFrame(self):
        pws = WorkspaceCreationHelper.createPeaksWorkspace(1, True)
        p = pws.getPeak(0)
        try:
            p.setQLabFrame(V3D(1, 1, 1))
        except Exception:
            self.fail("Tried setQLabFrame with one V3D argument")

        self.assertAlmostEqual(p.getQLabFrame().X(), 1.0, places=10)
        self.assertAlmostEqual(p.getQLabFrame().Y(), 1.0, places=10)
        self.assertAlmostEqual(p.getQLabFrame().Z(), 1.0, places=10)

        try:
            p.setQLabFrame(V3D(1, 1, 1), 1)
        except Exception:
            self.fail("Tried setQLabFrame with one V3D argument and a double distance")
        self.assertAlmostEqual(p.getQLabFrame().X(), 1.0, places=10)
        self.assertAlmostEqual(p.getQLabFrame().Y(), 1.0, places=10)
        self.assertAlmostEqual(p.getQLabFrame().Z(), 1.0, places=10)

    def test_peak_setQSampleFrame(self):
        pws = WorkspaceCreationHelper.createPeaksWorkspace(1, True)
        p = pws.getPeak(0)

        try:
            p.setQSampleFrame(V3D(1, 1, 1))
        except Exception:
            self.fail("Tried setQSampleFrame with one V3D argument")

        self.assertAlmostEqual(p.getQSampleFrame().X(), 1.0, places=10)
        self.assertAlmostEqual(p.getQSampleFrame().Y(), 1.0, places=10)
        self.assertAlmostEqual(p.getQSampleFrame().Z(), 1.0, places=10)

        try:
            p.setQSampleFrame(V3D(1, 1, 1), 1)
        except Exception:
            self.fail("Tried setQSampleFrame with one V3D argument and a double distance")
        self.assertAlmostEqual(p.getQSampleFrame().X(), 1.0, places=10)
        self.assertAlmostEqual(p.getQSampleFrame().Y(), 1.0, places=10)
        self.assertAlmostEqual(p.getQSampleFrame().Z(), 1.0, places=10)

    def test_setCell_with_column_name(self):
        pws = WorkspaceCreationHelper.createPeaksWorkspace(1, True)
        pws.setCell("h", 0, 1)
        pws.setCell("k", 0, 2)
        pws.setCell("l", 0, 3)
        pws.setCell("QLab", 0, V3D(1, 1, 1))
        pws.setCell("QSample", 0, V3D(1, 1, 1))

        self.assertEqual(pws.cell("h", 0), 1)
        self.assertEqual(pws.cell("k", 0), 2)
        self.assertEqual(pws.cell("l", 0), 3)
        self.assertEqual(pws.cell("QLab", 0), V3D(1, 1, 1))
        self.assertEqual(pws.cell("QSample", 0), V3D(1, 1, 1))

    def test_iteration_support(self):
        pws = WorkspaceCreationHelper.createPeaksWorkspace(0, True)
        hkls = ([1, 1, 1], [2, 1, 1], [1, 2, 1])
        for hkl in hkls:
            pws.addPeak(pws.createPeakHKL(hkl))

        count = 0
        for index, peak in enumerate(pws):
            count += 1
            self.assertTrue(isinstance(peak, IPeak))
            self.assertAlmostEqual(V3D(*hkls[index]), peak.getHKL())

        self.assertEqual(len(hkls), count)

    def test_col_rol(self):
        pws = WorkspaceCreationHelper.createPeaksWorkspace(0, True)

        # Incident wavevector
        wavelength = 2.0  # Angstroms
        k = 2 * math.pi / wavelength
        ki = pws.getInstrument().getReferenceFrame().vecPointingAlongBeam() * k

        # Final Wavevector
        detector_id = 42  # column_id = 4, row_id = 2
        kf = pws.componentInfo().position(detector_id)
        kf = kf * (k / kf.norm())

        # Peak
        q_lab = ki - kf  # inelastic convention
        peak = pws.createPeak(q_lab)
        pws.addPeak(peak)
        col, row = pws.row(0)["Col"], pws.row(0)["Row"]
        self.assertAlmostEqual(col, 4)
        self.assertAlmostEqual(row, 2)

    def test_removePeaks(self):
        pws = WorkspaceCreationHelper.createPeaksWorkspace(5)
        pws.removePeaks([2, 3, 4])
        self.assertEqual(pws.getNumberPeaks(), 2)


if __name__ == "__main__":
    unittest.main()
