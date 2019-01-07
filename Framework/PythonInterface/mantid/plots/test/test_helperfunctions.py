import unittest

import numpy as np
from mock import Mock

from mantid.plots.helperfunctions import get_distribution, points_from_boundaries, validate_args
from mantid.simpleapi import CreateSampleWorkspace


class MockMantidAxes:
    def __init__(self):
        self.set_xlabel = Mock()


class HelperfunctionsTest(unittest.TestCase):
    def test_validate_args_success(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        result = validate_args([ws])
        # todo isnt true why?
        self.assertEqual(True, result)

    def test_get_distribution(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        result = get_distribution(ws)
        self.assertEqual((False, {}), result)

    def test_get_distribution_from_kwargs(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        result = get_distribution(ws, distribution=True)
        self.assertEqual((True, {}), result)
        result = get_distribution(ws, distribution=False)
        self.assertEqual((False, {}), result)

    def test_points_from_boundaries(self):
        arr = np.array([2, 4, 6, 8])
        result = points_from_boundaries(arr)

        self.assertEqual([3, 7], result)

    def test_points_from_boundaries_raise_length_less_than_2(self):
        arr = np.array([1])
        self.assertRaises(ValueError, points_from_boundaries, arr)

    def test_points_from_boundaries_raise_not_numpy_array(self):
        arr = [1, 2, 3, 4]
        self.assertRaises(AssertionError, points_from_boundaries, arr)
