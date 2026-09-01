# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm
from mantid.kernel import DateAndTime, Direction
from mantid.simpleapi import AddSampleLog

import re


class ReflectometrySliceEventWorkspace(DataProcessorAlgorithm):
    def category(self):
        return "Reflectometry"

    def name(self):
        return "ReflectometrySliceEventWorkspace"

    def summary(self):
        return "Split an input workspace into multiple slices according to time or log values"

    def seeAlso(self):
        return ["GenerateEventsFilter", "FilterEvents", "ReflectometryReductionOneAuto"]

    def version(self):
        return 2

    def PyInit(self):
        self.declareProperty(
            "InputWorkspaceName", "", direction=Direction.Input, doc="The name of an input event workspace or group of event workspaces."
        )

        # Add properties from child algorithm
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
        self.declareProperty(
            "MonitorWorkspaceName",
            "",
            direction=Direction.Input,
            doc="The name of the input monitor workspace or group of monitor workspaces.",
        )
        self.declareProperty("OutputWorkspaceName", "", direction=Direction.Output, doc="(Base)Name for the output workspace(s).")
        self.declareProperty(
            "FilterByLogValue", False, doc="If true, filter events via the FilterByLogValue algorithm. Uses FilterEvents otherwise."
        )

    def validateInputs(self):
        issues = {}
        if not self.getPropertyValue("OutputWorkspaceName"):
            issues["OutputWorkspaceName"] = "A base name for the output workspace must be provided."
        if not mtd.doesExist(self.getPropertyValue("InputWorkspaceName")):
            issues["InputWorkspaceName"] = "The input workspace must be present in the ADS."
        if not mtd.doesExist(self.getPropertyValue("MonitorWorkspaceName")):
            issues["MonitorWorkspaceName"] = "The monitor workspace must be present in the ADS."
        if issues:
            return issues
        input_ws = mtd.retrieve(self.getPropertyValue("InputWorkspaceName"))
        monitor_ws = mtd.retrieve(self.getPropertyValue("MonitorWorkspaceName"))

        if input_ws.isGroup():
            return self._validate_group_inputs(input_ws, monitor_ws, issues)
        if monitor_ws.isGroup():
            issues["MonitorWorkspaceName"] = "A monitor workspace group may only be provided alongside an eqivalent input workspace group."
        return self._validate_single_workspace(input_ws, issues)

    def _validate_single_workspace(self, workspace, issues):
        if workspace.run().getProtonCharge() < 1e-9:
            issues["InputWorkspaceName"] = "Cannot slice workspace with zero proton charge"
        return issues

    def _validate_group_inputs(self, ws_group, monitors, issues: dict):
        if monitors.isGroup() and len(monitors) != len(ws_group):
            issues["InputWorkspaceName"] = "Monitor and Input workspace groups must be the same size."
        for ws in ws_group:
            issues = self._validate_single_workspace(ws, issues) | issues
        return issues

    def PyExec(self):
        input_ws = mtd.retrieve(self.getPropertyValue("InputWorkspaceName"))
        input_monitor_ws = mtd.retrieve(self.getPropertyValue("MonitorWorkspaceName"))
        output_ws_base_name = self.getPropertyValue("OutputWorkspaceName")

        if input_ws.isGroup():
            if not input_monitor_ws.isGroup():
                input_monitor_ws = [input_monitor_ws] * len(input_ws)
            sliced_workspaces = []
            for i, (input_ws, monitor_ws) in enumerate(zip(input_ws, input_monitor_ws)):
                self._output_ws_group_name = f"{input_ws.name()}_{monitor_ws.name()}_{output_ws_base_name}"
                output_ws_group, monitor_ws_group = self._exec_single_workspace(input_ws, monitor_ws, output_workspace_suffix=f"_{i}")
                sliced_workspaces.append(output_ws_group)
                self._clean_up(monitor_ws_group)
            # Transform the workspace groups so that each slice has a workspace group with the same shape as the original input workspace.
            for i, slice in enumerate(zip(*sliced_workspaces)):
                slice_group = self._group_workspaces(list(slice))
                mtd.addOrReplace(self._create_name_for_slice_group(output_ws_base_name, slice_group), slice_group)
            for ws in sliced_workspaces:
                self._ungroup_ws(ws)
        else:
            self._output_ws_group_name = output_ws_base_name
            output_ws_group, monitor_ws_group = self._exec_single_workspace(input_ws, input_monitor_ws)
            self._clean_up(monitor_ws_group)
            mtd.addOrReplace(output_ws_group.name(), output_ws_group)

    def _exec_single_workspace(self, input_ws, input_monitor_ws, output_workspace_suffix=""):
        output_ws_group = self._slice_input_workspace(input_ws, output_workspace_suffix)
        monitor_ws_group = self._scale_monitors_for_each_slice(input_monitor_ws, input_ws, output_ws_group)
        # This step is done in-place.
        self._rebin_to_monitors(output_ws_group, monitor_ws_group)
        self._add_monitors_to_sliced_output(output_ws_group, monitor_ws_group)
        return output_ws_group, monitor_ws_group

    def _create_name_for_slice_group(self, output_base_name, slice_group):
        if self._slice_by_log():
            regex = re.compile(r"\.From\.(\d+)\.To\.(\d+)")
        else:
            regex = re.compile(r"_(\d+)_(\d+)$")
        start, end = regex.search(slice_group[0].name()).groups()
        return f"{output_base_name}_{start}_{end}"

    def _slice_input_workspace(self, input_ws, output_suffix):
        if self._slice_by_log():
            return self._slice_input_workspace_with_filter_by_log_value(input_ws, output_suffix)
        else:
            return self._slice_input_workspace_with_filter_events(input_ws, output_suffix)

    def _slice_by_log(self):
        """Return true if we are slicing by log value"""
        return self._property_set("LogName") and self.getProperty("FilterByLogValue").value

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

    def _slice_input_workspace_with_filter_by_log_value(self, input_ws, output_suffix=""):
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
        group = self._group_workspaces(slice_names)
        mtd.addOrReplace(self._output_ws_group_name + output_suffix, group)
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
        monitor_ws_group = self._group_workspaces(monitors_ws_list)
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

    def _group_workspaces(self, ws_list):
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

    def _ungroup_ws(self, ws_group):
        alg = self.createChildAlgorithm("UnGroupWorkspace")
        alg.setProperty("InputWorkspace", ws_group.name())
        alg.execute()

    def _clean_up(self, monitor_ws_group):
        """Remove worspaces added to the ADS"""
        monitor_ws_names = [ws.name() for ws in monitor_ws_group]
        self._ungroup_ws(monitor_ws_group)
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
            raise RuntimeError("Multiple intervals are not supported if using FilterByLogValue.")
        if len(value_as_list) < 1:
            raise RuntimeError("Interval was not specified")
        return float(value_as_list[0])


AlgorithmFactory.subscribe(ReflectometrySliceEventWorkspace())
