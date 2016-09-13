import unittest
from mantid.simpleapi import CreateSampleWorkspace

class SeparateMonitorSpectraTest(unittest.TestCase):

    def setUp(self):
        return super(SeparateMonitorSpectraTest, self).setUp()

    def tearDown(self):
        return super(SeparateMonitorSpectraTest, self).tearDown()

    def test_workspace_can_be_separated_into_monitors_and_detectors(self):
        CreateSampleWorkspace(OutputWorkspace='testWS', NumBanks=5, BankPixelWidth=1)

if __name__=="__main__":
    unittest.main()
