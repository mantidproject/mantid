import unittest

from Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils

import mantid.simpleapi as mantid

from six import iteritems


class LoadUtilsTest(unittest.TestCase):
    def setUp(self):
        self.test_path = r"test\path\to\ral012345.rooth2020.dat"
        self.bad_path = r"test\path\to\ral012345.rooth2042"
        self.test_ws_name = "1_Delayed_rooth2020"
        self.var_ws_name = "{}_Total_rooth2020"
        self.test_ws_names = [self.var_ws_name.format(x) for x in range(1, 9)]
        self.test_workspaces = [mantid.CreateSampleWorkspace(
            OutputWorkspace=s) for s in self.test_ws_names]
        self.test_run = 5

    def test_pad_run(self):
        tests = {123: "00123", 0: "00000", 12345: "12345", 123456: "123456"}
        for i, s in iteritems(tests):
            assert lutils.pad_run(i) == s

    def test_get_detector_num_from_ws(self):
        assert lutils.get_detector_num_from_ws(self.test_ws_name) == 1

    def test_get_detectors_num(self):
        assert lutils.get_detectors_num(self.test_path) == 1

    def test_get_end_num(self):
        assert lutils.get_end_num(self.test_path) == "rooth2020"

    def test_get_run_type(self):
        assert lutils.get_run_type(self.test_path) == "Delayed"
        with self.assertRaises(KeyError):
            lutils.get_run_type(self.bad_path)

    def test_get_filename(self):
        assert lutils.get_filename(self.test_path) == self.test_ws_name

    def test_hyphenise(self):
        tests = {"1-5": [1, 2, 3, 4, 5],
                 "1, 4-5": [1, 4, 5], "1-3, 5": [1, 3, 2, 5], "1, 3, 5": [1, 5, 3]}
        for out, arg in iteritems(tests):
            assert lutils.hyphenise(arg) == out

    def test_group_by_detector(self):
        output, workspaces = [], []
        for x in range(1, 5):
            ws = self.var_ws_name.format(x)
            workspaces.append(ws)
            mantid.CreateSampleWorkspace(OutputWorkspace=ws).getName()
            output.append("{}; Detector {}".format(self.test_run, x))
        assert lutils.group_by_detector(self.test_run, workspaces) == output

    def test_flatten_run_data(self):
        workspaces = []
        for x in range(0, len(self.test_workspaces), 2):
            name = str(x)
            mantid.GroupWorkspaces(
                self.test_workspaces[x:x + 2], OutputWorkspace=name)
            workspaces.append(name)
        assert lutils.flatten_run_data(workspaces) == [self.test_ws_names]


if __name__ == "__main__":
    unittest.main()
