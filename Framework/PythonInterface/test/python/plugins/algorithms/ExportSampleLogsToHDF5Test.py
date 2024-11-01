# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import h5py
import numpy as np
import os
import tempfile
import unittest

from mantid.kernel import (
    BoolTimeSeriesProperty,
    FloatTimeSeriesProperty,
    Int32TimeSeriesProperty,
    PropertyFactory,
    StringTimeSeriesProperty,
)
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

        test_alg = run_algorithm(self.ALG_NAME, InputWorkspace=input_ws, Filename=self.TEMP_FILE_NAME)

        self.assertTrue(test_alg.isExecuted())

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            self.assertTrue("Sample Logs" in output_file)
            logs_group = output_file["Sample Logs"]
            self.assertEqual(logs_group["Test1"][()], 1.0)
            self.assertEqual(logs_group["Test2"][()], b"Test2")

    def test_blacklistExcludesLogs(self):
        input_ws = self._create_sample_workspace()
        self._add_log_to_workspace(input_ws, "ToInclude", 1)
        self._add_log_to_workspace(input_ws, "ToExclude", 2)

        run_algorithm(self.ALG_NAME, InputWorkspace=input_ws, Filename=self.TEMP_FILE_NAME, BlackList="ToExclude")

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            logs_group = output_file["Sample Logs"]
            self.assertTrue("ToInclude" in logs_group)
            self.assertFalse("ToExclude" in logs_group)

    def test_timeSeriesAreTimeAveraged(self):
        input_ws = self._create_sample_workspace()
        VALUES = [1.0, 2.0, 3.0]
        self._add_log_to_workspace(input_ws, "TestLog", VALUES)
        run_algorithm(self.ALG_NAME, InputWorkspace=input_ws, Filename=self.TEMP_FILE_NAME)

        # time average mean is equal to simple mean
        # because the values are equally spaced
        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            logs_group = output_file["Sample Logs"]
            self.assertEqual(logs_group["TestLog"][()], np.mean(VALUES))

    def test_unitAreAddedIfPresent(self):
        input_ws = self._create_sample_workspace()
        mantid.AddSampleLog(Workspace=input_ws, LogName="TestLog", LogText="1", LogType="Number", LogUnit="uAmps")
        run_algorithm(self.ALG_NAME, InputWorkspace=input_ws, Filename=self.TEMP_FILE_NAME)

        with h5py.File(self.TEMP_FILE_NAME, "r") as output_file:
            logs_group = output_file["Sample Logs"]
            self.assertEqual(logs_group["TestLog"].attrs["Units"], "uAmps")

    def test_create_timeSeries(self):
        """Tests that the correct TimeSeriesProperty is returned when given a
        name and a list of values of a given type."""

        # Test for Int32TimeSeriesProperty
        int_log_name = "Int32Series"
        int_log_values = [1, 2, 3, 4, 5, 6, 7]
        int_prop = PropertyFactory.createTimeSeries(int_log_name, int_log_values)
        self.assertEqual(type(int_prop), Int32TimeSeriesProperty)

        # Test for BoolTimeSeriesProperty
        bool_log_name = "BoolSeries"
        bool_log_values = [True, False, False, True, False]
        bool_prop = PropertyFactory.createTimeSeries(bool_log_name, bool_log_values)
        self.assertEqual(type(bool_prop), BoolTimeSeriesProperty)

        # Test for StringSeriesProperty
        str_log_name = "StringSeries"
        str_log_values = ["Testing", "string", "time", "series", "property"]
        str_prop = PropertyFactory.createTimeSeries(str_log_name, str_log_values)
        self.assertEqual(type(str_prop), StringTimeSeriesProperty)

        # Test for FloatTimeSeriesProperty
        float_log_name = "FloatSeries"
        float_log_values = [1.0, 2.1, 3.2, 4.3, 5.6, 6.7, 7.8]
        float_prop = PropertyFactory.createTimeSeries(float_log_name, float_log_values)
        self.assertTrue(type(float_prop), FloatTimeSeriesProperty)

    def _add_log_to_workspace(self, ws, log_name, log_value):
        if isinstance(log_value, list):
            ws.mutableRun()[log_name] = self._create_time_series_log(log_name, log_value)

        else:
            mantid.AddSampleLog(
                Workspace=ws, LogName=log_name, LogText=str(log_value), LogType="String" if isinstance(log_value, str) else "Number"
            )

    def _create_sample_workspace(self):
        x = np.array([1.0, 2.0, 3.0, 4.0])
        y = np.array([1.0, 2.0, 3.0])
        e = np.sqrt(y)

        ws = mantid.CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1, UnitX="TOF", OutputWorkspace=self.TEST_WS_NAME)
        return ws

    def _create_time_series_log(self, log_name, log_value):
        prop = PropertyFactory.createTimeSeries(log_name, log_value)
        for i, value in enumerate(log_value):
            prop.addValue(i, value)

        return prop


if __name__ == "__main__":
    unittest.main()
