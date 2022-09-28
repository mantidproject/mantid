# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateMDWorkspace

from AlgorithmDialogsStartupTestBase import AlgorithmDialogsStartupTestBase


class AlgorithmDialogsMDWorkspaceStartupTest(AlgorithmDialogsStartupTestBase):
    """
    A system test for testing that the Algorithm Dialogs open ok with a MDWorkspace in the ADS.
    """

    def _setup_test(self) -> None:
        self._workspace_type = "MDWorkspace"

        CreateMDWorkspace(Dimensions="3", EventType="MDEvent", Extents='-10,10,-5,5,-1,1',
                          Names="Q_lab_x,Q_lab_y,Q_lab_z", Units="1\\A,1\\A,1\\A", OutputWorkspace="ws")
