# pylint: disable=no-init,invalid-name,bare-except
from mantid.kernel import *
from mantid.simpleapi import *
from mantid.api import *

from datetime import date


class PoldiLoadRuns(PythonAlgorithm):
    _nameTemplate = ""
    _mergeCheckEnabled = True
    _autoMaskBadDetectors = True

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

        self.declareProperty('OverwriteExistingWorkspace', False, direction=Direction.Input,
                             doc="If a WorkspaceGroup already exists, overwrite it.")

        self.declareProperty('EnableMergeCheck', True, direction=Direction.Input,
                             doc="Enable all the checks in PoldiMerge. Do not deactivate without very good reason.")

        self.declareProperty('MaskBadDetectors', True, direction=Direction.Input,
                             doc=('Automatically disable detectors with unusually small or large values, in addition'
                                  ' to those masked in the instrument definition.'))

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

        # If lastRun is smaller than firstRun, just swap and continue
        if firstRun > lastRun:
            firstRun, lastRun = lastRun, firstRun

        # Get mergewidth
        mergeWidth = self.getProperty('MergeWidth').value

        # Construct the names of the workspaces using the output workspace name to avoid ambiguities.
        outputWorkspaceName = self.getProperty("OutputWorkspace").valueAsStr
        self._nameTemplate = outputWorkspaceName + "_data_"

        # If any output was produced, it needs to be checked what to do with it.
        overwriteWorkspaces = self.getProperty('OverwriteExistingWorkspace').value

        # One case can be caught before loading any data. If it's not a WorkspaceGroup and it should not be
        # overwritten, there's nothing to do, so there's no need to load anything.
        if AnalysisDataService.doesExist(outputWorkspaceName):
            if not self.isGroupWorkspace(AnalysisDataService.retrieve(outputWorkspaceName)) and not overwriteWorkspaces:
                self.log().error("Workspace '" + outputWorkspaceName + "' already exists, is not a WorkspaceGroup "
                                                                       "and is not supposed to be overwritten, aborting.")
                return


        # Get the actual merge range, if the number of files is not compatible with the number of files to merge.
        mergeRange = self.getActualMergeRange(firstRun, lastRun, mergeWidth)

        # PoldiMerge checks that instruments are compatible, but it can be disabled (calibration measurements)
        self._mergeCheckEnabled = self.getProperty('EnableMergeCheck').value

        # The same for removing additional dead or misbehaving wires
        self._autoMaskBadDetectors = self.getProperty('MaskBadDetectors').value

        # Get a list of output workspace names.
        outputWorkspaces = self.getLoadedWorkspaceNames(year, mergeRange, mergeWidth)

        # No workspaces, return - the algorithm will fail with an error. Additional log entry.
        if len(outputWorkspaces) == 0:
            self.log().error("No output workspaces loaded.")
            return

        # If a workspace with that name already exists, check what type it is.
        if AnalysisDataService.doesExist(outputWorkspaceName):
            existingWorkspace = AnalysisDataService.retrieve(outputWorkspaceName)

            if self.isGroupWorkspace(existingWorkspace):
                self.log().notice("WorkspaceGroup'" + outputWorkspaceName + "' already exists, adding new data.")

                # Put all created workspaces into the existing group
                self.addWorkspacesToWorkspaceGroup(outputWorkspaces, existingWorkspace)

                # If the mode is set to overwrite, the WorkspaceGroup is not actually deleted but "refilled".
                if overwriteWorkspaces:
                    self.log().notice("Deleting old data from existing WorkspaceGroup.")

                    # Workspaces in that group that were not loaded now are deleted.
                    self.deleteWorkspaceFromGroupIfNotInList(outputWorkspaces, existingWorkspace)

                # End of this case - provide output workspace.
                self.setProperty("OutputWorkspace", outputWorkspaceName)
                return

        # Otherwise, we can just group the output workspaces from above.
        self.setProperty("OutputWorkspace", GroupWorkspaces(outputWorkspaces))

    # Get the actual range of run numbers (truncated if number of files is not compatible with merge width)
    def getActualMergeRange(self, firstRun, lastRun, mergeWidth):
        actualLastRun = lastRun
        rangeWidth = actualLastRun - firstRun + 1
        remainder = rangeWidth % mergeWidth

        if remainder != 0:
            self.log().warning(("Number of runs is not compatible with selected merge width. "
                                "Leaving out the last " + str(remainder) + " file(s)."))

            actualLastRun = lastRun - remainder

        return (firstRun, actualLastRun)

    # Load workspaces and return a list of workspaces that were actually loaded.
    def getLoadedWorkspaceNames(self, year, mergeRange, mergeWidth):
        outputWorkspaces = []
        for i in range(mergeRange[0], mergeRange[1] + 1, mergeWidth):
            # The name of the possibly merged workspace is this the last name of the merged series.
            currentNameNumor = i + mergeWidth - 1
            currentTotalWsName = self._nameTemplate + str(currentNameNumor)

            workspaceNames = []
            for j in range(i, i + mergeWidth):
                currentWsName = self._nameTemplate + str(j)

                # Errors are handled by writing a message to the log, so the user can check the files.
                try:
                    self.loadAndTruncateData(currentWsName, year, j)
                    workspaceNames.append(currentWsName)
                except:
                    self.log().warning("Could not load run no. " + str(j) + ", skipping.")

            # If there are multiple workspaces, they need to be merged.
            if mergeWidth > 1 and len(workspaceNames) > 1:
                # If workspaces are not compatible, the range is skipped and the workspaces deleted.
                try:
                    PoldiMerge(workspaceNames, OutputWorkspace=currentTotalWsName,
                               CheckInstruments=self._mergeCheckEnabled)
                except:
                    self.log().warning(
                        "Could not merge range [" + str(i) + ", " + str(currentNameNumor) + "], skipping.")

                # Delete all workspaces that contributed to the merged one.
                for j in range(i, i + mergeWidth - 1):
                    DeleteWorkspace(self._nameTemplate + str(j))

            # If the workspace is still valid (merging could have failed), it's processed further
            if AnalysisDataService.doesExist(currentTotalWsName):
                # If the option is enabled, mask detectors that are likely to be misbehaving
                if self._autoMaskBadDetectors:
                    self.log().information("Masking bad detectors automatically.")
                    self.autoMaskBadDetectors(currentTotalWsName)

                outputWorkspaces.append(currentTotalWsName)

        return outputWorkspaces

    # Execute LoadSINQ, LoadInstrument and PoldiTruncateData
    def loadAndTruncateData(self, workspaceName, year, j):
        LoadSINQ("POLDI", year, j, OutputWorkspace=workspaceName)
        LoadInstrument(workspaceName, InstrumentName="POLDI")
        PoldiTruncateData(InputWorkspace=workspaceName, OutputWorkspace=workspaceName)

    # Automatically determine bad detectors and mask them
    def autoMaskBadDetectors(self, currentTotalWsName):
        Integration(currentTotalWsName, OutputWorkspace='integrated')

        MedianDetectorTest('integrated', SignificanceTest=4.0, HighOutlier=400, CorrectForSolidAngle=False,
                           OutputWorkspace='maskWorkspace')

        MaskDetectors(Workspace=AnalysisDataService.retrieve(currentTotalWsName), MaskedWorkspace='maskWorkspace')

        # Clean up
        DeleteWorkspace('integrated')
        DeleteWorkspace('maskWorkspace')

    # Returns true if the supplied workspace is a WorkspaceGroup
    def isGroupWorkspace(self, workspace):
        return issubclass(type(workspace), WorkspaceGroup)

    # Add workspaces with names in list to WorkspaceGroup
    def addWorkspacesToWorkspaceGroup(self, workspaces, workspaceGroup):
        for ws in workspaces:
            workspaceGroup.add(ws)

    # Delete all workspaces in WorkspaceGroup that are NOT in workspaceList
    def deleteWorkspaceFromGroupIfNotInList(self, workspaceList, workspaceGroup):
        for ws in workspaceGroup.getNames():
            if ws not in workspaceList:
                DeleteWorkspace(ws)


AlgorithmFactory.subscribe(PoldiLoadRuns)
