# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateEmptyTableWorkspace

from AlgorithmDialogsStartupTestBase import AlgorithmDialogsStartupTestBase


class AlgorithmDialogsTableWorkspaceStartupTest(AlgorithmDialogsStartupTestBase):
    """
    A system test for testing that the Algorithm Dialogs open ok with a TableWorkspace in the ADS.
    """

    def _setup_test(self) -> None:
        self._workspace_type = "TableWorkspace"

        CreateEmptyTableWorkspace(OutputWorkspace="ws")
