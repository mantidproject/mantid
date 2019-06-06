# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from testhelpers import (assertRaisesNothing, create_algorithm)


class ReflectometrySliceEventWorkspace(unittest.TestCase):
    def setUp(self):
        self.__class__._input_ws = self._create_test_workspace()
        self.__class__._input_ws_group = self._create_test_workspace_group()
        self.__class__._monitor_ws = self._create_monitor_workspace()
        self.__class__._monitor_ws_group = self._create_monitor_workspace_group()
        self._default_args = {
            'InputWorkspace' : 'input_ws',
            'MonitorWorkspace' : 'monitor_ws',
            'OutputWorkspace': 'output'
        }

    def tearDown(self):
        mtd.clear()

    def test_missing_input_workspace(self):
        self._assert_run_algorithm_throws()

    def test_missing_monitors(self):
        self._assert_run_algorithm_throws({'InputWorkspace' : 'input_ws'})

    def test_missing_output_ws(self):
        self._assert_run_algorithm_throws({'InputWorkspace' : 'input_ws',
                                           'MonitorWorkspace' : 'monitor_ws'})

    def test_default_inputs_return_single_slice(self):
        output = self._assert_run_algorithm_succeeds(self._default_args)
        self.assertEqual(output.getNumberOfEntries(), 1)
        first_slice = output[0]
        self.assertEqual(first_slice.getNumberHistograms(), 5)
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 14)
        self._assert_delta(first_slice.dataY(3)[51], 16)
        self._assert_delta(first_slice.dataY(3)[99], 8)

    def test_setting_time_interval(self):
        args = self._default_args
        args['TimeInterval'] = 600
        output = self._assert_run_algorithm_succeeds(args)
        self.assertEqual(output.getNumberOfEntries(), 7)
        first_slice = output[0]
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 2)
        self._assert_delta(first_slice.dataY(3)[51], 6)
        self._assert_delta(first_slice.dataY(3)[99], 1)

    def test_setting_time_interval_and_limits(self):
        args = self._default_args
        args['TimeInterval'] = 600
        args['StartTime'] = '1800'
        args['StopTime'] = '3300'
        output = self._assert_run_algorithm_succeeds(args)
        self.assertEqual(output.getNumberOfEntries(), 3)
        first_slice = output[0]
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 4)
        self._assert_delta(first_slice.dataY(3)[51], 2)
        self._assert_delta(first_slice.dataY(3)[99], 2)

    def test_setting_log_interval_without_log_name_produces_single_slice(self):
        args = self._default_args
        args['LogValueInterval'] = 600
        output = self._assert_run_algorithm_succeeds(args)
        self.assertEqual(output.getNumberOfEntries(), 1)
        first_slice = output[0]
        self.assertEqual(first_slice.getNumberHistograms(), 5)
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 14)
        self._assert_delta(first_slice.dataY(3)[51], 16)
        self._assert_delta(first_slice.dataY(3)[99], 8)

    def test_setting_log_interval_and_limits(self):
        args = self._default_args
        args['LogName'] = 'proton_charge'
        args['LogValueInterval'] = 20
        args['MinimumLogValue'] = '75'
        args['MaximumLogValue'] = '110'
        output = self._assert_run_algorithm_succeeds(args)
        self.assertEqual(output.getNumberOfEntries(), 2)
        first_slice = output[0]
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 4)
        self._assert_delta(first_slice.dataY(3)[51], 5)
        self._assert_delta(first_slice.dataY(3)[99], 2)

    def test_when_input_is_a_workspace_group(self):
        args = self._default_args
        args['TimeInterval'] = 600
        args['InputWorkspace'] = 'input_ws_group'
        output = self._assert_run_algorithm_succeeds(args)
        self.assertEqual(output.getNumberOfEntries(), 3)
        first_subgroup = output[0]
        self.assertEqual(first_subgroup.getNumberOfEntries(), 7)
        first_slice = first_subgroup[0]
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 2)
        self._assert_delta(first_slice.dataY(3)[51], 6)
        self._assert_delta(first_slice.dataY(3)[99], 1)

    def test_when_input_and_monitors_are_both_workspace_groups(self):
        args = self._default_args
        args['TimeInterval'] = 600
        args['InputWorkspace'] = 'input_ws_group'
        args['MonitorWorkspace'] = 'monitor_ws_group'
        output = self._assert_run_algorithm_succeeds(args)
        self.assertEqual(output.getNumberOfEntries(), 3)
        first_subgroup = output[0]
        self.assertEqual(first_subgroup.getNumberOfEntries(), 7)
        first_slice = first_subgroup[0]
        self.assertEqual(first_slice.dataX(0).size, 101)
        self._assert_delta(first_slice.dataY(3)[0], 2)
        self._assert_delta(first_slice.dataY(3)[51], 6)
        self._assert_delta(first_slice.dataY(3)[99], 1)

    def test_fails_when_input_groups_are_different_sizes(self):
        group = self._create_monitor_workspace_group_with_two_members()
        args = self._default_args
        args['TimeInterval'] = 600
        args['InputWorkspace'] = 'input_ws_group'
        args['MonitorWorkspace'] = 'test_monitor_ws_group'
        self._assert_run_algorithm_fails(args)
        mtd.remove('test_monitor_ws_group')

    def _create_test_workspace(self):
        input_ws = CreateSampleWorkspace("Event",BankPixelWidth=1, BinWidth=20000)
        AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
        AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
        AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
        AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
        AddTimeSeriesLog(input_ws, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)
        return input_ws

    def _create_test_workspace_group(self):
        ws1 = CloneWorkspace(self.__class__._input_ws)
        ws2 = CloneWorkspace(self.__class__._input_ws)
        ws3 = CloneWorkspace(self.__class__._input_ws)
        mtd.addOrReplace('ws1', ws1)
        mtd.addOrReplace('ws2', ws2)
        mtd.addOrReplace('ws3', ws3)
        group = GroupWorkspaces(InputWorkspaces='ws1,ws2,ws3')
        mtd.addOrReplace('input_ws_group', group)
        return group

    def _create_monitor_workspace(self):
        monitor_ws = CreateSampleWorkspace(OutputWorkspace='monitor_ws', NumBanks=0, NumMonitors=3,
                                           BankPixelWidth=1, NumEvents=10000, Random=True)
        return monitor_ws

    def _create_monitor_workspace_group(self):
        mon1 = CloneWorkspace(self.__class__._monitor_ws)
        mon2 = CloneWorkspace(self.__class__._monitor_ws)
        mon3 = CloneWorkspace(self.__class__._monitor_ws)
        mtd.addOrReplace('mon1', mon1)
        mtd.addOrReplace('mon2', mon2)
        mtd.addOrReplace('mon3', mon3)
        group = GroupWorkspaces(InputWorkspaces='mon1,mon2,mon3')
        mtd.addOrReplace('monitor_ws_group', group)
        return group

    def _create_monitor_workspace_group_with_two_members(self):
        testmon1 = CloneWorkspace(self.__class__._monitor_ws)
        testmon2 = CloneWorkspace(self.__class__._monitor_ws)
        mtd.addOrReplace('testmon1', testmon1)
        mtd.addOrReplace('testmon2', testmon2)
        group = GroupWorkspaces(InputWorkspaces='testmon1,testmon2')
        mtd.addOrReplace('test_monitor_ws_group', group)
        return group

    def _assert_run_algorithm_succeeds(self, args):
        """Run the algorithm with the given args and check it succeeds"""
        alg = create_algorithm('ReflectometrySliceEventWorkspace', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertEqual(mtd.doesExist('output'), True)
        return mtd['output']

    def _assert_run_algorithm_fails(self, args):
        """Run the algorithm with the given args and check it fails to produce output"""
        alg = create_algorithm('ReflectometrySliceEventWorkspace', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertEqual(mtd.doesExist('output'), False)

    def _assert_run_algorithm_throws(self, args = {}):
        """Run the algorithm with the given args and check it throws"""
        throws = False
        alg = create_algorithm('ReflectometrySliceEventWorkspace', **args)
        try:
            alg.execute()
        except:
            throws = True
        self.assertEqual(throws, True)

    def _assert_delta(self, value1, value2):
        self.assertEqual(round(value1, 6), round(value2, 6))


if __name__ == '__main__':
    unittest.main()
