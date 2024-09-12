# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
"""
System test for ARCS reduction
"""

import systemtesting
from mantid.simpleapi import *


class CompareMDWorkspacesTest(systemtesting.MantidSystemTest):
    compare_result_1 = ""
    compare_result_2 = ""
    compare_result_3 = ""

    def requiredFiles(self):
        return []

    def requiredMemoryMB(self):
        return 4000

    def cleanup(self):
        for ws_name in ["md_1", "md_2", "ev_ws_1", "ev_ws_2"]:
            if mtd.doesExist(ws_name):
                mtd.remove(ws_name)

        return True

    def runTest(self):
        # create saple workspace 1
        ev_ws_1 = CreateSampleWorkspace(
            WorkspaceType="Event",
            Function="Flat background",
            XUnit="TOF",
            XMin=-9,
            XMax=9,
            BinWidth=1,
            NumEvents=54,
            NumBanks=1,
            BankPixelWidth=1,
        )

        md_1 = ConvertToMD(
            InputWorkspace=ev_ws_1, dEAnalysisMode="Elastic", MinValues="-10", MaxValues="10", SplitInto="1", MaxRecursionDepth=1
        )

        # create saple workspace 1
        ev_ws_2 = ChangeBinOffset(InputWorkspace=ev_ws_1, Offset=0.1)

        ConvertToMD(
            InputWorkspace=ev_ws_2,
            dEAnalysisMode="Elastic",
            MinValues="-10",
            MaxValues="10",
            SplitInto="1",
            MaxRecursionDepth=1,
            OutputWorkspace="md_2",
        )

        # compare md1 and md2
        self.compare_result_1 = CompareMDWorkspaces(
            Workspace1="md_1", Workspace2="md_2", Tolerance=0.000001, CheckEvents=True, IgnoreBoxID=False
        )

        # merge some MD workspaces
        merged_1 = MergeMD(InputWorkspaces="md_1, md_2", SplitInto=1, MaxRecursionDepth=1)
        merged_2 = MergeMD(InputWorkspaces="md_2, md_1", SplitInto=1, MaxRecursionDepth=1)

        # compare
        self.compare_result_2 = CompareMDWorkspaces(Workspace1=merged_1, Workspace2=merged_2, CheckEvents=True)

        # create merge3
        merged_3 = md_1 + md_1
        # compare
        self.compare_result_3 = CompareMDWorkspaces(Workspace1=merged_1, Workspace2=merged_3, CheckEvents=True, Tolerance=1e-14)

    def validate(self):
        # compare result 1
        self.assertTrue(self.compare_result_1 is not None)
        self.assertTrue(self.compare_result_1.Equals is False)
        self.assertTrue(self.compare_result_1.Result.endswith(" 54"))

        # compare result 2
        self.assertTrue(self.compare_result_2 is not None)
        self.assertTrue(self.compare_result_2.Equals)

        # compare result 3
        self.assertTrue(self.compare_result_3 is not None)
        self.assertTrue(self.compare_result_3.Equals is False)
        self.assertTrue(self.compare_result_3.Result.endswith(" 66"))
