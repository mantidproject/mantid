# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,attribute-defined-outside-init
import systemtesting
from mantid.simpleapi import CalMuonDeadTime, GroupWorkspaces, Load


class CalMuonDeadTimeTest(systemtesting.MantidSystemTest):
    """Tests the CalMuonDeadTime algorithm"""

    def runTest(self):
        Load(Filename="EMU30604.nxs", OutputWorkspace="EMU30604")
        CalMuonDeadTime(InputWorkspace="EMU30604", DeadTimeTable="deadTable", FirstGoodData=0.5, LastGoodData=10, DataFitted="fitTable")
        GroupWorkspaces(InputWorkspaces="deadTable,fitTable", OutputWorkspace="EMUCalMuonDeadTime")

    def validate(self):
        self.tolerance = 1e-3
        return ("EMUCalMuonDeadTime", "EMUCalMuonDeadTime.nxs")
