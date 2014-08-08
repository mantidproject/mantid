import unittest, os
import mantid
from mantid.simpleapi import *

class IndirectTransmissionMonitorTest(unittest.TestCase):

    def setUp(self):
        self.kwargs = {}
        self.kwargs['SampleFile'] = 'IRS26176.RAW'
        self.kwargs['CanFile'] = 'IRS26173.RAW'
        self.kwargs['Verbose'] = True
        self.kwargs['Plot'] = False

        self.run_number, _ = os.path.splitext(self.kwargs['SampleFile'].split('_')[0])

    def tearDown(self):
        # Clean up saved nexus files
        path = os.path.join(config['defaultsave.directory'], self.run_number + '_Transmission.nxs')
        if os.path.isfile(path):
            try:
                os.remove(path)
            except IOError, _:
                pass

    def test_basic(self):
        IndirectTransmissionMonitor(**self.kwargs)

        trans_workspace = mtd[self.run_number + '_Transmission']

        self.assertTrue(isinstance(trans_workspace, mantid.api.WorkspaceGroup), msg='Result should be a workspace group')
        self.assertEqual(trans_workspace.size(), 3, msg='Transmission workspace group should have 3 workspaces: sample, can and transfer')

    def test_nexus_save(self):
        self.kwargs['Save'] = True

        IndirectTransmissionMonitor(**self.kwargs)

        path = os.path.join(config['defaultsave.directory'], self.run_number + '_Transmission.nxs')
        self.assertTrue(os.path.isfile(path), msg='Transmission workspace should be saved to default save directory')

    def test_empty_filenames(self):
        self.kwargs['CanFile'] = ''

        self.assertRaises(ValueError, IndirectTransmissionMonitor, **self.kwargs)

    def test_nonexistent_file(self):
        self.kwargs['CanFile'] = 'NoFile.raw'

        self.assertRaises(ValueError, IndirectTransmissionMonitor, **self.kwargs)

if __name__ == '__main__':
    unittest.main()
