import stresstesting

from mantid.simpleapi import LoadILLDiffraction
from mantid import config


class LoadILLDiffractionTest(stresstesting.MantidStressTest):

    def __init__(self):
        super(LoadILLDiffractionTest, self).__init__()
        self.setUp()

    def requiredFiles(self):

        return ["967076.nxs"]

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
        # tests the loading for D20 calibration run (detector scan)
        ws = LoadILLDiffraction('967076.nxs')
        self.assertEquals(ws.blocksize(), 1)
        self.assertEquals(ws.getNumberHistograms(), (2 * 1536 + 1) * 571)
        self.assertEquals(ws.readY(0)[0], 523944)
        self.assertDelta(ws.readE(0)[0], 723.8397, 0.0001)
        self.assertEquals(ws.readY(483879)[0], 6541)
        self.assertDelta(ws.readE(483879)[0], 80.8764, 0.0001)

    def runTest(self):

        self.d20_detector_scan_test()

        self.tearDown()
