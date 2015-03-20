#pylint: disable=no-init
import stresstesting

from mantid.simpleapi import *
from ISISCommandInterface import *
from mantid.simpleapi import *
from mantid import config
from SANSBatchMode import *
import os.path

# test batch mode with sans2d and selecting a period in batch mode
class SANS2DMultiPeriodSingle(stresstesting.MantidStressTest):

    def runTest(self):

        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('MASKSANS2Doptions.091A')
        Gravity(True)

        AssignSample('5512')
        self.reduced = WavRangeReduction()

    def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
        self.disableChecking.append('SpectraMap')
        self.disableChecking.append('Axes')
        self.disableChecking.append('Instrument')

        return mtd[self.reduced][6].name(),'SANS2DBatch.nxs'

class SANS2DMultiPeriodBatch(SANS2DMultiPeriodSingle):

    def runTest(self):

        SANS2D()
        Set1D()
        Detector("rear-detector")
        MaskFile('MASKSANS2Doptions.091A')
        Gravity(True)

        csv_file = FileFinder.getFullPath('SANS2D_multiPeriodTests.csv')

        BatchReduce(csv_file, 'nxs', saveAlgs={})
        self.reduced = '5512_SANS2DBatch'
    