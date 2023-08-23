# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import os
import pathlib

from systemtesting import MantidSystemTest
from mantid.api import AlgorithmFactory, FunctionFactory

path_to_docs = (pathlib.Path(__file__).resolve() / "../../../../../docs/source").resolve()
ALG_DOCS_PATH = path_to_docs / "algorithms"
FIT_DOCS_PATH = path_to_docs / "fitting/fitfunctions"


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
                file_path = ALG_DOCS_PATH / file_name
                if not os.path.exists(file_path):
                    missing.append(str(file_path))

        self.assertEqual(len(missing), 0, msg="Missing documentation for the following algorithms:\n" + "\n".join(missing))


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
            file_path = FIT_DOCS_PATH / file_name
            if not os.path.exists(file_path):
                missing.append(str(file_path))

        self.assertEqual(len(missing), 0, msg="Missing documentation for the following fit functions:\n" + "\n".join(missing))
