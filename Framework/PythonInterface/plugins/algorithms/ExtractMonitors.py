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
        spectrumInfo = in_ws.spectrumInfo()
        for i in range(in_ws.getNumberHistograms()):
            try:
                monitors.append(i) if spectrumInfo.isMonitor(i) else detectors.append(i)
            except RuntimeError:
                self.log().warning("Missing detector at " + str(i))

        if detector_ws_name:
            if detectors:
                extract_alg = self.createChildAlgorithm("ExtractSpectra")
                extract_alg.setProperty("InputWorkspace", in_ws)
                extract_alg.setProperty("WorkspaceIndexList", detectors)
                extract_alg.execute()
                detector_ws = extract_alg.getProperty("OutputWorkspace").value
                self.setProperty("DetectorWorkspace", detector_ws)
            else:
                self.log().error("No detectors found in input workspace. No detector output workspace created.")

        if monitor_ws_name:
            if monitors:
                extract_alg = self.createChildAlgorithm("ExtractSpectra")
                extract_alg.setProperty("InputWorkspace", in_ws)
                extract_alg.setProperty("WorkspaceIndexList", monitors)
                extract_alg.execute()
                monitor_ws = extract_alg.getProperty("OutputWorkspace").value
                self.setProperty("MonitorWorkspace", monitor_ws)
            else:
                self.log().error("No monitors found in input workspace. No monitor output workspace created.")

        if detector_ws_name and detectors and monitor_ws_name and monitors:
            detector_ws.setMonitorWorkspace(monitor_ws)

AlgorithmFactory.subscribe(ExtractMonitors)
