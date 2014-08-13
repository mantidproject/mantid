import unittest, os
import mantid
from mantid.simpleapi import *

class IndirectTransmissionMonitorTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        ##TODO: Create sample workspaces
        sample_workspace = Load('IRS26176.RAW')
        can_workspace = Load('IRS26173.RAW')

        cls._sample_workspace = sample_workspace
        cls._can_workspace = can_workspace

    def setUp(self):
        self.kwargs = {}
        self.kwargs['SampleWorkspace'] = self._sample_workspace
        self.kwargs['CanWorkspace'] = self._can_workspace
        self.kwargs['Verbose'] = True
        self.kwargs['Plot'] = False

    def tearDown(self):
        # Clean up saved nexus files
        path = os.path.join(config['defaultsave.directory'], 'IndirectTransmissionMonitorTest.nxs')
        if os.path.isfile(path):
            try:
                os.remove(path)
            except IOError, _:
                pass

    def test_basic(self):
        trans_workspace = IndirectTransmissionMonitor(**self.kwargs)

        self.assertTrue(isinstance(trans_workspace, mantid.api.WorkspaceGroup), msg='Result should be a workspace group')
        self.assertEqual(trans_workspace.size(), 3, msg='Transmission workspace group should have 3 workspaces: sample, can and transfer')

    def test_nexus_save(self):
        self.kwargs['Save'] = True
        self.kwargs['OutputWorkspace'] = 'IndirectTransmissionMonitorTest'

        IndirectTransmissionMonitor(**self.kwargs)

        path = os.path.join(config['defaultsave.directory'], 'IndirectTransmissionMonitorTest.nxs')
        self.assertTrue(os.path.isfile(path), msg='Transmission workspace should be saved to default save directory')

if __name__ == '__main__':
    unittest.main()
