# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import ClearMaskFlag, DeleteWorkspace, LoadEmptyInstrument, MaskBTP

# tests run x10 slower with this on, but it may be useful to track down issues refactoring
CHECK_CONSISTENCY = False


class MaskBTPTest(systemtesting.MantidSystemTest):
    """MaskBTP on IMAGINE, which has 80 banks of 512x512 pixels. This is a system test
    rather than a unit test purely because of the memory needed to hold the instrument."""

    def requiredMemoryMB(self):
        """Requires 30Gb"""
        return 30000

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

    def runTest(self):
        ws_name = mtd.unique_hidden_name()
        LoadEmptyInstrument(InstrumentName="IMAGINE", OutputWorkspace=ws_name)
        wksp = mtd[ws_name]

        # Test masking individual bank 11 (512*512 pixels)
        masked_bank11 = MaskBTP(Workspace=ws_name, Bank="11")
        self.assertEqual(len(masked_bank11), 512 * 512)
        self.checkConsistentMask(wksp, masked_bank11)

        # Clear mask to start fresh
        ClearMaskFlag(Workspace=ws_name)

        # Test masking multiple banks (banks 11-18, first series)
        masked_banks = MaskBTP(Workspace=ws_name, Bank="11-18")
        self.assertEqual(len(masked_banks), 8 * 512 * 512)
        self.checkConsistentMask(wksp, masked_banks)

        # Clear mask to start fresh
        ClearMaskFlag(Workspace=ws_name)

        # Test masking specific pixels (first 10 pixels in all banks, 0-indexed)
        masked_pixels = MaskBTP(Workspace=ws_name, Pixel="0-9")
        # 10 pixels * 512 tubes * 80 banks
        self.assertEqual(len(masked_pixels), 10 * 512 * 80)
        self.checkConsistentMask(wksp, masked_pixels)

        DeleteWorkspace(ws_name)
