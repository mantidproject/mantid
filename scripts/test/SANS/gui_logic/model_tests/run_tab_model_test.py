# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans.common.enums import SaveType
from sans.gui_logic.models.run_tab_model import RunTabModel


class RunTabModelTest(unittest.TestCase):
    def setUp(self) -> None:
        self.model = RunTabModel()

    def test_setting_save_types_marks_as_user_set(self):
        self.model.update_save_types([])
        save_types = self.model.get_save_types()
        self.assertTrue(save_types.user_modified)

    def test_setting_all_save_types(self):
        all_on = [SaveType.CAN_SAS, SaveType.NX_CAN_SAS, SaveType.RKH]
        self.model.update_save_types(all_on)
        selected = self.model.get_save_types()
        self.assertTrue(selected.can_sas_1d)
        self.assertTrue(selected.nxs_can_sas)
        self.assertTrue(selected.rkh)

    def test_setting_some_save_types(self):
        partial = [SaveType.CAN_SAS, SaveType.RKH]
        self.model.update_save_types(partial)
        selected = self.model.get_save_types()
        self.assertTrue(selected.can_sas_1d)
        self.assertTrue(selected.rkh)
        self.assertFalse(selected.nxs_can_sas)


if __name__ == '__main__':
    unittest.main()
