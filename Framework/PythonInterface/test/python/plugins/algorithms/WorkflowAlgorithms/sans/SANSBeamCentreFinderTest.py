# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from plugins.algorithms.WorkflowAlgorithms.SANS.SANSBeamCentreFinder import SANSBeamCentreFinder, _ResidualsDetails


class SANSBeamCentreFinderTest(unittest.TestCase):
    @staticmethod
    def gen_mock_data(q_vals, y_vals):
        assert len(q_vals) == len(y_vals)
        mocked_quartile = mock.Mock()
        mocked_quartile.readX.return_value = q_vals
        mocked_quartile.readY.return_value = y_vals
        return mocked_quartile

    def test_points_calculates_correctly(self):
        points_1 = [1.0, 2.0, 4.0]
        points_2 = [1.0, 3.0, 5.0]

        q1_data = self.gen_mock_data(points_1, [0., 0., 0.])
        q2_data = self.gen_mock_data(points_2, [0., 0., 0.])

        expected_points = len(set().union(points_1, points_2))
        mismatched_points = len(set(points_1).symmetric_difference(set(points_2)))

        obj = SANSBeamCentreFinder()
        result = obj._calculate_residuals(q1_data, q2_data)
        self.assertIsInstance(result, _ResidualsDetails)
        self.assertEqual(mismatched_points, result.mismatched_points)
        self.assertEqual(expected_points, result.num_points_considered)

    def test_residual_diff_with_only_mismatched(self):
        q1_data = self.gen_mock_data([1.0, 3.0], [0.0, 1.0])
        q2_data = self.gen_mock_data([1.0, 2.0], [0.0, 1.0])

        # Mismatched points should always contribute their entire val, not the diff (i.e. 1. above)
        obj = SANSBeamCentreFinder()
        result = obj._calculate_residuals(q1_data, q2_data)
        self.assertEqual(2.0, result.total_residual)

    def test_residual_diff_with_only_matched(self):
        y_data_1 = [1.0, 10.0]
        y_data_2 = [2.0, 10.0]
        q1_data = self.gen_mock_data([1.0, 2.0], y_data_1)
        q2_data = self.gen_mock_data([1.0, 2.0], y_data_2)

        expected_matched = (y_data_1[0] - y_data_2[0]) ** 2

        obj = SANSBeamCentreFinder()
        result = obj._calculate_residuals(q1_data, q2_data)

        self.assertEqual(expected_matched, result.total_residual)

    def test_residual_diff_with_mixed_mismatched(self):
        y_data_1 = [1.0, 10.0]
        y_data_2 = [2.0, 10.0]
        q1_data = self.gen_mock_data([1.0, 3.0], y_data_1)
        q2_data = self.gen_mock_data([1.0, 2.0], y_data_2)

        obj = SANSBeamCentreFinder()
        result = obj._calculate_residuals(q1_data, q2_data)

        # Matched bins contribute the diff ([0]) whilst mismatched ([1]) contribute all
        expected_matched = (y_data_1[0] - y_data_2[0]) ** 2
        expected_mismatched = (y_data_1[1] ** 2) + (y_data_2[1] ** 2)
        squared_expected = expected_matched + expected_mismatched
        self.assertEqual(squared_expected, result.total_residual)


if __name__ == '__main__':
    unittest.main()
