# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import Enum
from sans.common.enums import ISISReductionMode, ReductionMode


class SANSEnumsTest(unittest.TestCase):
    """
    Before refactoring SANS enums to use the Enum class, ISISReductionMode derived
    from ReductionMode, therefore ISISReductionMode.Merged/All == ReductionMode.Merged/All.
    For compatibility with old code, we add a custom __eq__ to ISISReductionMode to ensure
    the comparison still holds
    """
    def test_that_ISISReductionMode_Merged_equals_ReductionMode_Merged(self):
        self.assertEqual(ReductionMode.Merged, ISISReductionMode.Merged)
        self.assertEqual(ISISReductionMode.Merged, ReductionMode.Merged)

    def test_that_ISISReductionMode_All_equals_ReductionMode_All(self):
        self.assertEqual(ReductionMode.All, ISISReductionMode.All)
        self.assertEqual(ISISReductionMode.All, ReductionMode.All)

    def test_that_ReductionMode_Enums_do_not_equal_any_enum(self):
        test_enum = Enum("test_enum", "Merged All")

        self.assertNotEqual(test_enum.Merged, ReductionMode.Merged)
        self.assertNotEqual(test_enum.Merged, ISISReductionMode.Merged)

        self.assertNotEqual(test_enum.All, ReductionMode.All)
        self.assertNotEqual(test_enum.All, ISISReductionMode.All)

    def test_that_other_ISISReductionMode_equalities_have_default_behaviour(self):
        self.assertEqual(ISISReductionMode.LAB, ISISReductionMode.LAB)
        self.assertNotEqual(ISISReductionMode.LAB, ISISReductionMode.Merged)

        test_enum = Enum("test_enum", "HAB LAB")
        self.assertNotEqual(test_enum.HAB, ISISReductionMode.HAB)


if __name__ == '__main__':
    unittest.main()
