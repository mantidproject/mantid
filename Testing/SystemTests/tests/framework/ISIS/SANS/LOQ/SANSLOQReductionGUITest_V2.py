# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=attribute-defined-outside-init
import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.kernel import config
from sans.command_interface.ISISCommandInterface import UseCompatibilityMode, LOQ, MaskFile, BatchReduce
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSLOQMinimalBatchReductionTest_V2(systemtesting.MantidSystemTest):
    def __init__(self):
        super(SANSLOQMinimalBatchReductionTest_V2, self).__init__()
        config["default.instrument"] = "LOQ"

    def runTest(self):
        UseCompatibilityMode()
        LOQ()
        MaskFile("MaskLOQData.txt")
        BatchReduce("loq_batch_mode_reduction.csv", ".nxs", combineDet="merged", saveAlgs={})

    def validate(self):
        # note increased tolerance to something which quite high
        # this is partly a temperary measure, but also justified by
        # when overlaying the two options they overlap very well --> what does this comment mean?
        self.tolerance = 1.0e1
        self.disableChecking.append("Instrument")
        return "first_time_merged_1D_2.2_10.0", "LOQReductionMergedData.nxs"
