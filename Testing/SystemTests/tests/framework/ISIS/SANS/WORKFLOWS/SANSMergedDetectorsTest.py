# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import DeleteWorkspace, mtd
import ISISCommandInterface as i
import systemtesting
from sans.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANSMergedDetectorsTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)

    def runTest(self):
        # Select instrument and user file
        i.SANS2DTUBES()
        i.MaskFile(file_name="USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")

        # Setup detector positions
        i.SetDetectorOffsets(bank="REAR", x=-16.0, y=58.0, z=0.0, rot=0.0, radius=0.0, side=0.0)
        i.SetDetectorOffsets(bank="FRONT", x=-44.0, y=-20.0, z=47.0, rot=0.0, radius=1.0, side=1.0)
        i.Gravity(False)

        # Set the front detector fitting
        i.SetFrontDetRescaleShift(scale=1.0, shift=0.0, fitScale=True, fitShift=True)
        i.Set1D()

        # Assign data
        i.AssignSample(r"SANS2D00028797.nxs", reload=True)
        i.AssignCan(r"SANS2D00028793.nxs", reload=True)
        i.TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        i.TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        # Run the reduction and request FRONT and BACK to be merged
        i.WavRangeReduction(combineDet="merged")

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        self.tolerance = 1e-07
        return "28797merged_1D_1.75_16.5", "SANS2DTUBES_Merged_Reduction.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
