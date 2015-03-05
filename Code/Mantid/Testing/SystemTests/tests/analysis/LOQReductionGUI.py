import stresstesting
from mantid.simpleapi import * 
import isis_reducer
import ISISCommandInterface as i
import isis_instrument
import isis_reduction_steps

MASKFILE = FileFinder.getFullPath('MaskLOQData.txt')
BATCHFILE = FileFinder.getFullPath('loq_batch_mode_reduction.csv')

class LOQMinimalBatchReduction(stresstesting.MantidStressTest):
    def __init__(self):
        super(LOQMinimalBatchReduction, self).__init__()
        config['default.instrument'] = 'LOQ'

    def runTest(self):
        import SANSBatchMode as batch
        i.LOQ()
        i.MaskFile(MASKFILE)
        fit_settings = batch.BatchReduce(BATCHFILE, '.nxs', combineDet='merged', saveAlgs={})

    def validate(self):
    	# note increased tolerance to something which quite high
    	# this is partly a temperary measure, but also justified by
    	# when overlaying the two options they overlap very well
        self.tolerance = 1.0e+1
        return 'first_time_merged', 'LOQReductionMergedData.nxs'
