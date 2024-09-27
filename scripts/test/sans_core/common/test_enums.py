# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans_core.common.enums import ReductionMode


class TestSANSEnums(unittest.TestCase):
    def test_converter_accepts_legacy_and_new(self):
        # Mixed case intentional
        for val, expected in [
            ("hAb", ReductionMode.HAB),
            ("fRont", ReductionMode.HAB),
            ("laB", ReductionMode.LAB),
            ("reaR", ReductionMode.LAB),
            ("MerGed", ReductionMode.MERGED),
            ("AlL", ReductionMode.ALL),
        ]:
            self.assertEqual(expected, ReductionMode.convert(val))

    def test_converter_throws_unknown(self):
        with self.assertRaises(ValueError):
            ReductionMode.convert("unknown")

    def test_converter_throws_deprecated(self):
        for val in ["hab", "lab"]:
            with self.assertRaisesRegex(ValueError, "deprecated"):
                ReductionMode.convert(val, support_deprecated=False)


if __name__ == "__main__":
    unittest.main()
