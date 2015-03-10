#pylint: disable=no-init,invalid-name
from mantid.kernel import StringArrayProperty, Direction
from mantid.simpleapi import *
from mantid.api import *

import numpy as np

class PoldiMerge(PythonAlgorithm):
    comparedPropertyNames = ["TablePositionX", "TablePositionY", "TablePositionZ"]
    comparedInstrumentParameters = [("detector", "two_theta"),
                                    ("chopper", "t0"),
                                    ("chopper", "t0_const")]
    outputWorkspaceName = None
    checkInstruments = True

    def category(self):
        return "SINQ\\Poldi"

    def name(self):
        return "PoldiMerge"

    def summary(self):
        return "PoldiMerge takes a list of workspace names and adds the counts, resulting in a new workspace."

    def PyInit(self):
        self.declareProperty(StringArrayProperty(name="WorkspaceNames",
                                                 direction=Direction.Input),
                             doc="List of Workspace names to merge.")

        self.declareProperty(WorkspaceProperty(name="OutputWorkspace",
                                               defaultValue="MergedPoldiWorkspaces",
                                               direction=Direction.Output),
                             doc="Workspace where all counts from the list workspaces have been added")

        self.declareProperty("CheckInstruments", True, "If checked, only workspaces with equal"\
                                "instrument parameters are merged. Do not disable without a very good reason.")

    def PyExec(self):
        self.checkInstruments = self.getProperty("CheckInstruments").value

        workspaceNames = self.getProperty("WorkspaceNames").value
        self.outputWorkspaceName = self.getProperty("OutputWorkspace").valueAsStr

        self.log().information("Workspaces to merge: %i" % (len(workspaceNames)))

        workspaces = []

        for wsName in workspaceNames:
            if not AnalysisDataService.doesExist(wsName):
                raise KeyError("Not all strings in the input list are valid workspace names.")

            ws = AnalysisDataService.retrieve(wsName)
            workspaces += self.getWorkspacesRecursive(ws)


        workspaceCount = len(workspaces)

        for i in range(workspaceCount):
            currentWorkspace = workspaces[i]
            for j in range(i + 1, workspaceCount):
                try:
                    self.canMerge(currentWorkspace, workspaces[j])
                except RuntimeError as error:
                    self.handleError(error)

        output = MergeRuns(workspaceNames)

        self.setProperty("OutputWorkspace", output)

    def canMerge(self, leftWorkspace, rightWorkspace):
        if not self.timingsMatch(leftWorkspace.dataX(0), rightWorkspace.dataX(0)):
            raise RuntimeError("Timings don't match")

        leftRun = leftWorkspace.getRun()
        rightRun = rightWorkspace.getRun()

        if not self.chopperSpeedsMatch(leftRun, rightRun):
            raise RuntimeError("Chopper speeds do not match (" + '&'.join((leftWorkspace.getName(), rightWorkspace.getName())) + ")")

        return self.propertiesMatch(leftRun, rightRun) and self.instrumentsMatch(leftWorkspace, rightWorkspace)

    def timingsMatch(self, leftXData, rightXData):
        leftDeltaX = leftXData[1] - leftXData[0]
        rightDeltaX = rightXData[1] - rightXData[0]

        return abs(leftDeltaX - rightDeltaX) < 1e-4 and abs(rightXData[0] - leftXData[0]) < 1e-4

    def chopperSpeedsMatch(self, leftRun, rightRun):
        chopperSpeedLeft = self.makePlausibleChopperSpeed(self.getPropertyValue(leftRun.getProperty("ChopperSpeed")))
        chopperSpeedRight = self.makePlausibleChopperSpeed(self.getPropertyValue(rightRun.getProperty("ChopperSpeed")))

        return abs(chopperSpeedLeft - chopperSpeedRight) < 1e-4

    def makePlausibleChopperSpeed(self, chopperSpeed):
        # This is related to ticket #10090, where a new field in new data is used
        # when that ticket is finished, new data files will not need this
        # cleanup method anymore.
        return np.floor((chopperSpeed + 250.0) / 500.0) * 500.0

    def instrumentsMatch(self, leftWorkspace, rightWorkspace):
        leftInstrument = leftWorkspace.getInstrument()
        rightInstrument = rightWorkspace.getInstrument()

        return (not self.checkInstruments) or self.instrumentParametersMatch(leftInstrument, rightInstrument)

    def instrumentParametersMatch(self, leftInstrument, rightInstrument):
        if not leftInstrument.getDetector(0).getPos() == rightInstrument.getDetector(0).getPos():
            raise RuntimeError("Detector positions are not equal")

        for parameterTuple in self.comparedInstrumentParameters:
            leftValue = self.getParameterValue(leftInstrument, parameterTuple)
            rightValue = self.getParameterValue(rightInstrument, parameterTuple)

            if abs(leftValue - rightValue) > 1e-12:
                raise RuntimeError("Instrument parameter '%s'/'%s' does not match" % parameterTuple)

        return True

    def getParameterValue(self, instrument, parameterTuple):
        return instrument.getComponentByName(parameterTuple[0]).getNumberParameter(parameterTuple[1])[0]

    def propertiesMatch(self, leftRun, rightRun):
        for propertyName in self.comparedPropertyNames:
            if abs(self.getPropertyValue(leftRun.getProperty(propertyName)) - self.getPropertyValue(rightRun.getProperty(propertyName))) > 5e-3:
                raise RuntimeError("Property '%s' does not match" % (propertyName))

        return True

    def getPropertyValue(self, runProperty):
        try:
            return runProperty.value[0]
        except:
            return runProperty.value

    def handleError(self, error):
        if AnalysisDataService.doesExist(self.outputWorkspaceName):
            AnalysisDataService.remove(self.outputWorkspaceName)

        raise RuntimeError("Workspaces can not be merged. %s. Aborting." % (str(error)))

    def getWorkspacesRecursive(self, workspace):
        returnList = []
        if isinstance(workspace, WorkspaceGroup):
            for i in range(workspace.getNumberOfEntries()):
                returnList += self.getWorkspacesRecursive(workspace.getItem(i))

        elif isinstance(workspace, MatrixWorkspace):
            returnList.append(workspace)

        else:
            raise RuntimeError("Can only merge MatrixWorkspaces, this is " + type(workspace))

        return returnList

AlgorithmFactory.subscribe(PoldiMerge)
