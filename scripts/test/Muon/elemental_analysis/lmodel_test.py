from __future__ import (absolute_import, division, unicode_literals)

import unittest

from mantid.py3compat import mock

from Muon.GUI.ElementalAnalysis.LoadWidget.load_utils import LModel


class LModelTest(unittest.TestCase):
    def setUp(self):
        self.model = LModel()

    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.mantid')
    def test_load_calls_loadAscii_with_correct_parameters(self, mock_mantid):
        items = {'path1': 'name1', 'path2': 'name2'}
        self.model._load(items)

        mock_mantid.LoadAscii.assert_called_with('path1', OutputWorkspace='name1')
        self.assertEqual(mock_mantid.LoadAscii.call_count, 2)

    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.group_by_detector')
    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.search_user_dirs')
    def test_load_run_calls_search_user_dirs(self, mock_search_user_dirs, mock_group):
        self.model.run = 1234
        self.model.load_run()

        self.assertEqual(mock_search_user_dirs.call_count, 1)
        self.assertEqual(mock_group.call_count, 1)
        mock_search_user_dirs.assert_called_with(1234)

    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.search_user_dirs')
    def test_load_run_calls_returns_none_if_run_not_found(self, mock_search_user_dirs):
        mock_search_user_dirs.return_value = None
        self.model._load = mock.Mock()
        ret = self.model.load_run()

        self.assertEqual(mock_search_user_dirs.call_count, 1)
        self.assertEqual(self.model._load.call_count, 0)
        self.assertEqual(ret, None)


if __name__ == '__main__':
    unittest.main()
