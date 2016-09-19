from __future__ import (absolute_import, division, print_function)

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


    def test_basic(self):
        trans_workspace = IndirectTransmissionMonitor(**self.kwargs)

        self.assertTrue(isinstance(trans_workspace, mantid.api.WorkspaceGroup), msg='Result should be a workspace group')
        self.assertEqual(trans_workspace.size(), 3, msg='Transmission workspace group should have 3 workspaces: sample, can and transfer')

        expected_names = set()
        expected_names.add(self._sample_workspace + '_Can')
        expected_names.add(self._sample_workspace + '_Sam')
        expected_names.add(self._sample_workspace + '_Trans')
        self.assertEqual(set(trans_workspace.getNames()), expected_names)


if __name__ == '__main__':
    unittest.main()
