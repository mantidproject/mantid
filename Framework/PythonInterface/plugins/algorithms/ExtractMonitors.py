from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import (MatrixWorkspaceProperty, DataProcessorAlgorithm, PropertyMode)


class ExtractMonitors(DataProcessorAlgorithm):
    def category(self):
        return 'Utility\\Workspaces'

    def summary(self):
        return 'Separates the monitors and/or detectors into separate workspaces.'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',
                                                     direction=Direction.Input),
                             doc='A workspace with detectors and monitors')

        self.declareProperty(MatrixWorkspaceProperty('DetectorWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output workspace with detectors only')

        self.declareProperty(MatrixWorkspaceProperty('MonitorWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output workspace with monitors only')

    def validateInputs(self):
        issues = {}

        detector_ws_name = self.getProperty("DetectorWorkspace").valueAsStr
        monitor_ws_name = self.getProperty("MonitorWorkspace").valueAsStr

        if not detector_ws_name and not monitor_ws_name:
            msg = "Must specify one of DetectorsWorkspace or MonitorsWorkspace"
            issues["DetectorWorkspace"] = msg
            issues["MonitorWorkspace"] = msg
        elif detector_ws_name == monitor_ws_name:
            msg = "DetectorWorkspace and MonitorWorkspace must be different"
            issues["DetectorWorkspace"] = msg
            issues["MonitorWorkspace"] = msg

        return issues

    def PyExec(self):
        in_ws = self.getProperty("InputWorkspace").value
        detector_ws_name = self.getProperty("DetectorWorkspace").valueAsStr
        monitor_ws_name = self.getProperty("MonitorWorkspace").valueAsStr

        try:
            mon = in_ws.getMonitorWorkspace()
            raise ValueError("Monitor workspace already exists, called: " + mon.name() + ".")
        except RuntimeError:
            pass

        monitors = []
        detectors = []
        for i in range(in_ws.getNumberHistograms()):
            try:
                monitors.append(i) if in_ws.getDetector(i).isMonitor() else detectors.append(i)
            except RuntimeError:
                self.log().warning("Missing detector at " + str(i))

        if detector_ws_name:
            if detectors:
                detector_ws = ExtractSpectra(InputWorkspace=in_ws,
                                             WorkspaceIndexList=detectors)
            else:
                self.log().error("No detectors found in input workspace. No detector output workspace created.")

        if monitor_ws_name:
            if monitors:
                monitor_ws = ExtractSpectra(InputWorkspace=in_ws,
                                            WorkspaceIndexList=monitors)
            else:
                self.log().error("No monitors found in input workspace. No monitor output workspace created.")

        if detector_ws_name and detectors:
            self.setProperty("DetectorWorkspace", detector_ws)
            DeleteWorkspace(detector_ws)

        if monitor_ws_name and monitors:
            self.setProperty("MonitorWorkspace", monitor_ws)
            DeleteWorkspace(monitor_ws)

        if detector_ws_name and detectors and monitor_ws_name and monitors:
            detector_ws.setMonitorWorkspace(monitor_ws)

AlgorithmFactory.subscribe(ExtractMonitors)
