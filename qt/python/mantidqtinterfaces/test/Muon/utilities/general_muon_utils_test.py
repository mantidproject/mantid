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


if __name__ == '__main__':
    unittest.main()
