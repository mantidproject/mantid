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

from Muon.GUI.Common.results_tab_widget.results_tab_model import (
    DEFAULT_TABLE_NAME, ALLOWED_NON_TIME_SERIES_LOGS, ResultsTabModel)


class ResultsTabModelTest(unittest.TestCase):
    def tearDown(self):
        AnalysisDataService.Instance().clear()

    # ------------------------- success tests ----------------------------
    def test_default_model_has_results_table_name(self):
        model = ResultsTabModel()
        self.assertEqual(model.results_table_name, DEFAULT_TABLE_NAME)

    def test_updating_model_results_table_name(self):
        model = ResultsTabModel()
        table_name = 'table_name'
        model.results_table_name = table_name
        self.assertEqual(model.results_table_name, table_name)

    def test_log_names_from_workspace_with_logs(self):
        fake_ws = create_test_workspace(add_logs=True)
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
        model = ResultsTabModel()
        allowed_logs = model.log_names(fake_ws.name())
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
        model = ResultsTabModel()
        allowed_logs = model.log_names(fake_ws.name())
        self.assertEqual(0, len(allowed_logs))

    # ------------------------- failure tests ----------------------------
    def test_log_names_from_workspace_not_in_ADS_raises_exception(self):
        model = ResultsTabModel()
        self.assertRaises(KeyError, model.log_names, 'not a workspace in ADS')


def create_test_workspace():
    fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
    ws_name = 'results_tab_model_test_log_names_fake_ws'
    AnalysisDataService.Instance().addOrReplace(ws_name, fake_ws)
    return fake_ws


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
