# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, unicode_literals)

import unittest

from mantid.api import AnalysisDataService, WorkspaceFactory
from mantid.kernel import FloatTimeSeriesProperty, StringPropertyWithValue
from mantid.py3compat import mock

from Muon.GUI.Common.contexts.fitting_context import FitInformation


def create_test_workspace(ws_name=None,
                          time_series_logs=None,
                          string_value_logs=None):
    """
    Create a test workspace.
    :param ws_name: An optional name for the workspace
    :param time_series_logs: A set of (name, (values,...))
    :param string_value_logs: A set of (name, value) pairs
    :return: The new workspace
    """
    fake_ws = WorkspaceFactory.create('Workspace2D', 1, 1, 1)
    run = fake_ws.run()
    if time_series_logs is not None:
        for name, values in time_series_logs:
            tsp = FloatTimeSeriesProperty(name)
            for item in values:
                try:
                    time, value = item[0], item[1]
                except TypeError:
                    time, value = "2000-05-01T12:00:00", item
                tsp.addValue(time, value)
            run.addProperty(name, tsp, replace=True)

    if string_value_logs is not None:
        for name, value in string_value_logs:
            run.addProperty(
                name, StringPropertyWithValue(name, value), replace=True)

    ws_name = ws_name if ws_name is not None else 'fitting_context_model_test'
    AnalysisDataService.Instance().addOrReplace(ws_name, fake_ws)
    return fake_ws


class FitInformationTest(unittest.TestCase):
    def tearDown(self):
        AnalysisDataService.Instance().clear()

    def test_equality_with_no_globals(self):
        fit_info = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                  mock.MagicMock(), mock.MagicMock())
        self.assertEqual(fit_info, fit_info)

    def test_equality_with_globals(self):
        fit_info = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                  mock.MagicMock(), mock.MagicMock(), ['A'])
        self.assertEqual(fit_info, fit_info)

    def test_inequality_with_globals(self):
        fit_info1 = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                   mock.MagicMock(), ['A'])
        fit_info2 = FitInformation(mock.MagicMock(), 'MuonGuassOsc',
                                   mock.MagicMock(), ['B'])
        self.assertNotEqual(fit_info1, fit_info2)

    def test_empty_global_parameters_if_none_specified(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock())

        self.assertEqual([],
                         fit_information_object.parameters.global_parameters)

    def test_global_parameters_are_captured(self):
        fit_information_object = FitInformation(mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(),
                                                mock.MagicMock(), ['A'])

        self.assertEqual(['A'],
                         fit_information_object.parameters.global_parameters)

    def test_parameters_are_readonly(self):
        fit_info = FitInformation(mock.MagicMock(), mock.MagicMock(),
                                  mock.MagicMock(), mock.MagicMock())

        self.assertRaises(AttributeError, setattr, fit_info, "parameters",
                          mock.MagicMock())

    def test_logs_from_workspace_without_logs_returns_emtpy_list(self):
        fake_ws = create_test_workspace()
        fit = FitInformation(mock.MagicMock(), 'func1', fake_ws.name(),
                             fake_ws.name())

        allowed_logs = fit.log_names()
        self.assertEqual(0, len(allowed_logs))

    def test_logs_for_single_workspace_return_all_time_series_logs(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )))
        single_value_logs = (('sv_1', 'val1'), ('sv_2', 'val2'))
        fake_ws = create_test_workspace(time_series_logs=time_series_logs)
        fit = FitInformation(mock.MagicMock(), 'func1', fake_ws.name(),
                             fake_ws.name())

        log_names = fit.log_names()
        for name, _ in time_series_logs:
            self.assertTrue(
                name in log_names, msg="{} not found in log list".format(name))
        for name, _ in single_value_logs:
            self.assertFalse(
                name in log_names, msg="{} found in log list".format(name))

    def test_log_names_from_list_of_workspaces_gives_combined_set(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))

        fake1 = create_test_workspace(
            ws_name='fake1', time_series_logs=time_series_logs[:2])
        fake2 = create_test_workspace(
            ws_name='fake2', time_series_logs=time_series_logs[2:])
        fit = FitInformation(mock.MagicMock(), 'func1',
                             [fake1.name(), fake2.name()], [fake1.name(), fake2.name()])

        log_names = fit.log_names()
        self.assertEqual(len(time_series_logs), len(log_names))
        for name, _ in time_series_logs:
            self.assertTrue(
                name in log_names, msg="{} not found in log list".format(name))

    def test_log_names_uses_filter_fn(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )), ('ts_3', [2.]),
                            ('ts_4', [3.]))
        fake1 = create_test_workspace(
            ws_name='fake1', time_series_logs=time_series_logs)
        fit = FitInformation(mock.MagicMock(), 'func1', fake1.name(),
                             fake1.name())

        log_names = fit.log_names(lambda log: log.name == 'ts_1')
        self.assertEqual(1, len(log_names))
        self.assertEqual(time_series_logs[0][0], log_names[0])

    def test_has_log_returns_true_if_all_workspaces_have_the_log(self):
        time_series_logs = (('ts_1', (1., )), ('ts_2', (3., )))
        fake1 = create_test_workspace(
            ws_name='fake1', time_series_logs=time_series_logs)
        fake2 = create_test_workspace(
            ws_name='fake2', time_series_logs=time_series_logs)
        fit = FitInformation(mock.MagicMock(), 'func1',
                             [fake1.name(), fake2.name()], [fake1.name(), fake2.name()])

        self.assertTrue(fit.has_log('ts_1'))

    def test_has_log_returns_false_if_all_workspaces_do_not_have_log(self):
        time_series_logs = [('ts_1', (1., ))]
        fake1 = create_test_workspace(
            ws_name='fake1', time_series_logs=time_series_logs)
        fake2 = create_test_workspace(ws_name='fake2')
        fit = FitInformation(mock.MagicMock(), 'func1',
                             [fake1.name(), fake2.name()], [fake1.name(), fake2.name()])

        self.assertFalse(
            fit.has_log('ts_1'),
            msg='All input workspaces should have the requested log')

    def test_string_log_value_from_fit_with_single_workspace(self):
        single_value_logs = [('sv_1', '5')]
        fake1 = create_test_workspace(
            ws_name='fake1', string_value_logs=single_value_logs)
        fit = FitInformation(mock.MagicMock(), 'func1', [fake1.name()],
                             [fake1.name()])

        self.assertEqual(
            float(single_value_logs[0][1]),
            fit.log_value(single_value_logs[0][0]))

    def test_time_series_log_value_from_fit_with_single_workspace_uses_time_average(
            self):
        time_series_logs = \
            [('ts_1', (("2000-05-01T12:00:00", 5.),
             ("2000-05-01T12:00:10", 20.),
             ("2000-05-01T12:05:00", 30.)))]
        fake1 = create_test_workspace('fake1', time_series_logs)
        fit = FitInformation(mock.MagicMock(), 'func1', [fake1.name()], [fake1.name()], )

        time_average = (10 * 5 + 290 * 20) / 300.
        self.assertAlmostEqual(time_average, fit.log_value('ts_1'), places=6)

    def test_time_series_log_value_from_fit_with_multiple_workspaces_uses_average_of_time_average(
            self):
        time_series_logs1 = \
            [('ts_1', (("2000-05-01T12:00:00", 5.),
             ("2000-05-01T12:00:10", 20.),
             ("2000-05-01T12:05:00", 30.)))]
        fake1 = create_test_workspace('fake1', time_series_logs1)
        time_series_logs2 = \
            [('ts_1', (("2000-05-01T12:00:30", 10.),
             ("2000-05-01T12:01:45", 30.),
             ("2000-05-01T12:05:00", 40.)))]
        fake2 = create_test_workspace('fake2', time_series_logs2)
        fit = FitInformation(mock.MagicMock(), 'func1',
                             [fake1.name(), fake2.name()], [fake1.name(), fake2.name()])

        time_average1 = (10 * 5 + 290 * 20) / 300.
        time_average2 = (75 * 10 + 195 * 30) / 270.
        all_average = 0.5 * (time_average1 + time_average2)
        self.assertAlmostEqual(all_average, fit.log_value('ts_1'), places=6)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
