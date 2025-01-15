# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import time
import os

from mantid.kernel import amend_config
from mantid.simpleapi import CreateSampleWorkspace, Max
import threading
from mantid.api import AlgoTimeRegister


def get_recorded_timedata(filename):
    with open(filename, "r") as f:
        lines = f.readlines()
        timeentry = lines[-1].replace("\n", "").split(",")
        recorded_start_time = recorded_end_time = -1
        entry_name = ""
        for value in timeentry:
            timedata = value.strip()
            if timedata.startswith("StartTime"):
                recorded_start_time = int(timedata.replace("StartTime=", ""))
            if timedata.startswith("EndTime"):
                recorded_end_time = int(timedata.replace("EndTime=", ""))
            if timedata.startswith("AlgorithmName"):
                entry_name = timedata.replace("AlgorithmName=", "")
            if timedata.startswith("ThreadID"):
                thread_id = int(timedata.replace("ThreadID=", ""))
            if recorded_start_time != -1 and recorded_end_time != -1 and entry_name != "" and thread_id != "":
                break
    return recorded_start_time, recorded_end_time, entry_name, thread_id


def get_perf_counter_ns() -> int:
    return time.perf_counter_ns()


class AlgoTimeRegisterAddTimeTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # performance log file
        f = tempfile.NamedTemporaryFile(delete=False)
        cls.PER_FILE = f.name
        f.close()
        # start clock
        cls.start_point = -1
        # performance configuration
        cls.performance_config = {"performancelog.write": "On", "performancelog.filename": cls.PER_FILE}

    @classmethod
    def tearDownClass(cls):
        if os.path.exists(cls.PER_FILE):
            os.remove(cls.PER_FILE)

    def setUp(self):
        with amend_config(**self.performance_config):
            AlgoTimeRegister.Instance()
            # add 1st time to read the start clock from the file
            AlgoTimeRegister.addTime("1st entry", get_perf_counter_ns(), get_perf_counter_ns())
            # based on the format `START_POINT: <start point number> MAX_THREAD: <thread number>`
            with open(self.PER_FILE, "r") as f:
                lines = f.readlines()
                timeentry = lines[0].split(" ")
                self.start_point = int(timeentry[1])

    def test_addTime_timeentry1(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "test_addTime_timeentry1"
            start_time = get_perf_counter_ns()
            time.sleep(3.009)
            end_time = get_perf_counter_ns()
            duration = end_time - start_time

            # add time
            AlgoTimeRegister.addTime(entry_name, start_time, end_time)

            # get time data from file
            rec_start_time, rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)
            rec_duration = rec_end_time - rec_start_time

            # check the values
            self.assertEqual(start_time, self.start_point + rec_start_time)
            self.assertEqual(end_time, self.start_point + rec_end_time)
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

    def test_addTime_timeentry2(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "test_addTime_timeentry2"
            start_time = get_perf_counter_ns()
            time.sleep(1.345)
            end_time = get_perf_counter_ns()
            duration = end_time - start_time

            # it should ignore this
            AlgoTimeRegister.Instance()

            # add time
            AlgoTimeRegister.addTime(entry_name, start_time, end_time)

            # get time data from file
            rec_start_time, rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)
            rec_duration = rec_end_time - rec_start_time

            # check the values
            self.assertEqual(start_time, self.start_point + rec_start_time)
            self.assertEqual(end_time, self.start_point + rec_end_time)
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

    def test_addTime_timeentry3(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "test_addTime_timeentry3"
            start_time = get_perf_counter_ns()
            time.sleep(2.543)
            end_time = get_perf_counter_ns()
            duration = end_time - start_time

            # add time
            AlgoTimeRegister.addTime(entry_name, start_time, end_time)

            # get time data from file
            rec_start_time, rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)
            rec_duration = rec_end_time - rec_start_time

            # check the values
            self.assertEqual(start_time, self.start_point + rec_start_time)
            self.assertEqual(end_time, self.start_point + rec_end_time)
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

    def test_addTime_timeentry4(self):
        with amend_config(**self.performance_config):
            # time entry
            start_time = get_perf_counter_ns()
            entry_name = "test_addTime_timeentry4"
            time.sleep(4.11100)
            end_time = get_perf_counter_ns()
            duration = end_time - start_time

            # add time
            AlgoTimeRegister.addTime(entry_name, start_time, end_time)

            # get time data from file
            rec_start_time, rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)
            rec_duration = rec_end_time - rec_start_time

            # check the values
            self.assertEqual(start_time, self.start_point + rec_start_time)
            self.assertEqual(end_time, self.start_point + rec_end_time)
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

    def test_addTime_algo(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "CreateSampleWorkspace"
            # call algorithm
            CreateSampleWorkspace(OutputWorkspace="ws")

            # it should ignore this
            AlgoTimeRegister.Instance()

            # get time data from file
            _, _, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)

            # check the values
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

    def test_addTime_threading(self):
        with amend_config(**self.performance_config):
            # time entry
            # call the above algorithm from another thread to verfy the thread entry is correct
            x = threading.Thread(target=self.test_addTime_algo)
            x.start()
            x.join()


class AlgoTimeRegisterStartTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # performance log file
        f = tempfile.NamedTemporaryFile(delete=False)
        cls.PER_FILE = f.name
        f.close()
        # start clock
        cls.start_point = -1
        # performance configuration
        cls.performance_config = {"performancelog.write": "On", "performancelog.filename": cls.PER_FILE}

    @classmethod
    def tearDownClass(cls):
        if os.path.exists(cls.PER_FILE):
            os.remove(cls.PER_FILE)

    def setUp(self):
        # do not initialize algotimeregister explicit
        with amend_config(**self.performance_config):
            # clock start here
            CreateSampleWorkspace(Function="Multiple Peaks", OutputWorkspace="ws")
            # based on the format `START_POINT: <start point number> MAX_THREAD: <thread number>`
            with open(self.PER_FILE, "r") as f:
                timeentry = f.readlines()[0].split(" ")
                self.start_point = int(timeentry[1])

    def test_starttime_workspace(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "CreateSampleWorkspace"

            # call algorithm, internally it calls AlgoTimeRegister.addTime
            ws = CreateSampleWorkspace(Function="Multiple Peaks")

            # get time data from file
            rec_sample_start_time, rec_sample_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)

            # check values
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

            # it should ignore this and not restart the clock!
            AlgoTimeRegister.Instance()

            entry_name = "Max"
            Max(ws, OutputWorkspace="max_ws")

            # get time data from file
            rec_max_start_time, rec_max_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)

            # check the values
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

            self.assertLess(rec_sample_start_time, rec_max_start_time)
        self.assertLess(rec_sample_end_time, rec_max_end_time)

    def test_multiple_workspaces_times(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "CreateSampleWorkspace"

            # call the algorithm
            ws = CreateSampleWorkspace(Function="Multiple Peaks")

            # get time data from file
            rec_sample_start_time, rec_sample_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)

            # check the values
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

            entry_name = "Max"

            # call the algorithm
            Max(ws, OutputWorkspace="max_ws")

            # get time data from file
            rec_max_start_time, rec_max_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)

            # check the values
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

            self.assertLess(rec_sample_start_time, rec_max_start_time)
            self.assertLess(rec_sample_end_time, rec_max_end_time)

    def test_addtime_workspaces(self):
        with amend_config(**self.performance_config):
            # time entry
            entry_name = "CreateSampleWorkspace"

            # call the algorithm
            CreateSampleWorkspace(Function="Multiple Peaks", OutputWorkspace="ws")

            # get time data from file
            _, _, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)

            # check the values
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())

            # time entry
            entry_name = "test_addtime_workspaces"
            start_time = get_perf_counter_ns()
            time.sleep(2.01)
            end_time = get_perf_counter_ns()
            duration = end_time - start_time

            # add time
            AlgoTimeRegister.addTime(entry_name, start_time, end_time)

            # get time data from file
            rec_start_time, rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE)
            rec_duration = rec_end_time - rec_start_time

            # check the values
            self.assertEqual(start_time, self.start_point + rec_start_time)
            self.assertEqual(end_time, self.start_point + rec_end_time)
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id, threading.get_ident())


if __name__ == "__main__":
    unittest.main()
