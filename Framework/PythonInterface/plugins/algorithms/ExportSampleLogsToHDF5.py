from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.kernel import *
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
        self.declareProperty(MatrixWorkspaceProperty(name=self.PROP_INPUT_WS, defaultValue="",
                                                     direction=Direction.Input),
                             doc="The workspace containing sample logs to be exported")

        self.declareProperty(StringArrayProperty(name=self.PROP_BLACKLIST, direction=Direction.Input),
                             doc="A list of any sample logs that should not be included in the HDF5 file")

        self.declareProperty(FileProperty(name=self.PROP_FILENAME, defaultValue="", action=FileAction.Save,
                                          extensions=[".hdf5", ".h5", ".hdf"]), doc="HDF5 file to save to")

    def PyExec(self):
        output_file_name = self.getProperty(self.PROP_FILENAME).value

        with h5py.File(output_file_name, "a") as output_file:
            if self.LOGS_GROUP_NAME in output_file:
                del output_file[self.LOGS_GROUP_NAME]

            sample_logs_group = output_file.create_group(self.LOGS_GROUP_NAME)

            input_ws = self.getProperty(self.PROP_INPUT_WS).value
            run = input_ws.run()
            blacklist = self.getProperty(self.PROP_BLACKLIST).value

            log_properties = [prop for prop in run.getProperties()
                              if prop.name not in blacklist and not self._ignore_property(prop)]

            for log_property in log_properties:
                property_dtype = self._dtype_from_property_type(log_property)

                if self._is_time_series(log_property):
                    log_value = log_property.timeAverageValue()
                else:
                    log_value = log_property.value

                log_dataset = sample_logs_group.create_dataset(name=log_property.name, shape=(1,), dtype=property_dtype,
                                                               data=[log_value])
                log_dataset.attrs["Units"] = log_property.units

    def _dtype_from_property_type(self, prop):
        if isinstance(prop, (FloatPropertyWithValue, FloatTimeSeriesProperty)):
            return "f"
        if isinstance(prop, (IntPropertyWithValue, Int32TimeSeriesProperty, Int64TimeSeriesProperty)):
            return "i"
        if isinstance(prop, (BoolPropertyWithValue, BoolTimeSeriesProperty)):
            return "b"
        if isinstance(prop, StringPropertyWithValue):
            return "S{}".format(len(prop.value))
        raise RuntimeError("Unrecognised property type: \"{}\". Please contact the development team with this message".
                           format(prop.type))

    def _ignore_property(self, prop):
        return isinstance(prop, StringTimeSeriesProperty) or prop.value == ""

    def _is_time_series(self, prop):
        return isinstance(prop, (FloatTimeSeriesProperty, Int32TimeSeriesProperty, Int64TimeSeriesProperty,
                                 BoolTimeSeriesProperty, StringTimeSeriesProperty))

AlgorithmFactory.subscribe(ExportSampleLogsToHDF5)
