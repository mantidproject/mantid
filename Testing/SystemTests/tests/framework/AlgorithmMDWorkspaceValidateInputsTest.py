# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateMDWorkspace

from AlgorithmValidateInputsTestBase import AlgorithmValidateInputsTestBase, INPUT_WS_NAME


class AlgorithmMDWorkspaceValidateInputsTest(AlgorithmValidateInputsTestBase):
    """
    A system test for testing that the Algorithm validateInputs method works with a MDWorkspace in the ADS.
    """

    def _setup_test(self) -> None:
        self._workspace_type = "MDWorkspace"
        self._exclude_algorithms = []

        CreateMDWorkspace(
            Dimensions="3",
            EventType="MDEvent",
            Extents="-10,10,-5,5,-1,1",
            Names="Q_lab_x,Q_lab_y,Q_lab_z",
            Units="1\\A,1\\A,1\\A",
            OutputWorkspace=INPUT_WS_NAME,
        )
