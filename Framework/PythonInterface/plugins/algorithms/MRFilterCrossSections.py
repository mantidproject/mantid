#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import os
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi as api


class MRFilterCrossSections(PythonAlgorithm):

    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "MRFilterCrossSections"

    def summary(self):
        return "This algorithm loads a Magnetism Reflectometer file and returns workspaces for each cross-section."

    def PyInit(self):
        self.declareProperty(FileProperty("Filename", "", action=FileAction.Load, extensions=[".nxs", ".nxs.h5"]))
        self.declareProperty("PolState", "SF1",
                             doc="Name of the log entry determining the polarizer state")
        self.declareProperty("AnaState", "SF2",
                             doc="Name of the log entry determining the analyzer state")
        self.declareProperty("PolVeto", "",
                             doc="Name of the log entry determining the polarizer veto [optional]")
        self.declareProperty("AnaVeto", "",
                             doc="Name of the log entry determining the analyzer veto [optional]")
        self.declareProperty(WorkspaceGroupProperty("CrossSectionWorkspaces", "",
                                                    direction=Direction.Output,
                                                    optional=PropertyMode.Optional),
                             doc="Workspace group containing cross-sections")
    def PyExec(self):
        """ Execute filtering """
        file_path = self.getProperty("Filename").value

        # Allow for the processing of legacy data
        if file_path.endswith('.nxs'):
            cross_sections = self.load_legacy_cross_Sections(file_path)
        else:
            cross_sections = self.filter_cross_sections(file_path)
        output_wsg = self.getPropertyValue("CrossSectionWorkspaces")

        api.GroupWorkspaces(InputWorkspaces=cross_sections,
                            OutputWorkspace=output_wsg)
        self.setProperty("CrossSectionWorkspaces", output_wsg)

    def filter_cross_sections(self, file_path):
        """
            Filter events according to the polarization states
            :param str file_path: data file path
        """
        pol_state = self.getProperty("PolState").value
        pol_veto = self.getProperty("PolVeto").value
        raw_ws = os.path.basename(file_path)
        ws = api.LoadEventNexus(Filename=file_path, OutputWorkspace=raw_ws)
        run_number = ws.getRunNumber()

        # Check whether we have a polarizer
        polarizer = ws.getRun().getProperty("Polarizer").value[0]
    
        # Determine cross-sections
        cross_sections = list()
        if polarizer > 0:
            xs_name = "%s_Off" % run_number
            ws_off = api.FilterByLogValue(InputWorkspace=ws, LogName=pol_state, TimeTolerance=0.1,
                                      MinimumValue=-.01, MaximumValue=0.01, LogBoundary='Left',
                                      OutputWorkspace=xs_name)
            if not pol_veto == '':
                ws_off = api.FilterByLogValue(InputWorkspace=ws_off, LogName=pol_veto, TimeTolerance=0.1,
                                          MinimumValue=-.01, MaximumValue=0.01, LogBoundary='Left',
                                          OutputWorkspace=xs_name)

            xs_events = self.filter_analyzer(ws_off, 'Off')
            cross_sections.extend(xs_events)

            xs_name = "%s_On" % run_number
            ws_on = api.FilterByLogValue(InputWorkspace=ws, LogName=pol_state, TimeTolerance=0.1,
                                     MinimumValue=0.99, MaximumValue=1.01, LogBoundary='Left',
                                     OutputWorkspace=xs_name)
            if not pol_veto == '':
                ws_on = api.FilterByLogValue(InputWorkspace=ws_on, LogName=pol_veto, TimeTolerance=0.1,
                                         MinimumValue=-.01, MaximumValue=0.01, LogBoundary='Left',
                                         OutputWorkspace=xs_name)

            xs_events = self.filter_analyzer(ws_on, 'On')
            cross_sections.extend(xs_events)
        else:
            xs_events = self.filter_analyzer(ws, 'Off')
            cross_sections.extend(xs_events)

        AnalysisDataService.remove(raw_ws)

        return cross_sections

    def filter_analyzer(self, ws, pol_state='Off'):
        """
            Filter events according to the analyzer.
            :param Workspace ws: Mantid workspace
            :param str pol_state: polarization state On/Off
        """
        ana_state = self.getProperty("AnaState").value
        ana_veto = self.getProperty("AnaVeto").value
        run_number = ws.getRunNumber()
        cross_sections = list()

        analyzer = ws.getRun().getProperty("Analyzer").value[0]
        if analyzer > 0:
            try:
                xs_name = "%s_%s_Off" % (run_number, pol_state)
                ws_ana_off = api.FilterByLogValue(InputWorkspace=ws, LogName=ana_state,
                                              MinimumValue=-0.01, MaximumValue=0.01, LogBoundary='Left',
                                              OutputWorkspace=xs_name)
                if not ana_veto == '':
                    ws_ana_off = api.FilterByLogValue(InputWorkspace=ws_ana_off, LogName=ana_veto, TimeTolerance=0.1,
                                                  MinimumValue=-.01, MaximumValue=0.01, LogBoundary='Left',
                                                  OutputWorkspace=xs_name)
                api.AddSampleLog(Workspace=ws_ana_off, LogName='cross_section_id',
                                      LogText="%s_Off" % pol_state)
                cross_sections.append(xs_name)
            except:
                api.logger.error("Could not filter %s-Off" % pol_state)

            try:
                xs_name = "%s_%s_On" % (run_number, pol_state)
                ws_ana_on = api.FilterByLogValue(InputWorkspace=ws, LogName=ana_state,
                                             MinimumValue=1, MaximumValue=1, LogBoundary='Left',
                                             OutputWorkspace=xs_name)
                if not ana_veto == '':
                    ws_ana_on = api.FilterByLogValue(InputWorkspace=ws_ana_on, LogName=ana_veto, TimeTolerance=0.1,
                                                 MinimumValue=-.01, MaximumValue=0.01, LogBoundary='Left',
                                                 OutputWorkspace=xs_name)
                api.AddSampleLog(Workspace=ws_ana_on, LogName='cross_section_id',
                                      LogText="%s_On" % pol_state)
                cross_sections.append(xs_name)
            except:
                api.logger.error("Could not filter %s-On" % pol_state)
            AnalysisDataService.remove(str(ws))
        else:
            api.AddSampleLog(Workspace=ws, LogName='cross_section_id',
                                      LogText="%s_Off" % pol_state)
            cross_sections.append(str(ws))

        return cross_sections

    def load_legacy_cross_Sections(self, file_path):
        """
            For legacy MR data, we need to load each cross-section independently.
            :param str file_path: data file path
        """
        ws_base_name = os.path.basename(file_path)
        cross_sections = list()

        for entry in ['Off_Off', 'On_Off', 'Off_On', 'On_On']:
            ws_name = "%s_%s" % (ws_base_name, entry)
            ws = api.LoadEventNexus(Filename=file_path,
                                    NXentryName='entry-%s' % entry,
                                    OutputWorkspace=ws_name)
            api.AddSampleLog(Workspace=ws, LogName='cross_section_id',
                                      LogText=entry)
            cross_sections.append(ws_name)

        return cross_sections

# Register
api.AlgorithmFactory.subscribe(MRFilterCrossSections)
