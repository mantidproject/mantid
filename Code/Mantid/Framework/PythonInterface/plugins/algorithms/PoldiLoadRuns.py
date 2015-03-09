# pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *

from datetime import date


class PoldiLoadRuns(PythonAlgorithm):
    def category(self):
        return "SINQ\\Poldi"

    def name(self):
        return "PoldiLoadRuns"

    def summary(self):
        return "PoldiLoadRuns loads, checks and merges ranges of POLDI data files for further processing."

    def PyInit(self):
        today = date.today()

        self.declareProperty('Year', today.year, direction=Direction.Input,
                             doc="The year in which all runs were recorded.")

        firstRunValidator = CompositeValidator()
        firstRunValidator.add(IntMandatoryValidator())
        firstRunValidator.add(IntBoundedValidator(lower=1))
        self.declareProperty('FirstRun', 1, direction=Direction.Input,
                             doc=("Run number of the first run. "
                                  "If only this number is supplied, only this run is processed."),
                             validator=firstRunValidator)

        self.declareProperty('LastRun', 1, direction=Direction.Input, doc="Run number of the last run.",
                             validator=IntBoundedValidator(lower=1))

        self.declareProperty('MergeWidth', 1, direction=Direction.Input, doc="Number of runs to merge.",
                             validator=IntBoundedValidator(lower=1))

        self.declareProperty('OverwriteExistingGroups', False, direction=Direction.Input,
                             doc="If a WorkspaceGroup already exists, overwrite it.")

        self.declareProperty(WorkspaceProperty(name='OutputWorkspace',
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc="Name of the output group workspace that contains all data workspaces.")


    def PyExec(self):
        year = self.getProperty('Year').value

        # First run is mandatory, so it must be there.
        firstRun = self.getProperty('FirstRun').value

        # For cases where LastRun is not set, only a single file is loaded.
        lastRun = firstRun

        # Get number of last run, if not default
        lastRunProperty = self.getProperty('LastRun')
        if not lastRunProperty.isDefault:
            lastRun = lastRunProperty.value

        # Get mergewidth
        mergeWidth = self.getProperty('MergeWidth').value

        # Construct the names of the workspaces using the output workspace name to avoid ambiguities.
        outputWorkspaceName = self.getProperty("OutputWorkspace").valueAsStr
        nameTemplate = outputWorkspaceName + "_data_"

        # Get the actual merge range, if the number of files is not compatible with the number of files to merge.
        mergeRange = self.getActualMergeRange(firstRun, lastRun, mergeWidth)

        outputWorkspaces = []

        for i in range(mergeRange[0], mergeRange[1] + 1, mergeWidth):
            # The name of the possibly merged workspace is this the last name of the merged series.
            currentNameNumor = i + mergeWidth - 1
            currentTotalWsName = nameTemplate + str(currentNameNumor)

            workspaceNames = []
            for j in range(i, i + mergeWidth):
                currentWsName = nameTemplate + str(j)

                # Errors are handled by writing a message to the log, so the user can check the files.
                try:
                    LoadSINQ("POLDI", 2014, j, OutputWorkspace=currentWsName)
                    LoadInstrument(currentWsName, InstrumentName="POLDI")
                    PoldiTruncateData(InputWorkspace=currentWsName, OutputWorkspace=currentWsName)
                    workspaceNames.append(currentWsName)
                except:
                    self.log().warning("Could not load run no. " + str(j) + ", skipping.")

            # If there are multiple workspaces, they need to be merged.
            if mergeWidth > 1 and len(workspaceNames) > 1:
                # If workspaces are not compatible, the range is skipped and the workspaces deleted.
                try:
                    PoldiMerge(workspaceNames, OutputWorkspace=currentTotalWsName)
                except:
                    self.log().warning(
                        "Could not merge range [" + str(i) + ", " + str(currentNameNumor) + "], skipping.")

                for j in range(i, i + mergeWidth - 1):
                    DeleteWorkspace(nameTemplate + str(j))

            # If the workspace is still valid (merging could have failed), it's appended to the output.
            if AnalysisDataService.doesExist(currentTotalWsName):
                outputWorkspaces.append(currentTotalWsName)

        # No workspaces, return - the algorithm will fail with an error. Additional log entry.
        if len(outputWorkspaces) == 0:
            self.log().error("No output workspaces loaded.")
            return

        # If any output was produced, it needs to be checked what to do with it.
        overwriteWorkspaces = self.getProperty('OverwriteExistingGroups').value

        # If a workspace with that name already exists, check what type it is.
        if AnalysisDataService.doesExist(outputWorkspaceName):
            existingWorkspace = AnalysisDataService.retrieve(outputWorkspaceName)

            if self.isGroupWorkspace(existingWorkspace):
                self.log().notice("WorkspaceGroup'" + outputWorkspaceName + "' already exists, adding new data.")

                # Put all created workspaces into the existing group
                self.addWorkspacesToWorkspaceGroup(outputWorkspaces, existingWorkspace)

                if overwriteWorkspaces:
                    self.log().notice("Deleting old data from existing WorkspaceGroup.")

                    # Workspaces in that group that were not loaded now are deleted.
                    self.deleteWorkspaceFromGroupIfNotInList(outputWorkspaces, existingWorkspace)

                # End of this case - provide output workspace.
                self.setProperty("OutputWorkspace", outputWorkspaceName)
            else:
                if overwriteWorkspaces:
                    # In this case, we don't need to do anything, just log what's happening.
                    self.log().notice("Workspace '" + outputWorkspaceName + "' already exists, deleting data.")
                else:
                    self.log().error(
                        "Workspace already exists, is not a WorkspaceGroup and is not supposed to be overwritten")
                    return

        # Otherwise, we can just group the output workspaces from above.
        self.setProperty("OutputWorkspace", GroupWorkspaces(outputWorkspaces))


    def getActualMergeRange(self, firstRun, lastRun, mergeWidth):
        actualLastRun = lastRun
        rangeWidth = actualLastRun - firstRun + 1
        remainder = rangeWidth % mergeWidth

        if remainder != 0:
            self.log().warning(("Number of runs is not compatible with selected merge width. "
                                "Leaving out the last " + str(remainder) + " file(s)."))

            actualLastRun = lastRun - remainder

        return (firstRun, actualLastRun)

    def isGroupWorkspace(self, workspace):
        return issubclass(type(workspace), WorkspaceGroup)

    def addWorkspacesToWorkspaceGroup(self, workspaces, workspaceGroup):
        for ws in workspaces:
            workspaceGroup.add(ws)

    def deleteWorkspaceFromGroupIfNotInList(self, workspaceList, workspaceGroup):
        for ws in workspaceGroup.getNames():
            if ws not in workspaceList:
                DeleteWorkspace(ws)


AlgorithmFactory.subscribe(PoldiLoadRuns)
