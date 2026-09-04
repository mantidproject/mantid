# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import mtd
from mantid.simpleapi import AddTimeSeriesLog, CloneWorkspace, CreateSampleWorkspace, GroupWorkspaces
from mantid.dataobjects import Workspace2D
from testhelpers import assertRaisesNothing, create_algorithm


class ReflectometrySliceEventWorkspaceV2Test(unittest.TestCase):
    def setUp(self):
        self.__class__._input_ws = self._create_test_workspace()
        self.__class__._input_ws_group = self._create_test_workspace_group()
        self.__class__._monitor_ws = self._create_monitor_workspace()
        self.__class__._monitor_ws_group = self._create_monitor_workspace_group()
        self._default_args = {"InputWorkspaceName": "input_ws", "MonitorWorkspaceName": "monitor_ws", "OutputWorkspaceName": "output"}

    def tearDown(self):
        mtd.clear()

    def test_missing_input_workspace(self):
        self._assert_run_algorithm_throws()

    def test_missing_monitors(self):
        self._assert_run_algorithm_throws({"InputWorkspaceName": "input_ws"})

    def test_missing_output_ws(self):
        self._assert_run_algorithm_throws({"InputWorkspaceName": "input_ws", "MonitorWorkspaceName": "monitor_ws"})

    def test_default_inputs_return_single_slice(self):
        output = self._assert_run_algorithm_succeeds(self._default_args)
        self._check_slices(output, ["output_0_4200"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[14, 16, 8])

    def test_setting_time_interval(self):
        args = self._default_args
        args["TimeInterval"] = 600
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(
            output,
            [
                "output_0_600",
                "output_600_1200",
                "output_1200_1800",
                "output_1800_2400",
                "output_2400_3000",
                "output_3000_3600",
                "output_3600_4200",
            ],
        )
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[2, 6, 1])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[2, 3, 2])
        self._check_y(output, child=2, spec=3, expected_bins=101, expected_values=[0, 3, 0])
        self._check_y(output, child=3, spec=3, expected_bins=101, expected_values=[4, 2, 2])
        self._check_y(output, child=4, spec=3, expected_bins=101, expected_values=[4, 1, 2])
        self._check_y(output, child=5, spec=3, expected_bins=101, expected_values=[2, 1, 1])
        self._check_y(output, child=6, spec=3, expected_bins=101, expected_values=[0, 0, 0])

    def test_setting_time_interval_and_limits(self):
        args = self._default_args
        args["TimeInterval"] = 600
        args["StartTime"] = "1800"
        args["StopTime"] = "3300"
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_1800_2400", "output_2400_3000", "output_3000_3300"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[4, 2, 2])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[4, 1, 2])
        self._check_y(output, child=2, spec=3, expected_bins=101, expected_values=[1, 1, 0])

    def test_setting_multiple_time_intervals(self):
        args = self._default_args
        args["TimeInterval"] = "600, 1200"
        args["StopTime"] = "3600"
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_0_600", "output_600_1800", "output_1800_2400", "output_2400_3600"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[2, 6, 1])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[2, 6, 2])
        self._check_y(output, child=2, spec=3, expected_bins=101, expected_values=[4, 2, 2])
        self._check_y(output, child=3, spec=3, expected_bins=101, expected_values=[6, 2, 3])

    def test_setting_log_interval_without_log_name_produces_single_slice(self):
        args = self._default_args
        args["LogValueInterval"] = 600
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_0_4200"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[14, 16, 8])

    def test_setting_log_interval_without_log_name_produces_single_slice_FilterByLogValue(self):
        args = self._default_args
        args["LogValueInterval"] = 600
        args["FilterByLogValue"] = True
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_0_4200"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[14, 16, 8])

    def test_setting_log_interval(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["LogValueInterval"] = 20
        output = self._assert_run_algorithm_succeeds(args)
        # Note that default tolerance is half the interval, so we slice +/-10 either side.
        # Also note that empty slices are not included in the output.
        self._check_slices(
            output,
            [
                "output_Log.proton_charge.From.10.To.30.Value-change-direction:both",
                "output_Log.proton_charge.From.70.To.90.Value-change-direction:both",
                "output_Log.proton_charge.From.90.To.110.Value-change-direction:both",
            ],
        )
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[4, 1, 2])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[4, 5, 2])
        self._check_y(output, child=2, spec=3, expected_bins=101, expected_values=[6, 10, 4])

    def test_setting_log_tolerance(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["LogValueInterval"] = 20
        # Set tolerance to zero to give similar behaviour to FilterByLogValue, although
        # note that empty slices are not output so we have fewer workspaces.
        args["LogValueTolerance"] = 0
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(
            output,
            [
                "output_Log.proton_charge.From.0.To.20.Value-change-direction:both",
                "output_Log.proton_charge.From.80.To.100.Value-change-direction:both",
            ],
        )
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[4, 1, 2])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[4, 5, 2])

    def test_setting_log_interval_FilterByLogValue(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["LogValueInterval"] = 20
        args["MinimumLogValue"] = 0
        args["MaximumLogValue"] = 100
        args["FilterByLogValue"] = True
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_0.0_20.0", "output_20.0_40.0", "output_40.0_60.0", "output_60.0_80.0", "output_80.0_100.0"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[0, 0, 0])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[0, 0, 0])
        self._check_y(output, child=2, spec=3, expected_bins=101, expected_values=[0, 0, 0])
        self._check_y(output, child=3, spec=3, expected_bins=101, expected_values=[0, 3, 0])
        self._check_y(output, child=4, spec=3, expected_bins=101, expected_values=[4, 12, 3])

    def test_setting_log_without_interval_produces_single_slice(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_Log.proton_charge.From.0.To.100.Value-change-direction:both"])
        # Note that the min/max log value are 0->100 (taken from the sample logs) but this is
        # exclusive of values at 100 so excludes quite a few counts
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[8, 6, 4])

    def test_setting_log_limits_without_interval_produces_single_slice(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["MinimumLogValue"] = 0
        args["MaximumLogValue"] = 101
        output = self._assert_run_algorithm_succeeds(args)
        # We set the max to be over 100 to be inclusive of the values up to 100 so this includes all
        # of the counts from the input workspace
        self._check_slices(output, ["output_Log.proton_charge.From.0.To.101.Value-change-direction:both"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[14, 16, 8])

    def test_setting_log_limits_without_interval_produces_single_slice_FilterByLogValue(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["MinimumLogValue"] = 0
        args["MaximumLogValue"] = 101
        args["FilterByLogValue"] = True
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_0.0_101.0"])
        # These values don't seem right - I think they should contain all the counts from the
        # input workspace, i.e. [14, 16, 8]. Adding this test though to confirm the current
        # behaviour so we can check against it if we fix this in future. We may be phasing this
        # algorithm out though so this is not currently a high priority.
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[12, 15, 7])

    def test_setting_log_interval_and_limits(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["LogValueInterval"] = 20
        args["MinimumLogValue"] = "75"
        args["MaximumLogValue"] = "110"
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(
            output,
            [
                "output_Log.proton_charge.From.65.To.85.Value-change-direction:both",
                "output_Log.proton_charge.From.85.To.105.Value-change-direction:both",
            ],
        )
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[4, 5, 2])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[6, 10, 4])

    def test_setting_log_interval_and_limits_FilterByLogValue(self):
        args = self._default_args
        args["LogName"] = "proton_charge"
        args["LogValueInterval"] = 20
        args["MinimumLogValue"] = "75"
        args["MaximumLogValue"] = "110"
        args["FilterByLogValue"] = True
        output = self._assert_run_algorithm_succeeds(args)
        self._check_slices(output, ["output_75.0_95.0", "output_95.0_115.0"])
        self._check_y(output, child=0, spec=3, expected_bins=101, expected_values=[0, 3, 0])
        self._check_y(output, child=1, spec=3, expected_bins=101, expected_values=[2, 6, 1])

    def test_when_input_is_a_workspace_group(self):
        args = self._default_args
        time_interval = 600
        args["TimeInterval"] = time_interval
        args["InputWorkspaceName"] = "input_ws_group"
        self._assert_run_algorithm_succeeds(args, len(mtd["input_ws_group"]))
        self._check_group(time_interval, ["ws1", "ws2", "ws3"], ["monitor_ws"] * 3)

    def test_when_input_and_monitors_are_both_workspace_groups(self):
        args = self._default_args
        time_interval = 600
        args["TimeInterval"] = time_interval
        args["InputWorkspaceName"] = "input_ws_group"
        args["MonitorWorkspaceName"] = "monitor_ws_group"
        self._assert_run_algorithm_succeeds(args, len(mtd["input_ws_group"]))
        self._check_group(time_interval, ["ws1", "ws2", "ws3"], ["mon1", "mon2", "mon3"])

    def test_fails_when_input_groups_are_different_sizes(self):
        self._create_monitor_workspace_group_with_two_members()
        args = self._default_args
        args["TimeInterval"] = 600
        args["InputWorkspaceName"] = "input_ws_group"
        args["MonitorWorkspaceName"] = "test_monitor_ws_group"
        self._assert_run_algorithm_throws(args)
        mtd.remove("test_monitor_ws_group")

    def test_validation_fails_when_workspace_has_zero_counts(self):
        input_ws = Workspace2D()
        mtd.addOrReplace("test_ws", input_ws)
        args = self._default_args
        args["InputWorkspaceName"] = input_ws.name()
        self._assert_run_algorithm_throws(args)

    def _create_test_workspace(self):
        input_ws = CreateSampleWorkspace("Event", BankPixelWidth=1, BinWidth=20000)
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
        mtd.addOrReplace("ws1", ws1)
        mtd.addOrReplace("ws2", ws2)
        mtd.addOrReplace("ws3", ws3)
        group = GroupWorkspaces(InputWorkspaces="ws1,ws2,ws3")
        mtd.addOrReplace("input_ws_group", group)
        return group

    def _create_monitor_workspace(self):
        monitor_ws = CreateSampleWorkspace(
            OutputWorkspace="monitor_ws", NumBanks=0, NumMonitors=3, BankPixelWidth=1, NumEvents=10000, Random=False
        )
        return monitor_ws

    def _create_monitor_workspace_group(self):
        mon1 = CloneWorkspace(self.__class__._monitor_ws)
        mon2 = CloneWorkspace(self.__class__._monitor_ws)
        mon3 = CloneWorkspace(self.__class__._monitor_ws)
        mtd.addOrReplace("mon1", mon1)
        mtd.addOrReplace("mon2", mon2)
        mtd.addOrReplace("mon3", mon3)
        group = GroupWorkspaces(InputWorkspaces="mon1,mon2,mon3")
        mtd.addOrReplace("monitor_ws_group", group)
        return group

    def _create_monitor_workspace_group_with_two_members(self):
        testmon1 = CloneWorkspace(self.__class__._monitor_ws)
        testmon2 = CloneWorkspace(self.__class__._monitor_ws)
        mtd.addOrReplace("testmon1", testmon1)
        mtd.addOrReplace("testmon2", testmon2)
        group = GroupWorkspaces(InputWorkspaces="testmon1,testmon2")
        mtd.addOrReplace("test_monitor_ws_group", group)
        return group

    """
    Does not return the output workspaces when groups have been used as the input.
    The resulting groups need to be selected manually in the main test methods.
    """

    def _assert_run_algorithm_succeeds(self, args, is_group=False):
        """Run the algorithm with the given args and check it succeeds"""
        alg = create_algorithm("ReflectometrySliceEventWorkspace", **args)
        assertRaisesNothing(self, alg.execute)
        if is_group:
            return
        self.assertEqual(mtd.doesExist("output"), True)
        return mtd["output"]

    def _assert_run_algorithm_fails(self, args):
        """Run the algorithm with the given args and check it fails to produce output"""
        alg = create_algorithm("ReflectometrySliceEventWorkspace", **args)
        assertRaisesNothing(self, alg.execute)
        self.assertEqual(mtd.doesExist("output"), False)

    def _assert_run_algorithm_throws(self, args={}):
        """Run the algorithm with the given args and check it throws"""
        throws = False
        alg = create_algorithm("ReflectometrySliceEventWorkspace", **args)
        try:
            alg.execute()
        except:
            throws = True
        self.assertEqual(throws, True)

    def _check_group(self, time_interval, expected_ws_names, expected_monitor_names):
        expected_values_set = [[2, 6, 1], [2, 3, 2], [0, 3, 0], [4, 2, 2], [4, 1, 2], [2, 1, 1], [0, 0, 0]]
        # Check for the 3 workspaces contained in each slice group.
        for i, expected_values in zip(range(7), expected_values_set):
            self.assertTrue(mtd.doesExist(f"output_{i * time_interval}_{(i + 1) * time_interval}"))
            slice_group = mtd.retrieve(f"output_{i * time_interval}_{(i + 1) * time_interval}")
            self.assertEqual(slice_group.getNumberOfEntries(), 3)
            self._check_slices(
                slice_group,
                [
                    f"{expected_ws_names[0]}_{expected_monitor_names[0]}_output_0_{i * time_interval}_{(i + 1) * time_interval}",
                    f"{expected_ws_names[1]}_{expected_monitor_names[1]}_output_1_{i * time_interval}_{(i + 1) * time_interval}",
                    f"{expected_ws_names[2]}_{expected_monitor_names[2]}_output_2_{i * time_interval}_{(i + 1) * time_interval}",
                ],
            )
            self._check_y(slice_group, child=0, spec=3, expected_bins=101, expected_values=expected_values)

    def _check_slices(self, workspace_group, expected_names):
        number_of_slices = workspace_group.getNumberOfEntries()
        self.assertEqual(number_of_slices, len(expected_names))
        for child in range(number_of_slices):
            self.assertEqual(workspace_group[child].name(), expected_names[child])

    def _check_y(self, workspace_group, child, spec, expected_bins, expected_values):
        """Check Y values for bins 0, 51 and 99 match the list of expected values"""
        ws = workspace_group[child]
        self.assertEqual(ws.x(spec).size, expected_bins)
        self._assert_delta(ws.y(spec)[0], expected_values[0])
        self._assert_delta(ws.y(spec)[51], expected_values[1])
        self._assert_delta(ws.y(spec)[99], expected_values[2])

    def _assert_delta(self, value1, value2):
        self.assertEqual(round(value1, 6), round(value2, 6))


if __name__ == "__main__":
    unittest.main()
