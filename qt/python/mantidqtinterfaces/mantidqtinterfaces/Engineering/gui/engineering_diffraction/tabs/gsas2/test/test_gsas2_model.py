# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

# from unittest import mock
# from unittest.mock import patch
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.model import \
    GSAS2Model
# from testhelpers import assertRaisesNothing

model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.model"


class TestGSAS2Model(unittest.TestCase):
    def setUp(self):
        self.model = GSAS2Model()

    # @patch(model_path + '.settings.settings_helper.get_setting')
    # @patch(model_path + '.settings.settings_helper.get_setting')
    # def test_run_model(self, mock_timeout, mock_dSpacing_min):
    #     mock_timeout.return_value = "5"
    #     mock_dSpacing_min.return_value = "1"
    #     number_histograms = self.model.run_model(
    #         [["/home/danielmurphy/Desktop/GSASMantiddata_030322/ENGINX_305738_bank_1.prm"],
    #          ["/home/danielmurphy/Desktop/GSASMantiddata_030322/FE_GAMMA.cif"],
    #          ["/home/danielmurphy/Desktop/GSASMantiddata_030322/Save_gss_305761_307521_bank_1_bgsub.gsa"],
    #          ["1"]],
    #         ["Pawley", "3.65, 3.65, 3.65", True, True, True], "220321script3", "rb_number",
    #         [["18401"], ["50000"]])
    #     assert number_histograms == 1, "run_model failed!"


if __name__ == '__main__':
    unittest.main()
