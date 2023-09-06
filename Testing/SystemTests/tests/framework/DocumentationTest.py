# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import pathlib
from pathlib import Path

from systemtesting import MantidSystemTest
from mantid.api import AlgorithmFactory, FunctionFactory

path_to_docs = (pathlib.Path(__file__).resolve() / "../../../../../docs/source").resolve()
ALG_DOCS_PATH = path_to_docs / "algorithms"
FIT_DOCS_PATH = path_to_docs / "fitting/fitfunctions"

# This should be empty
ALGORITHMS_WITH_NO_DOCS = [
    "ApplyNegMuCorrection-v1.rst",
    "DPDFreduction-v1.rst",
    "LoadCSNSNexus-v1.rst",
    "LoadEMUHdf-v1.rst",
    "LoadILLLagrange-v1.rst",
    "LoadILLSALSA-v1.rst",
    "MagnetismReflectometryReduction-v1.rst",
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


def file_exists_under_dir(dir_path: Path, file_name: str) -> bool:
    """
    Search for file name in hte given directory and all its subdirectories
    :param dir_path: directory to search under
    :param file_name: name of the file
    :return: bool, true if file found
    """
    found_files = [path for path in dir_path.rglob(file_name) if path.is_file]
    return len(found_files) > 0


class AlgorithmDocumentationTest(MantidSystemTest):
    """
    This test checks that all registered algorithms have a documentation page.
    """

    def runTest(self):
        all_algs = AlgorithmFactory.getRegisteredAlgorithms(False)
        self.assertGreaterThan(len(all_algs), 0, msg="No registered algorithms found")
        missing = []

        for name, versions in all_algs.items():
            for version in versions:
                file_name = f"{name}-v{version}.rst"
                if not file_exists_under_dir(ALG_DOCS_PATH, file_name):
                    if file_name not in ALGORITHMS_WITH_NO_DOCS:
                        missing.append(str(file_name))
                elif file_name in ALGORITHMS_WITH_NO_DOCS:
                    raise FileExistsError(f"{file_name} exists but is still in the exceptions list. Please update the list.")

        self.assertEqual(len(missing), 0, msg="Missing the following algorithm docs:\n" + "\n".join(missing))


class FittingDocumentationTest(MantidSystemTest):
    """
    This test checks that all registered fit functions have a documentation page.
    """

    def runTest(self):
        all_fit_funcs = FunctionFactory.getFunctionNames()
        self.assertGreaterThan(len(all_fit_funcs), 0, msg="No available fit functions found")
        missing = []

        for name in all_fit_funcs:
            file_name = f"{name}.rst"
            if not file_exists_under_dir(FIT_DOCS_PATH, file_name):
                if file_name not in FIT_FUNCTIONS_WITH_NO_DOCS:
                    missing.append(str(file_name))
            elif file_name in FIT_FUNCTIONS_WITH_NO_DOCS:
                raise FileExistsError(f"{file_name} exists but is still in the exceptions list. Please update the list.")

        self.assertEqual(len(missing), 0, msg="Missing the following fit function docs:\n" + "\n".join(missing))
