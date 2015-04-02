#pylint: disable=no-init
import stresstesting
from mantid import *
from mantid.simpleapi import *

class LRPrimaryFractionTest(stresstesting.MantidStressTest):
    def runTest(self):
        workspace = LoadEventNexus(Filename="REF_L_123711")
        self.scaling_factor = LRPrimaryFraction(InputWorkspace=workspace)

    def validate(self):
        ref = [0.887220655191, 0.00257167461136]
        for i in range(2):
            if abs(self.scaling_factor[i]-ref[i])>0.00001:
                logger.error("Output did not match [%s +- %s]: got [%s +- %s]" % (ref[0], ref[1],
                                                                                  self.scaling_factor[0],
                                                                                  self.scaling_factor[1]))
                return False
        return True

