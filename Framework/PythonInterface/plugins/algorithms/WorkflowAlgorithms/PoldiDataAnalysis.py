# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,attribute-defined-outside-init,too-many-instance-attributes
from mantid.api import AlgorithmFactory, AnalysisDataService, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import (
    Direction,
    StringListValidator,
)
from mantid.simpleapi import (
    plotSpectrum,
    DeleteWorkspace,
    Integration,
    GroupWorkspaces,
    Plus,
    PoldiAnalyseResiduals,
    PoldiFitPeaks1D,
    PoldiFitPeaks2D,
    PoldiIndexKnownCompounds,
    PoldiPeakSearch,
    PoldiAutoCorrelation,
    RenameWorkspace,
    SumSpectra,
)


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
        self._allowedFunctions = ["AsymmetricPearsonVII", "Gaussian", "Lorentzian", "PseudoVoigt", "Voigt"]

        self._globalParameters = {
            "AsymmetricPearsonVII": [],
            "Gaussian": [],
            "Lorentzian": [],
            "PseudoVoigt": ["Mixing"],
            "Voigt": ["LorentzFWHM"],
        }

        self._boundedParameters = {
            "AsymmetricPearsonVII": ["LeftShape", "RightShape"],
            "Gaussian": [],
            "Lorentzian": [],
            "PseudoVoigt": [],
            "Voigt": [],
        }

        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input),
            doc="MatrixWorkspace with 2D POLDI data and valid POLDI instrument.",
        )

        self.declareProperty("MaximumPeakNumber", 10, direction=Direction.Input, doc="Maximum number of peaks to process in the analysis.")

        self.declareProperty(
            "MinimumPeakSeparation", 10, direction=Direction.Input, doc="Minimum number of points between neighboring peaks."
        )

        self.declareProperty(
            "MinimumPeakHeight",
            0.0,
            direction=Direction.Input,
            doc=("Minimum height of peaks. If it is left at 0, the minimum peak height is calculated from background noise."),
        )

        self.declareProperty(
            "MaximumRelativeFwhm",
            0.02,
            direction=Direction.Input,
            doc="Peaks with a relative FWHM larger than this are removed during the 1D fit.",
        )

        self.declareProperty(
            "ScatteringContributions",
            "1",
            direction=Direction.Input,
            doc=(
                "If there is more than one compound, you may supply estimates of their scattering "
                "contributions, which sometimes improves indexing."
            ),
        )

        self.declareProperty(
            WorkspaceProperty("ExpectedPeaks", defaultValue="", direction=Direction.Input),
            doc="TableWorkspace or WorkspaceGroup with expected peaks used for indexing.",
        )

        self.declareProperty(
            "RemoveUnindexedPeaksFor2DFit",
            defaultValue=False,
            direction=Direction.Input,
            doc="Discard unindexed peaks for 2D fit, this is always the case if PawleyFit is active.",
        )

        allowedProfileFunctions = StringListValidator(self._allowedFunctions)
        self.declareProperty("ProfileFunction", "Gaussian", validator=allowedProfileFunctions, direction=Direction.Input)

        self.declareProperty(
            "TieProfileParameters",
            True,
            direction=Direction.Input,
            doc=(
                "If this option is activated, certain parameters are kept the same for all peaks. "
                "An example is the mixing parameter of the PseudoVoigt function."
            ),
        )

        self.declareProperty(
            "BoundProfileParameters",
            True,
            direction=Direction.Input,
            doc=(
                "If this option is activated, certain parameters will be bound to a specific range "
                'of values for all peaks. It is implemented for the "LeftShape" and "RightShape" '
                "parameters of the asymmetric Pearson VII function, which are set to be bound to "
                "a [0, 20] range."
            ),
        )

        self.declareProperty("PawleyFit", False, direction=Direction.Input, doc="Should the 2D-fit determine lattice parameters?")

        self.declareProperty(
            "MultipleRuns",
            False,
            direction=Direction.Input,
            doc=("If this is activated, peaks are searched again in the residuals and the 1D- and 2D-fit is repeated with these data."),
        )

        self.declareProperty(
            "PlotResult",
            True,
            direction=Direction.Input,
            doc=(
                "If this is activated, plot the sum of residuals and calculated spectrum together "
                "with the theoretical spectrum and the residuals."
            ),
        )

        self.declareProperty(
            "OutputIntegratedIntensities",
            False,
            direction=Direction.Input,
            doc=(
                "If this option is checked the peak intensities of the 2D-fit will be integrated, "
                "otherwise they will be the maximum intensity."
            ),
        )

        self.declareProperty(
            "OutputRawFitParameters",
            False,
            direction=Direction.Input,
            doc=("Activating this option produces an output workspace which contains the raw fit parameters."),
        )

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="WorkspaceGroup with result data from all processing steps.",
        )

    def PyExec(self):
        self.outputWorkspaces = []

        self.baseName = self.getProperty("InputWorkspace").valueAsStr
        self.inputWorkspace = self.getProperty("InputWorkspace").value
        self.expectedPeaks = self.getProperty("ExpectedPeaks").value
        self.profileFunction = self.getProperty("ProfileFunction").value
        self.useGlobalParameters = self.getProperty("TieProfileParameters").value
        self.useBoundedParameters = self.getProperty("BoundProfileParameters").value
        self.maximumRelativeFwhm = self.getProperty("MaximumRelativeFwhm").value
        self.outputIntegratedIntensities = self.getProperty("OutputIntegratedIntensities").value

        self.globalParameters = ""
        if self.useGlobalParameters:
            self.globalParameters = ",".join(self._globalParameters[self.profileFunction])

        self.boundedParameters = ""
        if self.useBoundedParameters:
            self.boundedParameters = ",".join(self._boundedParameters[self.profileFunction])

        if not self.workspaceHasCounts(self.inputWorkspace):
            raise RuntimeError("Aborting analysis since workspace " + self.baseName + " does not contain any counts.")

        correlationSpectrum = self.runCorrelation()
        self.outputWorkspaces.append(correlationSpectrum)

        self.numberOfExecutions = 0
        self.outputWorkspaces += self.runMainAnalysis(correlationSpectrum)

        outputWs = GroupWorkspaces(self.outputWorkspaces[0])

        for ws in self.outputWorkspaces[1:]:
            outputWs.add(ws.name())

        RenameWorkspace(outputWs, self.getProperty("OutputWorkspace").valueAsStr)

        self.setProperty("OutputWorkspace", outputWs)

    def workspaceHasCounts(self, workspace):
        integrated = Integration(workspace)
        summed = SumSpectra(integrated)

        counts = summed.readY(0)[0]

        DeleteWorkspace(integrated)
        DeleteWorkspace(summed)

        return counts > 0

    def runCorrelation(self):
        correlationName = self.baseName + "_correlation"
        PoldiAutoCorrelation(self.inputWorkspace, OutputWorkspace=correlationName, Version=5)

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

        pawleyFit = self.getProperty("PawleyFit").value
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

        runTwice = self.getProperty("MultipleRuns").value
        if runTwice and self.numberOfExecutions == 1:
            return self.runMainAnalysis(total)
        else:
            return outputWorkspaces

    def runPeakSearch(self, correlationWorkspace):
        peaksName = self.baseName + "_peaks_raw"

        PoldiPeakSearch(
            InputWorkspace=correlationWorkspace,
            MaximumPeakNumber=self.getProperty("MaximumPeakNumber").value,
            MinimumPeakSeparation=self.getProperty("MinimumPeakSeparation").value,
            MinimumPeakHeight=self.getProperty("MinimumPeakHeight").value,
            OutputWorkspace=peaksName,
        )

        return AnalysisDataService.retrieve(peaksName)

    def runPeakFit1D(self, correlationWorkspace, rawPeaks):
        refinedPeaksName = self.baseName + "_peaks_refined_1d"
        plotNames = self.baseName + "_fit_plots"

        PoldiFitPeaks1D(
            InputWorkspace=correlationWorkspace,
            PoldiPeakTable=rawPeaks,
            FwhmMultiples=3.0,
            MaximumRelativeFwhm=self.maximumRelativeFwhm,
            PeakFunction=self.profileFunction,
            OutputWorkspace=refinedPeaksName,
            FitPlotsWorkspace=plotNames,
        )

        return AnalysisDataService.retrieve(refinedPeaksName), AnalysisDataService.retrieve(plotNames)

    def runIndex(self, peaks):
        indexedPeaksName = self.baseName + "_indexed"

        PoldiIndexKnownCompounds(
            InputWorkspace=peaks,
            CompoundWorkspaces=self.expectedPeaks,
            ScatteringContributions=self.getProperty("ScatteringContributions").value,
            OutputWorkspace=indexedPeaksName,
        )

        indexedPeaks = AnalysisDataService.retrieve(indexedPeaksName)

        # Remove unindexed peaks from group for pawley fit
        unindexedPeaks = indexedPeaks.getItem(indexedPeaks.getNumberOfEntries() - 1)
        pawleyFit = self.getProperty("PawleyFit").value
        removeUnindexed = self.getProperty("RemoveUnindexedPeaksFor2DFit").value
        if removeUnindexed or pawleyFit:
            indexedPeaks.remove(unindexedPeaks.name())

        self._removeEmptyTablesFromGroup(indexedPeaks)

        return indexedPeaks, unindexedPeaks

    def runPeakFit2D(self, peaks):
        spectrum2DName = self.baseName + "_fit2d"
        spectrum1DName = self.baseName + "_fit1d"
        refinedPeaksName = self.baseName + "_peaks_refined_2d"
        refinedCellName = self.baseName + "_cell_refined"

        pawleyFit = self.getProperty("PawleyFit").value

        rawFitParametersWorkspaceName = ""
        outputRawFitParameters = self.getProperty("OutputRawFitParameters").value
        if outputRawFitParameters:
            rawFitParametersWorkspaceName = self.baseName + "_raw_fit_parameters"

        PoldiFitPeaks2D(
            InputWorkspace=self.inputWorkspace,
            PoldiPeakWorkspace=peaks,
            PeakProfileFunction=self.profileFunction,
            GlobalParameters=self.globalParameters,
            BoundedParameters=self.boundedParameters,
            PawleyFit=pawleyFit,
            MaximumIterations=100,
            OutputWorkspace=spectrum2DName,
            Calculated1DSpectrum=spectrum1DName,
            RefinedPoldiPeakWorkspace=refinedPeaksName,
            OutputIntegratedIntensities=self.outputIntegratedIntensities,
            RefinedCellParameters=refinedCellName,
            RawFitParameters=rawFitParametersWorkspaceName,
        )

        workspaces = [
            AnalysisDataService.retrieve(spectrum2DName),
            AnalysisDataService.retrieve(spectrum1DName),
            AnalysisDataService.retrieve(refinedPeaksName),
        ]
        if AnalysisDataService.doesExist(refinedCellName):
            workspaces.append(AnalysisDataService.retrieve(refinedCellName))

        if AnalysisDataService.doesExist(rawFitParametersWorkspaceName):
            workspaces.append(AnalysisDataService.retrieve(rawFitParametersWorkspaceName))

        return workspaces

    def runResidualAnalysis(self, calculated2DSpectrum):
        residualName = self.baseName + "_residuals"

        PoldiAnalyseResiduals(
            MeasuredCountData=self.inputWorkspace, FittedCountData=calculated2DSpectrum, MaxIterations=5, OutputWorkspace=residualName
        )

        return AnalysisDataService.retrieve(residualName)

    def _removeEmptyTablesFromGroup(self, groupWorkspace):
        deleteNames = []
        for i in range(groupWorkspace.getNumberOfEntries()):
            ws = groupWorkspace.getItem(i)
            if ws.rowCount() == 0:
                deleteNames.append(ws.name())
        for name in deleteNames:
            DeleteWorkspace(name)

    def _plotResult(self, total, spectrum1D, residuals):
        plotResults = self.getProperty("PlotResult").value

        if plotResults:
            plotWindow = plotSpectrum(total, 0, type=1)
            plotWindow = plotSpectrum(spectrum1D, 0, type=0, window=plotWindow)
            plotWindow = plotSpectrum(residuals, 0, type=0, window=plotWindow)
            plotWindow.activeLayer().setTitle("Fit result for " + self.baseName)
            plotWindow.activeLayer().removeLegend()


AlgorithmFactory.subscribe(PoldiDataAnalysis())
