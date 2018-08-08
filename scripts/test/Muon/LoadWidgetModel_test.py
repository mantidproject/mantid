import unittest

from Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils

from six import iteritems

try:
    from unittest import mock
except ImportError:
    import mock


class LoadUtilsTest(unittest.TestCase):
    def setUp(self):
        self.test_path = "test\path\to\ral012345.rooth2020.dat"
        self.test_ws_name = "1_Delayed_rooth2020"
        self.bad_path = "test\path\to\ral012345.rooth2042"

    def test_pad_run(self):
        tests = {123: "00123", 0: "00000", 12345: "12345", 123456: "123456"}
        for i, s in iteritems(tests):
            assert lutils.pad_run(i) == s

    def test_get_detector_num_from_ws(self):
        assert lutils.get_detector_num_from_ws(self.test_ws_name) == 1

    def test_get_detectors_num(self):
        nums = [lutils.get_detectors_num(x)
                for x in [self.test_path, self.bad_path]]
        assert nums == [1, 1]

    def test_get_end_num(self):
        endings = [
            lutils.get_end_num(x) for x in [self.test_path, self.bad_path]]
        assert endings == ["rooth2020", "rooth2042"]

    def test_get_run_type(self):
        assert lutils.get_run_type(self.test_path) == "Delayed"
        with self.assertRaises(KeyError):
            lutils.get_run_type(self.bad_path)

    def test_get_filename(self):
        print(lutils.get_filename(self.test_path))
        assert lutils.get_filename(self.test_path) == self.test_ws_name

    def test_hyphenise(self):
        tests = {"1-5": [1, 2, 3, 4, 5],
                 "1, 4-5": [1, 4, 5], "1-3, 5": [1, 3, 2, 5], "1, 3, 5": [1, 5, 3]}
        for out, arg in iteritems(tests):
            assert lutils.hyphenise(arg) == out


if __name__ == "__main__":
    unittest.main()
