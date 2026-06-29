# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import CompareMDWorkspaces, HB3AAdjustSampleNorm, DeleteWorkspaces, mtd


class SingleFileOutputNormalizationWorkspace(systemtesting.MantidSystemTest):
    """Verify that OutputNormalizationWorkspace is produced for a single input file with
    NormalizeData=False. The normalization workspace must be a 3-D MDEvent workspace with
    at least as many events as the data workspace, since it covers all detector pixels
    (via vanadium replication) while the data workspace contains only pixels with counts."""

    def requiredFiles(self):
        return ["HB3A_exp0724_scan0183.nxs", "HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        ws_name = "SingleFileOutputNormalizationWorkspace"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0724_scan0183.nxs",
            VanadiumFile="HB3A_exp0722_scan0220.nxs",
            NormaliseBy="None",
            NormalizeData=False,
            OutputType="Q-sample events",
            OutputWorkspace=ws_name + "_data",
            OutputNormalizationWorkspace=ws_name + "_norm",
        )

        data = mtd[ws_name + "_data"]
        norm = mtd[ws_name + "_norm"]

        self.assertEqual(data.getNumDims(), 3)
        self.assertEqual(norm.getNumDims(), 3)
        self.assertGreaterThan(data.getNEvents(), 0)
        self.assertGreaterThan(norm.getNEvents(), data.getNEvents())

        DeleteWorkspaces([ws_name + "_data", ws_name + "_norm"])


class MultiFileOutputNormalizationWorkspace(systemtesting.MantidSystemTest):
    """Verify that OutputNormalizationWorkspace is produced when merging two input files
    with NormalizeData=False and MergeInputs=True. Both the merged data and normalization
    workspaces must be 3-D MDEvent workspaces. The normalization workspace must have at
    least as many events as the data workspace."""

    def requiredFiles(self):
        return ["HB3A_exp0724_scan0182.nxs", "HB3A_exp0724_scan0183.nxs", "HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        ws_name = "MultiFileOutputNormalizationWorkspace"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0724_scan0182.nxs, HB3A_exp0724_scan0183.nxs",
            VanadiumFile="HB3A_exp0722_scan0220.nxs",
            NormaliseBy="None",
            NormalizeData=False,
            OutputType="Q-sample events",
            MergeInputs=True,
            OutputWorkspace=ws_name + "_data",
            OutputNormalizationWorkspace=ws_name + "_norm",
        )

        data = mtd[ws_name + "_data"]
        norm = mtd[ws_name + "_norm"]

        self.assertEqual(data.getNumDims(), 3)
        self.assertEqual(norm.getNumDims(), 3)
        self.assertGreaterThan(data.getNEvents(), 0)
        self.assertGreaterThan(norm.getNEvents(), data.getNEvents())

        DeleteWorkspaces([ws_name + "_data", ws_name + "_norm"])


class SingleScanDataAsVanadiumOutputNormalizationWorkspace(systemtesting.MantidSystemTest):
    """Verify that OutputNormalizationWorkspace is produced when the input data contains only one scan.
    In this test, we reuse the same input file as data and vanadium. With `NormaliseBy="None"`,
    it is expected that the output "Q-sample events" workspaces for the data and the normalization are equal.
    """

    def requiredFiles(self):
        return ["HB3A_exp0722_scan0220.nxs"]

    def runTest(self):
        ws_name = "SingleScanDataAsVanadiumOutputNormalizationWorkspace"
        HB3AAdjustSampleNorm(
            Filename="HB3A_exp0722_scan0220.nxs",
            VanadiumFile="HB3A_exp0722_scan0220.nxs",
            NormaliseBy="None",
            NormalizeData=False,
            OutputType="Q-sample events",
            MergeInputs=False,
            OutputWorkspace=ws_name + "_data",
            OutputNormalizationWorkspace=ws_name + "_norm",
        )

        comparison = CompareMDWorkspaces(Workspace1=ws_name + "_data", Workspace2=ws_name + "_norm", CheckEvents=True, IgnoreBoxID=True)
        self.assertEqual(comparison[0], True, comparison[1])

        DeleteWorkspaces([ws_name + "_data", ws_name + "_norm"])
