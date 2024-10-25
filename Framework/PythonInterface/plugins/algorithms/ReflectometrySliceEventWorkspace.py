# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, WorkspaceGroupProperty
from mantid.kernel import DateAndTime, Direction
from mantid.simpleapi import AddSampleLog


class ReflectometrySliceEventWorkspace(DataProcessorAlgorithm):
    def category(self):
        return "Reflectometry"

    def name(self):
        return "ReflectometrySliceEventWorkspace"

    def summary(self):
        return "Split an input workspace into multiple slices according to time or log values"

    def seeAlso(self):
        return ["GenerateEventsFilter", "FilterEvents", "ReflectometryReductionOneAuto"]

    def PyInit(self):
        # Add properties from child algorithm
        self._filter_properties = [
            "InputWorkspace",
            "StartTime",
            "StopTime",
            "TimeInterval",
            "LogName",
            "MinimumLogValue",
            "MaximumLogValue",
            "LogValueInterval",
            "LogBoundary",
            "LogValueTolerance",
        ]
        self.copyProperties("GenerateEventsFilter", self._filter_properties)

        # Add our own properties
        self.declareProperty(MatrixWorkspaceProperty("MonitorWorkspace", "", direction=Direction.Input), "Input monitor workspace")
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Group name for the output workspace(s)."
        )
        self.declareProperty("UseNewFilterAlgorithm", True, doc="If true, use the new FilterEvents algorithm instead of FilterByTime.")

    def validateInputs(self):
        issues = {}
        workspace = self.getProperty("InputWorkspace").value
        # Skip check for workspace groups
        if not workspace:
            return issues
        if workspace.run().getProtonCharge() < 1e-9:
            issues["InputWorkspace"] = "Cannot slice workspace with zero proton charge"
        return issues

    def PyExec(self):
        self._input_ws = self.getProperty("InputWorkspace").value
        self._output_ws_group_name = self.getPropertyValue("OutputWorkspace")

        output_ws_group = self._slice_input_workspace()
        self._scale_monitors_for_each_slice(output_ws_group)
        output_ws_group = self._rebin_to_monitors()
        output_ws_group = self._add_monitors_to_sliced_output()

        self.setProperty("OutputWorkspace", self._output_ws_group_name)
        self._clean_up()

    def _slice_input_workspace(self):
        if self.getProperty("UseNewFilterAlgorithm").value:
            return self._slice_input_workspace_with_filter_events()
        elif self._slice_by_log():
            return self._slice_input_workspace_with_filter_by_log_value()
        else:
            return self._slice_input_workspace_with_filter_by_time()

    def _slice_by_log(self):
        """Return true if we are slicing by log value"""
        return self._property_set("LogName")

    def _property_set(self, property_name):
        """Return true if the given property is set"""
        return not self.getProperty(property_name).isDefault

    def _slice_input_workspace_with_filter_events(self):
        """Perform the slicing of the input workspace"""
        self._create_filter()
        alg = self.createChildAlgorithm("FilterEvents")
        alg.setProperty("InputWorkspace", self._input_ws)
        alg.setProperty("SplitterWorkspace", self._split_ws)
        alg.setProperty("InformationWorkspace", self._info_ws)
        alg.setProperty("OutputWorkspaceBaseName", self._output_ws_group_name)
        alg.setProperty("GroupWorkspaces", True)
        alg.setProperty("FilterByPulseTime", False)
        alg.setProperty("OutputWorkspaceIndexedFrom1", True)
        alg.setProperty("CorrectionToSample", "None")
        alg.setProperty("SpectrumWithoutDetector", "Skip")
        alg.setProperty("SplitSampleLogs", False)
        alg.setProperty("OutputTOFCorrectionWorkspace", "__mock")
        alg.setProperty("ExcludeSpecifiedLogs", False)
        alg.setProperty("TimeSeriesPropertyLogs", "proton_charge")
        alg.setProperty("DescriptiveOutputNames", True)
        alg.execute()
        # Ensure the run number for the child workspaces is stored in the
        # sample logs as a string (FilterEvents converts it to a double).
        group = mtd[self._output_ws_group_name]
        for ws in group:
            self._copy_run_number_to_sample_log(ws, ws)
        return group

    def _create_filter(self):
        """Generate the splitter workspace for performing the filtering for each required slice"""
        alg = self.createChildAlgorithm("GenerateEventsFilter")
        for property_name in self._filter_properties:
            alg.setProperty(property_name, self.getPropertyValue(property_name))
        alg.setProperty("OutputWorkspace", "__split")
        alg.setProperty("InformationWorkspace", "__info")
        alg.execute()
        self._split_ws = alg.getProperty("OutputWorkspace").value
        self._info_ws = alg.getProperty("InformationWorkspace").value

    def _slice_input_workspace_with_filter_by_time(self):
        # Get the start/stop times, or use the run start/stop times if they are not provided
        run_start = DateAndTime(self._input_ws.run().startTime())
        run_stop = DateAndTime(self._input_ws.run().endTime())
        start_time = self._get_property_or_default_as_datetime("StartTime", default_value=run_start, relative_start=run_start)
        stop_time = self._get_property_or_default_as_datetime("StopTime", default_value=run_stop, relative_start=run_start)
        # Get the time interval, or use the total interval if it's not provided
        total_interval = (stop_time - start_time).total_seconds()
        time_interval = self._get_interval_as_float("TimeInterval", total_interval)
        # Calculate start/stop times in seconds relative to the start of the run
        relative_start_time = (start_time - run_start).total_seconds()
        relative_stop_time = relative_start_time + total_interval
        # Loop through each slice
        slice_names = list()
        slice_start_time = relative_start_time
        while slice_start_time < relative_stop_time:
            slice_stop_time = slice_start_time + time_interval
            slice_name = self._output_ws_group_name + "_" + str(slice_start_time) + "_" + str(slice_stop_time)
            slice_names.append(slice_name)
            alg = self.createChildAlgorithm("FilterByTime")
            alg.setProperty("InputWorkspace", self._input_ws)
            alg.setProperty("OutputWorkspace", slice_name)
            alg.setProperty("StartTime", str(slice_start_time))
            alg.setProperty("StopTime", str(slice_stop_time))
            alg.execute()
            sliced_workspace = alg.getProperty("OutputWorkspace").value
            mtd.addOrReplace(slice_name, sliced_workspace)
            # Proceed to the next interval
            slice_start_time = slice_stop_time
        # Group the sliced workspaces
        group = self._group_workspaces(slice_names, self._output_ws_group_name)
        mtd.addOrReplace(self._output_ws_group_name, group)
        # Ensure the run number for the child workspaces is stored in the
        # sample logs as a string (FilterEvents converts it to a double).
        for ws in group:
            self._copy_run_number_to_sample_log(ws, ws)
        return group

    def _slice_input_workspace_with_filter_by_log_value(self):
        # Get the min/max log value, or use the values from the sample logs if they're not provided
        log_name = self.getProperty("LogName").value
        run_log_start = min(self._input_ws.run().getProperty(log_name).value)
        run_log_stop = max(self._input_ws.run().getProperty(log_name).value)
        log_min = self._get_property_or_default("MinimumLogValue", run_log_start)
        log_max = self._get_property_or_default("MaximumLogValue", run_log_stop)
        log_interval = self._get_interval_as_float("LogValueInterval", log_max - log_min)
        slice_names = list()
        slice_start_value = log_min
        while slice_start_value < log_max:
            slice_stop_value = slice_start_value + log_interval
            slice_name = self._output_ws_group_name + "_" + str(slice_start_value) + "_" + str(slice_stop_value)
            slice_names.append(slice_name)
            alg = self.createChildAlgorithm("FilterByLogValue")
            alg.setProperty("InputWorkspace", self._input_ws)
            alg.setProperty("OutputWorkspace", slice_name)
            alg.setProperty("LogName", log_name)
            alg.setProperty("LogBoundary", self.getProperty("LogBoundary").value)
            alg.setProperty("MinimumValue", slice_start_value)
            alg.setProperty("MaximumValue", slice_stop_value)
            alg.execute()
            sliced_workspace = alg.getProperty("OutputWorkspace").value
            mtd.addOrReplace(slice_name, sliced_workspace)
            # Proceed to the next interval
            slice_start_value = slice_stop_value
        # Group the sliced workspaces
        group = self._group_workspaces(slice_names, self._output_ws_group_name)
        mtd.addOrReplace(self._output_ws_group_name, group)
        # Ensure the run number for the child workspaces is stored in the
        # sample logs as a string (FilterEvents converts it to a double).
        for ws in group:
            self._copy_run_number_to_sample_log(ws, ws)
        return group

    def _scale_monitors_for_each_slice(self, sliced_ws_group):
        """Create a group workspace which contains a copy of the monitors workspace for
        each slice, scaled by the relative proton charge for that slice"""
        input_monitor_ws = self.getProperty("MonitorWorkspace").value
        total_proton_charge = self._total_proton_charge()
        monitors_ws_list = []
        i = 1
        for slice in sliced_ws_group:
            slice_monitor_ws_name = input_monitor_ws.name() + "_" + str(i)
            slice_monitor_ws = self._clone_workspace(input_monitor_ws, slice_monitor_ws_name)
            scale_factor = slice.run().getProtonCharge() / total_proton_charge
            slice_monitor_ws = self._scale_workspace(slice_monitor_ws, slice_monitor_ws_name, scale_factor)
            # The workspace must be in the ADS for grouping and updating the sample log
            mtd.addOrReplace(slice_monitor_ws_name, slice_monitor_ws)
            monitors_ws_list.append(slice_monitor_ws_name)
            self._copy_run_number_to_sample_log(slice, slice_monitor_ws)
            i += 1

        self._monitor_ws_group_name = input_monitor_ws.name() + "_sliced"
        self._monitor_ws_group = self._group_workspaces(monitors_ws_list, self._monitor_ws_group_name)
        mtd.addOrReplace(self._monitor_ws_group_name, self._monitor_ws_group)

    def _clone_workspace(self, ws_to_clone, output_ws_name):
        alg = self.createChildAlgorithm("CloneWorkspace")
        alg.setProperty("InputWorkspace", ws_to_clone)
        alg.setProperty("OutputWorkspace", output_ws_name)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _scale_workspace(self, ws_to_scale, output_ws_name, scale_factor):
        alg = self.createChildAlgorithm("Scale")
        alg.setProperty("InputWorkspace", ws_to_scale)
        alg.setProperty("OutputWorkspace", output_ws_name)
        alg.setProperty("Factor", scale_factor)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _group_workspaces(self, ws_list, output_ws_name):
        alg = self.createChildAlgorithm("GroupWorkspaces")
        alg.setProperty("InputWorkspaces", ws_list)
        alg.setProperty("OutputWorkspace", output_ws_name)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _total_proton_charge(self):
        """Get the proton charge for the input workspace"""
        return self._input_ws.run().getProtonCharge()

    def _rebin_to_monitors(self):
        """Rebin the output workspace group to the monitors workspace group"""
        alg = self.createChildAlgorithm("RebinToWorkspace")
        alg.setProperty("WorkspaceToRebin", self._output_ws_group_name)
        alg.setProperty("WorkspaceToMatch", self._monitor_ws_group_name)
        alg.setProperty("OutputWorkspace", self._output_ws_group_name)
        alg.setProperty("PreserveEvents", False)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _add_monitors_to_sliced_output(self):
        """Add the monitors for each slice to the output workspace for each slice"""
        alg = self.createChildAlgorithm("AppendSpectra")
        alg.setProperty("InputWorkspace1", self._monitor_ws_group_name)
        alg.setProperty("InputWorkspace2", self._output_ws_group_name)
        alg.setProperty("MergeLogs", False)
        alg.setProperty("OutputWorkspace", self._output_ws_group_name)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _clean_up(self):
        """Remove worspaces added to the ADS"""
        monitor_ws_names = [ws.name() for ws in self._monitor_ws_group]
        alg = self.createChildAlgorithm("UnGroupWorkspace")
        alg.setProperty("InputWorkspace", self._monitor_ws_group_name)
        alg.execute()
        for ws_name in monitor_ws_names:
            mtd.remove(ws_name)

    def _get_property_or_default(self, property_name, default_value):
        """Get a property value. Return the given default value if the property is not set."""
        if self.getProperty(property_name).isDefault:
            return default_value
        else:
            return self.getProperty(property_name).value

    def _get_property_or_default_as_datetime(self, property_name, default_value, relative_start):
        """Get a property value as a DateAndTime. Return the given default value if the property is not set.
        If the property is in datetime format, return it directly. Otherwise if it is in seconds, then convert
        it to a datetime by adding it to the given relative_start time."""
        if self.getProperty(property_name).isDefault:
            return default_value
        else:
            value = self.getProperty(property_name).value
            try:
                result = DateAndTime(value)
            except:
                value_ns = int(value) * 1000000000
                result = relative_start + value_ns
            return result

    def _copy_run_number_to_sample_log(self, ws_with_run_number, ws_to_update):
        if ws_with_run_number.run().hasProperty("run_number"):
            run_number = int(ws_with_run_number.run()["run_number"].value)
            AddSampleLog(Workspace=ws_to_update, LogName="run_number", LogType="String", LogText=str(run_number))

    def _get_interval_as_float(self, property_name, default_value):
        """Get an interval property value (could be time interval or log value interval)
        as a float. Checks if the user has entered a list of floats and for now throws
        if this is the case (this is only used in backwards compatibility mode and multiple
        intervals are not currently supported in that mode)"""
        if self.getProperty(property_name).isDefault:
            return float(default_value)
        value_as_string = self.getPropertyValue(property_name)
        value_as_list = value_as_string.split(",")
        if len(value_as_list) > 1:
            raise RuntimeError("Multiple intervals are not currently supported if UseNewFilterAlgorithm is False")
        if len(value_as_list) < 1:
            raise RuntimeError("Interval was not specified")
        return float(value_as_list[0])


AlgorithmFactory.subscribe(ReflectometrySliceEventWorkspace())
