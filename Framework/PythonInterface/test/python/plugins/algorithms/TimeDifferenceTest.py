# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from enum import IntEnum
from mantid.api import AnalysisDataService
from mantid.kernel import DateAndTime
from mantid.simpleapi import TimeDifference, CreateWorkspace, GroupWorkspaces, CreateEmptyTableWorkspace
import numpy as np
import unittest

ISO_TIME_STAMP = "2025-06-12T08:00:00.000000000"
TEST_NAME = "test"
GROUP_NAME = "group"
REFERENCE_NAME = "reference"
# Conversion factor between nanoseconds and seconds
NANO_FACTOR = 1000000000
# Conversion factor between seconds and hours
HOUR_FACTOR = 3600
# 1 second
DEFAULT_DURATION = 1


class TimeTable(IntEnum):
    ws_name = 0
    time_stamp = 1
    seconds = 2
    seconds_error = 3
    hours = 4
    hours_error = 5


def _add_time_log_to_workspace(ws_name: str = TEST_NAME, time: str = ISO_TIME_STAMP, duration: int = DEFAULT_DURATION):
    """
    Start and End Time logs.
    We are going to use native setStartAndEndtime method that uses the C++ DateAndTime class as a parameter,
    adding the start_time and end_time logs to the workspace run.
    This class accepts an iso time stamp, and overloads the addition operator so that a time delta
    can be added as nanoseconds.
    :param ws_name: Name of the workspace in which time logs are added.
    :param time: Time stamp. Reference for start time log.
    :param duration: Duration of experiment. Used to define end time log.

    """
    start_time = DateAndTime(time)
    end_time = DateAndTime(start_time + int(duration * NANO_FACTOR))
    AnalysisDataService.retrieve(ws_name).getRun().setStartAndEndTime(start_time, end_time)


def _correct_midtimes(time_stamps, duration=DEFAULT_DURATION):
    """
    This correction is done for the test data, as TimeDifference algorithm will retrieve the time stamps
    of the middle of the run.
    :param time_stamps: Middle of the run time stamps
    :param duration: Set duration of each experiment, by default is 1 second
    :return: Corrected time stamps
    """
    return [time + np.timedelta64(int(duration / 2 * NANO_FACTOR), "ns") for time in time_stamps]


def _prepare_workspaces(start_times, unit: str = "s"):
    names = []
    times = []
    t0 = np.datetime64(ISO_TIME_STAMP)
    for t in start_times:
        names.append(f"{TEST_NAME}_{t}")
        times.append(t0 + np.timedelta64(t, unit))
        CreateWorkspace(OutputWorkspace=names[-1], DataX=0, DataY=1)
        _add_time_log_to_workspace(ws_name=names[-1], time=str(times[-1]))
    return names, times


class TimeDifferenceTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        AnalysisDataService.clear()

    def test_time_difference_throws_for_incorrect_workspace_entry(self):
        CreateEmptyTableWorkspace(OutputWorkspace="test_table")
        with self.assertRaisesRegex(RuntimeError, "Workspace test_table is not a Group or Matrix Workspace."):
            _ = TimeDifference(InputWorkspaces="test_table")

    def test_algorithm_creates_NaT_if_time_logs_are_not_present(self):
        CreateWorkspace(OutputWorkspace=TEST_NAME, DataX=0, DataY=1)

        table = TimeDifference(InputWorkspaces=TEST_NAME)

        # NaT : Not a Time
        self.assertEqual(["NaT"], table.column(TimeTable.time_stamp))

    def test_time_difference_uses_first_workspace_as_reference_if_not_provided(self):
        start_times = [0, 1, 2, 3]
        names, times = _prepare_workspaces(start_times)

        table = TimeDifference(InputWorkspaces=names)

        self._assert_table(table, names, _correct_midtimes(times), start_times)

    def test_time_difference_takes_run_logs_and_any_other_available_allowed_time_log(self):
        start_logs = ["run_start", "start_time"]
        end_logs = ["run_end", "end_time"]

        for start in start_logs:
            for end in end_logs:
                CreateWorkspace(OutputWorkspace=TEST_NAME, DataX=0, DataY=1)
                run = AnalysisDataService.retrieve(TEST_NAME).getRun()
                run.addProperty(name=start, value=ISO_TIME_STAMP, replace=False)
                run.addProperty(name=end, value=ISO_TIME_STAMP.replace("00.", "02."), replace=False)

                table = TimeDifference(InputWorkspaces=TEST_NAME)

                midtime = ISO_TIME_STAMP.replace("00.", "01.")
                self.assertEqual([midtime], table.column(1))
                AnalysisDataService.clear()

    def test_time_difference_puts_reference_workspace_on_top_of_result_table(self):
        start_times = [0, 1, 2, 3]
        reference = 5
        # Create Ref Workspace
        ref_time = np.datetime64(ISO_TIME_STAMP) - np.timedelta64(reference, "s")
        CreateWorkspace(OutputWorkspace=REFERENCE_NAME, DataX=0, DataY=1)
        _add_time_log_to_workspace(ws_name=REFERENCE_NAME, time=str(ref_time))
        names, times = _prepare_workspaces(start_times, "s")

        table = TimeDifference(InputWorkspaces=names, ReferenceWorkspace=AnalysisDataService.retrieve(REFERENCE_NAME))

        start_times = [d + reference for d in start_times]
        start_times.insert(0, 0)
        times.insert(0, ref_time)
        names.insert(0, REFERENCE_NAME)
        self._assert_table(table, names, _correct_midtimes(times), start_times)

    def test_time_difference_takes_group_workspace_midtime(self):
        start_times = [0, 1, 2, 3]
        names, _ = _prepare_workspaces(start_times)
        GroupWorkspaces(names, OutputWorkspace=GROUP_NAME)
        table = TimeDifference(InputWorkspaces="group")

        # Notice every experiment lasts for 1 seconds by default, so final time is at 4 seconds
        # and midtime is 2 seconds.
        midtime = ISO_TIME_STAMP.replace("00.", "02.")
        self.assertEqual([str(midtime)], table.column(TimeTable.time_stamp))

    def test_time_difference_can_take_a_list_of_matrix_workspaces_and_groups(self):
        start_times = [0, 1, 2, 3]
        start_times_group = [4, 5]
        names, times = _prepare_workspaces(start_times)
        names_group, times_group = _prepare_workspaces(start_times_group)
        GroupWorkspaces(names_group, OutputWorkspace=GROUP_NAME)

        # Add group to names lists
        names.append(GROUP_NAME)

        table = TimeDifference(InputWorkspaces=names)

        # Add test data for group column
        errors = [DEFAULT_DURATION * 2] * len(start_times)
        # Group workspace duration is 2 seconds in this test, so the error is 3(duration + reference duration)
        errors.append(3)
        times = _correct_midtimes(times)
        times.append(times_group[-1])
        # Midtime of reference is 0.5 second and midtime of group is 5 seconds.
        start_times.append(4.5)

        self._assert_table(table, names, times, start_times, errors)

    def test_time_difference_works_with_nano_second_precision(self):
        # second run will start 100 nanoseconds later.
        start_times = [0, 100]
        names, times = _prepare_workspaces(start_times, "ns")

        table = TimeDifference(InputWorkspaces=names)

        # convert initial times back to seconds to compare results in table
        start_times = [time / NANO_FACTOR for time in start_times]
        self._assert_table(table, names, _correct_midtimes(times), start_times)

    def _assert_table(self, table, names, time_stamps, start_times, default_errors=None):
        if default_errors is None:
            default_errors = [DEFAULT_DURATION * 2] * len(start_times)

        all_columns = {
            0: names,
            1: [str(time) for time in time_stamps],
            2: start_times,
            3: default_errors,
            4: [delta / HOUR_FACTOR for delta in start_times],
            5: [err / HOUR_FACTOR for err in default_errors],
        }
        types = table.columnTypes()
        for column_number, column_values in all_columns.items():
            match types[column_number]:
                case "str":
                    self.assertEqual(column_values, table.column(column_number))
                case "float":
                    np.testing.assert_almost_equal(column_values, table.column(column_number), 4)


if __name__ == "__main__":
    unittest.main()
