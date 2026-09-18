# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import CalculateLogLikelihoodEvidence, Load
from mantid.api import mtd


class CalculateLogLikelihoodEvidenceSystemTest(systemtesting.MantidSystemTest):
    """Tests the CalculateLogLikelihoodEvidence algorithm"""

    def runTest(self):
        Load(Filename="PDF_Gauss_FABADA_chi2.nxs", OutputWorkspace="PDF_Gauss_FABADA_chi2")
        Load(Filename="PDF_GC4_FABADA_chi2.nxs", OutputWorkspace="PDF_GC4_FABADA_chi2")

        CalculateLogLikelihoodEvidence(
            WorkspaceList=["PDF_Gauss_FABADA_chi2", "PDF_GC4_FABADA_chi2"],
            OutputWorkspace="PDF_EvidenceWorkspace",
            OutputRelativeFactors="EvidenceRelativeFactorsWorkspace",
        )

        Load(Filename="PDF_EvidenceWorkspaceExpectedOutput.nxs", OutputWorkspace="PDF_EvidenceWorkspaceExpectedOutput")
        Load(
            Filename="PDF_Gauss_FABADA_chi2_RelativeFactorsExpectedOutput.nxs",
            OutputWorkspace="PDF_Gauss_FABADA_chi2_RelativeFactorsExpectedOutput",
        )

    def validateMethod(self):
        return "ValidateWorkspaceToWorkspace"

    def requiredFiles(self):
        return [
            "PDF_Gauss_FABADA_chi2",
            "PDF_GC4_FABADA_chi2",
            "PDF_EvidenceWorkspaceExpectedOutput",
            "PDF_Gauss_FABADA_chi2_RelativeFactorsExpectedOutput",
        ]

    def validate(self):
        self.checkInstrument = False
        self.nanEqual = True
        return (
            "PDF_EvidenceWorkspace",
            "PDF_EvidenceWorkspaceExpectedOutput",
            "PDF_Gauss_FABADA_chi2_RelativeFactors",
            "PDF_Gauss_FABADA_chi2_RelativeFactorsExpectedOutput",
        )

    def cleanup(self):
        mtd.clear()
