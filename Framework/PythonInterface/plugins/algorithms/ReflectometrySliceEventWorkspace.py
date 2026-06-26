# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, WorkspaceGroupProperty, WorkspaceProperty
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
        self.declareProperty(WorkspaceProperty("InputWorkspace", "", direction=Direction.Input), "An input event workspace.")

        self._filter_properties = [
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
        self.declareProperty(WorkspaceProperty("MonitorWorkspace", "", direction=Direction.Input), "Input monitor workspace")
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Group name for the output workspace(s)."
        )
        self.declareProperty("UseNewFilterAlgorithm", True, doc="If true, use the new FilterEvents algorithm instead of FilterByTime.")

    def validateInputs(self):
        issues = {}
        workspace = self.getProperty("InputWorkspace").value

        if workspace.isGroup():
            return self._validate_group_inputs(workspace, issues)

        # Skip check for workspace groups
        if not workspace:
            return issues
        if workspace.run().getProtonCharge() < 1e-9:
            issues["InputWorkspace"] = "Cannot slice workspace with zero proton charge"
        return issues

    def _validate_group_inputs(self, ws_group, issues_dict: dict):
        monitors = self.getProperty("MonitorWorkspace").value
        if monitors.isGroup() and not len(monitors) != len(ws_group):
            issues_dict["InputWorkspace"] = "Monitor and Input workspace groups must be the same length."

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        input_monitor_ws = self.getProperty("MonitorWorkspace").value
        self._output_ws_group_name = self.getPropertyValue("OutputWorkspace")

        output_ws_group = self._slice_input_workspace(input_ws)
        monitor_ws_group = self._scale_monitors_for_each_slice(input_monitor_ws, input_ws, output_ws_group)
        # This step is done in-place.
        self._rebin_to_monitors(output_ws_group, monitor_ws_group)
        self._add_monitors_to_sliced_output(output_ws_group, monitor_ws_group)
        self.setProperty("OutputWorkspace", output_ws_group)
        self._clean_up(monitor_ws_group)

    def _slice_input_workspace(self, input_ws):
        if self.getProperty("UseNewFilterAlgorithm").value:
            return self._slice_input_workspace_with_filter_events(input_ws)
        elif self._slice_by_log():
            return self._slice_input_workspace_with_filter_by_log_value(input_ws)
        else:
            return self._slice_input_workspace_with_filter_by_time(input_ws)

    def _slice_by_log(self):
        """Return true if we are slicing by log value"""
        return self._property_set("LogName")

    def _property_set(self, property_name):
        """Return true if the given property is set"""
        return not self.getProperty(property_name).isDefault

    def _slice_input_workspace_with_filter_events(self, input_ws, output_suffix=""):
        """Perform the slicing of the input workspace"""
        split_ws, info_ws = self._create_filter(input_ws)
        output_group_name = self._output_ws_group_name + output_suffix
        alg = self.createChildAlgorithm("FilterEvents")
        alg.setProperty("InputWorkspace", input_ws)
        alg.setProperty("SplitterWorkspace", split_ws)
        alg.setProperty("InformationWorkspace", info_ws)
        alg.setProperty("OutputWorkspaceBaseName", output_group_name)
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
        group = alg.getProperty("OutputWorkspace").value
        for ws in group:
            self._copy_run_number_to_sample_log(ws, ws)
        return group

    def _create_filter(self, input_ws):
        """Generate the splitter workspace for performing the filtering for each required slice"""
        alg = self.createChildAlgorithm("GenerateEventsFilter")
        for property_name in self._filter_properties:
            alg.setProperty(property_name, self.getPropertyValue(property_name))
        alg.setProperty("OutputWorkspace", "__split")
        alg.setProperty("InformationWorkspace", "__info")
        alg.setProperty("InputWorkspace", input_ws)
        alg.execute()
        _split_ws = alg.getProperty("OutputWorkspace").value
        _info_ws = alg.getProperty("InformationWorkspace").value
        return _split_ws, _info_ws

    def _slice_input_workspace_with_filter_by_time(self, input_ws):
        # Get the start/stop times, or use the run start/stop times if they are not provided
        run_start = DateAndTime(input_ws.run().startTime())
        run_stop = DateAndTime(input_ws.run().endTime())
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
            alg.setProperty("InputWorkspace", input_ws)
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

    def _slice_input_workspace_with_filter_by_log_value(self, input_ws):
        # Get the min/max log value, or use the values from the sample logs if they're not provided
        log_name = self.getProperty("LogName").value
        run_log_start = min(input_ws.run().getProperty(log_name).value)
        run_log_stop = max(input_ws.run().getProperty(log_name).value)
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
            alg.setProperty("InputWorkspace", input_ws)
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

    def _scale_monitors_for_each_slice(self, input_monitor_ws, input_ws, sliced_ws_group):
        """Create a group workspace which contains a copy of the monitors workspace for
        each slice, scaled by the relative proton charge for that slice"""
        total_proton_charge = self._total_proton_charge(input_ws)
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

        monitor_ws_group_name = input_monitor_ws.name() + "_sliced"
        monitor_ws_group = self._group_workspaces(monitors_ws_list, monitor_ws_group_name)
        mtd.addOrReplace(monitor_ws_group_name, monitor_ws_group)
        return monitor_ws_group

    def _clone_workspace(self, ws_to_clone, output_ws_name):
        alg = self.createChildAlgorithm("CloneWorkspace")
        alg.setProperty("InputWorkspace", ws_to_clone)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _scale_workspace(self, ws_to_scale, output_ws_name, scale_factor):
        alg = self.createChildAlgorithm("Scale")
        alg.setProperty("InputWorkspace", ws_to_scale)
        alg.setProperty("Factor", scale_factor)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _group_workspaces(self, ws_list, output_ws_name):
        alg = self.createChildAlgorithm("GroupWorkspaces")
        alg.setProperty("InputWorkspaces", ws_list)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _total_proton_charge(self, input_ws):
        """Get the proton charge for the input workspace"""
        return input_ws.run().getProtonCharge()

    def _rebin_to_monitors(self, ws_to_rebin, monitor_ws_group):
        """Rebin the output workspace group to the monitors workspace group"""
        alg = self.createChildAlgorithm("RebinToWorkspace")
        alg.setProperty("WorkspaceToRebin", ws_to_rebin.name())
        alg.setProperty("WorkspaceToMatch", monitor_ws_group.name())
        alg.setProperty("PreserveEvents", False)
        alg.setProperty("OutputWorkspace", ws_to_rebin.name())
        alg.execute()

    def _add_monitors_to_sliced_output(self, input_ws_group, monitor_ws_group):
        """Add the monitors for each slice to the output workspace for each slice"""
        alg = self.createChildAlgorithm("AppendSpectra")
        alg.setProperty("InputWorkspace1", monitor_ws_group.name())
        alg.setProperty("InputWorkspace2", input_ws_group.name())
        alg.setProperty("OutputWorkspace", f"{input_ws_group.name()}")
        alg.setProperty("MergeLogs", False)
        alg.execute()

    def _clean_up(self, monitor_ws_group):
        """Remove worspaces added to the ADS"""
        monitor_ws_names = [ws.name() for ws in monitor_ws_group]
        alg = self.createChildAlgorithm("UnGroupWorkspace")
        alg.setProperty("InputWorkspace", monitor_ws_group.name())
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
