# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import *
from mantid import AnalysisDataService

from quickBayesHelper import QuickBayesTemplate


SAMPLE_NAME = "__BayesStretchTest_Sample"
RES_NAME = "__BayesStretchTest_Resolution"


class QuickBayesHelperTest(unittest.TestCase):
    """
    These tests are for checking the helper class
    for quickBayes. This algorithm is not registered
    to Mantid.

    Going to test each method in isolation.
    """

    @classmethod
    def setUpClass(cls) -> None:
        cls._res_ws = Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace=RES_NAME)
        cls._sample_ws = Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace=SAMPLE_NAME)
        cls._alg = QuickBayesTemplate()
        cls._alg.initialize()

    @classmethod
    def tearDownClass(cls):
        """
        Remove workspaces from ADS.
        """
        AnalysisDataService.clear()

    # ----------------------------------Algorithm tests----------------------------------------

    def test_point_data(self):
        self.assertEqual(len(self._sample_ws.readY(0)) + 1, len(self._sample_ws.readX(0)))
        ws, _ = self._alg.point_data(self._sample_ws.name())
        self.assertEqual(len(ws.readY(0)), len(ws.readX(0)))

    def test_group_ws(self):
        ws_list = [self._sample_ws, self._res_ws]
        name = self._alg.group_ws(ws_list, "test")
        self.assertEqual(name, "test")
        group = AnalysisDataService.retrieve(name)
        group_names = group.getNames()
        self.assertEqual(len(group_names), 2)
        self.assertTrue(SAMPLE_NAME in group_names)
        self.assertTrue(RES_NAME in group_names)

    def test_add_sample_logs(self):
        # know that a new workspace has no sample logs
        ws = CreateWorkspace([1, 2], [3, 4])
        self._alg.add_sample_logs(ws, [("unit", "test")], self._sample_ws)
        run = ws.getRun()
        self.assertEqual(run.getLogData("unit").value, "test")
        self.assertEqual(run.getLogData("dur").value, 56200)

    def test_create_ws(self):
        name = self._alg.create_ws("test", [1, 2, 3, 4], [4, 5, 6, 7], 2, "energy", "TOF", "Text", ["unit", "python"])
        ws = AnalysisDataService.retrieve(name)
        self.assertEqual(ws.name(), "test")
        self.assertEqual(ws.getNumberHistograms(), 2)
        self.assertListEqual(list(ws.readX(0)), [1, 2])
        self.assertListEqual(list(ws.readX(1)), [3, 4])
        self.assertListEqual(list(ws.readY(0)), [4, 5])
        self.assertListEqual(list(ws.readY(1)), [6, 7])

        ax = ws.getAxis(0)
        self.assertEqual(ax.getUnit().caption(), "Energy")
        self.assertEqual(str(ax.getUnit().symbol()), "meV")
        self.assertEqual(ws.YUnitLabel(), "TOF")

        ax = ws.getAxis(1)
        self.assertEqual(ax.label(0), "unit")
        self.assertEqual(ax.label(1), "python")

    def test_create_ws_with_errors(self):
        name = self._alg.create_ws(
            "test", [1, 2, 3, 4], [4, 5, 6, 7], 2, "energy", "TOF", "Text", ["unit", "python"], DataE=[0.1, 0.2, 0.3, 0.4]
        )
        ws = AnalysisDataService.retrieve(name)
        self.assertListEqual(list(ws.readE(0)), [0.1, 0.2])
        self.assertListEqual(list(ws.readE(1)), [0.3, 0.4])

    def test_duplicate_res(self):
        N = 3
        ws = CreateWorkspace([1, 2], [3, 4])
        ws_list = self._alg.duplicate_res(ws, N)
        self.assertEqual(len(ws_list), N)
        for j in range(N):
            data = ws_list[j]
            self.assertListEqual(list(ws.readX(0)), list(data["x"]))
            self.assertListEqual(list(ws.readY(0)), list(data["y"]))

    def test_unique_res(self):
        N = 2
        ws = CreateWorkspace([1, 2, -1, -2], [3, 4, -3, -4], NSpec=N)
        ws_list = self._alg.unique_res(ws, N)
        self.assertEqual(len(ws_list), N)
        for j in range(N):
            data = ws_list[j]
            self.assertListEqual(list(ws.readX(j)), list(data["x"]))
            self.assertListEqual(list(ws.readY(j)), list(data["y"]))

    # -------------------------------- Failure cases ------------------------------------------
    def test_start_greater_end(self):
        self._alg.setProperty("EMin", 10)
        self._alg.setProperty("EMax", 0.2)
        self._alg.setProperty("SampleWorkspace", self._sample_ws)
        issues = self._alg.validateInputs()
        keys = list(issues.keys())
        self.assertEqual(len(keys), 1)
        self.assertEqual(keys[0], "EMax")

    def test_start_outside_data_range(self):
        self._alg.setProperty("EMin", -100)
        self._alg.setProperty("EMax", 0.2)
        self._alg.setProperty("SampleWorkspace", self._sample_ws)
        issues = self._alg.validateInputs()

        keys = list(issues.keys())
        self.assertEqual(len(keys), 1)
        self.assertEqual(keys[0], "EMin")

    def test_end_outside_data_range(self):
        self._alg.setProperty("EMin", -0.2)
        self._alg.setProperty("EMax", 200.0)
        self._alg.setProperty("SampleWorkspace", self._sample_ws)
        issues = self._alg.validateInputs()

        keys = list(issues.keys())
        self.assertEqual(len(keys), 1)
        self.assertEqual(keys[0], "EMax")


if __name__ == "__main__":
    unittest.main()
