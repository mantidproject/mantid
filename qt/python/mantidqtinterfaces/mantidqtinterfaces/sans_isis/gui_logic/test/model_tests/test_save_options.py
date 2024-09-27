# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans_core.common.enums import SaveType
from mantidqtinterfaces.sans_isis.gui_logic.models.POD.save_options import SaveOptions


class SaveOptionsTest(unittest.TestCase):
    def test_empty_parses_to_list_correctly(self):
        result = SaveOptions().to_all_states()
        self.assertEqual([SaveType.NO_TYPE], result)

    def test_partial_parses_to_list_correctly(self):
        opts = SaveOptions()
        opts.rkh = True
        opts.nxs_can_sas = True

        self.assertEqual([SaveType.NX_CAN_SAS, SaveType.RKH], opts.to_all_states())

    def test_single_options_parses_correctly(self):
        opts = SaveOptions()
        opts.can_sas_1d = True
        self.assertEqual([SaveType.CAN_SAS], opts.to_all_states())


if __name__ == "__main__":
    unittest.main()
