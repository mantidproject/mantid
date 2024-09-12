# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
# from mantid.api import AlgorithmFactory
# from mantid.simpleapi import PythonAlgorithm, WorkspaceProperty
# from mantid.kernel import Direction
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import csv
import os
from string import ascii_letters, digits  # pylint: disable=deprecated-module

######################################################################
# Remove artifacts such as prompt pulse
######################################################################


def RemoveArtifact(WS, Xmin, Xmax, Xa, Delta):
    CropWorkspace(InputWorkspace=WS, OutputWorkspace="__aux0", XMin=str(Xmin), XMax=str(Xa))
    CropWorkspace(InputWorkspace=WS, OutputWorkspace="__aux3", XMin=str(Xa + Delta), XMax=str(Xmax))
    CropWorkspace(InputWorkspace=WS, OutputWorkspace="__aux1", XMin=str(Xa - Delta), XMax=str(Xa))
    CropWorkspace(InputWorkspace=WS, OutputWorkspace="__aux2", XMin=str(Xa + Delta), XMax=str(Xa + 2 * Delta))

    ScaleX(InputWorkspace="__aux1", OutputWorkspace="__aux1", Factor=str(Delta), Operation="Add")
    ScaleX(InputWorkspace="__aux2", OutputWorkspace="__aux2", Factor=str(-Delta), Operation="Add")
    Scale(InputWorkspace="__aux1", OutputWorkspace="__aux1", Factor="0.5", Operation="Multiply")
    Scale(InputWorkspace="__aux2", OutputWorkspace="__aux2", Factor="0.5", Operation="Multiply")

    Plus(LHSWorkspace="__aux0", RHSWorkspace="__aux1", OutputWorkspace=WS)
    Plus(LHSWorkspace=WS, RHSWorkspace="__aux2", OutputWorkspace=WS)
    Plus(LHSWorkspace=WS, RHSWorkspace="__aux3", OutputWorkspace=WS)


class VisionReduction(PythonAlgorithm):
    __CalFile = "/SNS/VIS/shared/autoreduce/VIS_CalTab-03-03-2014.csv"
    __MonFile = "/SNS/VIS/shared/autoreduce/VIS_5447-5450_MonitorL-corrected-hist.nxs"
    # Pixels to be reduced
    ListPX = []
    ListPXF = []
    ListPXB = []
    # Binning parameters
    # binT='10,1,33333'
    binL = "0.281,0.0002,8.199"
    binE = "-2,0.005,5,-0.001,1000"

    def FormatFilename(self, s):
        valid_chars = "-_.() %s%s" % (ascii_letters, digits)
        outfilename = "".join(c for c in s if c in valid_chars)
        outfilename = outfilename.replace(" ", "_")
        return outfilename

    def category(self):
        return "Workflow\\Inelastic;Utility\\Development"

    def name(self):
        return "VisionReduction"

    def summary(self):
        return "This algorithm reduces the inelastic detectors on VISION. ** Under Development **"

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=[".nxs.h5"]))
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output))

    # pylint: disable=too-many-locals
    def PyExec(self):
        NexusFile = self.getProperty("Filename").value

        FileName = NexusFile.split(os.sep)[-1]
        # IPTS = NexusFile.split(os.sep)[-3]
        RunNumber = int(FileName.strip("VIS_").replace(".nxs.h5", ""))

        # *********************************************************************

        # *********************************************************************

        # *********************************************************************
        # Banks to be reduced
        BanksForward = [2, 3, 4, 5, 6]
        BanksBackward = [8, 9, 10, 11, 12, 13, 14]
        Banks = BanksForward + BanksBackward
        # *********************************************************************

        # *********************************************************************

        PXs = (
            list(range(2 * 128 + 48, 2 * 128 + 80))
            + list(range(3 * 128 + 32, 3 * 128 + 96))
            + list(range(4 * 128 + 32, 4 * 128 + 96))
            + list(range(5 * 128 + 48, 5 * 128 + 80))
        )
        for i in BanksForward:
            offset = (i - 1) * 1024
            self.ListPX = self.ListPX + [j + offset for j in PXs]
            self.ListPXF = self.ListPXF + [j + offset for j in PXs]
        for i in BanksBackward:
            offset = (i - 1) * 1024
            self.ListPX = self.ListPX + [j + offset for j in PXs]
            self.ListPXB = self.ListPXB + [j + offset for j in PXs]

        # Create a list of pixels to mask
        # Inelastic Pixels = 0-14335
        allPixels = set(range(14336))
        toKeep = set(self.ListPX)
        mask = allPixels.difference(toKeep)
        MaskPX = list(mask)

        # Read calibration table
        CalTab = [[0 for _ in range(2)] for _ in range(1024 * 14)]
        tab = list(csv.reader(open(self.__CalFile, "r")))
        for i in range(0, len(tab)):
            for j in [0, 1]:
                tab[i][j] = int(tab[i][j])
            for j in [2, 3]:
                tab[i][j] = float(tab[i][j])
            j = (tab[i][0] - 1) * 1024 + tab[i][1]
            CalTab[j][0] = tab[i][2]
            CalTab[j][1] = tab[i][3]

        logger.information("Loading inelastic banks from {}".format(NexusFile))
        bank_list = ["bank%d" % i for i in range(1, 15)]
        bank_property = ",".join(bank_list)
        LoadEventNexus(Filename=NexusFile, BankName=bank_property, OutputWorkspace="__IED_T", LoadMonitors="0")
        LoadInstrument(Workspace="__IED_T", Filename="/SNS/VIS/shared/autoreduce/VISION_Definition_no_efixed.xml", RewriteSpectraMap=True)
        MaskDetectors(Workspace="__IED_T", DetectorList=MaskPX)

        logger.information("Title: {}".format(mtd["__IED_T"].getTitle()))
        logger.information("Proton charge: {}".format(mtd["__IED_T"].getRun().getProtonCharge()))
        if "Temperature" in mtd["__IED_T"].getTitle():
            logger.error("Error: Non-equilibrium runs will not be reduced")
            # sys.exit()
        if mtd["__IED_T"].getRun().getProtonCharge() < 5.0:
            logger.error("Error: Proton charge is too low")
            # sys.exit()

        NormaliseByCurrent(InputWorkspace="__IED_T", OutputWorkspace="__IED_T")
        RemoveArtifact("__IED_T", 10, 33333, 16660, 240)

        LoadNexusProcessed(Filename=self.__MonFile, OutputWorkspace="__DBM_L", LoadHistory=False)

        for i, Pixel in enumerate(self.ListPX):
            Ef = CalTab[Pixel][0]
            Df = CalTab[Pixel][1]
            Efe = (0.7317 / Df) ** 2 * Ef
            mtd["__IED_T"].setEFixed(Pixel, Efe)
        ConvertUnits(InputWorkspace="__IED_T", OutputWorkspace="__IED_L", EMode="Indirect", Target="Wavelength")
        Rebin(InputWorkspace="__IED_L", OutputWorkspace="__IED_L", Params=self.binL, PreserveEvents="0")
        InterpolatingRebin(InputWorkspace="__DBM_L", OutputWorkspace="__DBM_L", Params=self.binL)
        # RebinToWorkspace(WorkspaceToRebin='__DBM_L',WorkspaceToMatch='__IED_L',OutputWorkspace='__DBM_L')
        Divide(LHSWorkspace="__IED_L", RHSWorkspace="__DBM_L", OutputWorkspace="__IED_L")
        for i, Pixel in enumerate(self.ListPX):
            Ef = CalTab[Pixel][0]
            mtd["__IED_L"].setEFixed(Pixel, Ef)
        ConvertUnits(InputWorkspace="__IED_L", OutputWorkspace="__IED_E", EMode="Indirect", Target="DeltaE")
        Rebin(InputWorkspace="__IED_E", OutputWorkspace="__IED_E", Params=self.binE, PreserveEvents="0", IgnoreBinErrors=True)
        CorrectKiKf(InputWorkspace="__IED_E", OutputWorkspace="__IED_E", EMode="Indirect")

        GroupDetectors(InputWorkspace="__IED_E", OutputWorkspace="__IED_E_Forward", DetectorList=self.ListPXF)
        GroupDetectors(InputWorkspace="__IED_E", OutputWorkspace="__IED_E_Backward", DetectorList=self.ListPXB)
        GroupDetectors(InputWorkspace="__IED_E", OutputWorkspace="__IED_E_Average", DetectorList=self.ListPX)

        Scale(
            InputWorkspace="__IED_E_Forward", OutputWorkspace="__IED_E_Forward", Factor=str(1.0 / len(BanksForward)), Operation="Multiply"
        )
        Scale(
            InputWorkspace="__IED_E_Backward",
            OutputWorkspace="__IED_E_Backward",
            Factor=str(1.0 / len(BanksBackward)),
            Operation="Multiply",
        )
        Scale(InputWorkspace="__IED_E_Average", OutputWorkspace="__IED_E_Average", Factor=str(1.0 / len(Banks)), Operation="Multiply")

        AppendSpectra(InputWorkspace1="__IED_E_Backward", InputWorkspace2="__IED_E_Forward", OutputWorkspace="__IED_reduced")
        AppendSpectra(InputWorkspace1="__IED_reduced", InputWorkspace2="__IED_E_Average", OutputWorkspace="__IED_reduced")

        Title = mtd["__IED_reduced"].getTitle()
        Note = Title.split(">")[0]
        Note = self.FormatFilename(Note)
        INS = str(RunNumber) + "_" + Note

        ws = Scale(InputWorkspace="__IED_reduced", OutputWorkspace=INS, Factor="500", Operation="Multiply")
        mtd[INS].setYUnitLabel("Normalized intensity")

        RemoveLogs(INS)
        RemoveWorkspaceHistory(INS)

        self.setProperty("OutputWorkspace", ws)
        DeleteWorkspace(INS)


# Register
AlgorithmFactory.subscribe(VisionReduction)
