# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting

from mantid.simpleapi import LoadILLDiffraction
from mantid import config


# TODO: Once the nexus saver for a scanned workspace is implemented,
# replace the assertions with compare workspaces with the reference
class ILLPowderLoadDetectorScanTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILLPowderLoadDetectorScanTest, self).__init__()
        self.setUp()

    def requiredFiles(self):

        return ["967076.nxs"]

    def setUp(self):

        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D20'
        config.appendDataSearchSubDir('ILL/D20/')

    def d20_detector_scan_test(self):
        # tests the loading for D20 calibration run (detector scan)
        ws = LoadILLDiffraction('967076.nxs')
        self.assertEquals(ws.blocksize(), 1)
        self.assertEquals(ws.getNumberHistograms(), (2 * 1536 + 1) * 571)
        self.assertEquals(ws.readY(0)[0], 523944)
        self.assertDelta(ws.readE(0)[0], 723.8397, 0.0001)
        self.assertEquals(ws.readY(570)[0], 523819)
        self.assertDelta(ws.readE(570)[0], 723.7534, 0.0001)
        self.assertEquals(ws.readY(571)[0], 0)
        self.assertEquals(ws.readE(571)[0], 0)
        self.assertEquals(ws.readY(37114)[0], 0)
        self.assertEquals(ws.readE(37114)[0], 0)
        self.assertEquals(ws.readY(37115)[0], 6111)
        self.assertDelta(ws.readE(37115)[0], 78.1728, 0.0001)
        self.assertEquals(ws.readY(1754682)[0], 4087)
        self.assertDelta(ws.readE(1754682)[0], 63.9296, 0.0001)

    def runTest(self):

        self.d20_detector_scan_test()
