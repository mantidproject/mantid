# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from DocumentationTest import PATH_TO_DOCS, DocumentationTestBase
from mantid.api import AlgorithmFactory

ALG_DOCS_PATH = PATH_TO_DOCS / "algorithms"

# This should be empty
ALGORITHMS_WITH_NO_DOCS = [
    "ApplyNegMuCorrection-v1.rst",
    "DPDFreduction-v1.rst",
    "LoadCSNSNexus-v1.rst",
    "LoadEMUHdf-v1.rst",
    "LoadILLLagrange-v1.rst",
    "LoadILLSALSA-v1.rst",
    "OrderWorkspaceHistory-v1.rst",
    "PatchBBY-v1.rst",
    "RefinePowderInstrumentParameters-v2.rst",
    "ReflectometryISISPreprocess-v1.rst",
    "ReflectometryISISSumBanks-v1.rst",
    "SANSBeamCentreFinderCore-v1.rst",
    "SANSBeamCentreFinderMassMethod-v1.rst",
    "SANSBeamCentreFinder-v1.rst",
    "SANSPatchSensitivity-v1.rst",
    "SANSReductionCoreEventSlice-v1.rst",
    "SANSReductionCorePreprocess-v1.rst",
    "SANSReductionCore-v1.rst",
    "SANSSingleReduction-v1.rst",
    "SANSSingleReduction-v2.rst",
]


class AlgorithmDocumentationTest(DocumentationTestBase):
    """
    This test checks that all registered algorithms have a documentation page
    """

    def _setup_test(self):
        self._known_no_docs = ALGORITHMS_WITH_NO_DOCS
        self._docs_path = ALG_DOCS_PATH
        self._files = [
            [f"{name}-v{version}.rst"] for name, versions in AlgorithmFactory.getRegisteredAlgorithms(False).items() for version in versions
        ]
        self._test_type = "Algorithm"
