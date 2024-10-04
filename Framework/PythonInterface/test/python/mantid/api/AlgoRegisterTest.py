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



def get_recorded_timedata(filename):
    with open(filename, "r") as f:
        timeentry = f.readlines()[-1].replace("\n","").split(",")
        recorded_start_time = recorded_end_time = -1
        entry_name = ""
        for value in timeentry:
            timedata = value.strip()
            if timedata.startswith("StartTime"):
                recorded_start_time = int(timedata.replace("StartTime=",""))
            if timedata.startswith("EndTime"):
                recorded_end_time = int(timedata.replace("EndTime=",""))
            if timedata.startswith("AlgorithmName"):
                entry_name = timedata.replace("AlgorithmName=","")
            if (recorded_start_time!= -1 and recorded_end_time !=-1 and entry_name!=""):
                break
    return recorded_start_time, recorded_end_time, entry_name

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
            print("durartion", duration)
            AlgoTimeRegister.addTime(entry_name,start_time,end_time)
            rec_start_time,rec_end_time, rec_entry_name = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)


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
            rec_start_time,rec_end_time, rec_entry_name = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)

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
            rec_start_time,rec_end_time, rec_entry_name = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(entry_name, rec_entry_name)
            self.assertEqual(rec_start_time, rec_start_time)
            self.assertEqual(end_time, end_time)

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
            rec_start_time,rec_end_time, rec_entry_name = get_recorded_timedata(self.PER_FILE.name)
            rec_duration = rec_end_time - rec_start_time
            self.assertEqual(duration, rec_duration)
            self.assertEqual(rec_start_time, rec_start_time)
            self.assertEqual(end_time, end_time)
            self.assertEqual(entry_name, rec_entry_name)

    def test_addTime_algo(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):

            # time entry
            entry_name = "CreateSampleWorkspace"
            # algotimeregister is initialized through algorithm class
            ws = CreateSampleWorkspace()
            rec_start_time,rec_end_time, rec_entry_name = get_recorded_timedata(self.PER_FILE.name)
            self.assertEqual(entry_name, rec_entry_name)


if __name__ == "__main__":
    unittest.main()