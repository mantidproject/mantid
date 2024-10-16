# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import mtd
from mantid.simpleapi import CreateSampleWorkspace
from testhelpers import assertRaisesNothing, create_algorithm


class ReflectometryReductionOne2Test(unittest.TestCase):
    def tearDown(self):
        mtd.clear()

    def test_execution_AveragePixelFit(self):
        """
        Check that the algorithm executes using background method AveragePixelFit
        """
        self._create_workspace_with_pixel_background("workspace_with_peak")
        args = {
            "InputWorkspace": "workspace_with_peak",
            "ThetaIn": 0.5,
            "I0MonitorIndex": 1,
            "WavelengthMin": 0,
            "WavelengthMax": 5,
            "ProcessingInstructions": "2",
            "SubtractBackground": True,
            "BackgroundProcessingInstructions": "1-3",
            "BackgroundCalculationMethod": "AveragePixelFit",
            "OutputWorkspace": "output",
        }
        output = self._assert_run_algorithm_succeeds(args)
        history = [
            "ExtractSpectra",
            "ReflectometryBackgroundSubtraction",
            "GroupDetectors",
            "ConvertUnits",
            "CropWorkspace",
            "ConvertUnits",
        ]
        self._check_history(output, history)
        self._check_history_algorithm_properties(
            output, 1, 1, {"ProcessingInstructions": "1-3", "BackgroundCalculationMethod": "AveragePixelFit", "PeakRange": "2"}
        )

    def _assert_run_algorithm_succeeds(self, args):
        """Run the algorithm with the given args and check it succeeds"""
        alg = create_algorithm("ReflectometryReductionOne", **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(mtd.doesExist("output"))
        return mtd["output"]

    def _create_workspace_with_pixel_background(self, name):
        # Create a workspace with a background of 2 in the pixels adjacent to
        # a peak of 5 in the 2nd index
        empty = [0, 0, 0, 0]
        background = [2, 2, 2, 2]
        peak = [5, 5, 5, 5]
        nspec = 5
        ws = CreateSampleWorkspace(
            NumBanks=nspec,
            BankPixelWidth=1,
            XMin=1,
            XMax=5,
            BinWidth=1,
            XUnit="TOF",
            WorkspaceType="Histogram",
            NumMonitors=0,
            OutputWorkspace=name,
        )
        ws.setY(0, empty)
        ws.setY(1, background)
        ws.setY(2, peak)
        ws.setY(3, background)
        ws.setY(4, empty)

    def _check_history(self, ws, expected, unroll=True):
        """Return true if algorithm names listed in algorithmNames are found in the
        workspace's history. If unroll is true, checks the child histories, otherwise
        checks the top level history (the latter is required for sliced workspaces where
        the child workspaces have lost their parent's history)
        """
        history = ws.getHistory()
        if unroll:
            reductionHistory = history.getAlgorithmHistory(history.size() - 1)
            algHistories = reductionHistory.getChildHistories()
            algNames = [alg.name() for alg in algHistories]
        else:
            algNames = [alg.name() for alg in history]
        self.assertEqual(algNames, expected)

    def _check_history_algorithm_properties(self, ws, toplevel_idx, child_idx, property_values):
        parent_hist = ws.getHistory().getAlgorithmHistory(toplevel_idx)
        child_hist = parent_hist.getChildHistories()[child_idx]
        for prop, val in property_values.items():
            self.assertEqual(child_hist.getPropertyValue(prop), val)


if __name__ == "__main__":
    unittest.main()
