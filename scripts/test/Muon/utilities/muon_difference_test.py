# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from Muon.GUI.Common.muon_group import MuonDiff


class MuonDifferenceTest(unittest.TestCase):

    def test_that_cannot_initialize_MuonDiff_without_name(self):
        with self.assertRaises(TypeError):
            MuonDiff(positive="positive", negative="negative")

    def test_that_cannot_initialize_MuonDiff_without_positive(self):
        with self.assertRaises(TypeError):
            MuonDiff(diff_name="diff", negative="negative")

    def test_that_cannot_initialize_MuonDiff_without_negative(self):
        with self.assertRaises(TypeError):
            MuonDiff(diff_name="diff", positive="positive")

    def test_that_can_get_positive_and_negative(self):
        diff = MuonDiff("diff1", "positive", "negative")

        self.assertEqual("positive", diff.forward_group)
        self.assertEqual("negative", diff.backward_group)

    def test_is_group_diff_by_default(self):
        diff = MuonDiff("diff1", "positive", "negative")

        self.assertEqual("group", diff.group_or_pair)

    def test_can_create_pair_diff(self):
        diff = MuonDiff("diff1", "positive", "negative", "pair")

        self.assertEqual("pair", diff.group_or_pair)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
