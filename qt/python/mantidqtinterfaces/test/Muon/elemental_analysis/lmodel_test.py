# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils import LModel


class LModelTest(unittest.TestCase):
    def setUp(self):
        self.model = LModel()

    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.mantid')
    def test_load_calls_loadAscii_with_correct_parameters(self, mock_mantid):
        items = {'path1': 'name1', 'path2': 'name2'}
        self.model._load(items)
        call_list = [mock.call('path1', OutputWorkspace='name1'),
                     mock.call('path2', OutputWorkspace='name2')]
        mock_mantid.LoadAscii.assert_has_calls(call_list, any_order=True)
        self.assertEqual(mock_mantid.LoadAscii.call_count, 2)

    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.merge_workspaces')
    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.search_user_dirs')
    def test_load_run_calls_search_user_dirs(self, mock_search_user_dirs, mock_merged):
        self.model.run = 1234
        self.model.load_run()

        self.assertEqual(mock_search_user_dirs.call_count, 1)
        self.assertEqual(mock_merged.call_count, 1)
        mock_search_user_dirs.assert_called_with(1234)

    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.search_user_dirs')
    def test_load_run_calls_returns_none_if_run_not_found(self, mock_search_user_dirs):
        mock_search_user_dirs.return_value = None
        self.model._load = mock.Mock()
        ret = self.model.load_run()

        self.assertEqual(mock_search_user_dirs.call_count, 1)
        self.assertEqual(self.model._load.call_count, 0)
        self.assertEqual(ret, None)


if __name__ == '__main__':
    unittest.main()
