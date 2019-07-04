# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *


class ReflectometrySliceEventWorkspace(DataProcessorAlgorithm):
    def category(self):
        return "Reflectometry"

    def name(self):
        return "ReflectometrySliceEventWorkspace"

    def summary(self):
        return "Split an input workspace into multiple slices according to time or log values"

    def seeAlso(self):
        return [ "GenerateEventsFilter","FilterEvents" ,"ReflectometryReductionOneAuto"]

    def PyInit(self):
        # Add properties from child algorithm
        self._filter_properties = [
            'InputWorkspace', 'StartTime', 'StopTime','TimeInterval',
            'LogName','MinimumLogValue','MaximumLogValue', 'LogValueInterval']
        self.copyProperties('GenerateEventsFilter', self._filter_properties)

        # Add our own properties
        self.declareProperty(MatrixWorkspaceProperty("MonitorWorkspace", "", direction=Direction.Input),
                             "Input monitor workspace")
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='Group name for the output workspace(s).')

    def PyExec(self):
        self._input_ws = self.getProperty("InputWorkspace").value
        self._output_ws_group_name = self.getPropertyValue("OutputWorkspace")

        self._create_filter()
        output_ws_group = self._slice_input_workspace()
        self._scale_monitors_for_each_slice(output_ws_group)
        output_ws_group = self._rebin_to_monitors()
        output_ws_group = self._add_monitors_to_sliced_output()

        self.setProperty("OutputWorkspace", self._output_ws_group_name)
        self._clean_up()

    def _create_filter(self):
        """Generate the splitter workspace for performing the filtering for each required slice"""
        alg = self.createChildAlgorithm("GenerateEventsFilter")
        for property in self._filter_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        alg.setProperty("OutputWorkspace", '__split')
        alg.setProperty("InformationWorkspace", '__info')
        alg.execute()
        self._split_ws = alg.getProperty("OutputWorkspace").value
        self._info_ws = alg.getProperty("InformationWorkspace").value

    def _slice_input_workspace(self):
        """Perform the slicing of the input workspace"""
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
        alg.setProperty("TimeSeriesPropertyLogs", 'proton_charge')
        alg.setProperty("DescriptiveOutputNames", True)
        alg.execute()
        # Ensure the run number for the child workspaces is stored in the
        # sample logs as a string (FilterEvents converts it to a double).
        group = mtd[self._output_ws_group_name]
        for ws in group:
            if ws.run().hasProperty('run_number'):
                run_number = int(ws.run()['run_number'].value)
                AddSampleLog(Workspace=ws, LogName='run_number', LogType='String',
                             LogText=str(run_number))
        return group

    def _scale_monitors_for_each_slice(self, sliced_ws_group):
        """Create a group workspace which contains a copy of the monitors workspace for
        each slice, scaled by the relative proton charge for that slice"""
        input_monitor_ws = self.getProperty("MonitorWorkspace").value
        total_proton_charge = self._total_proton_charge()
        monitors_ws_list = []
        i=1
        for slice in sliced_ws_group:
            slice_monitor_ws_name = input_monitor_ws.name() + '_'+str(i)
            slice_monitor_ws = self._clone_workspace(input_monitor_ws, slice_monitor_ws_name)
            scale_factor = slice.run().getProtonCharge() / total_proton_charge
            slice_monitor_ws = self._scale_workspace(slice_monitor_ws, slice_monitor_ws_name,
                                                     scale_factor)
            # The workspace must be in the ADS for grouping
            mtd.addOrReplace(slice_monitor_ws_name, slice_monitor_ws)
            monitors_ws_list.append(slice_monitor_ws_name)
            i+=1

        self._monitor_ws_group_name = input_monitor_ws.name() + '_sliced'
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
        alg.setProperty("MergeLogs", True)
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

AlgorithmFactory.subscribe(ReflectometrySliceEventWorkspace())
