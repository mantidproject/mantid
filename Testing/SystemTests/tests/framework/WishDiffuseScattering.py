# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
Tests diffuse scattering reduction as used on WISH
If this breaks for whatever reason, there is a good chance that unregistered scripts will also be broken.
- Email Pascal Manuel @ ISIS if things break here and let him know how his scripts may need to be modified.
"""

import systemtesting
from mantid.api import mtd
from mantid.simpleapi import (
    AddSampleLog,
    BinMD,
    ConvertToDiffractionMDWorkspace,
    CropWorkspace,
    Divide,
    Load,
    LoadIsawUB,
    Minus,
    NormaliseByCurrent,
    Rebin,
    ReplaceSpecialValues,
    SetGoniometer,
    SmoothData,
    SmoothNeighbours,
)


class WishDiffuseScattering(systemtesting.MantidSystemTest):
    def requiredMemoryMB(self):
        return 2000

    def runTest(self):
        Load(Filename="Wish_Diffuse_Scattering_C.nxs", OutputWorkspace="C", LoadLogFiles="0", LoadMonitors="Exclude")
        NormaliseByCurrent(InputWorkspace="C", OutputWorkspace="C")
        CropWorkspace(InputWorkspace="C", OutputWorkspace="C", XMin="6000", XMax="99000")
        Rebin(InputWorkspace="C", OutputWorkspace="C", Params="6000,-0.004,99900")
        SmoothNeighbours(
            InputWorkspace="C", OutputWorkspace="Csn", RadiusUnits="NumberOfPixels", Radius="3", NumberOfNeighbours="25", PreserveEvents="0"
        )

        Load(Filename="Wish_Diffuse_Scattering_B.nxs", OutputWorkspace="B", LoadLogFiles="0", LoadMonitors="Exclude")
        NormaliseByCurrent(InputWorkspace="B", OutputWorkspace="B")
        CropWorkspace(InputWorkspace="B", OutputWorkspace="B", XMin="6000", XMax="99000")
        Rebin(InputWorkspace="B", OutputWorkspace="B", Params="6000,-0.004,99900")
        SmoothNeighbours(
            InputWorkspace="B", OutputWorkspace="Bsn", RadiusUnits="NumberOfPixels", Radius="3", NumberOfNeighbours="25", PreserveEvents="0"
        )

        Load(Filename="Wish_Diffuse_Scattering_A.nxs", OutputWorkspace="A", LoadLogFiles="0", LoadMonitors="Exclude")
        NormaliseByCurrent(InputWorkspace="A", OutputWorkspace="A")
        CropWorkspace(InputWorkspace="A", OutputWorkspace="A", XMin="6000", XMax="99000")
        Rebin(InputWorkspace="A", OutputWorkspace="A", Params="6000,-0.004,99900")
        SmoothNeighbours(
            InputWorkspace="A", OutputWorkspace="Asn", RadiusUnits="NumberOfPixels", Radius="3", NumberOfNeighbours="25", PreserveEvents="0"
        )
        SmoothData(InputWorkspace="Asn", OutputWorkspace="Asn-smooth", NPoints="50")

        Divide(LHSWorkspace="Csn", RHSWorkspace="Asn-smooth", OutputWorkspace="C_div_A_sn_smooth")
        ReplaceSpecialValues(
            InputWorkspace="C_div_A_sn_smooth",
            OutputWorkspace="C_div_A_sn_smooth",
            NaNValue="0",
            InfinityValue="100000",
            BigNumberThreshold="99000",
        )

        Divide(LHSWorkspace="Bsn", RHSWorkspace="Asn-smooth", OutputWorkspace="B_div_A_sn_smooth")
        ReplaceSpecialValues(
            InputWorkspace="B_div_A_sn_smooth",
            OutputWorkspace="B_div_A_sn_smooth",
            NaNValue="0",
            InfinityValue="100000",
            BigNumberThreshold="99000",
        )

        Minus(LHSWorkspace="C_div_A_sn_smooth", RHSWorkspace="B_div_A_sn_smooth", OutputWorkspace="CminusB_smooth")

        LoadIsawUB(InputWorkspace="CminusB_smooth", Filename="Wish_Diffuse_Scattering_ISAW_UB.mat")

        AddSampleLog(Workspace="CminusB_smooth", LogName="psi", LogText="0.0", LogType="Number Series")
        SetGoniometer(Workspace="CminusB_smooth", Axis0="psi,0,1,0,1")
        ConvertToDiffractionMDWorkspace(
            InputWorkspace="CminusB_smooth", OutputWorkspace="CminusB_smooth_MD_HKL", OutputDimensions="HKL", Version=2
        )

        BinMD(
            InputWorkspace="CminusB_smooth_MD_HKL",
            AlignedDim0="[H,0,0],-1.0,8.0,200",
            AlignedDim1="[0,K,0],-1.0,8.0,200",
            AlignedDim2="[0,0,L],0,1.5,200",
            OutputWorkspace="test_rebin",
        )

        # Quick sanity checks. No comparison with a saved workspace because SliceMD is too expensive compared to BinMD.
        result = mtd["test_rebin"]
        self.assertEqual(result.getNumDims(), 3)
        self.assertEqual(result.getNPoints(), 8000000)

        return True

    def doValidate(self):
        return True
