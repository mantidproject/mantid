# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
import os
import tempfile
import unittest

from mantid.simpleapi import CreateSampleWorkspace, SaveNexus, mtd


class AlignAndFocusPowderFromFilesTest(unittest.TestCase):
    def setUp(self):
        self._tmp_dir = tempfile.TemporaryDirectory()

    def tearDown(self):
        mtd.clear()
        self._tmp_dir.cleanup()

    def _make_non_event_slim_setup(self):
        from AlignAndFocusPowderFromFiles import AlignAndFocusPowderFromFiles

        input_ws = "aafpff_non_event_input"
        output_ws = "aafpff_non_event_output"
        filename = os.path.join(self._tmp_dir.name, "aafpff_non_event_input.nxs.h5")

        CreateSampleWorkspace(OutputWorkspace=input_ws, WorkspaceType="Histogram", NumBanks=1, BankPixelWidth=1)
        SaveNexus(InputWorkspace=input_ws, Filename=filename)

        alg = AlignAndFocusPowderFromFiles()
        alg.initialize()
        alg.setProperty("Filename", filename)
        alg.setProperty("OutputWorkspace", output_ws)
        alg.setProperty("AllowSlimProcess", True)
        alg.setProperty("PreserveEvents", False)
        alg.setProperty("PrimaryFlightPath", 15.0)
        alg.setProperty("L2", [1.0])
        alg.setProperty("Polar", [90.0])
        alg.setProperty("Azimuthal", [0.0])
        alg.setProperty("Params", [0.5, -0.004, 7.0])

        alg._filenames = [filename]
        alg.filterBadPulses = 0.0
        return alg, output_ws

    def test_non_event_nexus_should_not_be_slim_eligible(self):
        alg, _ = self._make_non_event_slim_setup()
        can_use_slim = alg._AlignAndFocusPowderFromFiles__canUseSlim()

        self.assertFalse(
            can_use_slim,
            "Expected __canUseSlim() to reject non-event Nexus input to avoid: No NXevent_data entries found in file",
        )

    def test_forced_slim_on_non_event_nexus_does_not_raise(self):
        alg, output_ws = self._make_non_event_slim_setup()
        alg._AlignAndFocusPowderFromFiles__runSlim(output_ws)


if __name__ == "__main__":
    unittest.main()
