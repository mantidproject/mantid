import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace, AddSampleLogMultiple, DeleteWorkspace, GroupWorkspaces


class CompareSampleLogsTest(unittest.TestCase):
    def setUp(self):
        ws1 = CreateSampleWorkspace()
        self.ws1 = ws1
        ws2 = CreateSampleWorkspace()
        self.ws2 = ws2
        lognames = 'run_title,deterota,wavelength,polarisation,flipper'
        logvalues = 'ws1,-10.0,4.2,x,ON'
        AddSampleLogMultiple(Workspace=ws1, LogNames=lognames, LogValues=logvalues, ParseType=True)
        logvalues = 'ws2,-10.0,4.2,x,OFF'
        AddSampleLogMultiple(Workspace=ws2, LogNames=lognames, LogValues=logvalues, ParseType=True)

    def test_workspaces(self):
        wslist = [self.ws1, self.ws2]
        lognames = 'deterota,wavelength,polarisation,flipper'
        alg_test = run_algorithm("CompareSampleLogs", InputWorkspaces=wslist, SampleLogs=lognames,
                                 Tolerance=0.01, DoNotMatchAction='warning')
        self.assertTrue(alg_test.isExecuted())

    def test_groups(self):
        GroupWorkspaces([self.ws1, self.ws2], OutputWorkspace='group')
        lognames = 'deterota,wavelength,polarisation,flipper'
        alg_test = run_algorithm("CompareSampleLogs", InputWorkspaces='group', SampleLogs=lognames,
                                 Tolerance=0.01, DoNotMatchAction='warning')
        self.assertTrue(alg_test.isExecuted())

    def tearDown(self):
        if AnalysisDataService.doesExist('ws1'):
            DeleteWorkspace('ws1')
        if AnalysisDataService.doesExist('ws2'):
            DeleteWorkspace('ws2')

if __name__ == "__main__":
    unittest.main()
