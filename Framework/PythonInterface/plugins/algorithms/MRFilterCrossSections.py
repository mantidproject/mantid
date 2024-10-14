# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import os
from operator import itemgetter
from mantid.api import (
    AnalysisDataService,
    FileAction,
    FileProperty,
    PropertyMode,
    PythonAlgorithm,
    WorkspaceGroupProperty,
    WorkspaceProperty,
)
from mantid.kernel import Direction
import mantid.simpleapi as api


def extract_times(times, is_start, is_sf1=False, is_sf2=False, is_veto1=False, is_veto2=False):
    """
    Extract a list of times
    """
    return [(times[i], is_start, [is_sf1, is_sf2, is_veto1, is_veto2]) for i in range(len(times))]


class MRFilterCrossSections(PythonAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "MRFilterCrossSections"

    def summary(self):
        return "This algorithm loads a Magnetism Reflectometer file and returns workspaces for each cross-section."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.OptionalLoad, extensions=[".nxs", ".nxs.h5"]))
        self.declareProperty(
            WorkspaceProperty("InputWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Optionally, we can provide a scattering workspace directly",
        )
        self.declareProperty("PolState", "SF1", doc="Name of the log entry determining the polarizer state")
        self.declareProperty("AnaState", "SF2", doc="Name of the log entry determining the analyzer state")
        self.declareProperty("PolVeto", "", doc="Name of the log entry determining the polarizer veto [optional]")
        self.declareProperty("AnaVeto", "", doc="Name of the log entry determining the analyzer veto [optional]")
        self.declareProperty("CheckDevices", True, doc="If true, the analyzer/polarizer devices will be checked before filtering")
        self.declareProperty(
            WorkspaceGroupProperty("CrossSectionWorkspaces", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Workspace group containing cross-sections",
        )

    def PyExec(self):
        """Execute filtering"""
        file_path = self.getProperty("Filename").value

        # Allow for the processing of legacy data
        if file_path.endswith(".nxs"):
            self.load_legacy_cross_Sections(file_path)
        else:
            self.filter_cross_sections(file_path)

    def create_table(self, change_list, start_time, has_polarizer=True, has_analyzer=True):
        split_table_ws = api.CreateEmptyTableWorkspace()
        split_table_ws.addColumn("float", "start")
        split_table_ws.addColumn("float", "stop")
        split_table_ws.addColumn("str", "target")

        current_state = [False, False, False, False]
        current_state_t0 = 0

        # Keep track of when we have a fully specified state
        specified = [not has_polarizer, not has_analyzer]

        for item in change_list:
            # We have a change of state, add an entry for the state that just ended
            if specified[0] and specified[1] and not current_state[2] and not current_state[3]:
                xs = "%s_%s" % ("On" if current_state[0] else "Off", "On" if current_state[1] else "Off")
                split_table_ws.addRow([int(current_state_t0 - start_time) * 1e-9, (item[0] - start_time) * 1e-9, xs])

            # Now update the current state
            for i in range(len(current_state)):
                if item[2][i]:
                    if i < 2:
                        specified[i] = True
                    current_state[i] = item[1]
            current_state_t0 = item[0]
        return split_table_ws

    def filter_cross_sections(self, file_path):
        """
        Filter events according to the polarization states
        :param str file_path: data file path
        """
        output_wsg = self.getPropertyValue("CrossSectionWorkspaces")
        pol_state = self.getProperty("PolState").value
        pol_veto = self.getProperty("PolVeto").value
        ana_state = self.getProperty("AnaState").value
        ana_veto = self.getProperty("AnaVeto").value
        ws_event_data = self.getProperty("InputWorkspace").value

        if ws_event_data is not None:
            ws_raw_name = str(ws_event_data)
            ws_raw = ws_event_data
        else:
            ws_raw_name = os.path.basename(file_path)
            ws_raw = api.LoadEventNexus(Filename=file_path, OutputWorkspace=ws_raw_name)

        if self.getProperty("CheckDevices").value:
            # Check whether we have a polarizer
            polarizer = ws_raw.getRun().getProperty("Polarizer").value[0]
            # Check whether we have an analyzer
            analyzer = ws_raw.getRun().getProperty("Analyzer").value[0]
        else:
            polarizer = 1
            analyzer = 1

        change_list = []
        if polarizer > 0:
            # SF1 ON
            splitws, _ = api.GenerateEventsFilter(
                InputWorkspace=ws_raw_name,
                LogName=pol_state,
                MinimumLogValue=0.99,
                MaximumLogValue=1.01,
                TimeTolerance=0,
                OutputWorkspace="filter",
                InformationWorkspace="filter_info",
                LogBoundary="Left",
                UnitOfTime="Seconds",
            )
            time_dict = splitws.toDict()
            change_list.extend(extract_times(time_dict["start"], True, is_sf1=True))
            change_list.extend(extract_times(time_dict["stop"], False, is_sf1=True))

            # SF1 OFF
            splitws, _ = api.GenerateEventsFilter(
                InputWorkspace=ws_raw_name,
                LogName=pol_state,
                MinimumLogValue=-0.01,
                MaximumLogValue=0.01,
                TimeTolerance=0,
                OutputWorkspace="filter",
                InformationWorkspace="filter_info",
                LogBoundary="Left",
                UnitOfTime="Seconds",
            )
            time_dict = splitws.toDict()
            change_list.extend(extract_times(time_dict["start"], False, is_sf1=True))
            change_list.extend(extract_times(time_dict["stop"], True, is_sf1=True))

            # SF1 VETO
            if not pol_veto == "":
                splitws, _ = api.GenerateEventsFilter(
                    InputWorkspace=ws_raw_name,
                    LogName=pol_veto,
                    MinimumLogValue=0.99,
                    MaximumLogValue=1.01,
                    TimeTolerance=0,
                    OutputWorkspace="filter",
                    InformationWorkspace="filter_info",
                    LogBoundary="Left",
                    UnitOfTime="Seconds",
                )
                time_dict = splitws.toDict()
                change_list.extend(extract_times(time_dict["start"], True, is_veto1=True))
                change_list.extend(extract_times(time_dict["stop"], False, is_veto1=True))

        if analyzer > 0:
            # SF2 ON
            splitws, _ = api.GenerateEventsFilter(
                InputWorkspace=ws_raw_name,
                LogName=ana_state,
                MinimumLogValue=0.99,
                MaximumLogValue=1.01,
                TimeTolerance=0,
                OutputWorkspace="filter",
                InformationWorkspace="filter_info",
                LogBoundary="Left",
                UnitOfTime="Seconds",
            )
            time_dict = splitws.toDict()
            change_list.extend(extract_times(time_dict["start"], True, is_sf2=True))
            change_list.extend(extract_times(time_dict["stop"], False, is_sf2=True))

            # SF2 OFF
            splitws, _ = api.GenerateEventsFilter(
                InputWorkspace=ws_raw_name,
                LogName=ana_state,
                MinimumLogValue=-0.01,
                MaximumLogValue=0.01,
                TimeTolerance=0,
                OutputWorkspace="filter",
                InformationWorkspace="filter_info",
                LogBoundary="Left",
                UnitOfTime="Seconds",
            )
            time_dict = splitws.toDict()
            change_list.extend(extract_times(time_dict["start"], False, is_sf2=True))
            change_list.extend(extract_times(time_dict["stop"], True, is_sf2=True))

            # SF2 VETO
            if not ana_veto == "":
                splitws, _ = api.GenerateEventsFilter(
                    InputWorkspace=ws_raw_name,
                    LogName=ana_veto,
                    MinimumLogValue=0.99,
                    MaximumLogValue=1.01,
                    TimeTolerance=0,
                    OutputWorkspace="filter",
                    InformationWorkspace="filter_info",
                    LogBoundary="Left",
                    UnitOfTime="Seconds",
                )
                time_dict = splitws.toDict()
                change_list.extend(extract_times(time_dict["start"], True, is_veto2=True))
                change_list.extend(extract_times(time_dict["stop"], False, is_veto2=True))

        start_time = ws_raw.run().startTime().totalNanoseconds()

        change_list = sorted(change_list, key=itemgetter(0))
        split_table_ws = self.create_table(change_list, start_time, has_polarizer=polarizer > 0, has_analyzer=analyzer > 0)

        # Filter events if we found enough information to do so
        if split_table_ws.rowCount() > 0:
            outputs = api.FilterEvents(
                InputWorkspace=ws_raw,
                SplitterWorkspace=split_table_ws,
                OutputWorkspaceBaseName=output_wsg,
                GroupWorkspaces=True,
                FilterByPulseTime=False,
                OutputWorkspaceIndexedFrom1=False,
                CorrectionToSample="None",
                SpectrumWithoutDetector="Skip",
                SplitSampleLogs=True,
                RelativeTime=True,
                ExcludeSpecifiedLogs=True,
                OutputTOFCorrectionWorkspace="_tmp",
            )
            AnalysisDataService.remove("_tmp")
            for ws in outputs[-1]:
                pol_state = str(ws).replace(output_wsg + "_", "")
                api.AddSampleLog(Workspace=ws, LogName="cross_section_id", LogText=pol_state)

            if ws_event_data is None:
                AnalysisDataService.remove(ws_raw_name)
            self.setProperty("CrossSectionWorkspaces", output_wsg)

        # If we don't have a splitter table, it might be because we don't have analyzer/polarizer
        # information. In this case don't filter and return the raw workspace.
        elif polarizer <= 0 and analyzer <= 0:
            api.logger.warning("No polarizer/analyzer information available")
            self.setProperty("CrossSectionWorkspaces", api.GroupWorkspaces([ws_raw]))
        else:
            api.logger.error("No events remained after filtering")
            if ws_event_data is None:
                AnalysisDataService.remove(ws_raw_name)

    def load_legacy_cross_Sections(self, file_path):
        """
        For legacy MR data, we need to load each cross-section independently.
        :param str file_path: data file path
        """
        ws_base_name = os.path.basename(file_path)
        cross_sections = list()

        for entry in ["Off_Off", "On_Off", "Off_On", "On_On"]:
            try:
                ws_name = "%s_%s" % (ws_base_name, entry)
                ws = api.LoadEventNexus(Filename=file_path, NXentryName="entry-%s" % entry, OutputWorkspace=ws_name)
                api.AddSampleLog(Workspace=ws, LogName="cross_section_id", LogText=entry)
                cross_sections.append(ws_name)
            except:
                api.logger.information("Could not load %s from legacy data file" % entry)

        # Prepare output workspace group
        output_wsg = self.getPropertyValue("CrossSectionWorkspaces")

        api.GroupWorkspaces(InputWorkspaces=cross_sections, OutputWorkspace=output_wsg)
        self.setProperty("CrossSectionWorkspaces", output_wsg)


# Register
api.AlgorithmFactory.subscribe(MRFilterCrossSections)
