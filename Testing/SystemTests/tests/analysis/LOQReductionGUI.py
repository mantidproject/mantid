#pylint: disable=attribute-defined-outside-init

from __future__ import (absolute_import, division, print_function)
import stresstesting
from mantid.simpleapi import *
import ISISCommandInterface as ii
import sans.command_interface.ISISCommandInterface as ii2

MASKFILE = FileFinder.getFullPath('MaskLOQData.txt')
BATCHFILE = FileFinder.getFullPath('loq_batch_mode_reduction.csv')


class LOQMinimalBatchReduction(stresstesting.MantidStressTest):
    def __init__(self):
        super(LOQMinimalBatchReduction, self).__init__()
        config['default.instrument'] = 'LOQ'

    def runTest(self):
        import SANSBatchMode as batch
        ii.LOQ()
        ii.MaskFile(MASKFILE)
        batch.BatchReduce(BATCHFILE, '.nxs', combineDet='merged', saveAlgs={})

    def validate(self):
        # note increased tolerance to something which quite high
        # this is partly a temperary measure, but also justified by
        # when overlaying the two options they overlap very well
        self.tolerance = 1.0e+1
        self.disableChecking.append('Instrument')
        return 'first_time_merged_1D_2.2_10.0', 'LOQReductionMergedData.nxs'


class LOQMinimalBatchReductionTest_V2(stresstesting.MantidStressTest):
    def __init__(self):
        super(LOQMinimalBatchReductionTest_V2, self).__init__()
        config['default.instrument'] = 'LOQ'

    def runTest(self):
        ii2.UseCompatibilityMode()
        ii2.LOQ()
        ii2.MaskFile(MASKFILE)
        ii2.BatchReduce(BATCHFILE, '.nxs', combineDet='merged', saveAlgs={})

    def validate(self):
        # note increased tolerance to something which quite high
        # this is partly a temperary measure, but also justified by
        # when overlaying the two options they overlap very well --> what does this comment mean?
        self.tolerance = 1.0e+1
        self.disableChecking.append('Instrument')
        return 'first_time_merged', 'LOQReductionMergedData.nxs'
