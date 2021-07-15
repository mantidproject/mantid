# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.gui_logic.models.POD.save_options import SaveOptions
from sans.gui_logic.models.run_tab_model import RunTabModel


class RunTabModelTest(unittest.TestCase):
    def setUp(self) -> None:
        self.model = RunTabModel()

    def test_setting_save_types_marks_as_user_set(self):
        default_options = SaveOptions()
        self.assertFalse(default_options.user_modified)
        self.model.update_save_types(default_options)
        save_types = self.model.get_save_types()
        self.assertTrue(save_types.user_modified)


if __name__ == '__main__':
    unittest.main()
