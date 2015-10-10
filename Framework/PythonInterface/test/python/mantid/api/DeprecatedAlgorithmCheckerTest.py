import unittest
from mantid.api import DeprecatedAlgorithmChecker

class DeprecatedAlgorithmCheckerTest(unittest.TestCase):

    def test_constructor_throws_for_non_existant_algorithm(self):
        self.assertRaises(RuntimeError, DeprecatedAlgorithmChecker, "A_Very_Silly_Alg_Name",-1)

    def test_non_deprecated_algorithm_returns_empty_string_from_isDeprecated(self):
        deprecation_check = DeprecatedAlgorithmChecker("LoadRaw",-1)
        msg = deprecation_check.isDeprecated()
        self.assertTrue(len(msg) == 0)

    def test_deprecated_algorithm_returns_non_empty_string_from_isDeprecated(self):
        deprecation_check = DeprecatedAlgorithmChecker("DiffractionFocussing",1)
        msg = deprecation_check.isDeprecated()
        self.assertTrue(len(msg) > 0)


if __name__ == '__main__':
    unittest.main()
