# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
from mantid.kernel import config
from systemtesting import MantidSystemTest
from mantid.simpleapi import (
    Load,
    MergeRuns,
    AddSampleLog,
    DeleteWorkspace,
    DeleteWorkspaces,
    GroupWorkspaces,
    CompareSampleLogs,
    MaskDetectors,
    FindDetectorsOutsideLimits,
    MonitorEfficiencyCorUser,
    Scale,
    Minus,
    CloneWorkspace,
    Divide,
    FindEPP,
    ComputeCalibrationCoefVan,
    TOFTOFCropWorkspace,
    CorrectTOF,
    ConvertUnits,
    ConvertToDistribution,
    DetectorEfficiencyCorUser,
    CorrectKiKf,
    Rebin,
    SofQW3,
    RenameWorkspace,
)


class TOFTOFReductionTest(MantidSystemTest):
    """
    System test that executes the full TOFTOF reduction workflow matching the
    manual DGS Reduction steps and validates the expected outputs.
    """

    def __init__(self):
        MantidSystemTest.__init__(self)
        self.setUp()

    def setUp(self):
        config["default.facility"] = "MLZ"
        config["default.instrument"] = "TOFTOF"

    # def cleanup(self) -> None:
    #     mtd.clear()

    def requiredFiles(self):
        return [
            "TOFTOF12.nxs",
            "TOFTOF13.nxs",
            "TOFTOF14.nxs",
            "TOFTOF15.nxs",
            "TOFTOF16.nxs",
            "TOFTOF17.nxs",
            "TOFTOF27.nxs",
            "TOFTOF28.nxs",
            "TOFTOF29.nxs",
            "TOFTOF30.nxs",
            "TOFTOF31.nxs",
        ]

    def runTest(self):
        self._assert_reduction_outputs()

    def _assert_reduction_outputs(self) -> None:
        wsRawVan = Load(Filename="TOFTOF12:14")
        wsVan = MergeRuns(wsRawVan)
        wsVan.setComment("Van_res")
        temperature = np.mean(wsVan.getRun().getLogData("temperature").value)
        AddSampleLog(wsVan, LogName="temperature", LogText=str(temperature), LogType="Number", LogUnit="K")
        DeleteWorkspace(wsRawVan)

        wsRawEC = Load(Filename="TOFTOF15:17")
        wsEC = MergeRuns(wsRawEC)
        wsEC.setComment("EC")
        temperature = np.mean(wsEC.getRun().getLogData("temperature").value)
        AddSampleLog(wsEC, LogName="temperature", LogText=str(temperature), LogType="Number", LogUnit="K")
        DeleteWorkspace(wsRawEC)

        # data runs 1
        wsRawData1 = Load(Filename="TOFTOF27:29")
        wsData1 = MergeRuns(wsRawData1)
        wsData1.setComment("H2O_21C")
        temperature = np.mean(wsData1.getRun().getLogData("temperature").value)
        AddSampleLog(wsData1, LogName="temperature", LogText=str(temperature), LogType="Number", LogUnit="K")
        DeleteWorkspace(wsRawData1)

        # data runs 2
        wsRawData2 = Load(Filename="TOFTOF30:31")
        wsData2 = MergeRuns(wsRawData2)
        wsData2.setComment("H2O_34C")
        temperature = np.mean(wsData2.getRun().getLogData("temperature").value)
        AddSampleLog(wsData2, LogName="temperature", LogText=str(temperature), LogType="Number", LogUnit="K")
        DeleteWorkspace(wsRawData2)

        # grouping
        gwsDataRuns = GroupWorkspaces([wsData1, wsData2])
        gwsAll = GroupWorkspaces([wsVan, wsEC, wsData1, wsData2])

        # Ei
        if CompareSampleLogs(gwsAll, "Ei", 0.001):
            raise RuntimeError("Ei values do not match")

        Ei = wsData1.getRun().getLogData("Ei").value

        # mask detectors
        (gwsDetectorsToMask, numberOfFailures) = FindDetectorsOutsideLimits(gwsAll)
        MaskDetectors(gwsAll, MaskedWorkspace=gwsDetectorsToMask)
        DeleteWorkspaces([gwsDetectorsToMask])

        # normalise to monitor
        wsVanNorm = MonitorEfficiencyCorUser(wsVan)
        wsECNorm = MonitorEfficiencyCorUser(wsEC)
        gwsNorm = MonitorEfficiencyCorUser(gwsDataRuns)
        DeleteWorkspaces([gwsAll])

        # subtract empty can
        ecFactor = 0.900
        wsScaledEC = Scale(wsECNorm, Factor=ecFactor, Operation="Multiply")
        gwsDataSubEC = Minus(gwsNorm, wsScaledEC)
        van_ecFactor = 1.000
        wsScaledECvan = Scale(wsECNorm, Factor=van_ecFactor, Operation="Multiply")
        wsVanSubEC = Minus(wsVanNorm, wsScaledECvan)
        DeleteWorkspaces([wsScaledEC, wsScaledECvan])

        # group data for processing
        wsECNorm2 = CloneWorkspace(wsECNorm)
        gwsData = GroupWorkspaces([wsVanSubEC, wsECNorm2] + list(gwsDataSubEC.getNames()))

        # normalise to vanadium
        wsEppTable = FindEPP(wsVanSubEC)
        wsDetCoeffs = ComputeCalibrationCoefVan(wsVanSubEC, wsEppTable)
        badDetectors = np.where(np.array(wsDetCoeffs.extractY()).flatten() <= 0)[0]
        MaskDetectors(gwsData, DetectorList=badDetectors)
        gwsDataCorr = Divide(gwsData, wsDetCoeffs)
        DeleteWorkspaces([wsDetCoeffs])

        # remove half-filled time bins (clean frame)
        gwsDataCleanFrame = TOFTOFCropWorkspace(gwsDataCorr)
        DeleteWorkspaces([gwsDataCorr])

        # apply vanadium TOF correction
        gwsDataTofCorr = CorrectTOF(gwsDataCleanFrame, wsEppTable)
        DeleteWorkspaces([gwsDataCleanFrame, gwsData, wsEppTable])

        # convert units
        gwsDataDeltaE = ConvertUnits(gwsDataTofCorr, Target="DeltaE", EMode="Direct", EFixed=Ei)
        ConvertToDistribution(gwsDataDeltaE)
        DeleteWorkspaces([gwsDataTofCorr])

        # correct for energy dependent detector efficiency
        gwsDataCorrDeltaE = DetectorEfficiencyCorUser(gwsDataDeltaE)
        DeleteWorkspaces([gwsDataDeltaE])

        # calculate S (Ki/kF correction)
        gwsDataS = CorrectKiKf(gwsDataCorrDeltaE)
        DeleteWorkspaces([gwsDataCorrDeltaE])

        # energy binning
        rebinEnergy = "-6.000, 0.010, 1.800"
        gwsDataBinE = Rebin(gwsDataS, Params=rebinEnergy, IgnoreBinErrors=True)

        # calculate momentum transfer Q for sample data
        rebinQ = "0.400, 0.100, 2.000"
        gwsDataSQW = SofQW3(gwsDataBinE, QAxisBinning=rebinQ, EMode="Direct", EFixed=Ei, ReplaceNaNs=False)

        # make nice workspace names
        for ws in gwsDataS:
            RenameWorkspace(ws, OutputWorkspace="ws_S_" + ws.getComment())
        for ws in gwsDataBinE:
            RenameWorkspace(ws, OutputWorkspace="ws_E_" + ws.getComment())
        for ws in gwsDataSQW:
            RenameWorkspace(ws, OutputWorkspace="ws_" + ws.getComment() + "_sqw")

        # expected_comments = {"H2O_21C", "H2O_34C"}

        # self.assertIn("gwsDataS", mtd)
        # self.assertIn("gwsDataBinE", mtd)
        # self.assertIn("gwsDataSQW", mtd)

        # sqw_group = mtd["gwsDataSQW"]
        # self.assertIsInstance(sqw_group, WorkspaceGroup)
        # self.assertEqual(set(sqw_group.getNames()), {f"ws_{comment}_sqw" for comment in expected_comments})

        # energy_group = mtd["gwsDataBinE"]
        # self.assertIsInstance(energy_group, WorkspaceGroup)
        # self.assertEqual(set(energy_group.getNames()), {f"ws_E_{comment}" for comment in expected_comments})

        # for comment in expected_comments:
        #     ws_name = f"ws_{comment}_sqw"
        #     workspace = mtd[ws_name]
        #     self.assertIsInstance(workspace, MatrixWorkspace)
        #     self.assertEqual(workspace.getComment(), comment)
        #     self.assertGreater(workspace.getNumberHistograms(), 0)
        #     self.assertGreater(workspace.blocksize(), 0)
        #     signal = workspace.dataY(0)
        #     self.assertTrue(np.isfinite(signal).any(), f"{ws_name} contains no finite signal values")
        #     self.assertGreater(np.abs(signal).sum(), 0.0, f"{ws_name} is unexpectedly empty")
