#pylint: disable=no-init,unused-variable
import stresstesting
from mantid.simpleapi import *


class RawVNexus(stresstesting.MantidStressTest):
    ''' Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace'''

    def runTest(self):
        LoadRaw(Filename='SANS2D00000808.raw', OutputWorkspace='Raw')

    def validate(self):
        return 'Raw','SANS2D00000808.nxs'
