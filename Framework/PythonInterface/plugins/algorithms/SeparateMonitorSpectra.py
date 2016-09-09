from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import (MatrixWorkspaceProperty, DataProcessorAlgorithm, PropertyMode)

class SeparateMonitorSpectra(DataProcessorAlgorithm):
    def category(self):
        return 'Utility\\Workspaces'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',
                                                     direction=Direction.Input),
                             doc='A workspace with detectors and monitors')

        self.declareProperty(MatrixWorkspaceProperty('DetectorsWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output workspace with detectors only')

        self.declareProperty(MatrixWorkspaceProperty('MonitorsWorkspace', '',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='The output workspace with monitors only')

    def validateInputs(self):
        issues = {}

        if not self.getProperty("DetectorsWorkspace").valueAsStr and not self.getProperty("MonitorsWorkspace").valueAsStr:
            msg = "Must specify one of DetectorsWorkspace or MonitorsWorkspace"
            issues["DetectorsWorkspace"] = msg
            issues["MonitorsWorkspace"] = msg
        elif self.getProperty("DetectorsWorkspace").valueAsStr == self.getProperty("MonitorsWorkspace").valueAsStr:
            msg = "DetectorsWorkspace and MonitorsWorkspace must be different"
            issues["DetectorsWorkspace"] = msg
            issues["MonitorsWorkspace"] = msg

        return issues

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        detector_ws_name = self.getProperty("DetectorsWorkspace").valueAsStr
        monitor_ws_name = self.getProperty("MonitorsWorkspace").valueAsStr

        try:
            mon = ws.getMonitorWorkspace()
            raise ValueError("Monitor workspace already exists, called: " + mon.name() + ".")
        except RuntimeError:
            pass

        monitors = []
        detectors = []
        for i in range(ws.getNumberHistograms()):
            monitors.append(i) if ws.getDetector(i).isMonitor() else detectors.append(i)
        print(detectors, monitors)
        detector_ws = ExtractSpectra(InputWorkspace=ws, OutputWorkspace=detector_ws_name, WorkspaceIndexList=detectors)
        monitor_ws = ExtractSpectra(InputWorkspace=ws, OutputWorkspace=monitor_ws_name,  WorkspaceIndexList=monitors)

        self.setProperty("DetectorsWorkspace", detector_ws)
        self.setProperty("MonitorsWorkspace", monitor_ws)

AlgorithmFactory.subscribe(SeparateMonitorSpectra)
