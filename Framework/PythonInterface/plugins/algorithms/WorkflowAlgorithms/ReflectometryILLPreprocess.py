# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, FileAction, ITableWorkspaceProperty,
                        MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode, WorkspaceUnitValidator)
from mantid.kernel import (CompositeValidator, Direction,
                           IntArrayLengthValidator, IntArrayBoundedValidator, IntArrayProperty,
                           IntBoundedValidator, Property, StringListValidator)
from mantid.simpleapi import (AddSampleLog, CalculatePolynomialBackground, CloneWorkspace, ConvertUnits,
                              Divide, ExtractMonitors, FindReflectometryLines, LoadAndMerge, Minus, mtd,
                              NormaliseToMonitor, RebinToWorkspace, Scale, Transpose)
import numpy
import ReflectometryILL_common as common


class Prop:
    BEAM_ANGLE = 'BraggAngle'
    BEAM_CENTRE = 'BeamCentre'
    BEAM_POS_WS = 'BeamPositionWorkspace'
    BKG_METHOD = 'FlatBackground'
    CLEANUP = 'Cleanup'
    DIRECT_BEAM_POS_WS = 'DirectBeamPositionWorkspace'
    FLUX_NORM_METHOD = 'FluxNormalisation'
    FOREGROUND_HALF_WIDTH = 'ForegroundHalfWidth'
    HIGH_BKG_OFFSET = 'HighAngleBkgOffset'
    HIGH_BKG_WIDTH = 'HighAngleBkgWidth'
    INPUT_WS = 'InputWorkspace'
    LOW_BKG_OFFSET = 'LowAngleBkgOffset'
    LOW_BKG_WIDTH = 'LowAngleBkgWidth'
    OUTPUT_BEAM_POS_WS = 'OutputBeamPositionWorkspace'
    OUTPUT_WS = 'OutputWorkspace'
    RUN = 'Run'
    SLIT_NORM = 'SlitNormalisation'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    WATER_REFERENCE = 'WaterWorkspace'
    START_WS_INDEX = 'StartWorkspaceIndex'
    END_WS_INDEX = 'EndWorkspaceIndex'
    XMIN = 'RangeLower'
    XMAX = 'RangeUpper'


class BkgMethod:
    CONSTANT = 'Background Constant Fit'
    LINEAR = 'Background Linear Fit'
    OFF = 'Background OFF'


class FluxNormMethod:
    MONITOR = 'Normalise To Monitor'
    TIME = 'Normalise To Time'
    OFF = 'Normalisation OFF'


class SlitNorm:
    OFF = 'Slit Normalisation OFF'
    ON = 'Slit Normalisation ON'


class SubalgLogging:
    OFF = 'Logging OFF'
    ON = 'Logging ON'


def normalisationMonitorWorkspaceIndex(ws):
    """Return the spectrum number of the monitor used for normalisation."""
    paramName = 'default-incident-monitor-spectrum'
    instr = ws.getInstrument()
    if not instr.hasParameter(paramName):
        raise RuntimeError('Parameter ' + paramName + ' is missing from the IPF.')
    n = instr.getIntParameter(paramName)[0]
    return ws.getIndexFromSpectrumNumber(n)


class ReflectometryILLPreprocess(DataProcessorAlgorithm):

    def category(self):
        """Return the categories of the algrithm."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLPreprocess'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Loads, merges, normalizes and subtracts background from ILL reflectometry data."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryILLConvertToQ', 'ReflectometryILLPolarizationCor', 'ReflectometryILLSumForeground']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = common.WSCleanup(cleanupMode, self._subalgLogging)
        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = common.WSNameSource(wsPrefix, cleanupMode)

        ws = self._inputWS()

        ws, monWS = self._extractMonitors(ws)

        ws = self._peakFitting(ws)

        ws = self._waterCalibration(ws)

        ws = self._normaliseToSlits(ws)

        ws = self._normaliseToFlux(ws, monWS)
        self._cleanup.cleanup(monWS)

        ws = self._subtractFlatBkg(ws)

        ws = self._convertToWavelength(ws)

        self._finalize(ws)

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        nonnegativeInt = IntBoundedValidator(lower=0)
        wsIndexRange = IntBoundedValidator(lower=0, upper=255)
        nonnegativeIntArray = IntArrayBoundedValidator()
        nonnegativeIntArray.setLower(0)
        maxTwoNonnegativeInts = CompositeValidator()
        maxTwoNonnegativeInts.add(IntArrayLengthValidator(lenmin=0, lenmax=2))
        maxTwoNonnegativeInts.add(nonnegativeIntArray)
        self.declareProperty(MultipleFileProperty(Prop.RUN,
                                                  action=FileAction.OptionalLoad,
                                                  extensions=['nxs']),
                             doc='A list of input run numbers/files.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.INPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator('TOF'),
                                                     optional=PropertyMode.Optional),
                             doc='An input workspace (units TOF) if no Run is specified.')
        self.declareProperty(ITableWorkspaceProperty(Prop.BEAM_POS_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='A beam position table corresponding to InputWorkspace.')
        self.declareProperty(Prop.BEAM_ANGLE,
                             defaultValue=Property.EMPTY_DBL,
                             doc='A user-defined beam angle (unit degrees).')
        self.declareProperty(name=Prop.BEAM_CENTRE,
                             defaultValue=Property.EMPTY_DBL,
                             doc='A workspace index corresponding to the beam centre between 0.0 and 255.0')
        self.declareProperty(MatrixWorkspaceProperty(Prop.OUTPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='The preprocessed output workspace (unit wavelength), single histogram.')
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disable child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=common.WSCleanup.ON,
                             validator=StringListValidator([common.WSCleanup.ON, common.WSCleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(ITableWorkspaceProperty(Prop.DIRECT_BEAM_POS_WS,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='A beam position table from a direct beam measurement.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.WATER_REFERENCE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator("TOF"),
                                                     optional=PropertyMode.Optional),
                             doc='A (water) calibration workspace (unit TOF).')
        self.declareProperty(Prop.SLIT_NORM,
                             defaultValue=SlitNorm.OFF,
                             validator=StringListValidator([SlitNorm.OFF, SlitNorm.ON]),
                             doc='Enable or disable slit normalisation.')
        self.declareProperty(Prop.FLUX_NORM_METHOD,
                             defaultValue=FluxNormMethod.TIME,
                             validator=StringListValidator([FluxNormMethod.TIME, FluxNormMethod.MONITOR, FluxNormMethod.OFF]),
                             doc='Neutron flux normalisation method.')
        self.declareProperty(IntArrayProperty(Prop.FOREGROUND_HALF_WIDTH,
                                              validator=maxTwoNonnegativeInts),
                             doc='Number of foreground pixels at lower and higher angles from the centre pixel.')
        self.declareProperty(Prop.BKG_METHOD,
                             defaultValue=BkgMethod.CONSTANT,
                             validator=StringListValidator([BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]),
                             doc='Flat background calculation method for background subtraction.')
        self.declareProperty(Prop.LOW_BKG_OFFSET,
                             defaultValue=7,
                             validator=nonnegativeInt,
                             doc='Distance of flat background region towards smaller detector angles from the foreground centre, ' +
                                 'in pixels.')
        self.declareProperty(Prop.LOW_BKG_WIDTH,
                             defaultValue=5,
                             validator=nonnegativeInt,
                             doc='Width of flat background region towards smaller detector angles from the foreground centre, in pixels.')
        self.declareProperty(Prop.HIGH_BKG_OFFSET,
                             defaultValue=7,
                             validator=nonnegativeInt,
                             doc='Distance of flat background region towards larger detector angles from the foreground centre, in pixels.')
        self.declareProperty(Prop.HIGH_BKG_WIDTH,
                             defaultValue=5,
                             validator=nonnegativeInt,
                             doc='Width of flat background region towards larger detector angles from the foreground centre, in pixels.')
        self.declareProperty(ITableWorkspaceProperty(Prop.OUTPUT_BEAM_POS_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Output the beam position table.')
        self.declareProperty(Prop.START_WS_INDEX,
                             validator=wsIndexRange,
                             defaultValue=0,
                             doc='Start histogram index used for peak fitting.')
        self.declareProperty(Prop.END_WS_INDEX,
                             validator=wsIndexRange,
                             defaultValue=255,
                             doc='Last histogram index used for peak fitting.')
        self.declareProperty(Prop.XMIN,
                             defaultValue=Property.EMPTY_DBL,
                             doc='Minimum x value (unit wavelength) used for peak finding the peak position.')
        self.declareProperty(Prop.XMAX,
                             defaultValue=Property.EMPTY_DBL,
                             doc='Maximum x value (unit wavelength) used for peak finding the peak position.')

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        if self.getProperty(Prop.INPUT_WS).isDefault and len(self.getProperty(Prop.RUN).value) == 0:
            issues[Prop.RUN] = "Provide at least an input file or alternatively an input workspace."
        if self.getProperty(Prop.BKG_METHOD).value != BkgMethod.OFF:
            if self.getProperty(Prop.LOW_BKG_WIDTH).value == 0 and self.getProperty(Prop.HIGH_BKG_WIDTH).value == 0:
                issues[Prop.BKG_METHOD] = "Cannot calculate flat background if both upper and lower background widths are zero."
            if not self.getProperty(Prop.INPUT_WS).isDefault and self.getProperty(Prop.BEAM_POS_WS).isDefault \
                    and self.getProperty(Prop.BEAM_CENTRE).isDefault:
                issues[Prop.BEAM_POS_WS] = "Cannot subtract flat background without knowledge of peak position/foreground centre."
                issues[Prop.BEAM_CENTRE] = "Cannot subtract flat background without knowledge of peak position/foreground centre."
        if not self.getProperty(Prop.BEAM_CENTRE).isDefault:
            beamCentre = self.getProperty(Prop.BEAM_CENTRE).value
            if beamCentre < 0. or beamCentre > 255.:
                issues[Prop.BEAM_CENTRE] = "Value should be between 0 and 255."
        # Early input validation to prevent FindReflectometryLines to fail its validation
        if not self.getProperty(Prop.XMIN).isDefault and not self.getProperty(Prop.XMAX).isDefault:
            xmin = self.getProperty(Prop.XMIN).value
            xmax = self.getProperty(Prop.XMAX).value
            if xmax < xmin:
                issues[Prop.XMIN] = "Must be smaller than RangeUpper."
        if not self.getProperty(Prop.START_WS_INDEX).isDefault \
                and not self.getProperty(Prop.END_WS_INDEX).isDefault:
            minIndex = self.getProperty(Prop.START_WS_INDEX).value
            maxIndex = self.getProperty(Prop.END_WS_INDEX).value
            if maxIndex < minIndex:
                issues[Prop.START_WS_INDEX] = "Must be smaller then EndWorkspaceIndex."
        return issues

    def _convertToWavelength(self, ws):
        """Convert the X units of ws to wavelength."""
        wavelengthWSName = self._names.withSuffix('in_wavelength')
        wavelengthWS = ConvertUnits(
            InputWorkspace=ws,
            OutputWorkspace=wavelengthWSName,
            Target='Wavelength',
            EMode='Elastic',
            EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return wavelengthWS

    def _extractMonitors(self, ws):
        """Extract monitor spectra from ws to another workspace."""
        detWSName = self._names.withSuffix('detectors')
        monWSName = self._names.withSuffix('monitors')
        ExtractMonitors(InputWorkspace=ws,
                        DetectorWorkspace=detWSName,
                        MonitorWorkspace=monWSName,
                        EnableLogging=self._subalgLogging)
        if mtd.doesExist(detWSName) is None:
            raise RuntimeError('No detectors in the input data.')
        detWS = mtd[detWSName]
        monWS = mtd[monWSName] if mtd.doesExist(monWSName) else None
        self._cleanup.cleanup(ws)
        return detWS, monWS

    def _finalize(self, ws):
        """Set OutputWorkspace to ws and clean up."""
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _flatBkgRanges(self, ws):
        """Return spectrum number ranges for flat background fitting."""
        sign = self._workspaceIndexDirection(ws)
        if ws.getRun().hasProperty(common.SampleLogs.FOREGROUND_CENTRE):
            peakPos = ws.getRun().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value
        else:
            raise RuntimeError('Missing log entry for sample logs foreground centre.')
        # Convert to spectrum numbers
        peakPos = ws.getSpectrum(peakPos).getSpectrumNo()
        peakHalfWidths = self._foregroundWidths()
        lowPeakHalfWidth = peakHalfWidths[0]
        lowOffset = self.getProperty(Prop.LOW_BKG_OFFSET).value
        lowWidth = self.getProperty(Prop.LOW_BKG_WIDTH).value
        lowStartIndex = peakPos - sign * (lowPeakHalfWidth + lowOffset + lowWidth)
        lowEndIndex = lowStartIndex + sign * lowWidth
        highPeakHalfWidth = peakHalfWidths[1]
        highOffset = self.getProperty(Prop.HIGH_BKG_OFFSET).value
        highWidth = self.getProperty(Prop.HIGH_BKG_WIDTH).value
        highStartIndex = peakPos + sign * (highPeakHalfWidth + highOffset)
        highEndIndex = highStartIndex + sign * highWidth
        if sign > 0:
            lowRange = [lowStartIndex - sign * 0.5, lowEndIndex - sign * 0.5]
            highRange = [highStartIndex + sign * 0.5, highEndIndex + sign * 0.5]
            return lowRange + highRange
        # Indices decrease with increasing bragg angle. Swap everything.
        lowRange = [lowEndIndex - sign * 0.5, lowStartIndex - sign * 0.5]
        highRange = [highEndIndex + sign * 0.5, highStartIndex + sign * 0.5]
        return highRange + lowRange

    def _workspaceIndexDirection(self, ws):
        """Return 1 if workspace indices increase with Bragg angle, otherwise return -1."""
        firstDet = ws.getDetector(0)
        firstAngle = ws.detectorTwoTheta(firstDet)
        lastDet = ws.getDetector(ws.getNumberHistograms() - 1)
        lastAngle = ws.detectorTwoTheta(lastDet)
        return 1 if firstAngle < lastAngle else -1

    def _foregroundWidths(self):
        """Return an array of [low angle width, high angle width]."""
        halfWidths = self.getProperty(Prop.FOREGROUND_HALF_WIDTH).value
        if len(halfWidths) == 0:
            halfWidths = [0, 0]
        elif len(halfWidths) == 1:
            halfWidths = [halfWidths[0], halfWidths[0]]
        return halfWidths

    def _inputWS(self):
        """Return a raw input workspace."""
        beamPos = 0.
        inputFiles = self.getPropertyValue(Prop.RUN)
        inputFiles = inputFiles.replace(',', '+')
        if inputFiles:
            mergedWSName = self._names.withSuffix('merged')
            loadOption = {'XUnit': 'TimeOfFlight'}
            if not self.getProperty(Prop.BEAM_CENTRE).isDefault:
                loadOption['BeamCentre'] = self.getProperty(Prop.BEAM_CENTRE).value
            if not self.getProperty(Prop.DIRECT_BEAM_POS_WS).isDefault:
                loadOption['DirectBeamPosition'] = self.getPropertyValue(Prop.DIRECT_BEAM_POS_WS)
            if not self.getProperty(Prop.BEAM_ANGLE).isDefault:
                loadOption['BraggAngle'] = str(self.getProperty(Prop.BEAM_ANGLE).value)
            # MergeRunsOptions are defined by the parameter files and will not be modified here!
            ws = LoadAndMerge(Filename=inputFiles,
                              LoaderName='LoadILLReflectometry',
                              LoaderVersion=1,
                              LoaderOptions=loadOption,
                              OutputWorkspace=mergedWSName,
                              EnableLogging=self._subalgLogging)
        else:
            ws = self.getProperty(Prop.INPUT_WS).value
        return ws

    def _peakFitting(self, ws):
        """Return the raw input workspace with sample log information."""
        peakWSName = self._names.withSuffix('peak')
        args = {'InputWorkspace': ws, 'OutputWorkspace': peakWSName, 'EnableLogging': self._subalgLogging}
        # Convert to wavelength
        l1 = ws.spectrumInfo().l1()
        hists = ws.getNumberHistograms()
        mindex = int(hists / 2)
        l2 = ws.spectrumInfo().l2(mindex)
        theta = ws.spectrumInfo().twoTheta(mindex) / 2.
        if not self.getProperty(Prop.XMIN).isDefault:
            xmin = self.getProperty(Prop.XMIN).value
            args['RangeLower'] = common.inWavelength(xmin, l1, l2, theta)
        if not self.getProperty(Prop.XMAX).isDefault:
            xmax = self.getProperty(Prop.XMAX).value
            args['RangeUpper'] = common.inWavelength(xmax, l1, l2, theta)
        if not self.getProperty(Prop.START_WS_INDEX).isDefault:
            args['StartWorkspaceIndex'] = self.getProperty(Prop.START_WS_INDEX).value
        if not self.getProperty(Prop.END_WS_INDEX).isDefault:
            args['EndWorkspaceIndex'] = self.getPropertyValue(Prop.END_WS_INDEX).value
        # Fit peak position
        FindReflectometryLines(**args)
        beamPos = mtd[peakWSName].readY(0)[0]
        if self.getProperty(Prop.RUN).isDefault:
            if not self.getProperty(Prop.BEAM_POS_WS).isDefault:
                tableWithBeamPos = self.getProperty(Prop.BEAM_POS_WS).value
                beamPos = tableWithBeamPos.cell('PeakCentre', 0)
        if not self.getProperty(Prop.BEAM_CENTRE).isDefault:
            beamPos = self.getProperty(Prop.BEAM_CENTRE).value
        # Write beam position to sample logs
        AddSampleLog(ws,
                     LogName='peak_position',
                     LogText=str(beamPos),
                     LogType='Number',
                     NumberType='Double')
        # Add foreground start and end workspace indices to the sample logs of ws.
        hws = self._foregroundWidths()
        beamPosIndex = int(numpy.rint(beamPos))
        if beamPosIndex > 255:
            beamPosIndex = 255
        sign = self._workspaceIndexDirection(ws)
        start = beamPosIndex - sign * hws[0]
        end = beamPosIndex + sign * hws[1]
        if start > end:
            end, start = start, end
        AddSampleLog(ws,
                     LogName=common.SampleLogs.FOREGROUND_START,
                     LogText=str(start),
                     LogType='Number',
                     NumberType='Int',
                     EnableLogging=self._subalgLogging)
        AddSampleLog(ws,
                     LogName=common.SampleLogs.FOREGROUND_CENTRE,
                     LogText=str(beamPosIndex),
                     logType='Number',
                     NumberType='Int',
                     EnableLogging=self._subalgLogging)
        AddSampleLog(ws,
                     LogName=common.SampleLogs.FOREGROUND_END,
                     LogText=str(end),
                     LogType='Number',
                     NumberType='Int',
                     EnableLogging=self._subalgLogging)
        return ws

    def _normaliseToFlux(self, detWS, monWS):
        """Normalise ws to monitor counts or counting time."""
        method = self.getProperty(Prop.FLUX_NORM_METHOD).value
        if method == FluxNormMethod.MONITOR:
            if monWS is None:
                raise RuntimeError('Cannot normalise to monitor data: no monitors in input data.')
            normalisedWSName = self._names.withSuffix('normalised_to_monitor')
            monIndex = normalisationMonitorWorkspaceIndex(monWS)
            monXs = monWS.readX(0)
            minX = monXs[0]
            maxX = monXs[-1]
            NormaliseToMonitor(InputWorkspace=detWS,
                               OutputWorkspace=normalisedWSName,
                               MonitorWorkspace=monWS,
                               MonitorWorkspaceIndex=monIndex,
                               IntegrationRangeMin=minX,
                               IntegrationRangeMax=maxX,
                               EnableLogging=self._subalgLogging)
            normalisedWS = mtd[normalisedWSName]
            self._cleanup.cleanup(detWS)
            return normalisedWS
        elif method == FluxNormMethod.TIME:
            t = detWS.run().getProperty('time').value
            normalisedWSName = self._names.withSuffix('normalised_to_time')
            scaledWS = Scale(InputWorkspace=detWS,
                             OutputWorkspace=normalisedWSName,
                             Factor=1.0 / t,
                             EnableLogging=self._subalgLogging)
            self._cleanup.cleanup(detWS)
            return scaledWS
        return detWS

    def _normaliseToSlits(self, ws):
        """Normalise ws to slit opening."""
        if self.getProperty(Prop.SLIT_NORM).value == SlitNorm.OFF:
            return ws
        r = ws.run()
        instrumentName = r.get('instrument.name').value
        slit2width = r.get(common.slitSizeLogEntry(instrumentName, 1))
        slit3width = r.get(common.slitSizeLogEntry(instrumentName, 2))
        if slit2width is None or slit3width is None:
            self.log().warning('Slit information not found in sample logs. Slit normalisation disabled.')
            return ws
        f = slit2width.value * slit3width.value
        normalisedWSName = self._names.withSuffix('normalised_to_slits')
        normalisedWS = Scale(InputWorkspace=ws,
                             OutputWorkspace=normalisedWSName,
                             Factor=1.0 / f,
                             EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        return normalisedWS

    def _outputBeamPosition(self, ws):
        """Set ws as OUTPUT_BEAM_POS_WS, if desired."""
        if not self.getProperty(Prop.OUTPUT_BEAM_POS_WS).isDefault:
            self.setProperty(Prop.OUTPUT_BEAM_POS_WS, ws)

    def _subtractFlatBkg(self, ws):
        """Return a workspace where a flat background has been subtracted from ws."""
        method = self.getProperty(Prop.BKG_METHOD).value
        if method == BkgMethod.OFF:
            return ws
        clonedWSName = self._names.withSuffix('cloned_for_flat_bkg')
        clonedWS = CloneWorkspace(InputWorkspace=ws,
                                  OutputWorkspace=clonedWSName,
                                  EnableLogging=self._subalgLogging)
        transposedWSName = self._names.withSuffix('transposed_clone')
        transposedWS = Transpose(InputWorkspace=clonedWS,
                                 OutputWorkspace=transposedWSName,
                                 EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(clonedWS)
        ranges = self._flatBkgRanges(ws)
        polynomialDegree = 0 if self.getProperty(Prop.BKG_METHOD).value == BkgMethod.CONSTANT else 1
        transposedBkgWSName = self._names.withSuffix('transposed_flat_background')
        transposedBkgWS = CalculatePolynomialBackground(InputWorkspace=transposedWS,
                                                        OutputWorkspace=transposedBkgWSName,
                                                        Degree=polynomialDegree,
                                                        XRanges=ranges,
                                                        CostFunction='Unweighted least squares',
                                                        EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(transposedWS)
        bkgWSName = self._names.withSuffix('flat_background')
        bkgWS = Transpose(InputWorkspace=transposedBkgWS,
                          OutputWorkspace=bkgWSName,
                          EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(transposedBkgWS)
        subtractedWSName = self._names.withSuffix('flat_background_subtracted')
        subtractedWS = Minus(LHSWorkspace=ws,
                             RHSWorkspace=bkgWS,
                             OutputWorkspace=subtractedWSName,
                             EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(ws)
        self._cleanup.cleanup(bkgWS)
        return subtractedWS

    def _waterCalibration(self, ws):
        """Divide ws by a (water) reference workspace."""
        if self.getProperty(Prop.WATER_REFERENCE).isDefault:
            return ws
        waterWS = self.getProperty(Prop.WATER_REFERENCE).value
        # input validation for InputWorkspace compatibility, but runs?
        if waterWS.getNumberHistograms() != ws.getNumberHistograms():
            self.log().error('Water workspace and run do not have the same number of histograms.')
        rebinnedWaterWSName = self._names.withSuffix('water_rebinned')
        rebinnedWaterWS = RebinToWorkspace(WorkspaceToRebin=waterWS,
                                           WorkspaceToMatch=ws,
                                           OutputWorkspace=rebinnedWaterWSName,
                                           EnableLogging=self._subalgLogging)
        calibratedWSName = self._names.withSuffix('water_calibrated')
        calibratedWS = Divide(LHSWorkspace=ws,
                              RHSWorkspace=rebinnedWaterWS,
                              OutputWorkspace=calibratedWSName,
                              EnableLogging=self._subalgLogging)
        self._cleanup.cleanup(rebinnedWaterWS)
        self._cleanup.cleanup(ws)
        return calibratedWS

AlgorithmFactory.subscribe(ReflectometryILLPreprocess)
