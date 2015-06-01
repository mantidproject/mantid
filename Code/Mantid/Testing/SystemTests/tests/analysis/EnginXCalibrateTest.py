#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class EnginXCalibrateTest(stresstesting.MantidStressTest):

    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        self.difc = -1
        self.zero = -1

    def runTest(self):
        positions = EnginXCalibrateFull(Filename = 'ENGINX00193749.nxs',
                                      Bank = 1,
                                      ExpectedPeaks = '1.3529, 1.6316, 1.9132')

        (self.difc, self.zero) = EnginXCalibrate(Filename = 'ENGINX00193749.nxs',
                                               Bank = 1,
                                               ExpectedPeaks = '2.7057,1.9132,1.6316,1.5621,1.3528,0.9566',
                                               DetectorPositions = positions)

    def validate(self):
        import sys
        if sys.platform == "darwin":
          # Mac fitting tests produce differences for some reason.
            self.assertDelta(self.difc, 18405.4, 0.1)
            self.assertDelta(self.zero, 3.60, 0.05)
        else:
            self.assertDelta(self.difc, 18404.496, 0.001)
            self.assertDelta(self.zero, 4.4345, 0.001)

    def cleanup(self):
        mtd.remove('positions')
