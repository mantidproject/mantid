# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.sans_isis.gui_logic.models.model_common import ModelCommon


class ModelCommonTest(unittest.TestCase):
    def test_get_safe_val(self):
        should_return_val = ["a", 0, 0.0, 1, 1.2, False, [0]]
        should_return_default = ["", None, []]

        for i in should_return_val:
            self.assertEqual(i, ModelCommon._get_val_or_default(i))

        for i in should_return_default:
            self.assertEqual("default", ModelCommon._get_val_or_default(i, "default"))


if __name__ == "__main__":
    unittest.main()
