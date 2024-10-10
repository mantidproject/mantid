# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileAction, FileProperty, MatrixWorkspaceProperty, PythonAlgorithm
from mantid.kernel import (
    Direction,
    FloatArrayProperty,
    FloatTimeSeriesProperty,
    Int32TimeSeriesProperty,
    Int64TimeSeriesProperty,
    BoolTimeSeriesProperty,
    StringPropertyWithValue,
    StringTimeSeriesProperty,
    StringArrayProperty,
)
import h5py


class ExportSampleLogsToHDF5(PythonAlgorithm):
    PROP_INPUT_WS = "InputWorkspace"
    PROP_BLACKLIST = "Blacklist"
    PROP_FILENAME = "Filename"

    LOGS_GROUP_NAME = "Sample Logs"

    def category(self):
        return "DataHandling\\Logs"

    def name(self):
        return "ExportSampleLogsToHDF5"

    def summary(self):
        return "Export a workspace's sample logs to HDF5 format"

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(name=self.PROP_INPUT_WS, defaultValue="", direction=Direction.Input),
            doc="The workspace containing sample logs to be exported",
        )

        self.declareProperty(
            StringArrayProperty(name=self.PROP_BLACKLIST, direction=Direction.Input),
            doc="A list of any sample logs that should not be included in the HDF5 file",
        )

        self.declareProperty(
            FileProperty(name=self.PROP_FILENAME, defaultValue="", action=FileAction.Save, extensions=[".hdf5", ".h5", ".hdf"]),
            doc="HDF5 file to save to",
        )

    def PyExec(self):
        output_file_name = self.getProperty(self.PROP_FILENAME).value

        with h5py.File(output_file_name, "a") as output_file:
            if self.LOGS_GROUP_NAME in output_file:
                del output_file[self.LOGS_GROUP_NAME]

            sample_logs_group = output_file.create_group(self.LOGS_GROUP_NAME)

            input_ws = self.getProperty(self.PROP_INPUT_WS).value
            run = input_ws.run()
            blacklist = self.getProperty(self.PROP_BLACKLIST).value

            log_properties = [prop for prop in run.getProperties() if prop.name not in blacklist and not self._ignore_property(prop)]

            for log_property in log_properties:
                property_dtype = log_property.dtype()
                log_value = self._get_value_from_property(log_property)
                if log_value is None:
                    continue

                log_dataset = sample_logs_group.create_dataset(name=log_property.name, shape=(1,), dtype=property_dtype, data=[log_value])
                log_dataset.attrs["Units"] = log_property.units

    def _get_value_from_property(self, prop):
        if isinstance(prop, FloatArrayProperty):
            if len(prop.value) > 0:
                return prop.value[0]
            else:
                # The array is empty, so just skip it
                return None
        elif self._is_time_series(prop):
            return prop.timeAverageValue()
        else:
            value = prop.value
            if isinstance(value, str):
                # For h5py and Python 3 - h5py doesn't get on well with Unicode strings
                value = value.encode()
            return value

    def _ignore_property(self, prop):
        # Skip StringTimeSeriesProperty, as time-averaging them means nothing, and also skip empty strings
        return isinstance(prop, StringTimeSeriesProperty) or (isinstance(prop, StringPropertyWithValue) and prop.value == "")

    def _is_time_series(self, prop):
        return isinstance(
            prop,
            (FloatTimeSeriesProperty, Int32TimeSeriesProperty, Int64TimeSeriesProperty, BoolTimeSeriesProperty, StringTimeSeriesProperty),
        )


AlgorithmFactory.subscribe(ExportSampleLogsToHDF5)
