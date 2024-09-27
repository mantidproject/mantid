# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name

"""
One test has been removed from the port since it uses the ReductionSingleton.
"""

import mantid  # noqa
import systemtesting
import os

from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.api import AnalysisDataService
from mantid.kernel import config
from mantid.simpleapi import DeleteWorkspace
import SANSadd2
from sans_core.command_interface.ISISCommandInterface import (
    SANS2DTUBES,
    MaskFile,
    SetDetectorOffsets,
    Gravity,
    Set1D,
    AddRuns,
    AssignSample,
    AssignCan,
    TransmissionSample,
    TransmissionCan,
    WavRangeReduction,
    UseCompatibilityMode,
)
from sans_core.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithOverlayTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2DTUBES()
        MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()

        # add files (SAMPLE and CAN)
        AddRuns(
            runs=("28827", "28797"),
            instrument="SANS2DTUBES",
            defType=".nxs",
            rawTypes=(".add", ".raw", ".s*"),
            lowMem=False,
            saveAsEvent=True,
            isOverlay=True,
        )
        AddRuns(
            ("28823", "28793"),
            "SANS2DTUBES",
            defType=".nxs",
            rawTypes=(".add", ".raw", ".s*"),
            lowMem=False,
            saveAsEvent=True,
            isOverlay=True,
        )

        AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        WavRangeReduction()

    def validate(self):
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797_rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithOverlay.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in AnalysisDataService.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithOverlayAndTimeShiftTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2DTUBES()
        MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()

        # add files (SAMPLE and CAN)
        time_shifts = [1]
        SANSadd2.add_runs(
            ("28827", "28797"),
            "SANS2DTUBES",
            ".nxs",
            rawTypes=(".add", ".raw", ".s*"),
            lowMem=False,
            saveAsEvent=True,
            isOverlay=True,
            time_shifts=time_shifts,
        )
        SANSadd2.add_runs(
            ("28823", "28793"),
            "SANS2DTUBES",
            ".nxs",
            rawTypes=(".add", ".raw", ".s*"),
            lowMem=False,
            saveAsEvent=True,
            isOverlay=True,
            time_shifts=time_shifts,
        )

        AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        WavRangeReduction()

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797_rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithOverlayAndTimeShifts.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in AnalysisDataService.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithoutOverlayTest_V2(systemtesting.MantidSystemTest):
    def runTest(self):
        UseCompatibilityMode()
        SANS2DTUBES()
        MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        Gravity(False)
        Set1D()

        # add files (SAMPLE and CAN)
        SANSadd2.add_runs(
            ("28827", "28797"), "SANS2DTUBES", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False, saveAsEvent=True, isOverlay=False
        )
        SANSadd2.add_runs(
            ("28823", "28793"), "SANS2DTUBES", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False, saveAsEvent=True, isOverlay=False
        )

        AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        WavRangeReduction()

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797_rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithoutOverlay.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in AnalysisDataService.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))
