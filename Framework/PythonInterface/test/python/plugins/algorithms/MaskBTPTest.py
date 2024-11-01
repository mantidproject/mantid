# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, AnalysisDataService
from mantid.kernel import config
from mantid.simpleapi import DeleteWorkspace, LoadEmptyInstrument, MaskBTP
from testhelpers import WorkspaceCreationHelper
from numpy import concatenate, arange, sort, array_equal, where

# tests run x10 slower with this on, but it may be useful to track down issues refactoring
CHECK_CONSISTENCY = False


class MaskBTPTest(unittest.TestCase):
    def checkConsistentMask(self, wksp, masked):
        if not CHECK_CONSISTENCY:
            return
        compInfo = wksp.componentInfo()
        detInfo = wksp.detectorInfo()
        # detector ids are any number, detector index are 0->number of detectors
        detIds = detInfo.detectorIDs()
        for detIndex, detId in enumerate(detIds):
            if not compInfo.isDetector(detIndex):
                continue

            if detInfo.isMonitor(detIndex):
                self.assertFalse(detInfo.isMasked(detIndex), "DetID={} is a monitor and shouldn't be masked".format(detId))
            else:
                self.assertEqual(
                    detInfo.isMasked(detIndex),
                    detId in masked,
                    'DetID={} is has incorrect mask bit. "{}" should be "{}"'.format(detId, detInfo.isMasked(int(detId)), detId in masked),
                )

    def checkDetectorIndexes(self, wksp, detIndices):
        """This is use to spot check specific detector indices (not identifiers) as masked"""
        detInfo = wksp.detectorInfo()
        for detIndex in detIndices:
            self.assertTrue(detInfo.isMasked(detIndex), "Detector index={} should be masked".format(detIndex))

    def testMaskBTPWrongInstrument(self):
        w = WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(30, 5, False, False)
        AnalysisDataService.add("w", w)
        try:
            MaskBTP(Workspace=w, Pixel="1")
            self.fail("Should not have got here. Should throw because wrong instrument.")
        except RuntimeError:
            pass
        finally:
            DeleteWorkspace(w)

    def testMaskBTPWrongLimits(self):
        try:
            MaskBTP(Instrument="ARCS", Pixel="129")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        try:
            MaskBTP(Instrument="SEQUOIA", Bank="1")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        try:
            MaskBTP(Instrument="SEQUOIA", Bank="22")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        try:
            MaskBTP(Instrument="HYSPEC", Tube="18")
            self.fail("Should not have got here.")
        except RuntimeError:
            pass
        DeleteWorkspace("ARCSMaskBTP")
        DeleteWorkspace("HYSPECMaskBTP")
        DeleteWorkspace("SEQUOIAMaskBTP")

    def testMaskBTP(self):
        m1 = MaskBTP(Instrument="CNCS", Pixel="1-3,5")
        m2 = MaskBTP(Workspace="CNCSMaskBTP", Bank="1-2")
        m3 = MaskBTP(Workspace="CNCSMaskBTP", Bank="5-7", Tube="3")
        p1 = arange(400) * 128
        m1p = sort(concatenate((p1, p1 + 1, p1 + 2, p1 + 4)))
        self.assertTrue(array_equal(m1, m1p))
        self.assertTrue(array_equal(m2, arange(2048)))
        b5t3 = arange(128) + 4 * 1024 + 2 * 128
        self.assertTrue(array_equal(m3, concatenate((b5t3, b5t3 + 1024, b5t3 + 2048))))
        # check whether some pixels are masked when they should
        w = mtd["CNCSMaskBTP"]
        detInfo = w.detectorInfo()
        detIds = detInfo.detectorIDs()

        # check for some to be masked
        for id in (29696, 29697, 29698, 29700, 1020, 4400):  # 29699,
            index = int(where(detIds == id)[0][0])
            self.assertTrue(detInfo.isMasked(index), msg="detId={}, index={} should not be masked".format(id, index))

        # check for some to not be masked
        for id in [3071]:
            index = int(where(detIds == id)[0][0])
            self.assertFalse(detInfo.isMasked(index), msg="detId={}, index={} should not be masked".format(id, index))
        DeleteWorkspace(w)

    def testSEQMaskBTP(self):
        MaskBTP(Instrument="SEQUOIA", Bank="23")
        MaskBTP(Instrument="SEQUOIA", Bank="24")
        MaskBTP(Instrument="SEQUOIA", Bank="25")
        MaskBTP(Instrument="SEQUOIA", Bank="26")
        MaskBTP(Instrument="SEQUOIA", Bank="27")
        MaskBTP(Instrument="SEQUOIA", Bank="37")
        MaskBTP(Instrument="SEQUOIA", Bank="38")
        MaskBTP(Instrument="SEQUOIA", Bank="74")
        MaskBTP(Instrument="SEQUOIA", Bank="75")
        MaskBTP(Instrument="SEQUOIA", Bank="98")
        MaskBTP(Instrument="SEQUOIA", Bank="99")
        MaskBTP(Instrument="SEQUOIA", Bank="100")
        MaskBTP(Instrument="SEQUOIA", Bank="101")
        MaskBTP(Instrument="SEQUOIA", Bank="102")
        MaskBTP(Instrument="SEQUOIA", Bank="103")
        MaskBTP(Instrument="SEQUOIA", Bank="113")
        MaskBTP(Instrument="SEQUOIA", Bank="114")
        MaskBTP(Instrument="SEQUOIA", Bank="150")
        return

    def testEQSANSMaskBTP(self):
        w = LoadEmptyInstrument(InstrumentName="EQ-SANS", OutputWorkspace="empty_eqsans")
        m1 = MaskBTP(w, Bank="1-48", Pixel="1-8,249-256")  # tube tips
        m2 = MaskBTP(w, Bank="17", Tube="2")  # rogue tube
        m3 = MaskBTP(w, Components="back-panel")  # whole back panel
        for mask, n_masked in zip((m1, m2, m3), (3072, 256, 24576)):
            self.assertEqual(len(mask), n_masked)

    def testCG2MaskBTP(self):
        w = LoadEmptyInstrument(InstrumentName="EQ-SANS", OutputWorkspace="empty_eqsans")
        m1 = MaskBTP(w, Bank="1-48", Pixel="1-8,249-256")  # tube tips
        m2 = MaskBTP(w, Bank="17", Tube="2")  # rogue tube
        m3 = MaskBTP(w, Components="back-panel")  # whole back panel
        for mask, n_masked in zip((m1, m2, m3), (3072, 256, 24576)):
            self.assertEqual(len(mask), n_masked)

    def testEdges(self):
        # this combined option should probably be called corners
        masking = MaskBTP(Instrument="TOPAZ", Tube="edges", Pixel="EdGes")  # funny case just b/c
        self.assertEqual(4 * 25, len(masking))

        # keep on masking the same workspace to speed up the test
        masking = MaskBTP(Workspace="TOPAZMaskBTP", Tube="edges")
        self.assertEqual(2 * 256 * 25, len(masking))

    def test_eqsans_simple(self):
        ws_name = "eqsans"
        LoadEmptyInstrument(InstrumentName="EQ-SANS", OutputWorkspace=ws_name)

        # every other tube in a "bank"
        masked = MaskBTP(Workspace=ws_name, Tube="1,3")
        wksp = mtd[ws_name]
        self.assertEqual(int(192 * 256 / 2), len(masked))
        self.checkConsistentMask(wksp, masked)

    def test_cg2_simple(self):
        ws_name = "cg2"
        LoadEmptyInstrument(InstrumentName="CG2", OutputWorkspace=ws_name)

        # every other tube in a "bank"
        masked = MaskBTP(Workspace=ws_name, Tube="1,3")
        wksp = mtd[ws_name]
        self.assertEqual(int(192 * 256 / 2), len(masked))
        self.checkConsistentMask(wksp, masked)

    def test_eqsans_interleaved(self):
        ws_name = "eqsans"
        LoadEmptyInstrument(Filename="EQ-SANS_Definition_19000131_20190614.xml", OutputWorkspace=ws_name)

        # legacy instrument had wacky numbering
        masked = MaskBTP(Workspace=ws_name, Tube="5:200:8,6:200:8,7:200:8,8:200:8")
        wksp = mtd[ws_name]
        self.assertEqual(int(192 * 256 / 2), len(masked))
        self.checkConsistentMask(wksp, masked)

        # check for specific mask values
        masked = [i + 1 for i in range(256 * 4, 256 * 8 * 24, 2048)]  # overwrite previous version
        self.checkDetectorIndexes(wksp, masked)

    def test_biosans_wing_plane(self):
        ws_name = "biosans_wing"
        LoadEmptyInstrument(InstrumentName="BIOSANS", OutputWorkspace=ws_name)

        masked = MaskBTP(Workspace=ws_name, Bank="49-88", Tube="1,3")
        wksp = mtd[ws_name]
        self.assertEqual(int(160 * 256 / 2), len(masked))
        self.checkConsistentMask(wksp, masked)

    def test_biosans_wing_ends(self):
        masked = MaskBTP(Instrument="BIOSANS", Bank="49-88", Pixel="1-20,245-256")
        wksp = mtd["BIOSANSMaskBTP"]
        self.assertEqual(int(32 * 160), len(masked))
        self.checkConsistentMask(wksp, masked)

    def test_biosans_midrange(self):
        # load latest instrument definition, 104 banks and 4 tubes per bank
        workspace = LoadEmptyInstrument(Filename="CG3_Definition.xml", OutputWorkspace="CG3csejf234f")
        masked = MaskBTP(Workspace=workspace, Pixel="1-42")
        self.assertEqual(42 * 4 * 104, len(masked))
        # load previous instrument definition, 88 banks and 4 tubes per bank
        workspace = LoadEmptyInstrument(Filename="CG3_Definition_2019_2023.xml", OutputWorkspace="CG3csejf234f")
        masked = MaskBTP(Workspace=workspace, Pixel="1-42")
        self.assertEqual(42 * 4 * 88, len(masked))
        DeleteWorkspace("CG3csejf234f")

    def test_components(self):
        # this also verifies support for instruments that aren't explicitly in the list
        wksp = LoadEmptyInstrument(InstrumentName="GEM", OutputWorkspace="GEM")
        masked = MaskBTP(Workspace=wksp, Components="bank3-east,bank3-west", Tube="1-3")  # zero indexed b/c not supported instrument
        self.assertEqual(2 * 3 * 90, len(masked))

        wksp = LoadEmptyInstrument(InstrumentName="GEM", OutputWorkspace="GEM")
        masked = MaskBTP(Workspace=wksp, Components="bank3")
        self.assertEqual(10 * 90, len(masked))

    def test_d33(self):
        ws = LoadEmptyInstrument(InstrumentName="D33")
        mask_rear = MaskBTP(Workspace=ws, Components="back_detector")
        mask_front = MaskBTP(Workspace=ws, Bank="1-4", Tube="15", Pixel="0-10")

        self.assertEqual(len(mask_rear), 256 * 128)
        self.assertEqual(len(mask_front), 4 * 11)
        self.checkConsistentMask(ws, concatenate((mask_rear, mask_front)))

    def test_d11(self):
        ws = LoadEmptyInstrument(InstrumentName="d11")
        mask = MaskBTP(Workspace=ws, Pixel="0-10")

        self.assertEqual(len(mask), 256 * 11)
        self.checkConsistentMask(ws, mask)

    def test_d22(self):
        ws = LoadEmptyInstrument(InstrumentName="d22")
        mask = MaskBTP(Workspace=ws, Tube="2-5")

        self.assertEqual(len(mask), 256 * 4)
        self.checkConsistentMask(ws, mask)

    def test_d16(self):
        ws = LoadEmptyInstrument(InstrumentName="d16")
        mask = MaskBTP(Workspace=ws, Tube="319", Pixel="319")

        self.assertEqual(len(mask), 1)
        self.checkConsistentMask(ws, mask)

    def test_d11_lr(self):
        path = config["instrumentDefinition.directory"] + "D11lr_Definition.xml"

        ws = LoadEmptyInstrument(Filename=path)
        mask = MaskBTP(Workspace=ws, Tube="127")

        self.assertEqual(len(mask), 128)
        self.checkConsistentMask(ws, mask)

    def test_d22lr(self):
        path = config["instrumentDefinition.directory"] + "D22lr_Definition.xml"

        ws = LoadEmptyInstrument(Filename=path)
        mask = MaskBTP(Workspace=ws, Pixel="20-28")

        self.assertEqual(len(mask), 9 * 128)
        self.checkConsistentMask(ws, mask)

    def test_d22b(self):
        path = config["instrumentDefinition.directory"] + "D22B_Definition.xml"
        ws = LoadEmptyInstrument(Filename=path)
        mask = MaskBTP(Workspace=ws, Bank="2")

        self.assertEqual(len(mask), 256 * 96)
        self.checkConsistentMask(ws, mask)

    def test_d11b(self):
        path = config["instrumentDefinition.directory"] + "D11B_Definition.xml"
        ws = LoadEmptyInstrument(Filename=path)
        mask = MaskBTP(Workspace=ws, Components="detector_right, detector_left")

        self.assertEqual(len(mask), 256 * 32 * 2)
        self.checkConsistentMask(ws, mask)


if __name__ == "__main__":
    unittest.main()
