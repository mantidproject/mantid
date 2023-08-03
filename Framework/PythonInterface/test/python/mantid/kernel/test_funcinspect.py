# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel.funcinspect import lhs_info


class FuncInspectTest(unittest.TestCase):
    def test_lhs_info(self):
        n, names = self._function_returns_lhs_info()
        self.assertEqual(n, 2)
        self.assertEqual(names, ("n", "names"))

    def test_lhs_info_with_unpack(self):
        par = {"list1": [1, 2], "list2": [3, 4]}
        n, names = self._function_returns_lhs_info(**par)
        self.assertEqual(n, 2)
        self.assertEqual(names, ("n", "names"))

    def test_lhs_info_with_multiple_assignment(self):
        a, b = c, d = self._function_returns_lhs_info()
        self.assertEqual(a, c)
        self.assertEqual(b, d)
        self.assertEqual(a, 2)
        self.assertEqual(b, (["a", "b"], ["c", "d"]))

    def test_lhs_info_with_multiple_assignment_with_unpack(self):
        par = {"list1": [1, 2], "list2": [3, 4]}
        a, b = c, d = self._function_returns_lhs_info(**par)
        self.assertEqual(a, c)
        self.assertEqual(b, d)
        self.assertEqual(a, 2)
        self.assertEqual(b, (["a", "b"], ["c", "d"]))

    def test_lhs_info_with_function_call_on_multiple_lines(self):
        # fmt: off
        n, \
            names =\
            self._function_returns_lhs_info()
        # fmt: on
        self.assertEqual(n, 2)
        self.assertEqual(names, ("n", "names"))

    def test_lhs_info_with_tuple_on_lhs(self):
        a = self._function_returns_lhs_info()
        self.assertEqual(len(a), 2)
        self.assertEqual(a[0], 1)
        self.assertEqual(len(a[1]), 1)
        self.assertEqual(a[1][0], "a")

    @classmethod
    def _function_returns_lhs_info(cls, **kwargs):
        n_outputs, var_names = lhs_info("both")
        return n_outputs, var_names


if __name__ == "__main__":
    unittest.main()
