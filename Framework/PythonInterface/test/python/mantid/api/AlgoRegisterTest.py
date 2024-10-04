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
from mantid.api import AlgoTimeRegister
from mantid.simpleapi import CreateSampleWorkspace
import threading


def get_recorded_timedata(filename):
    with open(filename, "r") as f:
        timeentry = f.readlines()[-1].replace("\n","").split(",")
        recorded_start_time = recorded_end_time = -1
        entry_name = ""
        print("time entry", timeentry)

        for value in timeentry:
            timedata = value.strip()
            if timedata.startswith("StartTime"):
                recorded_start_time = int(timedata.replace("StartTime=",""))
            if timedata.startswith("EndTime"):
                recorded_end_time = int(timedata.replace("EndTime=",""))
            if timedata.startswith("AlgorithmName"):
                entry_name = timedata.replace("AlgorithmName=","")
            if timedata.startswith("ThreadID"):
                thread_id = int(timedata.replace("ThreadID=",""))
            if (recorded_start_time!= -1 and recorded_end_time !=-1 and entry_name!="" and thread_id!=""):
                break
    return recorded_start_time, recorded_end_time, entry_name, thread_id

class AlgoTimeRegisterTest(unittest.TestCase):

    PER_FILE = tempfile.NamedTemporaryFile()

    def test_addTime_timeentry1(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            entry_name = "test_addTime_timeentry1"
            start_time = time.time_ns()
            time.sleep(3.009)
            end_time = time.time_ns()
            duration = end_time-start_time
            AlgoTimeRegister.addTime(entry_name,start_time,end_time)
            rec_start_time,rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id,threading.get_ident())

    def test_addTime_timeentry2(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            entry_name = "test_addTime_timeentry2"
            start_time = time.time_ns()
            time.sleep(1.345)
            end_time = time.time_ns()
            duration = end_time-start_time
            AlgoTimeRegister.addTime(entry_name,start_time,end_time)
            rec_start_time,rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id,threading.get_ident())

    def ttest_addTime_timeentry3(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            entry_name = "test_addTime_timeentry3"
            start_time = time.time_ns()
            time.sleep(2.543)
            end_time = time.time_ns()
            duration = end_time-start_time
            AlgoTimeRegister.addTime(entry_name,start_time,end_time)
            rec_start_time,rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_start_time, rec_start_time)
            self.assertEqual(end_time, end_time)
            self.assertEqual(rec_thread_id,threading.get_ident())

    def test_addTime_timeentry4(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            start_time = time.time_ns()
            entry_name = "test_addTime_timeentry4"
            time.sleep(4.11100)
            end_time = time.time_ns()
            duration = end_time-start_time
            AlgoTimeRegister.addTime(entry_name,start_time,end_time)
            rec_start_time,rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(rec_start_time, rec_start_time)
            self.assertEqual(end_time, end_time)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id,threading.get_ident())

    def test_addTime_algo(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            entry_name = "CreateSampleWorkspace"
            # algotimeregister is initialized through algorithm class
            ws = CreateSampleWorkspace()
            rec_start_time,rec_end_time, rec_entry_name, rec_thread_id = get_recorded_timedata(self.PER_FILE.name)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_thread_id,threading.get_ident())

    def test_addTime_threading(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            # call the above algorithm from another thread to verfy the thread entry is correct
            x = threading.Thread(target=self.test_addTime_algo)
            x.start()
            x.join()


if __name__ == "__main__":
    unittest.main()