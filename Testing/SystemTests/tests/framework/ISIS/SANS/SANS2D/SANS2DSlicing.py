# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest

from mantid.simpleapi import mtd, RenameWorkspace, FileFinder
import ISISCommandInterface as i
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DMinimalBatchReductionSliced(systemtesting.MantidSystemTest):
    def __init__(self):
        super(SANS2DMinimalBatchReductionSliced, self).__init__()

    def runTest(self):
        import SANSBatchMode as batch

        i.SANS2D()
        i.MaskFile("MaskSANS2DReductionGUI.txt")
        i.SetEventSlices("0.0-451, 5-10")
        batch_file = FileFinder.getFullPath("sans2d_reduction_gui_batch.csv")
        batch.BatchReduce(batch_file, ".nxs", saveAlgs={}, combineDet="rear")

    def validate(self):
        self.tolerance = 0.02
        self.tolerance_is_rel_err = True
        self.disableChecking.append("Instrument")
        return str(mtd["trans_test_rear_1D_1.5_12.5"][0]), "SANSReductionGUI.nxs"


class SANS2DMinimalSingleReductionSliced(SANS2DMinimalBatchReductionSliced):
    def runTest(self):
        i.SANS2D()
        i.MaskFile("MaskSANS2DReductionGUI.txt")
        i.AssignSample("22048")
        i.AssignCan("22023")
        i.TransmissionSample("22041", "22024")
        i.TransmissionCan("22024", "22024")
        i.SetEventSlices("0.0-451, 5-10")
        reduced = i.WavRangeReduction()
        RenameWorkspace(reduced, OutputWorkspace="trans_test_rear_1D_1.5_12.5")


if __name__ == "__main__":
    test = SANS2DMinimalSingleReductionSliced()
    test.execute()
