import stresstesting
from mantid.simpleapi import *

''' Simply tests that our LoadRaw and LoadISISNexus algorithms produce the same workspace'''
class RawVNexus(stresstesting.MantidStressTest):
    
  def runTest(self):
    Raw = LoadRaw(Filename='SANS2D00000808.raw')

  def validate(self):
    return 'Raw','SANS2D00000808.nxs'
