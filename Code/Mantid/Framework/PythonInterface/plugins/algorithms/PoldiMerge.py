from mantid.kernel import StringArrayProperty, Direction
from mantid.simpleapi import *
from mantid.api import *

import numpy as np

class PoldiMerge(PythonAlgorithm):
    comparedPropertyNames = ["TablePositionX", "TablePositionY", "TablePositionZ", "ChopperSpeed"]
    outputWorkspaceName = None

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

    def PyExec(self):
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
          for j in range(workspaceCount):
            if i != j:
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

        return self.propertiesMatch(leftRun, rightRun)

    def timingsMatch(self, leftXData, rightXData):
        leftDeltaX = leftXData[1] - leftXData[0]
        rightDeltaX = rightXData[1] - rightXData[0]

        return abs(leftDeltaX - rightDeltaX) < 1e-4 and abs(rightXData[0] - leftXData[0]) < 1e-4

    def propertiesMatch(self, leftRun, rightRun):
        for propertyName in self.comparedPropertyNames:
            if abs(self.getPropertyValue(leftRun.getProperty(propertyName)) - self.getPropertyValue(rightRun.getProperty(propertyName))) > 1e-4:
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
