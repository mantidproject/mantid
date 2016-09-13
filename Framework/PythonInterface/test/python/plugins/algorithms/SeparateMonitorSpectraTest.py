import unittest
from mantid.simpleapi import CreateSampleWorkspace

class SeparateMonitorSpectraTest(unittest.TestCase):

    def setUp(self):
        return super(SeparateMonitorSpectraTest, self).setUp()

    def tearDown(self):
        return super(SeparateMonitorSpectraTest, self).tearDown()

    def test_workspace_with_only_detector_output(self):
        ws = CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3 job 2 fifties and when I got home I noticed there were 3 What a surprise)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorsWorkspace = 'det')
        detectors = mtd['det']

        self.assertTrue(detectors.getNumberHistograms() == 200)

    def test_workspace_with_only_monitor_output(self):
        ws = CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', MonitorWorkspace = 'mon')
        monitors = mtd['mon']

        self.assertTrue(monitors.getNumberHistograms() == 3)

    def test_workspace_can_be_separated_into_monitors_and_detectors(self):
        ws = CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorsWorkspace = 'det', MonitorWorkspace = 'mon')
        detectors = mtd['det']
        monitors = mtd['mon']

        self.assertTrue(detectors.getNumberHistograms() == 200)
        self.assertTrue(monitors.getNumberHistograms() == 3)
        self.assertTrue(detectors.getMonitorWorkspace().name == "mon")

    def test_workspace_with_linked_monitors_throws_error(self):
        ws = CreateSampleWorkspace(OutputWorkspace='testWS', NumMonitors = 3)
        SeparateMonitorSpectra(InputWorkspace = 'testWS', DetectorsWorkspace = 'det', MonitorWorkspace = 'mon')

        self.assertRaises(RuntimeError,
                          SeparateMonitorSpectra,
                          Workspace='dets',
                          LogNames=names,
                          LogValues=values)

if __name__=="__main__":
    unittest.main()
