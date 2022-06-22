# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace, GroupWorkspaces

from AlgorithmDialogsStartupTestBase import AlgorithmDialogsStartupTestBase


class AlgorithmDialogsWorkspaceGroupStartupTest(AlgorithmDialogsStartupTestBase):
    """
    A system test for testing that the Algorithm Dialogs open ok with a WorkspaceGroup in the ADS.
    """

    def _setup_test(self) -> None:
        self._workspace_type = "WorkspaceGroup"
        # These algorithms currently fail to open when the given workspace type is auto-selected from the ADS
        self._exclude_algorithms = ["AnvredCorrection", "CalculateDynamicRange", "CopyDataRange", "CreateEPP",
                                    "CropToComponent", "CylinderPaalmanPingsCorrection",
                                    "EnggEstimateFocussedBackground", "FlatPlatePaalmanPingsCorrection",
                                    "GetDetectorOffsets", "HB2AReduce", "HB3APredictPeaks", "LorentzCorrection",
                                    "MaskBinsIf", "MaskNonOverlappingBins", "MSDFit", "MuonPairingAsymmetry",
                                    "PDConvertRealSpace", "RebinRagged", "SetMDFrame", "Stitch1D", "Symmetrise",
                                    "TOFTOFCropWorkspace", "VesuvioResolution", "XrayAbsorptionCorrection"]

        ws1 = CreateSampleWorkspace()
        ws2 = CreateSampleWorkspace()
        GroupWorkspaces(InputWorkspaces=[ws1, ws2], OutputWorkspace="group")
