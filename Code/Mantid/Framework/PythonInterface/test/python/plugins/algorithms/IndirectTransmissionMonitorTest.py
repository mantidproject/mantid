import unittest
import os
import mantid
from mantid.simpleapi import *


class IndirectTransmissionMonitorTest(unittest.TestCase):

    def setUp(self):
        self._sample_workspace = 'IndirectTransmissionMonitorTest_sample'
        self._can_workspace = 'IndirectTransmissionMonitorTest_can'

        Load(Filename='IRS26176.RAW', OutputWorkspace=self._sample_workspace)
        Load(Filename='IRS26173.RAW', OutputWorkspace=self._can_workspace)

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

        expected_names = set()
        expected_names.add(self._sample_workspace + '_Can')
        expected_names.add(self._sample_workspace + '_Sam')
        expected_names.add(self._sample_workspace + '_Trans')
        self.assertEqual(set(trans_workspace.getNames()), expected_names)


    def test_nexus_save(self):
        self.kwargs['Save'] = True
        self.kwargs['OutputWorkspace'] = 'IndirectTransmissionMonitorTest'

        IndirectTransmissionMonitor(**self.kwargs)

        path = os.path.join(config['defaultsave.directory'], 'IndirectTransmissionMonitorTest.nxs')
        self.assertTrue(os.path.isfile(path), msg='Transmission workspace should be saved to default save directory')

if __name__ == '__main__':
    unittest.main()
