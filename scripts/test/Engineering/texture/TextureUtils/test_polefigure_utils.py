# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch
from Engineering.texture.TextureUtils import (
    create_pf_loop,
)

texture_utils_path = "Engineering.texture.TextureUtils.polefigure_utils"


class TextureUtilsPoleFigureTests(unittest.TestCase):
    def get_default_pf_kwargs(self):
        return {
            "wss": ["ws1"],
            "param_wss": [["p1"]],
            "include_scatt_power": False,
            "xtal_input": None,
            "hkls": [[1, 1, 1]],
            "readout_columns": ["X0", "I"],
            "dir1": [1, 0, 0],
            "dir2": [0, 1, 0],
            "dir3": [0, 0, 1],
            "dir_names": ["RD", "TD", "ND"],
            "scatter": "both",
            "kernel": 0.2,
            "scat_vol_pos": [0, 0, 0],
            "chi2_thresh": None,
            "peak_thresh": None,
            "save_root": "/save",
            "exp_name": "exp",
            "projection_method": "Azimuthal",
        }

    @patch(f"{texture_utils_path}.create_pf")
    def test_create_pf_loop_scatter_both_calls_create_twice_per_column(self, mock_create):
        create_pf_loop(**self.get_default_pf_kwargs())
        # 2 columns * 2 scatter variants = 4 calls
        self.assertEqual(mock_create.call_count, 4)

    @patch(f"{texture_utils_path}.create_pf")
    def test_create_pf_loop_loads_and_calls_create_pf(self, mock_create):
        with patch(f"{texture_utils_path}.ADS.doesExist", return_value=False):
            args = self.get_default_pf_kwargs()
            args["scatter"] = True
            args["wss"] = ["ws1", "ws2"]
            args["param_wss"] = (["p1", "p2"],)
            args["hkls"] = [[1, 1, 1], [1, 0, 0]]
            args["readout_columns"] = ["I"]
            create_pf_loop(**args)
        mock_create.assert_called_once()


if __name__ == "__main__":
    unittest.main()
