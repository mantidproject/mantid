# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateSampleWorkspace

from AlgorithmValidateInputsTestBase import AlgorithmValidateInputsTestBase, INPUT_WS_NAME


class AlgorithmMatrixWorkspaceValidateInputsTest(AlgorithmValidateInputsTestBase):
    """
    A system test for testing that the Algorithm validateInputs method works with a MatrixWorkspace in the ADS.
    """

    def _setup_test(self) -> None:
        self._workspace_type = "MatrixWorkspace"
        self._exclude_algorithms = []

        CreateSampleWorkspace(OutputWorkspace=INPUT_WS_NAME)
