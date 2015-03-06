# pylint: disable=no-init,invalid-name
from mantid.kernel import Direction, IntBoundedValidator
from mantid.simpleapi import *
from mantid.api import *

import numpy as np


class PoldiLoadRuns(PythonAlgorithm):
    def category(self):
        return "SINQ\\Poldi"

    def name(self):
        return "PoldiLoadRuns"

    def summary(self):
        return "PoldiLoadRuns loads, checks and merges ranges of POLDI data files for further processing."

    def PyInit(self):
        self.declareProperty('Year', 2014, direction=Direction.Input, doc="The year in which all runs were recorded.")
        self.declareProperty('FirstRun', direction=Direction.Input,
                             doc=("Run number of the first run. "
                                  "If only this number is supplied, only this run is processed."),
                             validator=IntBoundedValidator(lower=1))

        self.declareProperty('LastRun', 1, direction=Direction.Input, doc="Run number of the last run.",
                             validator=IntBoundedValidator(lower=1))

        self.declareProperty('MergeWidth', 1, direction=Direction.Input, doc="Number of runs to merge.",
                             validator=IntBoundedValidator(lower=1))

        self.declareProperty(WorkspaceProperty(name='OutputWorkspace',
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc="Name of the output group workspace that contains all data workspaces.")


    def PyExec(self):
        year = self.getProperty('Year').value

        firstRun = self.getProperty('FirstRun').value
        lastRun = self.getProperty('LastRun').value
        mergeWidth = self.getProperty('MergeWidth').value

        outputWorkspaceName = self.getProperty("OutputWorkspace").valueAsStr

        mergeRange = self.getActualMergeRange(firstRun, lastRun, mergeWidth)

        outputWorkspaces = []

        for i in range(mergeRange[0], mergeRange[1] + 1, mergeWidth):
            currentNameNumor = i + mergeWidth - 1

            workspaceNames = []
            for j in range(i, i + mergeWidth):
                LoadSINQ("POLDI", 2014, j, OutputWorkspace="data_" + str(j))
                LoadInstrument("data_" + str(j), InstrumentName="POLDI")
                PoldiTruncateData(InputWorkspace="data_" + str(j), OutputWorkspace="data_" + str(j))
                workspaceNames.append("data_" + str(j))

            if mergeWidth > 1:
                PoldiMerge(workspaceNames, OutputWorkspace="data_" + str(currentNameNumor))

                for j in range(i, i + mergeWidth - 1):
                    DeleteWorkspace("data_" + str(j))

            outputWorkspaces.append("data_" + str(currentNameNumor))

        GroupWorkspaces(outputWorkspaces, OutputWorkspace=outputWorkspaceName)

        self.setProperty("OutputWorkspace", outputWorkspaceName)

    def getActualMergeRange(self, firstRun, lastRun, mergeWidth):
        actualLastRun = lastRun
        rangeWidth = actualLastRun - firstRun + 1
        remainder = rangeWidth % mergeWidth

        if remainder != 0:
            self.log().warning(("Number of runs is not compatible with selected merge width. "
                                "Leaving out the last " + str(remainder) + " file(s)."))

            actualLastRun = lastRun - remainder

        return (firstRun, actualLastRun)


AlgorithmFactory.subscribe(PoldiLoadRuns)
