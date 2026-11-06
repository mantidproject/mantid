# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import tempfile

import systemtesting
from mantid.simpleapi import (
    CompareWorkspaces,
    DeleteWorkspace,
    LoadEventNexus,
    LoadEventNexusAsHistogram,
    LoadNexusProcessed,
    Rebin,
    SaveNexusProcessed,
)


class LoadEventNexusAsHistogramTest(systemtesting.MantidSystemTest):
    """Compare direct histogramming with the event-workspace workflow, including a NeXus round trip."""

    def requiredFiles(self):
        return ["PG3_5226_event.nxs"]

    def requiredMemoryMB(self):
        return 2048

    def runTest(self):
        filename = self.requiredFiles()[0]
        params = "0,100,17000"
        direct = LoadEventNexusAsHistogram(
            Filename=filename,
            OutputWorkspace="direct_histogram",
            Params=params,
        )
        self.assertEqual(direct.id(), "Workspace2D")
        self.assertEqual(direct.getInstrument().getName(), "POWGEN")

        events = LoadEventNexus(
            Filename=filename,
            OutputWorkspace="reference_events",
            LoadMonitors=False,
        )
        expected_events = events.getNumberEvents()
        self.assertGreater(expected_events, 0)
        reference = Rebin(
            InputWorkspace=events,
            OutputWorkspace="reference_histogram",
            Params=params,
            PreserveEvents=False,
        )
        DeleteWorkspace(events)
        self.assertEqual(direct.extractY().sum(), expected_events)
        self._compare(direct, reference)

        with tempfile.TemporaryDirectory() as directory:
            saved_file = os.path.join(directory, "PG3_5226_histogram.nxs")
            SaveNexusProcessed(InputWorkspace=direct, Filename=saved_file)
            reloaded = LoadNexusProcessed(Filename=saved_file, OutputWorkspace="reloaded_histogram")
            self._compare(reloaded, direct)

    def _compare(self, actual, expected):
        matches, messages = CompareWorkspaces(
            Workspace1=actual,
            Workspace2=expected,
            CheckAllData=True,
            CheckInstrument=True,
            CheckSpectraMap=True,
            Tolerance=1e-12,
        )
        self.assertTrue(matches, str(messages.toDict()))
