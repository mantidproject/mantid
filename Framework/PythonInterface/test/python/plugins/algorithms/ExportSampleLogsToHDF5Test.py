from __future__ import (absolute_import, division, print_function)

import h5py
import numpy as np
import os
import tempfile
import unittest

from mantid.kernel import *
import mantid.simpleapi as mantid
from testhelpers import run_algorithm


class ExportSampleLogsToHDF5Test(unittest.TestCase):

    ALG_NAME = "ExportSampleLogsToHDF5"
    TEST_WS_NAME = "SampleWorkspace"
    TEMP_FILE_NAME = os.path.join(tempfile.gettempdir(), "ExportSampleLogsToHDF5Test.hdf5")

    def tearDown(self):
        try:
            os.remove(self.TEMP_FILE_NAME)

        except OSError:
            pass

        if mantid.mtd.doesExist(self.TEST_WS_NAME):
            mantid.mtd.remove(self.TEST_WS_NAME)
            
    def test_saveFileWithSingleValueProperties(self):
        input_ws = self._create_sample_workspace()
        self._add_log_to_workspace(input_ws, "Test1", 1.0)
        self._add_log_to_workspace(input_ws, "Test2", "Test2")

        test_alg = run_algorithm(self.ALG_NAME,
                                 InputWorkspace=input_ws,
                                 Filename=self.TEMP_FILE_NAME)

        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Sample Logs" in output_file)
            logs_group = output_file["Sample Logs"]
            self.assertEquals(logs_group["Test1"].value, 1.0)
            self.assertEquals(logs_group["Test2"].value[0], b"Test2")

    def test_blacklistExcludesLogs(self):
        input_ws = self._create_sample_workspace()
        self._add_log_to_workspace(input_ws, "ToInclude", 1)
        self._add_log_to_workspace(input_ws, "ToExclude", 2)

        run_algorithm(self.ALG_NAME,
                      InputWorkspace=input_ws,
                      Filename=self.TEMP_FILE_NAME,
                      BlackList="ToExclude")

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            logs_group = output_file["Sample Logs"]
            self.assertTrue("ToInclude" in logs_group)
            self.assertFalse("ToExclude" in logs_group)

    def test_timeSeriesAreTimeAveraged(self):
        input_ws = self._create_sample_workspace()
        self._add_log_to_workspace(input_ws, "TestLog", [1.0, 2.0, 3.0])
        run_algorithm(self.ALG_NAME,
                      InputWorkspace=input_ws,
                      Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            logs_group = output_file["Sample Logs"]
            self.assertEquals(logs_group["TestLog"].value, 1.5)

    def test_unitAreAddedIfPresent(self):
        input_ws = self._create_sample_workspace()
        mantid.AddSampleLog(Workspace=input_ws, LogName="TestLog", LogText="1",
                            LogType="Number", LogUnit="uAmps")
        run_algorithm(self.ALG_NAME, InputWorkspace=input_ws, Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            logs_group = output_file["Sample Logs"]
            self.assertEquals(logs_group["TestLog"].attrs["Units"], "uAmps")

    def _add_log_to_workspace(self, ws, log_name, log_value):
        if isinstance(log_value, list):
            ws.mutableRun()[log_name] = self._create_time_series_log(log_name, log_value)

        else:
            mantid.AddSampleLog(Workspace=ws, LogName=log_name, LogText=str(log_value),
                                LogType="String" if isinstance(log_value, str) else "Number")

    def _create_sample_workspace(self):
        x = np.array([1.0, 2.0, 3.0, 4.0])
        y = np.array([1.0, 2.0, 3.0])
        e = np.sqrt(y)

        ws = mantid.CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="TOF",
                                    OutputWorkspace=self.TEST_WS_NAME)
        return ws

    def _create_time_series_log(self, log_name, log_value):
        prop = PropertyWithValueFactory.create(log_name, log_value)
        
        """
        if isinstance(log_value[0], int):
            prop = Int32TimeSeriesProperty(log_name)
            pass
        elif isinstance(log_value[0], float):
            prop = FloatTimeSeriesProperty(log_name)
            pass
        elif isinstance(log_value[0], str):
            prop = StringTimeSeriesProperty(log_name)
        elif isinstance(log_value[0], bool):
            prop = BoolTimeSeriesProperty(log_name)
        else:
            raise RuntimeError("Unsupported property type {}".format(type(log_value[0])))
         """

        for i, value in enumerate(log_value):
            prop.addValue(i, value)
        
        return prop

if __name__ == "__main__":
    unittest.main()