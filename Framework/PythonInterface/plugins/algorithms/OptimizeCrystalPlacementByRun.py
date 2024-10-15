# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import mtd, AnalysisDataService, PythonAlgorithm, AlgorithmFactory, ITableWorkspaceProperty
from mantid.kernel import logger, Direction
from mantid.simpleapi import (
    CombinePeaksWorkspaces,
    FilterPeaks,
    FindUBUsingIndexedPeaks,
    GroupWorkspaces,
    IndexPeaks,
    OptimizeCrystalPlacement,
    StatisticsOfTableWorkspace,
    RenameWorkspace,
)


class OptimizeCrystalPlacementByRun(PythonAlgorithm):
    def summary(self):
        return "Optimizes the sample position for each run in a peaks workspace."

    def category(self):
        return "Crystal\\Corrections"

    def seeAlso(self):
        return ["OptimizeCrystalPlacement"]

    def PyInit(self):
        # Declare properties
        self.declareProperty(
            ITableWorkspaceProperty("InputWorkspace", "", Direction.Input), "The name of the peaks workspace that will be optimized."
        )
        self.declareProperty("Tolerance", 0.15, "Tolerance of indexing of peaks.")
        self.declareProperty("OutputWorkspace", "", "The name of the peaks workspace that will be created.")

    def PyExec(self):
        ws = self.getProperty("InputWorkspace").value
        ws_append = self.getProperty("OutputWorkspace").value
        ws_group = "ws_group"
        tolerance = self.getProperty("Tolerance").value
        if not ws.sample().hasOrientedLattice():
            FindUBUsingIndexedPeaks(PeaksWorkspace=ws, Tolerance=tolerance)
        result = IndexPeaks(PeaksWorkspace=ws, Tolerance=tolerance)
        logger.notice("Initial Number indexed: %s error: %s\n" % (result[0], result[1]))
        stats = StatisticsOfTableWorkspace(InputWorkspace=ws)
        stat_col = stats.column("Statistic")
        minR = int(stats.column("RunNumber")[stat_col.index("Minimum")])
        maxR = int(stats.column("RunNumber")[stat_col.index("Maximum")]) + 1
        AnalysisDataService.remove(stats.name())
        group = []
        for run in range(minR, maxR):
            FilterPeaks(InputWorkspace=ws, OutputWorkspace=str(run), FilterVariable="RunNumber", FilterValue=run, Operator="=")
            run = mtd[str(run)]
            peaks = run.getNumberPeaks()
            if peaks == 0:
                AnalysisDataService.remove(str(run))
            else:
                group.append(str(run))
        GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=ws_group)
        OptimizeCrystalPlacement(
            PeaksWorkspace=ws_group,
            ModifiedPeaksWorkspace=ws_group,
            AdjustSampleOffsets=True,
            MaxSamplePositionChangeMeters=0.005,
            MaxIndexingError=tolerance,
        )
        RenameWorkspace(InputWorkspace=str(minR), OutputWorkspace=ws_append)
        for run in range(minR + 1, maxR):
            if AnalysisDataService.doesExist(str(run)):
                CombinePeaksWorkspaces(LHSWorkspace=ws_append, RHSWorkspace=str(run), OutputWorkspace=ws_append)
                logger.notice("Optimized %s sample position: %s\n" % (str(run), mtd[str(run)].getPeak(0).getSamplePos()))
                AnalysisDataService.remove(str(run))
        result = IndexPeaks(PeaksWorkspace=ws_append, Tolerance=tolerance)
        logger.notice("After Optimization Number indexed: %s error: %s\n" % (result[0], result[1]))
        AnalysisDataService.remove(ws_group)
        self.setProperty("OutputWorkspace", ws_append)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(OptimizeCrystalPlacementByRun)
