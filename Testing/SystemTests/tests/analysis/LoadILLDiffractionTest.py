import stresstesting

from mantid.simpleapi import CompareWorkspaces, LoadILLDiffraction, LoadNexusProcessed
from mantid import config


class LoadILLDiffractionTest(stresstesting.MantidStressTest):
    def setUp(self):
        # cache default instrument and datadirs
        self.facility = config['default.facility']
        self.instrument = config['default.instrument']
        self.datadirs = config['datasearch.directories']

        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def tearDown(self):
        config['default.facility'] = self.facility
        config['default.instrument'] = self.instrument
        config['datasearch.directories'] = self.datadirs

    def d20_detector_scan_test(self):
        ws = LoadILLDiffraction('967076.nxs')
        LoadNexusProcessed(Filename="967076_reference_load.nxs", OutputWorkspace="ref")
        result = CompareWorkspaces(Workspace1='967076', Workspace2='ref', Tolerance=1e-6)

        if not result[0]:
            self.assertTrue(result[0],"Mismatch in D20 detector scan: " + result[1].row(0)['Message'])

    def runTest(self):
        self.runTest()

        self.d20_detector_scan_test()

        self.tearDown()

