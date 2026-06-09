# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock
import tempfile
from Engineering.texture.TextureUtils import run_abs_corr, validate_abs_corr_inputs


texture_utils_path = "Engineering.texture.TextureUtils.correction_utils"


class TextureUtilsAbsCorrTests(unittest.TestCase):
    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.validate_abs_corr_inputs")
    @patch(f"{texture_utils_path}.TextureCorrectionModel")
    def test_run_abs_corr_executes_full_pipeline(self, mock_model_class, mock_validate, mock_logger):
        # Arrange
        mock_model = MagicMock()
        mock_model_class.return_value = mock_model
        mock_validate.return_value = (True, "")

        wss = ["ws1", "ws2"]
        # Act
        with tempfile.TemporaryDirectory() as d:
            run_abs_corr(
                wss=wss,
                ref_ws="ref",
                orientation_file="orient.txt",
                orient_file_is_euler=True,
                euler_scheme="xyz",
                euler_axes_sense="right",
                copy_ref=True,
                include_abs_corr=True,
                monte_carlo_args="Arg: Val",
                gauge_vol_preset="4mmCube",
                gauge_vol_shape_file="shape.xml",
                include_atten_table=True,
                eval_point="1.54",
                eval_units="Angstrom",
                root_dir=d,
                include_div_corr=True,
                div_hoz=1.0,
                div_vert=1.0,
                det_hoz=0.5,
                clear_ads_after=True,
            )

        mock_validate.assert_called_once_with(
            "ref",
            "orient.txt",
            True,
            "xyz",
            "right",
            True,
            True,
            "4mmCube",
            "shape.xml",
            True,
            "1.54",
            "Angstrom",
            True,
            1.0,
            1.0,
            0.5,
        )
        mock_logger.error.assert_not_called()

        # Orientation file triggers a load
        mock_model.load_all_orientations.assert_called_once_with(wss, "orient.txt", True, "xyz", "right")

        # Copy ref when requested
        mock_model.copy_sample_info.assert_called_once_with("ref", wss)

        # Set processing flags / metadata
        mock_model.set_include_abs.assert_called_once_with(True)
        mock_model.set_include_atten.assert_called_once_with(True)
        mock_model.set_include_div.assert_called_once_with(True)
        mock_model.set_remove_after_processing.assert_called_once_with(True)

        # corrections call: check args and kwargs
        mock_model.calc_all_corrections.assert_called_once()
        args, kwargs = mock_model.calc_all_corrections.call_args

        # Positional args
        self.assertEqual(args[0], ["ws1", "ws2"])
        self.assertEqual(args[1], ["Corrected_ws1", "Corrected_ws2"])

        # Keyword args
        self.assertEqual(
            kwargs["abs_args"],
            {"gauge_vol_preset": "4mmCube", "gauge_vol_file": "shape.xml", "mc_param_str": "Arg: Val"},
        )
        self.assertEqual(kwargs["atten_args"], {"atten_val": "1.54", "atten_units": "Angstrom"})
        self.assertEqual(kwargs["div_args"], {"hoz": 1.0, "vert": 1.0, "det_hoz": 0.5})

    @patch(f"{texture_utils_path}.logger")
    @patch(f"{texture_utils_path}.TextureCorrectionModel")
    def test_run_abs_corr_logs_error_on_invalid_input(self, mock_model, mock_logger):
        run_abs_corr(
            wss=["ws1"],
            orientation_file="orient.txt",  # Missing orient_file_is_euler
        )
        mock_logger.error.assert_called_once()
        self.assertIn("must flag orient_file_is_euler", mock_logger.error.call_args[0][0])

    def test_validate_abs_corr_inputs_when_orientation_file_given_requires_is_euler_flag(self):
        valid, error_msg = validate_abs_corr_inputs(orientation_file="some_file.txt")
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If orientation file is specified, must flag orient_file_is_euler.\n")

    def test_validate_abs_corr_inputs_when_is_euler_require_scheme_and_sense(self):
        valid, error_msg = validate_abs_corr_inputs(orientation_file="some_file.txt", orient_file_is_euler=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If orientation file is euler, must provide scheme and sense.\n")

    def test_validate_abs_corr_inputs_when_copy_ref_requires_ref_ws(self):
        valid, error_msg = validate_abs_corr_inputs(copy_ref=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If copy_ref is True, must provide ref_ws.\n")

    def test_validate_abs_corr_inputs_when_include_custom_gv_req_xml_file(self):
        valid, error_msg = validate_abs_corr_inputs(include_abs_corr=True, gauge_vol_preset="Custom")
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If custom gauge volume required, must provide shape xml as file.\n")

    def test_validate_abs_corr_inputs_when_attenuation_table_req_point_and_unit(self):
        valid, error_msg = validate_abs_corr_inputs(include_atten_table=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If attenuation table required, must provide valid point and units.\n")

    def test_validate_abs_corr_inputs_when_include_div_corr_require_divergence_vals(self):
        valid, error_msg = validate_abs_corr_inputs(include_div_corr=True)
        self.assertFalse(valid)
        self.assertEqual(error_msg, r"If divergence correction required, must provide valid values.\n")


if __name__ == "__main__":
    unittest.main()
