# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
import ISISCommandInterface as i
import copy
import SANS2DReductionGUI as sansgui

from sans_core.common.enums import SANSInstrument


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DReductionGUIAddedFiles(sansgui.SANS2DGUIReduction):
    def runTest(self):
        self.initialization()

        self.checkFirstPart()

        # add files (SAMPLE and CAN)
        import SANSadd2

        SANSadd2.add_runs(("22048", "22048"), "SANS2D", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False)
        SANSadd2.add_runs(("22023", "22023"), "SANS2D", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False)

        # load values:
        i.SetCentre("155.45", "-169.6", "rear")
        i.SetCentre("155.45", "-169.6", "front")
        i.AssignSample(r"SANS2D00022048-add.nxs", reload=True, period=1)
        i.AssignCan(r"SANS2D00022023-add.nxs", reload=True, period=1)
        i.TransmissionSample(r"SANS2D00022041.nxs", r"SANS2D00022024.nxs", period_t=1, period_d=1)
        i.TransmissionCan(r"SANS2D00022024.nxs", r"SANS2D00022024.nxs", period_t=1, period_d=1)

        self.checkAfterLoad()

        self.applyGUISettings()

        self.applySampleSettings()
        _user_settings_copy = copy.deepcopy(i.ReductionSingleton().user_settings)

        reduced = i.WavRangeReduction(full_trans_wav=False, resetSetup=False)
        RenameWorkspace(reduced, OutputWorkspace="trans_test_rear")

        self.checkFittingSettings()
        self.cleanReduction(_user_settings_copy)

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance_is_rel_err = True
        self.tolerance = 0.35
        self.disableChecking.append("Instrument")
        return "trans_test_rear", "SANSReductionGUI.nxs"


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithOverlay(sansgui.SANS2DGUIReduction):
    def runTest(self):
        i.SANS2DTUBES()
        i.MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        i.SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        i.SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        i.Gravity(False)
        i.Set1D()

        # add files (SAMPLE and CAN)
        import SANSadd2

        SANSadd2.add_runs(
            ("28827", "28797"), "SANS2DTUBES", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False, saveAsEvent=True, isOverlay=True
        )
        SANSadd2.add_runs(
            ("28823", "28793"), "SANS2DTUBES", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False, saveAsEvent=True, isOverlay=True
        )

        i.AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        i.AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        i.TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        i.TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        i.WavRangeReduction()

    def validate(self):
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithOverlay.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithOverlayAndTimeShift(sansgui.SANS2DGUIReduction):
    def runTest(self):
        i.SANS2DTUBES()
        i.MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        i.SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        i.SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        i.Gravity(False)
        i.Set1D()

        # add files (SAMPLE and CAN)
        time_shifts = [1]
        import SANSadd2

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

        i.AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        i.AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        i.TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        i.TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        i.WavRangeReduction()

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithOverlayAndTimeShifts.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithoutOverlay(sansgui.SANS2DGUIReduction):
    def runTest(self):
        i.SANS2DTUBES()
        i.MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        i.SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        i.SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        i.Gravity(False)
        i.Set1D()

        # add files (SAMPLE and CAN)
        import SANSadd2

        SANSadd2.add_runs(
            ("28827", "28797"), "SANS2DTUBES", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False, saveAsEvent=True, isOverlay=False
        )
        SANSadd2.add_runs(
            ("28823", "28793"), "SANS2DTUBES", ".nxs", rawTypes=(".add", ".raw", ".s*"), lowMem=False, saveAsEvent=True, isOverlay=False
        )

        i.AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        i.AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        i.TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        i.TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        i.WavRangeReduction()

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithoutOverlay.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DAddedEventFilesWithoutOverlayWithISISCommandInterface(sansgui.SANS2DGUIReduction):
    def runTest(self):
        i.SANS2DTUBES()
        i.MaskFile("USER_SANS2D_143ZC_2p4_4m_M4_Knowles_12mm.txt")
        i.SetDetectorOffsets("REAR", -16.0, 58.0, 0.0, 0.0, 0.0, 0.0)
        i.SetDetectorOffsets("FRONT", -44.0, -20.0, 47.0, 0.0, 1.0, 1.0)
        i.Gravity(False)
        i.Set1D()

        # add files (SAMPLE and CAN) using the ISISCommandInterface
        runs_sample = ("28827", "28797")
        i.AddRuns(runs_sample, instrument="SANS2DTUBES", saveAsEvent=True)
        runs_can = ("28823", "28793")
        i.AddRuns(runs_can, instrument="SANS2DTUBES", saveAsEvent=True)

        i.AssignSample(r"SANS2D00028797-add.nxs", reload=True)
        i.AssignCan(r"SANS2D00028793-add.nxs", reload=True)
        i.TransmissionSample(r"SANS2D00028808.nxs", r"SANS2D00028784.nxs")
        i.TransmissionCan(r"SANS2D00028823.nxs", r"SANS2D00028784.nxs")

        i.WavRangeReduction()

    def validate(self):
        # we have double the sample and the can, this means that the reduced data will be
        # almost the same
        self.tolerance = 0.01
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")
        self.disableChecking.append("Instrument")
        return "28797rear_1D_1.75_16.5", "SANS2DTUBES_AddedEventFilesWithoutOverlay.nxs"

    def cleanup(self):
        # Delete all workspaces
        for ws in mtd.getObjectNames():
            DeleteWorkspace(Workspace=ws)
        # Delete the stored files
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028793-add.nxs"))
        os.remove(os.path.join(config["defaultsave.directory"], "SANS2D00028797-add.nxs"))


if __name__ == "__main__":
    test = SANS2DReductionGUIAddedFiles()
    test.execute()
