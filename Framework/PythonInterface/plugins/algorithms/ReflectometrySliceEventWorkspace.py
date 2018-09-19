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
        self._input_ws_name = self.getPropertyValue("InputWorkspace")
        self._input_monitor_ws_name = self.getPropertyValue("MonitorWorkspace")

        self._sliced_output_ws_group_name = self.getPropertyValue("OutputWorkspace")
        self._create_filter()
        self._slice_input_workspace()
        self._scale_monitors_for_each_slice()
        self._rebin_to_monitors()
        self._add_monitors_to_sliced_output()

        self._set_outputs()
        self._clean_up()

    def _create_filter(self):
        """Generate the splitter workspace for performing the filtering for each required slice"""
        alg = AlgorithmManager.create("GenerateEventsFilter")
        alg.initialize()
        alg.setChild(True)
        self._copy_filter_property_values_to(alg)
        alg.setProperty("OutputWorkspace", '__split')
        alg.setProperty("InformationWorkspace", '__info')
        alg.execute()
        self._split_ws = alg.getProperty("OutputWorkspace")
        self._info_ws = alg.getProperty("InformationWorkspace")

    def _copy_filter_property_values_to(self, filter_alg):
        """Copy input properties from this algorithm to the child filter algorithm"""
        for prop in self._filter_properties:
            value = self.getPropertyValue(prop)
            filter_alg.setPropertyValue(prop, value)

    def _slice_input_workspace(self):
        """Perform the slicing of the input workspace"""
        self._mock_ws_name = '__mock'
        FilterEvents(InputWorkspace=self._input_ws_name,
                     SplitterWorkspace=self._split_ws.value,
                     InformationWorkspace=self._info_ws.value,
                     OutputWorkspaceBaseName=self._sliced_output_ws_group_name,
                     GroupWorkspaces=True,FilterByPulseTime = False,
                     OutputWorkspaceIndexedFrom1 = True,CorrectionToSample = "None",
                     SpectrumWithoutDetector = "Skip",SplitSampleLogs = False,
                     OutputTOFCorrectionWorkspace=self._mock_ws_name,ExcludeSpecifiedLogs=False,
                     TimeSeriesPropertyLogs='proton_charge')

    def _scale_monitors_for_each_slice(self):
        """Create a group workspace which contains a copy of the monitors workspace for
        each slice, scaled by the relative proton charge for that slice"""
        total_proton_charge = self._total_proton_charge()
        monitors_list = list()
        i=1
        for slice in mtd[self._sliced_output_ws_group_name]:
            slice_monitor_ws_name = self._input_monitor_ws_name + '_'+str(i)
            CloneWorkspace(InputWorkspace=self._input_monitor_ws_name,
                           OutputWorkspace=slice_monitor_ws_name)
            scale_factor = slice.run().getProtonCharge() / total_proton_charge
            Scale(InputWorkspace=slice_monitor_ws_name,OutputWorkspace=slice_monitor_ws_name,
                  Factor=scale_factor)
            monitors_list.append(slice_monitor_ws_name)
            i+=1
        self._sliced_monitor_ws_group_name = self._input_monitor_ws_name + '_sliced'
        GroupWorkspaces(InputWorkspaces=monitors_list,
                        OutputWorkspace=self._sliced_monitor_ws_group_name)

    def _total_proton_charge(self):
        """Get the proton charge for the input workspace"""
        return mtd[self._input_ws_name].run().getProtonCharge()

    def _rebin_to_monitors(self):
        """Rebin the output workspace group to the monitors workspace group"""
        RebinToWorkspace(WorkspaceToRebin=self._sliced_output_ws_group_name,
                         WorkspaceToMatch=self._sliced_monitor_ws_group_name,
                         OutputWorkspace=self._sliced_output_ws_group_name,PreserveEvents=False)

    def _add_monitors_to_sliced_output(self):
        """Add the monitors for each slice to the output workspace for each slice"""
        AppendSpectra(InputWorkspace1=self._sliced_monitor_ws_group_name,
                      InputWorkspace2=self._sliced_output_ws_group_name,
                      MergeLogs=True, OutputWorkspace=self._sliced_output_ws_group_name)

    def _set_outputs(self):
        """Set the output property values for this algorithm"""
        self.setPropertyValue("OutputWorkspace", self._sliced_output_ws_group_name)

    def _clean_up(self):
        AnalysisDataService.remove(self._mock_ws_name)
        
AlgorithmFactory.subscribe(ReflectometrySliceEventWorkspace())
