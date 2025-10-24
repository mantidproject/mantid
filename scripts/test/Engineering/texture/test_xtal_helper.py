# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock

from Engineering.texture.xtal_helper import get_xtal_structure
from numpy import ones

texture_utils_path = "Engineering.texture.xtal_helper"


class CrystalPhaseHelperMixin:
    def get_valid_xtal_string_kwargs(self):
        return {"lattice": "2.8665  2.8665  2.8665", "space_group": "I m -3 m", "basis": "Fe 0 0 0 1.0 0.05; Fe 0.5 0.5 0.5 1.0 0.05"}

    def get_valid_xtal_array_kwargs(self):
        return {"alatt": ones(3), "space_group": "I m -3 m", "basis": "Fe 0 0 0 1.0 0.05; Fe 0.5 0.5 0.5 1.0 0.05"}

    def get_valid_xtal_cif_kwargs(self):
        return {"cif_file": "example.cif"}

    def get_invalid_kwargs_dict(self, valid_kwarg_dict, key):
        valid_kwarg_dict["bad_val"] = valid_kwarg_dict.pop(key)
        return valid_kwarg_dict

    def get_invalid_kwargs_val_dict(self, valid_kwarg_dict, key, bad_val):
        valid_kwarg_dict[key] = bad_val
        return valid_kwarg_dict

    def exec_kwarg_checker(self, valid_kwargs_dict, input_type):
        valid_kwargs = list(valid_kwargs_dict.keys())
        for key in valid_kwargs:
            bad_kwargs_dict = self.get_invalid_kwargs_dict(valid_kwargs_dict, key)
            self.assertFalse(self.try_make_xtal_from_args(input_type, bad_kwargs_dict))

    def try_make_xtal_from_args(self, input_type, kwargs_dict):
        try:
            get_xtal_structure(input_type, **kwargs_dict)
            return True
        except Exception:
            return False

    def exec_bad_val_checker(self, valid_kwargs_dict, input_type, bad_val):
        valid_kwargs = valid_kwargs_dict.keys()
        for key in valid_kwargs:
            bad_kwargs_dict = self.get_invalid_kwargs_val_dict(valid_kwargs_dict, key, bad_val)
            self.assertFalse(self.try_make_xtal_from_args(input_type, bad_kwargs_dict))


class TextureUtilsTest(CrystalPhaseHelperMixin, unittest.TestCase):
    def test_make_xtal_from_string(self):
        string_kwargs = self.get_valid_xtal_string_kwargs()
        self.assertTrue(self.try_make_xtal_from_args("string", string_kwargs))

    def test_make_phase_from_array(self):
        array_kwargs = self.get_valid_xtal_array_kwargs()
        self.assertTrue(self.try_make_xtal_from_args("array", array_kwargs))

    @patch(f"{texture_utils_path}.LoadCIF")
    @patch(f"{texture_utils_path}.CreateSingleValuedWorkspace")
    def test_make_xtal_from_cif(self, mock_create, mock_load):
        cif_kwargs = self.get_valid_xtal_cif_kwargs()

        # setup mocks for the cif loading
        mock_ws = MagicMock()
        mock_sample = MagicMock()
        mock_ws.sample().return_value = mock_sample
        mock_create.return_value = mock_ws
        mock_load.return_value = MagicMock()

        # check
        self.assertTrue(self.try_make_xtal_from_args("cif", cif_kwargs))
        mock_create.assert_called_once()
        mock_load.assert_called_once()

    def test_make_xtal_from_string_gives_error_invalid_kwarg(self):
        self.exec_kwarg_checker(self.get_valid_xtal_string_kwargs(), "string")

    def test_make_xtal_from_array_gives_error_invalid_kwarg(self):
        self.exec_kwarg_checker(self.get_valid_xtal_array_kwargs(), "array")

    def test_make_xtal_from_cif_gives_error_invalid_kwarg(self):
        self.exec_kwarg_checker(self.get_valid_xtal_cif_kwargs(), "cif")

    def test_make_xtal_from_string_gives_error_bad_vals(self):
        self.exec_bad_val_checker(self.get_valid_xtal_string_kwargs(), "string", "test")

    def test_make_xtal_from_array_gives_error_bad_vals(self):
        self.exec_bad_val_checker(self.get_valid_xtal_string_kwargs(), "array", "test")

    def test_get_xtal_structure_invalid_input_method_raises(self):
        with self.assertRaises(ValueError):
            get_xtal_structure("invalid_method")


if __name__ == "__main__":
    unittest.main()
