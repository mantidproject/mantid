from __future__ import print_function

import unittest

from mantid.py3compat import mock

from Muon.GUI.ElementalAnalysis.LoadWidget import load_model


class LoadModel(unittest.TestCase):
    def setUp(self):
        self.model = load_model.LoadModel()

    def test_that_execute_calls_load_run_if_run_not_in_loaded_runs(self):
        self.model.load_run = mock.Mock()
        self.model.last_loaded_runs = mock.Mock()
        self.model.run = 1234
        self.model.loaded_runs = {1: mock.Mock(), 2: mock.Mock()}
        self.model.execute()

        self.assertEqual(self.model.load_run.call_count, 1)
        self.assertEqual(self.model.last_loaded_runs.append.call_count, 0)

    def test_that_execute_adds_run_to_last_loaded_run_if_run_in_loaded_runs(self):
        self.model.load_run = mock.Mock()
        self.model.last_loaded_runs = mock.Mock()
        self.model.run = 1234
        self.model.loaded_runs = {1234: mock.Mock(), 5678: mock.Mock()}
        self.model.execute()

        self.assertEqual(self.model.load_run.call_count, 0)
        self.assertEqual(self.model.last_loaded_runs.append.call_count, 1)
        self.model.last_loaded_runs.append.assert_called_with(1234)


class CoLoadModel(unittest.TestCase):
    def setUp(self):
        self.model = load_model.CoLoadModel()

    def txest_wipe_co_runs_clears_state(self):
        self.model.workspace = mock.Mock()
        self.model.co_runs = [1, 2, 3]
        self.model.wipe_co_runs()

        self.assertEqual(self.model.workspace, None)
        self.assertEqual(self.model.co_runs, [])

    def txest_that_execute_loads_run_if_not_already_loaded(self):
        self.model.run = 1234
        self.model.loaded_runs = {1: mock.Mock(), 2: mock.Mock()}
        self.model.load_run = mock.Mock(return_value=None)

        # Load failed
        self.model.execute()
        self.assertEqual(self.model.load_run.call_count, 1)
        self.assertTrue(1234 not in self.model.loaded_runs.keys())

        # Load succeeded
        mock_ws = mock.Mock()
        self.model.load_run.return_value = mock_ws
        self.model.execute()
        self.assertEqual(self.model.load_run.call_count, 2)
        self.assertTrue(1234 in self.model.loaded_runs.keys())
        self.assertEqual(self.model.loaded_runs[1234], mock_ws)

    def txest_that_run_is_appended_to_last_loaded_run_if_already_loaded(self):
        self.model.run = 1234
        self.model.loaded_runs = {1234: mock.Mock(), 5678: mock.Mock()}

        self.assertTrue(1234 not in self.model.last_loaded_runs)
        self.model.execute()
        self.assertTrue(1234 in self.model.last_loaded_runs)

    def test_that_run_appeneded_to_co_runs_if_not_present(self):
        self.model.run = 1234
        self.assertTrue(1234 not in self.model.co_runs)
        mock_ws = mock.Mock()
        self.model.load_run = mock.Mock(return_value=mock_ws)
        self.model.co_load_run = mock.Mock()

        self.model.workspace = None
        self.model.execute()
        self.assertTrue(1234 in self.model.co_runs)
        self.assertEqual(self.model.workspace, mock_ws)

        self.model.workspace = mock_ws
        self.model.co_runs = []
        self.assertTrue(1234 not in self.model.co_runs)
        self.model.execute()
        self.assertTrue(1234 in self.model.co_runs)
        self.assertEqual(self.model.workspace, mock_ws)
        self.model.co_load_run.assert_called_with(mock_ws)

    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_model.lutils.replace_workspace_name_suffix')
    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_model.mantid')
    def test_that_add_runs_renames_output_ws_and_adds_correctly(self, mock_mantid, mock_replace_ws):
        l = 'left_ws_1'
        r = 'right-ws'
        suff = 'my-suff'
        mock_replace_ws.return_value = 'left_ws_my-suff'
        ret = self.model.add_runs(l, r, suff)

        mock_replace_ws.assert_called_with(l, suff)
        mock_mantid.Plus.assert_called_with(l, r, OutputWorkspace='left_ws_my-suff')
        self.assertEqual(ret, 'left_ws_my-suff')

    @mock.patch('Muon.GUI.ElementalAnalysis.LoadWidget.load_model.lutils')
    def test_that_co_load_run_calls_right_functions(self, mock_lutils):
        mock_ws = mock.Mock()
        mock_lutils.hyphenise.return_value = 1234
        self.model.co_load_run(mock_ws)

        self.assertEqual(mock_lutils.hyphenise.call_count, 1)
        self.assertTrue(1234 in self.model.last_loaded_runs)
        self.assertEqual(mock_lutils.group_by_detector.call_count, 1)


if __name__ == '__main__':
    unittest.main()













