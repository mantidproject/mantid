import stresstesting
import os
from mantid.simpleapi import *

class ILLIndirectReductionTest(stresstesting.MantidStressTest):

    _run = None
    self.tolerance = 0.0001

    def __init__(self):
        super(ILLIndirectReductionTest, self).__init__()

    def runTest(self):
        pass