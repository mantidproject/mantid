# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import AlgoTimeRegister
import time

class AlgoTimeRegisterTest(unittest.TestCase):
    def test_addTime(self):
        start_time = time.perf_counter_ns()
        time.sleep(3.009)
        end_time = time.perf_counter_ns()
        AlgoTimeRegister.addTime("perf_test",start_time,end_time)
        self.assertTrue("D", "N")


if __name__ == "__main__":
    unittest.main()
#PYTHONPATH=/home/dwp/Projects/MantidSource/BUILD_PERF/bin python3 ../mantid/Framework/PythonInterface/test/python/mantid/api/AlgoRegisterTest.py AlgoTimeRegisterTest.test_addTime