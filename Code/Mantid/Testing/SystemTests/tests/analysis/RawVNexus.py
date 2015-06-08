#pylint: disable=no-init
import stresstesting
from mantid.simpleapi import *

class RawVNexus(stresstesting.MantidStressTest):
    ''' Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace'''

    def runTest(self):
        Raw = LoadRaw(Filename='SANS2D00000808.raw')

    def validate(self):
        return 'Raw','SANS2D00000808.nxs'
