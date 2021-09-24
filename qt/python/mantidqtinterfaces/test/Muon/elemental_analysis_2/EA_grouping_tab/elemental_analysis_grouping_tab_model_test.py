# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel
from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.ea_group import EAGroup


class EAGroupingTabModelTest(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()
        self.model = EAGroupingTabModel(self.context)

    def test_reset_group_and_pairs_to_default_for_no_loaded_runs_when_fails(self):
        self.context.current_runs = []

        status = self.model.reset_groups_to_default()
        self.assertEqual(status, "failed")

    def test_reset_group_and_pairs_to_default_for_no_loaded_runs_when_passes(self):
        self.context.current_runs = ["NotEmpty"]

        status = self.model.reset_groups_to_default()
        self.assertEqual(status, "success")

    def test_add_group_with_invalid_argument(self):
        with self.assertRaises(AssertionError):
            self.model.add_group("mock_group")

    def test_add_group_with_valid_argument(self):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.model.add_group(mock_group)
        self.model._groups.add_new_group.assert_called_once_with(mock_group, self.model._data._loaded_data)

    def test_add_group_from_table_with_invalid_argument(self):
        with self.assertRaises(AssertionError):
            self.model.add_group_from_table("mock_group")

    def test_add_group_from_table_with_valid_argument(self):
        mock_group = EAGroup("9999; Detector 1", "detector 1", "9999")
        self.model.add_group_from_table(mock_group)
        self.model._groups.add_group.assert_called_once_with(mock_group)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
