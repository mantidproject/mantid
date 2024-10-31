# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import OptionalBool, OptionalBoolValue


class OptionalBoolTest(unittest.TestCase):
    def test_default_construction(self):
        obj = OptionalBool()
        self.assertEqual(OptionalBoolValue.Unset, obj.getValue())

    def test_construction_unset(self):
        obj = OptionalBool(OptionalBoolValue.Unset)
        self.assertEqual(OptionalBoolValue.Unset, obj.getValue())

    def test_construction_false(self):
        obj = OptionalBool(OptionalBoolValue.False_)
        self.assertEqual(OptionalBoolValue.False_, obj.getValue())

    def test_construction_true(self):
        obj = OptionalBool(OptionalBoolValue.True_)
        self.assertEqual(OptionalBoolValue.True_, obj.getValue())


if __name__ == "__main__":
    unittest.main()
