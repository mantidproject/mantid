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

class AlgoTimeRegisterTest(unittest.TestCase):

    PER_FILE = tempfile.NamedTemporaryFile()

    def test_addTime(self):
        performance_config = {"performancelog.write": "On", "performancelog.filename": self.PER_FILE.name}
        with amend_config(**performance_config):
            print(self.PER_FILE.name)
            ws = CreateSampleWorkspace()
            print(performance_config)

            start_time = time.time_ns()
            time.sleep(3.009)
            end_time = time.time_ns()
            duration = end_time-start_time
            print("Python Duration 1: ",duration)
            AlgoTimeRegister.addTime("perf_test",start_time,end_time)


            start_time = time.time_ns()
            time.sleep(1.345)
            end_time = time.time_ns()
            duration = end_time-start_time
            print("Python Duration 2: ",duration)
            AlgoTimeRegister.addTime("perf_test",start_time,end_time)


            start_time = time.time_ns()
            time.sleep(2.543)
            end_time = time.time_ns()
            duration = end_time-start_time
            print("Python Duration 3: ",duration)
            AlgoTimeRegister.addTime("perf_test",start_time,end_time)


            start_time = time.time_ns()
            time.sleep(4.11100)
            end_time = time.time_ns()
            duration = end_time-start_time
            print("Python Duration 4: ",duration)
            AlgoTimeRegister.addTime("perf_test",start_time,end_time)

            print("end")
            # with open(self.PER_FILE.name,'r') as file:
            #     for line in file:
            #         print(line.strip())
if __name__ == "__main__":
    unittest.main()