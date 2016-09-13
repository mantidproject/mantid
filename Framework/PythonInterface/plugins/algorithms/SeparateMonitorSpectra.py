from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import (MatrixWorkspaceProperty, DataProcessorAlgorithm, PropertyMode)

class SeparateMonitorSpectra(DataProcessorAlgorithm):
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

        if not self.getProperty("DetectorWorkspace").valueAsStr and not self.getProperty("MonitorWorkspace").valueAsStr:
            msg = "Must specify one of DetectorsWorkspace or MonitorsWorkspace"
            issues["DetectorWorkspace"] = msg
            issues["MonitorWorkspace"] = msg
        elif self.getProperty("DetectorWorkspace").valueAsStr == self.getProperty("MonitorWorkspace").valueAsStr:
            msg = "DetectorWorkspace and MonitorWorkspace must be different"
            issues["DetectorWorkspace"] = msg
            issues["MonitorWorkspace"] = msg

        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        detector_ws_name = self.getProperty("DetectorWorkspace").valueAsStr
        monitor_ws_name = self.getProperty("MonitorWorkspace").valueAsStr

        try:
            mon = ws.getMonitorWorkspace()
            raise ValueError("Monitor workspace already exists, called: " + mon.name() + ".")
        except RuntimeError:
            pass

        monitors = []
        detectors = []
        for i in range(ws.getNumberHistograms()):
            try:
                monitors.append(i) if ws.getDetector(i).isMonitor() else detectors.append(i)
            except RuntimeError:
                self.log().warning("Missing detector at " + str(i))


        if self.getProperty("DetectorWorkspace").valueAsStr:
            detector_ws = ExtractSpectra(InputWorkspace=ws, OutputWorkspace=detector_ws_name, WorkspaceIndexList=detectors)
            self.setProperty("DetectorWorkspace", detector_ws)

        if self.getProperty("MonitorWorkspace").valueAsStr:
            monitor_ws = ExtractSpectra(InputWorkspace=ws, OutputWorkspace=monitor_ws_name,  WorkspaceIndexList=monitors)
            self.setProperty("MonitorWorkspace", monitor_ws)

        if self.getProperty("DetectorWorkspace").valueAsStr and self.getProperty("MonitorWorkspace").valueAsStr:
            detector_ws.setMonitorWorkspace(monitor_ws)

AlgorithmFactory.subscribe(SeparateMonitorSpectra)
