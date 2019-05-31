# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import DeprecatedAlgorithmChecker, FrameworkManagerImpl


class DeprecatedAlgorithmCheckerTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManagerImpl.Instance()

    def test_constructor_throws_for_non_existant_algorithm(self):
        self.assertRaises(RuntimeError, DeprecatedAlgorithmChecker, "A_Very_Silly_Alg_Name",-1)

    def test_non_deprecated_algorithm_returns_empty_string_from_isDeprecated(self):
        deprecation_check = DeprecatedAlgorithmChecker("LoadRaw",-1)
        msg = deprecation_check.isDeprecated()
        self.assertEqual(len(msg),  0)

    def test_deprecated_algorithm_returns_non_empty_string_from_isDeprecated(self):
        deprecation_check = DeprecatedAlgorithmChecker("DiffractionFocussing",1)
        msg = deprecation_check.isDeprecated()
        self.assertTrue(len(msg) > 0)


if __name__ == '__main__':
    unittest.main()
