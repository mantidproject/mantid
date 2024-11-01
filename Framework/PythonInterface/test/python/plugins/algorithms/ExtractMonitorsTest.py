# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd, AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace, DeleteWorkspace, ExtractMonitors


class ExtractMonitorsTest(unittest.TestCase):
    def setUp(self):
        return super(ExtractMonitorsTest, self).setUp()

    def tearDown(self):
        for ws in ["testWS", "det", "mon", "det2", "mon2"]:
            if AnalysisDataService.doesExist(ws):
                DeleteWorkspace(ws)
        return super(ExtractMonitorsTest, self).tearDown()

    def test_workspace_with_only_detector_output_gives_only_detectors_in_output(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=3)
        ExtractMonitors(InputWorkspace="testWS", DetectorWorkspace="det")
        detectors = mtd["det"]

        self.assertEqual(detectors.getNumberHistograms(), 200)

    def test_workspace_with_only_monitor_output_gives_only_monitors_in_output(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=3)
        ExtractMonitors(InputWorkspace="testWS", MonitorWorkspace="mon")
        monitors = mtd["mon"]

        self.assertEqual(monitors.getNumberHistograms(), 3)

    def test_workspace_with_detector_and_monitor_output_gives_two_workspaces_with_detectors_and_monitors(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=3)
        ExtractMonitors(InputWorkspace="testWS", DetectorWorkspace="det", MonitorWorkspace="mon")
        detectors = mtd["det"]
        monitors = mtd["mon"]

        self.assertEqual(detectors.getNumberHistograms(), 200)
        self.assertEqual(monitors.getNumberHistograms(), 3)
        self.assertEqual(detectors.getMonitorWorkspace().name(), "mon")

    def test_workspace_with_linked_monitors_throws_error(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=3)
        ExtractMonitors(InputWorkspace="testWS", DetectorWorkspace="det", MonitorWorkspace="mon")

        self.assertRaisesRegex(
            RuntimeError,
            "Monitor workspace already exists",
            ExtractMonitors,
            InputWorkspace="det",
            DetectorWorkspace="det2",
            MonitorWorkspace="mon2",
        )

    def test_workspace_with_no_detector_or_monitor_output_throws_error(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=3)

        self.assertRaisesRegex(
            RuntimeError, "Must specify one of DetectorsWorkspace or MonitorsWorkspace", ExtractMonitors, InputWorkspace="testWS"
        )

    def test_workspace_with_no_detectors_gives_no_detector_workspace(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumBanks=0, NumMonitors=3)
        ExtractMonitors(InputWorkspace="testWS", DetectorWorkspace="det", MonitorWorkspace="mon")

        self.assertRaises(KeyError, mtd.retrieve, "det")
        monitors = mtd["mon"]

        self.assertEqual(monitors.getNumberHistograms(), 3)

    def test_workspace_with_no_monitors_gives_no_monitor_workspace(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=0)
        ExtractMonitors(InputWorkspace="testWS", DetectorWorkspace="det", MonitorWorkspace="mon")

        detectors = mtd["det"]
        self.assertRaises(KeyError, mtd.retrieve, "mon")

        self.assertEqual(detectors.getNumberHistograms(), 200)

    def test_workspace_with_same_input_ws_and_detector_ws_names(self):
        CreateSampleWorkspace(OutputWorkspace="testWS", NumMonitors=3)
        ExtractMonitors(InputWorkspace="testWS", DetectorWorkspace="testWS", MonitorWorkspace="mon")
        detectors = mtd["testWS"]
        monitors = mtd["mon"]

        self.assertEqual(detectors.getNumberHistograms(), 200)
        self.assertEqual(monitors.getNumberHistograms(), 3)
        self.assertEqual(detectors.getMonitorWorkspace().name(), "mon")

        spectrumInfo = monitors.spectrumInfo()
        for i in range(monitors.getNumberHistograms()):
            self.assertTrue(spectrumInfo.isMonitor(i))

        spectrumInfo = detectors.spectrumInfo()
        for i in range(detectors.getNumberHistograms()):
            self.assertFalse(spectrumInfo.isMonitor(i))


if __name__ == "__main__":
    unittest.main()
