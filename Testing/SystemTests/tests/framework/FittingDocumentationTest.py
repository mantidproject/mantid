# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from DocumentationTest import PATH_TO_DOCS, DocumentationTestBase
from mantid.api import FunctionFactory

FIT_DOCS_PATH = PATH_TO_DOCS / "fitting/fitfunctions"

# This should be empty
FIT_FUNCTIONS_WITH_NO_DOCS = [
    "ConvTempCorrection.rst",
    "Example1DFunction.rst",
    "ExamplePeakFunction.rst",
    "PawleyParameterFunction.rst",
    "PeakParameterFunction.rst",
    "PoldiSpectrumConstantBackground.rst",
    "PoldiSpectrumDomainFunction.rst",
    "PoldiSpectrumLinearBackground.rst",
    "PoldiSpectrumPawleyFunction.rst",
    "ReflectivityMulf.rst",
    "SCDCalibratePanels2ObjFunc.rst",
    "SCgapSwave.rst",
    "VesuvioResolution.rst",
]


class FittingDocumentationTest(DocumentationTestBase):
    """
    This test checks that all registered fit functions have a documentation page.
    """

    def _setup_test(self):
        self._known_no_docs = FIT_FUNCTIONS_WITH_NO_DOCS
        self._docs_path = FIT_DOCS_PATH
        self._files = [[f"{name}.rst"] for name in FunctionFactory.getFunctionNames()]
        self._test_type = "Fitting Function"
