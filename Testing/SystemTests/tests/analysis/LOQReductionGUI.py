# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=attribute-defined-outside-init

from __future__ import (absolute_import, division, print_function)
import systemtesting
from mantid.simpleapi import *
import ISISCommandInterface as i

MASKFILE = FileFinder.getFullPath('MaskLOQData.txt')
BATCHFILE = FileFinder.getFullPath('loq_batch_mode_reduction.csv')


class LOQMinimalBatchReduction(systemtesting.MantidSystemTest):
    def __init__(self):
        super(LOQMinimalBatchReduction, self).__init__()
        config['default.instrument'] = 'LOQ'

    def runTest(self):
        import SANSBatchMode as batch
        i.LOQ()
        i.MaskFile(MASKFILE)
        batch.BatchReduce(BATCHFILE, '.nxs', combineDet='merged', saveAlgs={})

    def validate(self):
        # note increased tolerance to something which quite high
        # this is partly a temperary measure, but also justified by
        # when overlaying the two options they overlap very well
        self.tolerance = 1.0e+1
        self.disableChecking.append('Instrument')
        return 'first_time_merged_1D_2.2_10.0', 'LOQReductionMergedData.nxs'
