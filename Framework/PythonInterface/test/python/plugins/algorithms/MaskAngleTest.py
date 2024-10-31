# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AnalysisDataService
from mantid.simpleapi import CloneWorkspace, CreateWorkspace, DeleteWorkspace, GroupWorkspaces, MaskAngle
from testhelpers import WorkspaceCreationHelper
from numpy import arange, array_equal


class MaskAngleTest(unittest.TestCase):
    def testMaskAngle(self):
        w = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("w", w)
        masklist = MaskAngle(w, MinAngle=10, MaxAngle=20)
        detInfo = w.detectorInfo()
        for i in arange(w.getNumberHistograms()):
            if (i < 9) or (i > 18):
                self.assertFalse(detInfo.isMasked(int(i)))
            else:
                self.assertTrue(detInfo.isMasked(int(i)))
        DeleteWorkspace(w)
        self.assertTrue(array_equal(masklist, arange(10) + 10))

    def testMaskAnglePhi(self):
        w = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("w", w)
        MaskAngle(w, 0, 45, Angle="Phi")
        detInfo = w.detectorInfo()
        for i in arange(w.getNumberHistograms()):
            if i == 0:
                self.assertTrue(detInfo.isMasked(int(i)))
            else:
                self.assertFalse(detInfo.isMasked(int(i)))
        DeleteWorkspace(w)

    def testMaskAngleInPlane(self):
        w_inplane = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("w_inplane", w_inplane)
        MaskAngle(w_inplane, -5, 5, Angle="InPlane")
        detInfo = w_inplane.detectorInfo()
        print("num spectra = {}".format(w_inplane.getNumberHistograms()))
        for i in arange(w_inplane.getNumberHistograms()):
            # the entire instrument is at an in-plane angle of zero degrees
            self.assertTrue(detInfo.isMasked(int(i)), "index={}".format(i))
        DeleteWorkspace(w_inplane)

    def testGroupMaskAngle(self):
        ws1 = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("ws1GMA", ws1)
        ws2 = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("ws2GMA", ws2)

        group = GroupWorkspaces(["ws1GMA", "ws2GMA"])
        MaskAngle(group, 10, 20)

        for w in group:
            detInfo = w.detectorInfo()
            for i in arange(w.getNumberHistograms()):
                if (i < 9) or (i > 18):
                    self.assertFalse(detInfo.isMasked(int(i)))
                else:
                    self.assertTrue(detInfo.isMasked(int(i)))

        DeleteWorkspace(group)

    def testFailNoInstrument(self):
        w1 = CreateWorkspace(arange(5), arange(5))
        try:
            MaskAngle(w1, 10, 20)
            self.fail("Should not have got here. Should throw because no instrument.")
        except (RuntimeError, ValueError):
            pass
        finally:
            DeleteWorkspace(w1)

    def testGroupFailNoInstrument(self):
        ws1 = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("ws1GFNI", ws1)
        ws2 = CreateWorkspace(arange(5), arange(5))
        AnalysisDataService.add("ws2GFNI", ws2)

        group = GroupWorkspaces(["ws1GFNI", "ws2GFNI"])

        try:
            MaskAngle(group, 10, 20)
            self.fail("Should not have gotten here. Should throw because no instrument.")
        except (RuntimeError, ValueError, TypeError):
            pass
        finally:
            DeleteWorkspace(group)

    def testFailLimits(self):
        w2 = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("w2", w2)
        w3 = CloneWorkspace("w2")
        w4 = CloneWorkspace("w2")
        try:
            MaskAngle(w2, -100, 20)
            self.fail("Should not have got here. Wrong angle.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace("w2")
        try:
            MaskAngle(w3, 10, 200)
            self.fail("Should not have got here. Wrong angle.")
        except ValueError:
            pass
        finally:
            DeleteWorkspace("w3")
        try:
            MaskAngle(w4, 100, 20)
            self.fail("Should not have got here. Wrong angle.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace("w4")


if __name__ == "__main__":
    unittest.main()
