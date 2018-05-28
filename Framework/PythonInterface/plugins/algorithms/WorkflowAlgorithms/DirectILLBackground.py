from __future__ import (absolute_import, division, print_function)

import DirectILL_common as common
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, InstrumentValidator,
                        ITableWorkspaceProperty, MatrixWorkspaceProperty, Progress, PropertyMode,
                        WorkspaceProperty)
from mantid.kernel import (Direction, FloatBoundedValidator, StringArrayProperty, StringListValidator)
from mantid.simpleapi import (CalculatePolynomialBackground, CloneWorkspace,
                              CreateSingleValuedWorkspace, CreateWorkspace, CropToComponent,
                              Divide, Integration, MaskDetectors, Plus, Rebin, Transpose)
import numpy


def _emptyBkgWS(ws, wsNames, algorithmLogging):
    """Return a transposed clone of a single histogram ws with y and e zeroed."""
    emptyWSName = wsNames.withSuffix('zero_bkg')
    nHisto = ws.getNumberHistograms()
    Xs = numpy.tile(numpy.array([0., 1.]), nHisto)
    zeros = numpy.zeros(nHisto)
    emptyWS = CreateWorkspace(
        OutputWorkspace=emptyWSName,
        DataX=Xs,
        DataY=zeros,
        NSpec=nHisto,
        EnableLogging=algorithmLogging)
    return emptyWS

def _fittingRanges(ws, fitStatuses, rangeStart):
    """Return suitable background fitting ranges."""
    spectrumInfo = ws.spectrumInfo()
    nHist = ws.getNumberHistograms()
    ranges = numpy.empty(2 * nHist)
    j = 0
    for i in range(nHist):
        if spectrumInfo.isMasked(i) or fitStatuses[i] != 'success':
            continue
        # Make sure the ranges overlap where needed.
        x = float(rangeStart + i)
        ranges[j] = x - 0.51
        j += 1
        ranges[j] = x + 0.51
        j += 1
    return ranges[0:j]


def _wsWidePolynomialBkg(ws, peakStarts, peakEnds, fitStatuses, wsNames, wsCleanup, algorithmLogging):
    """Calculate a 2nd degree polynomial background over all histograms."""
    fitRanges = _fittingRanges(ws, fitStatuses, ws.getSpectrum(0).getSpectrumNo())
    if len(fitRanges) / 2 < 3:
        return _emptyBkgWS(ws, wsNames, algorithmLogging)
    def integrate(wsName, **kwargs):
        """Integrate a workspace."""
        bkgWSName = wsNames.withSuffix(wsName)
        bkgWS = Integration(
            InputWorkspace=ws,
            OutputWorkspace=bkgWSName,
            EnableLogging=algorithmLogging,
            **kwargs)
        return bkgWS
    leftBkgWSName = wsNames.withSuffix('left_bkg_integrated')
    leftBkgWS = integrate(leftBkgWSName, **{'RangeUpperList': peakStarts})
    rightBkgWSName = wsNames.withSuffix('right_bkg_integrated')
    rightBkgWS = integrate(rightBkgWSName, **{'RangeLowerList': peakEnds})
    X = ws.extractX()
    n = X.shape[0]
    lenLeftBkg = numpy.empty(n)
    lenRightBkg = numpy.empty(n)
    for i in range(n):
        lenLeftBkg[i] = numpy.searchsorted(X[i], peakStarts[i]) - 1
        lenRightBkg[i] = X.shape[1] - numpy.searchsorted(X[i], peakEnds[i]) - 1

    def average(ws, binCountWSName, averageWSName, binCounts):
        """Average a workspace by number of bins."""
        dummyPoints = numpy.zeros(n)
        dummyErrors = numpy.ones(n)
        binCountWS = CreateWorkspace(
            OutputWorkspace=binCountWSName,
            DataX=dummyPoints,
            DataY=binCounts,
            DataE=dummyErrors,
            NSpec=n,
            EnableLogging=algorithmLogging)
        averageWS = Divide(
            LHSWorkspace=ws,
            RHSWorkspace=binCountWS,
            OutputWorkspace=averageWSName,
            EnableLogging=algorithmLogging)
        wsCleanup.cleanup(binCountWS)
        return averageWS
    leftBkgBinCountWSName = wsNames.withSuffix('left_bkg_bin_count')
    averageLeftBkgWSName = wsNames.withSuffix('left_bkg_average')
    averageLeftBkgWS = average(leftBkgWS, leftBkgBinCountWSName, averageLeftBkgWSName, lenLeftBkg)
    wsCleanup.cleanup(leftBkgWS)
    rightBkgBinCountWSName = wsNames.withSuffix('righ_bkg_bin_count')
    averageRightBkgWSName = wsNames.withSuffix('right_bkg_average')
    averageRightBkgWS = average(rightBkgWS, rightBkgBinCountWSName, averageRightBkgWSName, lenRightBkg)
    wsCleanup.cleanup(rightBkgWS)
    bkgWSName = wsNames.withSuffix('bkg_average')
    bkgWS = Plus(
        LHSWorkspace=averageLeftBkgWS,
        RHSWorkspace=averageRightBkgWS,
        OutputWorkspace=bkgWSName,
        EnableLogging=algorithmLogging)
    wsCleanup.cleanup(averageLeftBkgWS)
    wsCleanup.cleanup(averageRightBkgWS)
    twoWSName = wsNames.withSuffix('single_value_two')
    twoWS = CreateSingleValuedWorkspace(
        OutputWorkspace=twoWSName,
        DataValue=2.,
        ErrorValue=0.,
        EnableLogging=algorithmLogging)
    bkgWS = Divide(
        LHSWorkspace=bkgWS,
        RHSWorkspace=twoWS,
        OutputWorkspace=bkgWSName,
        EnableLogging=algorithmLogging)
    wsCleanup.cleanup(twoWS)
    # Rebin to common bin boundaries. Otherwise Transpose won't accept the input.
    bkgWS = Rebin(
        InputWorkspace=bkgWS,
        OutputWorkspace=bkgWSName,
        Params='0, 1., 1.',
        EnableLogging=algorithmLogging)
    bkgWS = Transpose(
        InputWorkspace=bkgWS,
        OutputWorkspace=bkgWSName,
        EnableLogging=algorithmLogging)
    fittedBkgWSName = wsNames.withSuffix('bkg_fitted')
    fittedBkgWS = CalculatePolynomialBackground(
        InputWorkspace=bkgWS,
        OutputWorkspace=fittedBkgWSName,
        Degree=2,
        XRanges=fitRanges,
        EnableLogging=algorithmLogging)
    wsCleanup.cleanup(bkgWS)
    fittedBkgWS = Transpose(
        InputWorkspace=fittedBkgWS,
        OutputWorkspace=fittedBkgWSName,
        EnableLogging=algorithmLogging)
    return fittedBkgWS


class DirectILLBackground(DataProcessorAlgorithm):
    """A workflow algorithm for calculating time-independent backgrounds over tubes or boxes."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return common.CATEGORIES

    def seeAlso(self):
        return ['DirectILLBackground']

    def name(self):
        """Return the algorithm's name."""
        return 'DirectILLBackground'

    def summary(self):
        """Return a summary of the algorithm."""
        return 'Calculate time-independent backgrounds for the direct geometry TOF spectrometers at ILL.'

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Execute the data reduction workflow."""
        subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        wsNamePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        wsNames = common.NameSource(wsNamePrefix, cleanupMode)
        wsCleanup = common.IntermediateWSCleanup(cleanupMode, subalgLogging)

        mainWS = self._inputWS(wsNames, wsCleanup, subalgLogging)

        mainWS = self._applyDiagnostics(mainWS, wsNames, wsCleanup, subalgLogging)

        bkgWS = self._componentWideBkg(mainWS, wsNames, wsCleanup, subalgLogging)

        self._finalize(bkgWS, wsCleanup)

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        inputWorkspaceValidator = InstrumentValidator()
        positiveFloat = FloatBoundedValidator(lower=0)
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_INPUT_WS,
            defaultValue='',
            validator=inputWorkspaceValidator,
            direction=Direction.Input),
            doc='A workspace from which to determine the backgrounds.')
        self.declareProperty(WorkspaceProperty(
            name=common.PROP_OUTPUT_WS,
            defaultValue='',
            direction=Direction.Output),
            doc='A workspace containing the backgrounds.')
        self.declareProperty(name=common.PROP_CLEANUP_MODE,
                             defaultValue=common.CLEANUP_ON,
                             validator=StringListValidator([
                                 common.CLEANUP_ON,
                                 common.CLEANUP_OFF]),
                             direction=Direction.Input,
                             doc='What to do with intermediate workspaces.')
        self.declareProperty(name=common.PROP_SUBALG_LOGGING,
                             defaultValue=common.SUBALG_LOGGING_OFF,
                             validator=StringListValidator([
                                 common.SUBALG_LOGGING_OFF,
                                 common.SUBALG_LOGGING_ON]),
                             direction=Direction.Input,
                             doc='Enable or disable subalgorithms to print in the logs.')
        self.declareProperty(StringArrayProperty(
            name=common.PROP_COMPONENTS,
            direction=Direction.Input),
            doc='A list of component names for which to calculate the backgrounds.')
        self.declareProperty(ITableWorkspaceProperty(
            name=common.PROP_EPP_WS,
            defaultValue='',
            direction=Direction.Input),
            doc='Table workspace containing results from the FindEPP algorithm.')
        self.declareProperty(name=common.PROP_BKG_SIGMA_MULTIPLIER,
                             defaultValue=6.,
                             validator=positiveFloat,
                             direction=Direction.Input,
                             doc="Half width of the range excluded from background around " +
                                 "the elastic peaks in multiplies of 'Sigma' in the EPP table.'.")
        self.declareProperty(MatrixWorkspaceProperty(
            name=common.PROP_DIAGNOSTICS_WS,
            defaultValue='',
            direction=Direction.Input,
            optional=PropertyMode.Optional),
            doc='Detector diagnostics workspace for masking.')

    def _applyDiagnostics(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Mask workspace according to diagnostics."""
        if self.getProperty(common.PROP_DIAGNOSTICS_WS).isDefault:
            return mainWS
        diagnosticsWS = self.getProperty(common.PROP_DIAGNOSTICS_WS).value
        maskedWSName = wsNames.withSuffix('diagnostics_applied')
        maskedWS = CloneWorkspace(InputWorkspace=mainWS,
                                  OutputWorkspace=maskedWSName,
                                  EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        diagnosticsWS = self.getProperty(common.PROP_DIAGNOSTICS_WS).value
        MaskDetectors(Workspace=maskedWS,
                      MaskedWorkspace=diagnosticsWS,
                      EnableLogging=subalgLogging)
        wsCleanup.cleanup(mainWS)
        return maskedWS

    def _components(self, instrument):
        """Return detector grouping."""
        componentProperty = self.getProperty(common.PROP_COMPONENTS)
        if componentProperty.isDefault:
            COMPONENTS_PARAM = 'components-for-backgrounds'
            if not instrument.hasParameter(COMPONENTS_PARAM):
                raise RuntimeError("No " + common.PROP_COMPONENTS + " given and instrument parameter '" + COMPONENTS_PARAM + "' missing.")
            components = instrument.getStringParameter(COMPONENTS_PARAM)[0]
            components = components.split(',')
            components = map(lambda s: s.strip(), components)
            return components
        return componentProperty.value

    def _componentWideBkg(self, mainWS, wsNames, wsCleanup, subalgLogging):
        """Subtract a polynomial background fitted over individual components."""
        eppWS = self.getProperty(common.PROP_EPP_WS).value
        centres = numpy.array(eppWS.column('PeakCentre'))
        sigmaMultiplier = self.getProperty(common.PROP_BKG_SIGMA_MULTIPLIER).value
        centreHalfWidth = sigmaMultiplier / 2. * numpy.array(eppWS.column('Sigma'))
        fitStatuses = eppWS.column('FitStatus')
        peakStarts = numpy.array(centres - centreHalfWidth)
        peakEnds = numpy.array(centres + centreHalfWidth)
        instrument = mainWS.getInstrument()
        components = self._components(instrument)
        progress = Progress(self, 0.0, 1.0, len(components))
        bkgWSName = wsNames.withSuffix('bkg')
        bkgWS = CloneWorkspace(InputWorkspace=mainWS,
                               OutputWorkspace=bkgWSName,
                               EnableLogging=subalgLogging)
        for componentName in components:
            progress.report('Processing ' + componentName)
            component = instrument.getComponentByName(componentName)
            if component is None:
                raise RuntimeError("Component '" + componentName + "' not found in the instrument.")
            componentWSName = wsNames.withSuffix('cropped_to_component')
            componentWS = CropToComponent(
                InputWorkspace=mainWS,
                OutputWorkspace=componentWSName,
                ComponentNames=componentName,
                EnableLogging=subalgLogging
            )
            spectrumNo = componentWS.getSpectrum(0).getSpectrumNo()
            pixelStart = mainWS.getIndexFromSpectrumNumber(spectrumNo)
            spectrumNo = componentWS.getSpectrum(componentWS.getNumberHistograms() - 1).getSpectrumNo()
            pixelEnd = mainWS.getIndexFromSpectrumNumber(spectrumNo)
            componentPeakStarts = peakStarts[pixelStart:pixelEnd + 1]
            componentPeakEnds = peakEnds[pixelStart:pixelEnd + 1]
            statuses = fitStatuses[pixelStart:pixelEnd + 1]
            componentBkgWS = _wsWidePolynomialBkg(componentWS, componentPeakStarts, componentPeakEnds,
                                                  statuses, wsNames, wsCleanup, subalgLogging)
            wsCleanup.cleanup(componentWS)
            for i in range(componentBkgWS.getNumberHistograms()):
                index = pixelStart + i
                bkgWS.dataY(index).fill(componentBkgWS.readY(i)[0])
                bkgWS.dataE(index).fill(componentBkgWS.readE(i)[0])
            wsCleanup.cleanup(componentBkgWS)
        wsCleanup.cleanup(mainWS)
        return bkgWS

    def _finalize(self, outWS, wsCleanup):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        wsCleanup.cleanup(outWS)
        wsCleanup.finalCleanup()

    def _inputWS(self, wsNames, wsCleanup, subalgLogging):
        """Return the raw input workspace."""
        mainWS = self.getProperty(common.PROP_INPUT_WS).value
        wsCleanup.protect(mainWS)
        return mainWS


AlgorithmFactory.subscribe(DirectILLBackground)
