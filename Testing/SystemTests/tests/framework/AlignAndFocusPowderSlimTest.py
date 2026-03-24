# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import (
    AlignAndFocusPowderSlim,
    AlignAndFocusPowderFromFiles,
    PDLoadCharacterizations,
    MaskBins,
    CreateGroupingWorkspace,
)


class PG3_compare_AlignAndFocusPowderFromFiles_with_FilterBadPulses_and_LogBinning(systemtesting.MantidSystemTest):
    cal_file = "PG3_FERNS_d4832_2011_08_24.cal"
    char_file = "PG3_characterization_2012_02_23-HR-ILL.txt"
    data_file = "PG3_9829_event.nxs"

    def requiredMemoryMB(self):
        return 1024

    def requiredFiles(self):
        return [self.cal_file, self.char_file, self.data_file]

    def runTest(self):
        characterizations = PDLoadCharacterizations(Filename=self.char_file)

        self.aafpff = "AlignAndFocusPowderSlimTest_PG3_AAFPowderFromFiles"
        self.aafps = "AlignAndFocusPowderSlimTest_PG3_AAFPowderSlim"

        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            OutputWorkspace=self.aafpff,
            Characterizations="characterizations",
            CalFileName=self.cal_file,
            Params="0.1,-0.0016,2.0",
            PrimaryFlightPath=characterizations.PrimaryFlightPath,
            L2=characterizations.L2,
            Polar=characterizations.Polar,
            Azimuthal=characterizations.Azimuthal,
            PreserveEvents=False,
            FilterBadPulses=95,
        )

        AlignAndFocusPowderSlim(
            Filename=self.data_file,
            OutputWorkspace=self.aafps,
            CalFileName=self.cal_file,
            L1=characterizations.PrimaryFlightPath,
            L2=characterizations.L2,
            Polar=characterizations.Polar,
            Azimuthal=characterizations.Azimuthal,
            FilterBadPulses=True,
        )

        # we need to mask a few bins at the start and end of the data so that the workspaces match
        MaskBins(self.aafpff, OutputWorkspace=self.aafpff, XMin=6232, XMax=6240)
        MaskBins(self.aafpff, OutputWorkspace=self.aafpff, XMin=45000, XMax=50000)
        MaskBins(self.aafps, OutputWorkspace=self.aafps, XMin=6232, XMax=6240)
        MaskBins(self.aafps, OutputWorkspace=self.aafps, XMin=45000, XMax=50000)

    def validateMethod(self):
        self.tolerance = 0.1
        self.tolerance_is_rel_err = True
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.aafpff, self.aafps)


class SNAP_compare_AlignAndFocusPowderFromFiles_with_LogBinning_and_GroupingWorkspace(systemtesting.MantidSystemTest):
    data_file = "SNAP_45874"

    def requiredMemoryMB(self):
        return 1024

    def requiredFiles(self):
        return [self.data_file]

    def runTest(self):
        self.aafpff = "AlignAndFocusPowderSlimTest_SNAP_AAFPowderFromFiles"
        self.aafps = "AlignAndFocusPowderSlimTest_SNAP_AAFPowderSlim"

        L1 = 15
        L2 = [0.191317, 0.191317]
        Polar = [90.0, 90.0]
        Azimuthal = [0.0, 0.0]

        CreateGroupingWorkspace(InstrumentFilename="SNAP_Definition.xml", GroupDetectorsBy="Group", OutputWorkspace="groups")

        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            Params=(0.5, -0.008, 2),
            GroupingWorkspace="groups",
            PrimaryFlightPath=L1,
            L2=L2,
            Polar=Polar,
            Azimuthal=Azimuthal,
            PreserveEvents=False,
            OutputWorkspace=self.aafpff,
        )

        AlignAndFocusPowderSlim(
            Filename=self.data_file,
            XMin=0.5,
            XMax=2,
            XDelta=0.008,
            GroupingWorkspace="groups",
            L1=L1,
            L2=L2,
            Polar=Polar,
            Azimuthal=Azimuthal,
            OutputWorkspace=self.aafps,
        )

    def validateMethod(self):
        self.tolerance = 0.1
        self.tolerance_is_rel_err = True
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.aafps, self.aafpff)


class VULCAN_compare_AlignAndFocusPowderFromFiles_with_RaggedBinning(systemtesting.MantidSystemTest):
    data_file = "VULCAN_189186"
    cal_file = "VULCAN_calibrate_2019_06_27.h5"

    def requiredMemoryMB(self):
        return 1024

    def requiredFiles(self):
        return [self.data_file, self.cal_file]

    def runTest(self):
        self.aafpff = "AlignAndFocusPowderSlimTest_VULCAN_AAFPowderFromFiles"
        self.aafps = "AlignAndFocusPowderSlimTest_VULCAN_AAFPowderSlim"

        L1 = 43.755
        L2 = [2.0145, 2.0145, 2.0156]
        Polar = [90, 90, 150]
        Azimuthal = [180, 0, 0]

        dmin = [0.306, 0.306, 0.22]
        dmax = [2.0, 2.2, 2.0]
        delta = [-0.001, -0.001, -0.0003]

        AlignAndFocusPowderFromFiles(
            Filename=self.data_file,
            CalFileName=self.cal_file,
            PreserveEvents=False,
            Dspacing=True,
            DMin=dmin,
            DMax=dmax,
            DeltaRagged=delta,
            PrimaryFlightPath=L1,
            L2=L2,
            Polar=Polar,
            Azimuthal=Azimuthal,
            OutputWorkspace=self.aafpff,
        )

        AlignAndFocusPowderSlim(
            Filename=self.data_file,
            CalFileName=self.cal_file,
            L1=43.755,
            L2=L2,
            Polar=Polar,
            Azimuthal=Azimuthal,
            XMin=dmin,
            XMax=dmax,
            XDelta=delta,
            OutputWorkspace=self.aafps,
        )

    def validateMethod(self):
        self.tolerance = 1e-8
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return (self.aafps, self.aafpff)
