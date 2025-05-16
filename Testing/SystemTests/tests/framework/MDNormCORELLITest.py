# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init

"""
System test for MDNorm
"""

from mantid.simpleapi import (
    ConvertToMD,
    ConvertToMDMinMaxGlobal,
    ConvertUnits,
    CropWorkspaceForMDNorm,
    DeleteWorkspace,
    Load,
    LoadIsawUB,
    MaskDetectors,
    MDNorm,
    RecalculateTrajectoriesExtents,
    SetGoniometer,
)
import systemtesting


class MDNormCORELLITest(systemtesting.MantidSystemTest):
    def requiredFiles(self):
        return [
            "CORELLI_29782.nxs",
            "CORELLI_29792.nxs",
            "SingleCrystalDiffuseReduction_SA.nxs",
            "SingleCrystalDiffuseReduction_Flux.nxs",
            "SingleCrystalDiffuseReduction_UB.mat",
        ]

    def runTest(self):
        Load(Filename="CORELLI_29782.nxs", OutputWorkspace="data")
        Load(Filename="SingleCrystalDiffuseReduction_SA.nxs", OutputWorkspace="SolidAngle")
        Load(Filename="SingleCrystalDiffuseReduction_Flux.nxs", OutputWorkspace="Flux")
        MaskDetectors(Workspace="data", MaskedWorkspace="SolidAngle")
        ConvertUnits(InputWorkspace="data", OutputWorkspace="data", Target="Momentum")
        CropWorkspaceForMDNorm(InputWorkspace="data", XMin=2.5, XMax=10, OutputWorkspace="data")
        LoadIsawUB(InputWorkspace="data", Filename="SingleCrystalDiffuseReduction_UB.mat")
        SetGoniometer(Workspace="data", Axis0="BL9:Mot:Sample:Axis1,0,1,0,1")
        min_vals, max_vals = ConvertToMDMinMaxGlobal(InputWorkspace="data", QDimensions="Q3D", dEAnalysisMode="Elastic", Q3DFrames="Q")
        ConvertToMD(
            InputWorkspace="data",
            QDimensions="Q3D",
            dEAnalysisMode="Elastic",
            Q3DFrames="Q_sample",
            OutputWorkspace="md",
            MinValues=min_vals,
            MaxValues=max_vals,
        )
        RecalculateTrajectoriesExtents(InputWorkspace="md", OutputWorkspace="md")
        DeleteWorkspace("data")

        MDNorm(
            InputWorkspace="md",
            SolidAngleWorkspace="SolidAngle",
            FluxWorkspace="Flux",
            QDimension0="1,1,0",
            QDimension1="1,-1,0",
            QDimension2="0,0,1",
            Dimension0Name="QDimension0",
            Dimension0Binning="-10.0,0.1,10.0",
            Dimension1Name="QDimension1",
            Dimension1Binning="-10.0,0.1,10.0",
            Dimension2Name="QDimension2",
            Dimension2Binning="-0.1,0.1",
            SymmetryOperations="P 31 2 1",
            OutputWorkspace="result",
            OutputDataWorkspace="dataMD",
            OutputNormalizationWorkspace="normMD",
        )

        # Check that we test these problematic cases
        self.assertRaises(
            ValueError,
            MDNorm,
            InputWorkspace="md",
            SolidAngleWorkspace="SolidAngle",
            FluxWorkspace="Flux",
            QDimension0="1,1,0",
            QDimension1="1,-1,0",
            QDimension2="0,0,1",
            Dimension0Name="QDimension1",
            Dimension0Binning="-10.0,0.1,10.0",
            Dimension1Name="QDimension0",
            Dimension1Binning="-10.0,0.1,10.0",
            Dimension2Name="QDimension2",
            Dimension2Binning="-0.1,0.1",
            SymmetryOperations="P 31 2 1",
            TemporaryDataWorkspace="dataMD",
            TemporaryNormalizationWorkspace="normMD",
            OutputWorkspace="result",
            OutputDataWorkspace="dataMD",
            OutputNormalizationWorkspace="normMD",
        )
        self.assertRaises(
            ValueError,
            MDNorm,
            InputWorkspace="md",
            SolidAngleWorkspace="SolidAngle",
            FluxWorkspace="Flux",
            QDimension0="1,1,0",
            QDimension1="1,-1,0",
            QDimension2="0,0,1",
            Dimension0Name="QDimension0",
            Dimension0Binning="-10.0,0.15,10.0",
            Dimension1Name="QDimension1",
            Dimension1Binning="-10.0,0.1,10.0",
            Dimension2Name="QDimension2",
            Dimension2Binning="-0.1,0.1",
            SymmetryOperations="P 31 2 1",
            TemporaryDataWorkspace="dataMD",
            TemporaryNormalizationWorkspace="normMD",
            OutputWorkspace="result",
            OutputDataWorkspace="dataMD",
            OutputNormalizationWorkspace="normMD",
        )
        self.assertRaises(
            RuntimeError,
            MDNorm,
            InputWorkspace="md",
            SolidAngleWorkspace="SolidAngle",
            FluxWorkspace="Flux",
            QDimension0="1,1,0",
            QDimension1="1,1,0",
            QDimension2="0,0,1",
            Dimension0Name="QDimension0",
            Dimension0Binning="-10.0,0.1,10.0",
            Dimension1Name="QDimension1",
            Dimension1Binning="-10.0,0.1,10.0",
            Dimension2Name="QDimension2",
            Dimension2Binning="-0.1,0.1",
            SymmetryOperations="P 31 2 1",
            TemporaryDataWorkspace="dataMD",
            TemporaryNormalizationWorkspace="normMD",
            OutputWorkspace="result",
            OutputDataWorkspace="dataMD",
            OutputNormalizationWorkspace="normMD",
        )
        self.assertRaises(
            ValueError,
            MDNorm,
            InputWorkspace="md",
            SolidAngleWorkspace="SolidAngle",
            FluxWorkspace="Flux",
            RLU=False,
            QDimension0="1,1,0",
            QDimension1="1,-1,0",
            QDimension2="0,0,1",
            Dimension0Name="QDimension0",
            Dimension0Binning="-10.0,0.1,10.0",
            Dimension1Name="QDimension1",
            Dimension1Binning="-10.0,0.1,10.0",
            Dimension2Name="QDimension2",
            Dimension2Binning="-0.1,0.1",
            SymmetryOperations="P 31 2 1",
            TemporaryDataWorkspace="dataMD",
            TemporaryNormalizationWorkspace="normMD",
            OutputWorkspace="result",
            OutputDataWorkspace="dataMD",
            OutputNormalizationWorkspace="normMD",
        )
        DeleteWorkspace("md")

    def validate(self):
        self.tolerance = 1e-7
        return "result", "MDNormCORELLI.nxs"
