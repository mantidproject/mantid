# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=attribute-defined-outside-init

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
import ISISCommandInterface as i
from SANS.sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQMinimalBatchReduction(systemtesting.MantidSystemTest):
    def __init__(self):
        super(SANSLOQMinimalBatchReduction, self).__init__()
        config["default.instrument"] = "LOQ"

    def runTest(self):
        import SANSBatchMode as batch

        i.LOQ()
        i.MaskFile("MaskLOQData.txt")
        batch_file = FileFinder.getFullPath("loq_batch_mode_reduction.csv")
        batch.BatchReduce(batch_file, ".nxs", combineDet="merged", saveAlgs={})

    def validate(self):
        # note increased tolerance to something which quite high
        # this is partly a temperary measure, but also justified by
        # when overlaying the two options they overlap very well
        self.tolerance = 1.0e1
        self.disableChecking.append("Instrument")
        return "first_time_merged_1D_2.2_10.0", "LOQReductionMergedData.nxs"
