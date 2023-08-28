# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceGroup
from mantid.kernel import Direction
from mantid.simpleapi import mtd


class InterpolateBackground(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Reduction;Diffraction\\Utility"

    def summary(self):
        return "Interpolates the background temperature for two empty container runs."

    def version(self):
        return 1

    def PyInit(self):
        self.declareProperty(
            name="WorkspaceGroup",
            defaultValue="",
            direction=Direction.Input,
            doc="Workspace Group with two workspaces containing data from empty container runs",
        )
        self.declareProperty(name="InterpolateTemp", defaultValue="", direction=Direction.Input, doc="Target temperature to interpolate at")
        self.declareProperty(
            name="OutputWorkspace", defaultValue="Output", direction=Direction.Output, doc="Output Workspace with interpolated data"
        )

    def validateInputs(self):
        issues = dict()

        wsGroup = mtd[self.getProperty("WorkspaceGroup").value]
        if not isinstance(wsGroup, WorkspaceGroup):
            issues["WorkspaceGroup"] = "Input workspace group is not a workspace group"
        else:
            wsNames = wsGroup.getNames()
            if len(wsNames) != 2:
                issues["WorkspaceGroup"] = "Input Workspace Group must have exactly 2 workspaces"
            else:
                ws1 = mtd[wsNames[0]]
                ws2 = mtd[wsNames[1]]
                if ws1.blocksize() != ws2.blocksize():
                    issues["WorkspaceGroup"] = "Workspaces must have the same number of bins"
                if ws1.getRun().hasProperty("SampleTemp") and ws2.getRun().hasProperty("SampleTemp"):
                    temp1 = int(ws1.getRun().getPropertyAsSingleValue("SampleTemp"))
                    temp2 = int(ws2.getRun().getPropertyAsSingleValue("SampleTemp"))
                    if temp1 < temp2:
                        self.low_temp = temp1
                        self.high_temp = temp2
                        self.low_ws = wsNames[0]
                        self.high_ws = wsNames[1]
                    else:
                        self.low_temp = temp2
                        self.high_temp = temp1
                        self.low_ws = wsNames[1]
                        self.high_ws = wsNames[0]
                    self.interpo_temp = int(self.getProperty("InterpolateTemp").value)
                    if not self.low_temp < self.interpo_temp < self.high_temp:
                        issues["InterpolateTemp"] = f"Interpolated temp must be a value between {self.low_temp} and {self.high_temp}"
                else:
                    issues["WorkspaceGroup"] = "A workspace in WorkspaceGroup is missing 'SampleTemp' property"

        return issues

    def PyExec(self):
        lowWS = mtd[self.low_ws]
        highWS = mtd[self.high_ws]

        scalar = (self.interpo_temp - self.low_temp) / (self.high_temp - self.interpo_temp)
        outputWS = lowWS + scalar * highWS - scalar * lowWS

        self.setProperty("OutputWorkspace", outputWS.name())

        return outputWS


# Register algorithm with Mantid
AlgorithmFactory.subscribe(InterpolateBackground)
