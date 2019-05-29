# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, unicode_literals)

import itertools
import unittest

from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import FloatTimeSeriesProperty, StringPropertyWithValue
from mantid.py3compat import mock

from Muon.GUI.Common.results_tab_widget.results_tab_model import (
    DEFAULT_TABLE_NAME, ALLOWED_NON_TIME_SERIES_LOGS, ResultsTabModel)

# constants
FITTING_CONTEXT_CLS = 'Muon.GUI.Common.contexts.fitting_context.FittingContext'


class ResultsTabModelTest(unittest.TestCase):
    def setUp(self):
        self.context_patcher = mock.patch(FITTING_CONTEXT_CLS, autospec=True)
        self.fitting_context = self.context_patcher.start()
        self.model = ResultsTabModel(self.fitting_context)

    def tearDown(self):
        self.context_patcher.stop()
        AnalysisDataService.Instance().clear()

    # ------------------------- success tests ----------------------------
    def test_default_model_has_results_table_name(self):
        self.assertEqual(self.model.results_table_name(), DEFAULT_TABLE_NAME)

    def test_updating_model_results_table_name(self):
        table_name = 'table_name'
        self.model.set_results_table_name(table_name)
        self.assertEqual(self.model.results_table_name(), table_name)

    def test_log_names_from_workspace_with_logs(self):
        fake_ws = create_test_workspace()
        run = fake_ws.run()
        # populate with log data
        time_series_names = ('ts_1', 'ts_2')
        for name in time_series_names:
            run.addProperty(name, FloatTimeSeriesProperty(name), replace=True)
        single_value_log_names = ('sv_1', 'sv_2')
        for name in itertools.chain(single_value_log_names,
                                    ALLOWED_NON_TIME_SERIES_LOGS):
            run.addProperty(name,
                            StringPropertyWithValue(name, 'test'),
                            replace=True)
        # verify
        allowed_logs = self.model.log_names(fake_ws.name())
        for name in itertools.chain(time_series_names,
                                    ALLOWED_NON_TIME_SERIES_LOGS):
            self.assertTrue(
                name in allowed_logs,
                msg="{} not found in allowed log list".format(name))
        for name in single_value_log_names:
            self.assertFalse(name in allowed_logs,
                             msg="{} found in allowed log list".format(name))

    def test_log_names_from_workspace_without_logs(self):
        fake_ws = create_test_workspace()
        allowed_logs = self.model.log_names(fake_ws.name())
        self.assertEqual(0, len(allowed_logs))

    def test_default_model_has_zero_fit_functions(self):
        self.assertEqual(len(self.model.fit_functions()))

    def test_model_returns_fit_functions_from_context(self):
        test_functions = ['func_1', 'func2']
        self.fitting_context.fit_function_names.return_value = test_functions

        self.assertEqual(test_functions, self.model.fit_functions())

    def test_model_creates_expected_map_input_workspaces_to_include_state(self):
        test_fits, expected_list_state = create_test_fits()
        self.fitting_context.fit_list = test_fits

        self.assertEqual(expected_list_state, self.model.fit_input_workspaces())

    # ------------------------- failure tests ----------------------------
    def test_log_names_from_workspace_not_in_ADS_raises_exception(self):
        self.assertRaises(KeyError, self.model.log_names,
                          'not a workspace in ADS')


def create_test_workspace():
    fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
    ws_name = 'results_tab_model_test_log_names_fake_ws'
    AnalysisDataService.Instance().addOrReplace(ws_name, fake_ws)
    return fake_ws


def create_test_fits():
    input_workspaces = ['ws1', 'ws2']
    fits = []
    list_state = {}
    checked, enabled = True, True
    for index, name in enumerate(input_workspaces):
        fit_info = mock.MagicMock()
        fit_info.input_workspace = name
        fits.append(fit_info)
        list_state[name] = [index, checked, enabled]

    return fits, list_state


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
