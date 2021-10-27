# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantidqtinterfaces.Muon.GUI.Common.utilities.general_utils as utils
import unittest


class GeneralMuonUtilsTest(unittest.TestCase):
    def test_round_up_to_4_dp(self):
        self.assertEqual("0.0002", utils.round_value("0.00018",4))

    def test_round_down_to_4_dp(self):
        self.assertEqual("0.0001", utils.round_value("0.00011",4))

    def test_round_zero(self):
        self.assertEqual("0", utils.round_value("0.0000",4))

    def test_round_significant_figures_up(self):
        self.assertEqual("0.013", utils.round_significant_figures("0.0126",2))
        self.assertEqual("1.3e+03", utils.round_significant_figures("1263",2))

    def test_round_significant_figures_down(self):
        self.assertEqual("0.012", utils.round_significant_figures("0.012134",2))
        self.assertEqual("1.2e+03", utils.round_significant_figures("1223",2))

    def test_round_to_min_whole_number_or_sf_to_whole_number(self):
        self.assertEqual("1223", utils.round_to_min_whole_number_or_sf("1223.258",2))
        self.assertEqual("1224", utils.round_to_min_whole_number_or_sf("1223.658",2))

    def test_round_to_min_whole_number_or_sf_to_sf(self):
        self.assertEqual("0.012", utils.round_to_min_whole_number_or_sf("0.0123458",2))
        self.assertEqual("0.013", utils.round_to_min_whole_number_or_sf("0.0126456",2))


if __name__ == '__main__':
    unittest.main()
