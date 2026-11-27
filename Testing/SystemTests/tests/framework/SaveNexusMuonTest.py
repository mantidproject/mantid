# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile

import numpy as np
from systemtesting import MantidSystemTest
from mantid.simpleapi import Load, MuonPreProcess, SaveNexusMuon, mtd


def get_loader_name(workspace) -> str:
    history = workspace.getHistory().getAlgorithmHistories()
    return history[0].getPropertyValue("LoaderName")


class SaveNexusMuonRoundTripMixin:
    """
    Loads a muon file, saves it with SaveNexusMuon and loads the saved file again, as the Muon Analysis interface
    would. The saved file must be loaded by LoadMuonNexusV2 and give the same data and muon properties.
    """

    input_file = None
    original_loader = None

    def _set_up(self):
        self.saved_file = os.path.join(tempfile.gettempdir(), "SaveNexusMuonTest.nxs")

    def cleanup(self):
        if os.path.exists(self.saved_file):
            os.remove(self.saved_file)

    def runTest(self):
        self._set_up()
        original = Load(Filename=self.input_file, DeadTimeTable="original_dead_times", DetectorGroupingTable="original_grouping")
        self.assertEqual(self.original_loader, get_loader_name(original.OutputWorkspace))
        SaveNexusMuon(
            InputWorkspace=original.OutputWorkspace,
            Filename=self.saved_file,
            TimeZero=original.TimeZero,
            DeadTimeTable="original_dead_times",
            DetectorGroupingTable="original_grouping",
        )

        saved = Load(Filename=self.saved_file, DeadTimeTable="saved_dead_times", DetectorGroupingTable="saved_grouping")
        self.assertEqual("LoadMuonNexusV2", get_loader_name(saved.OutputWorkspace))

        original_ws, saved_ws = original.OutputWorkspace, saved.OutputWorkspace
        self.assertEqual(original_ws.getNumberHistograms(), saved_ws.getNumberHistograms())
        for i in range(original_ws.getNumberHistograms()):
            np.testing.assert_array_equal(original_ws.y(i), saved_ws.y(i))
            np.testing.assert_allclose(original_ws.x(i), saved_ws.x(i), atol=1e-5)
        self.assertDelta(original.TimeZero, saved.TimeZero, 1e-6)
        self.assertDelta(original.FirstGoodData, saved.FirstGoodData, 1e-6)
        self.assertEqual(original.MainFieldDirection, saved.MainFieldDirection)
        np.testing.assert_allclose(mtd["original_dead_times"].column(1), mtd["saved_dead_times"].column(1), atol=1e-6)
        self.assertEqual(
            [list(group) for group in mtd["original_grouping"].column(0)], [list(group) for group in mtd["saved_grouping"].column(0)]
        )
        for log in ("run_number", "goodfrm", "sample_temp", "sample_magn_field", "run_start", "run_end"):
            self.assertEqual(original_ws.run().getProperty(log).value, saved_ws.run().getProperty(log).value)

        # The pre-processing used by Muon Analysis gives the same result for both
        first_good_time = original.FirstGoodData - original.TimeZero
        original_processed = MuonPreProcess(
            InputWorkspace=original_ws, DeadTimeTable="original_dead_times", TimeMin=first_good_time, OutputWorkspace="original_processed"
        )
        saved_processed = MuonPreProcess(
            InputWorkspace=saved_ws, DeadTimeTable="saved_dead_times", TimeMin=first_good_time, OutputWorkspace="saved_processed"
        )
        original_processed_ws, saved_processed_ws = original_processed.getItem(0), saved_processed.getItem(0)
        for i in range(original_processed_ws.getNumberHistograms()):
            np.testing.assert_allclose(original_processed_ws.y(i), saved_processed_ws.y(i), rtol=1e-6)


class SaveNexusMuonRoundTripFromNexusV2(SaveNexusMuonRoundTripMixin, MantidSystemTest):
    """EMU00102347 is an ISIS Muon NeXus V2 (HDF5) file."""

    input_file = "EMU00102347.nxs_v2"
    original_loader = "LoadMuonNexusV2"


class SaveNexusMuonRoundTripFromNexusV1(SaveNexusMuonRoundTripMixin, MantidSystemTest):
    """emu00006473 is an older ISIS Muon NeXus V1 (HDF4) file, which is saved in the V2 format."""

    input_file = "emu00006473.nxs"
    original_loader = "LoadMuonNexus"
