# pylint: disable=no-init,invalid-name
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *

class PoldiDataAnalysis(PythonAlgorithm):
    """
    This workflow algorithm uses all of the POLDI specific algorithms to perform a complete data analysis,
    starting from the correlation method and preliminary 1D-fits, proceeding with either one or two passses
    of 2D-fitting.

    All resulting workspaces are grouped together at the end so that they are all in one place.
    """

    def category(self):
        return "SINQ\\Poldi"

    def name(self):
        return "PoldiDataAnalysis"

    def summary(self):
        return "Run all necessary steps for a complete analysis of POLDI data."

    def checkGroups(self):
        return False

    def PyInit(self):
        self.declareProperty(WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input),
                             doc='MatrixWorkspace with 2D POLDI data and valid POLDI instrument.')

        self.declareProperty("MaximumPeakNumber", 10, direction=Direction.Input,
                             doc='Maximum number of peaks to process in the analysis.')

        self.declareProperty("MinimumPeakSeparation", 10, direction=Direction.Input,
                             doc='Minimum number of points between neighboring peaks.')

        self.declareProperty(WorkspaceProperty("ExpectedPeaks", defaultValue="", direction=Direction.Input),
                             doc='TableWorkspace or WorkspaceGroup with expected peaks used for indexing.')

        allowedProfileFunctions = StringListValidator(["Gaussian", "Lorentzian", "PseudoVoigt", "Voigt"])
        self.declareProperty("ProfileFunction", "Gaussian", validator=allowedProfileFunctions,
                             direction=Direction.Input)

        self.declareProperty("PawleyFit", False, direction=Direction.Input,
                             doc='Should the 2D-fit determine lattice parameters?')

        self.declareProperty("MultipleRuns", False, direction=Direction.Input,
                             doc=('If this is activated, peaks are searched again in the'
                                  'residuals and the 1D- and 2D-fit is repeated '
                                  'with these data.'))

        self.declareProperty("PlotResult", True, direction=Direction.Input,
                             doc=('If this is activated, plot the sum of residuals and calculated spectrum together '
                                  'with the theoretical spectrum and the residuals.'))

        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
                             doc='WorkspaceGroup with result data from all processing steps.')

    def PyExec(self):
        self.outputWorkspaces = []

        self.baseName = self.getProperty("InputWorkspace").valueAsStr
        self.inputWorkspace = self.getProperty("InputWorkspace").value
        self.expectedPeaks = self.getProperty("ExpectedPeaks").value
        self.profileFunction = self.getProperty("ProfileFunction").value

        correlationSpectrum = self.runCorrelation()
        self.outputWorkspaces.append(correlationSpectrum)

        self.numberOfExecutions = 0
        self.outputWorkspaces += self.runMainAnalysis(correlationSpectrum)

        outputWs = GroupWorkspaces(self.outputWorkspaces[0])

        for ws in self.outputWorkspaces[1:]:
            outputWs.add(ws.getName())

        RenameWorkspace(outputWs, self.getProperty("OutputWorkspace").valueAsStr)

        self.setProperty("OutputWorkspace", outputWs)

    def runCorrelation(self):
        correlationName = self.baseName + "_correlation"
        PoldiAutoCorrelation(self.inputWorkspace, OutputWorkspace=correlationName)

        return AnalysisDataService.retrieve(correlationName)

    def runMainAnalysis(self, correlationSpectrum):
        self.numberOfExecutions += 1
        outputWorkspaces = []

        rawPeaks = self.runPeakSearch(correlationSpectrum)
        outputWorkspaces.append(rawPeaks)

        refinedPeaks, fitPlots = self.runPeakFit1D(correlationSpectrum, rawPeaks)
        outputWorkspaces.append(refinedPeaks)
        outputWorkspaces.append(fitPlots)

        indexedPeaks, unindexedPeaks = self.runIndex(refinedPeaks)
        outputWorkspaces.append(indexedPeaks)

        pawleyFit = self.getProperty('PawleyFit').value
        if pawleyFit:
            outputWorkspaces.append(unindexedPeaks)

        fitPeaks2DResult = self.runPeakFit2D(indexedPeaks)
        outputWorkspaces += fitPeaks2DResult

        spectrum2D = fitPeaks2DResult[0]
        spectrum1D = fitPeaks2DResult[1]

        residuals = self.runResidualAnalysis(spectrum2D)
        outputWorkspaces.append(residuals)

        totalName = self.baseName + "_sum"
        Plus(LHSWorkspace=spectrum1D, RHSWorkspace=residuals, OutputWorkspace=totalName)
        total = AnalysisDataService.retrieve(totalName)
        outputWorkspaces.append(total)

        if self.numberOfExecutions == 1:
            self._plotResult(total, spectrum1D, residuals)

        runTwice = self.getProperty('MultipleRuns').value
        if runTwice and self.numberOfExecutions == 1:
            return self.runMainAnalysis(total)
        else:
            return outputWorkspaces

    def runPeakSearch(self, correlationWorkspace):
        peaksName = self.baseName + "_peaks_raw"

        PoldiPeakSearch(InputWorkspace=correlationWorkspace,
                        MaximumPeakNumber=self.getProperty('MaximumPeakNumber').value,
                        MinimumPeakSeparation=self.getProperty('MinimumPeakSeparation').value,
                        OutputWorkspace=peaksName)

        return AnalysisDataService.retrieve(peaksName)

    def runPeakFit1D(self, correlationWorkspace, rawPeaks):
        refinedPeaksName = self.baseName + "_peaks_refined_1d"
        plotNames = self.baseName + "_fit_plots"

        PoldiFitPeaks1D(InputWorkspace=correlationWorkspace,
                        PoldiPeakTable=rawPeaks,
                        PeakFunction=self.profileFunction,
                        OutputWorkspace=refinedPeaksName,
                        FitPlotsWorkspace=plotNames)

        return AnalysisDataService.retrieve(refinedPeaksName), AnalysisDataService.retrieve(plotNames)


    def runIndex(self, peaks):
        indexedPeaksName = self.baseName + "_indexed"

        PoldiIndexKnownCompounds(InputWorkspace=peaks,
                                 CompoundWorkspaces=self.expectedPeaks,
                                 OutputWorkspace=indexedPeaksName)

        indexedPeaks = AnalysisDataService.retrieve(indexedPeaksName)

        # Remove unindexed peaks from group for pawley fit
        unindexedPeaks = indexedPeaks.getItem(indexedPeaks.getNumberOfEntries() - 1)
        pawleyFit = self.getProperty('PawleyFit').value
        if pawleyFit:
            indexedPeaks.remove(unindexedPeaks.getName())

        self._removeEmptyTablesFromGroup(indexedPeaks)

        return indexedPeaks, unindexedPeaks

    def runPeakFit2D(self, peaks):
        spectrum2DName = self.baseName + "_fit2d"
        spectrum1DName = self.baseName + "_fit1d"
        refinedPeaksName = self.baseName + "_peaks_refined_2d"
        refinedCellName = self.baseName + "_cell_refined"

        pawleyFit = self.getProperty('PawleyFit').value

        PoldiFitPeaks2D(InputWorkspace=self.inputWorkspace,
                        PoldiPeakWorkspace=peaks,
                        PeakProfileFunction=self.profileFunction,
                        PawleyFit=pawleyFit,
                        MaximumIterations=100,
                        OutputWorkspace=spectrum2DName,
                        Calculated1DSpectrum=spectrum1DName,
                        RefinedPoldiPeakWorkspace=refinedPeaksName,
                        RefinedCellParameters=refinedCellName)

        workspaces = [AnalysisDataService.retrieve(spectrum2DName),
                      AnalysisDataService.retrieve(spectrum1DName),
                      AnalysisDataService.retrieve(refinedPeaksName)]
        if AnalysisDataService.doesExist(refinedCellName):
            workspaces.append(AnalysisDataService.retrieve(refinedCellName))

        return workspaces

    def runResidualAnalysis(self, calculated2DSpectrum):
        residualName = self.baseName + "_residuals"

        PoldiAnalyseResiduals(MeasuredCountData=self.inputWorkspace,
                              FittedCountData=calculated2DSpectrum,
                              MaxIterations=5,
                              OutputWorkspace=residualName)

        return AnalysisDataService.retrieve(residualName)

    def _removeEmptyTablesFromGroup(self, groupWorkspace):
        deleteNames = []
        for i in range(groupWorkspace.getNumberOfEntries()):
            ws = groupWorkspace.getItem(i)
            if ws.rowCount() == 0:
                deleteNames.append(ws.getName())
        for name in deleteNames:
            DeleteWorkspace(name)

    def _plotResult(self, total, spectrum1D, residuals):
        plotResults = self.getProperty('PlotResult').value

        if plotResults:
            from IndirectImport import import_mantidplot
            plot = import_mantidplot()

            plotWindow = plot.plotSpectrum(total, 0, type=1)
            plotWindow = plot.plotSpectrum(spectrum1D, 0, type=0, window=plotWindow)
            plotWindow = plot.plotSpectrum(residuals, 0, type=0, window=plotWindow)
            plotWindow.activeLayer().setTitle('Fit result for ' + self.baseName)
            plotWindow.activeLayer().removeLegend()




AlgorithmFactory.subscribe(PoldiDataAnalysis())