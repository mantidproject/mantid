# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.grouping_widget.ea_grouping_tab_model import EAGroupingTabModel


class EAGroupingTabModelTest(unittest.TestCase):
    def setUp(self):
        self.context = mock.MagicMock()

    def test_reset_group_and_pairs_to_default_for_no_loaded_runs_when_fails(self):
        self.context.current_runs = []
        model = EAGroupingTabModel(self.context)

        status = model.reset_groups_to_default()
        self.assertEqual(status, "failed")

    def test_reset_group_and_pairs_to_default_for_no_loaded_runs_when_passes(self):
        self.context.current_runs = ["NotEmpty"]
        model = EAGroupingTabModel(self.context)

        status = model.reset_groups_to_default()
        self.assertEqual(status, "success")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
