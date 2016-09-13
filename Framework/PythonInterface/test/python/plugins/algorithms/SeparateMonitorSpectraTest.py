import unittest
from mantid.simpleapi import *

class SeparateMonitorSpectraTest(unittest.TestCase):

    def setUp(self):
        return super(SeparateMonitorSpectraTest, self).setUp()

    def tearDown(self):
        for ws in ['testWS', 'det', 'mon', 'det2', 'mon2']:
            if AnalysisDataService.doesExist(ws):
                DeleteWorkspace(ws)
        return super(SeparateMonitorSpectraTest, self).tearDown()

    def test_workspace_with_only_detector_output_gives_only_detectors_in_output(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorWorkspace = 'det')
        detectors = mtd['det']

        self.assertEquals(detectors.getNumberHistograms(), 200)

    def test_workspace_with_only_monitor_output_gives_only_monitors_in_output(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', MonitorWorkspace = 'mon')
        monitors = mtd['mon']

        self.assertEquals(monitors.getNumberHistograms(), 3)

    def test_workspace_with_detector_and_monitor_output_gives_two_workspaces_with_detectors_and_monitors(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorWorkspace = 'det', MonitorWorkspace = 'mon')
        detectors = mtd['det']
        monitors = mtd['mon']

        self.assertEquals(detectors.getNumberHistograms(), 200)
        self.assertEquals(monitors.getNumberHistograms(), 3)
        self.assertEquals(detectors.getMonitorWorkspace().name(), "mon")

    def test_workspace_with_linked_monitors_throws_error(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorWorkspace = 'det', MonitorWorkspace = 'mon')

        self.assertRaises(RuntimeError,
                          SeparateMonitorSpectra,
                          InputWorkspace='det',
                          DetectorWorkspace='det2',
                          MonitorWorkspace='mon2')

    def test_workspace_with_no_detector_or_monitor_output_throws_error(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)

        self.assertRaises(RuntimeError,
                          SeparateMonitorSpectra,
                          InputWorkspace='testWS')

    def test_workspace_with_no_detectors_gives_no_detector_workspace(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumBanks = 0, NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorWorkspace = 'det', MonitorWorkspace = 'mon')

        self.assertRaises(KeyError, mtd.retrieve, 'det')
        monitors = mtd['mon']

        self.assertEquals(monitors.getNumberHistograms(), 3)

    def test_workspace_with_no_monitors_gives_empty_monitor_workspace(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 0)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorWorkspace = 'det', MonitorWorkspace = 'mon')

        detectors = mtd['det']
        self.assertRaises(KeyError, mtd.retrieve, 'mon')

        self.assertEquals(detectors.getNumberHistograms(), 200)

if __name__=="__main__":
    unittest.main()
