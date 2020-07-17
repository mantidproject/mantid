# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import (CompositeValidator, Direction, IntArrayLengthValidator, IntArrayBoundedValidator,
                           IntArrayProperty, IntBoundedValidator, StringListValidator, EnabledWhenProperty, PropertyCriterion)
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty,
                        PropertyMode, WorkspaceUnitValidator)
from mantid.simpleapi import *
import ReflectometryILL_common as common
import ILL_utilities as utils


class Prop:
    START_WS_INDEX = 'FitStartWorkspaceIndex'
    END_WS_INDEX = 'FitEndWorkspaceIndex'
    XMIN = 'FitRangeLower'
    XMAX = 'FitRangeUpper'
    BKG_METHOD = 'FlatBackground'
    CLEANUP = 'Cleanup'
    DIRECT_LINE_WORKSPACE = 'DirectLineWorkspace'
    FLUX_NORM_METHOD = 'FluxNormalisation'
    FOREGROUND_HALF_WIDTH = 'ForegroundHalfWidth'
    HIGH_BKG_OFFSET = 'HighAngleBkgOffset'
    HIGH_BKG_WIDTH = 'HighAngleBkgWidth'
    INPUT_WS = 'InputWorkspace'
    LINE_POSITION = 'LinePosition'
    LOW_BKG_OFFSET = 'LowAngleBkgOffset'
    LOW_BKG_WIDTH = 'LowAngleBkgWidth'
    OUTPUT_WS = 'OutputWorkspace'
    RUN = 'Run'
    SLIT_NORM = 'SlitNormalisation'
    THETA = 'BraggAngle'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    WATER_REFERENCE = 'WaterWorkspace'


class BkgMethod:
    CONSTANT = 'Background Constant Fit'
    LINEAR = 'Background Linear Fit'
    OFF = 'Background OFF'


class FluxNormMethod:
    MONITOR = 'Normalise To Monitor'
    TIME = 'Normalise To Time'
    OFF = 'Normalisation OFF'


class SlitNorm:
    AUTO = 'Slit Normalisation AUTO'
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
        return ['ReflectometryILLConvertToQ', 'ReflectometryILLPolarizationCor',
                'ReflectometryILLSumForeground', 'ReflectometryILLAutoProcess']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)
        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(wsPrefix, cleanupMode)

        ws = self._inputWS()

        self._instrumentName = ws.getInstrument().getName()

        ws, monWS = self._extractMonitors(ws)

        self._addSampleLogInfo(ws)

        if self.getPropertyValue('AngleOption')=='DetectorAngle' and self.getPropertyValue('Measurement')=='ReflectedBeam':
            # we still have to do this after having loaded and found the foreground centre of the reflected beam
            ws = self._calibrateDetectorAngleByDirectBeam(ws)

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
        nonnegativeIntArray = IntArrayBoundedValidator(lower=0)
        maxTwoNonnegativeInts = CompositeValidator()
        maxTwoNonnegativeInts.add(IntArrayLengthValidator(lenmin=0, lenmax=2))
        maxTwoNonnegativeInts.add(nonnegativeIntArray)
        self.declareProperty(MultipleFileProperty(Prop.RUN,
                                                  extensions=['nxs']),
                             doc='A list of input run numbers/files.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.OUTPUT_WS,
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='The preprocessed output workspace (unit wavelength), single histogram.')
        self.declareProperty('Measurement', 'DirectBeam',
                             StringListValidator(['DirectBeam', 'ReflectedBeam']),
                             'Whether to process as direct or reflected beam.')
        self.declareProperty('AngleOption', 'SampleAngle',
                             StringListValidator(['SampleAngle', 'DetectorAngle', 'UserAngle']))
        self.declareProperty(Prop.THETA,
                             defaultValue=-1.,
                             doc='The bragg angle for reflected beam [Degree], used if angle option is UserAngle.')
        self.setPropertySettings(Prop.THETA,
                                 EnabledWhenProperty('AngleOption', PropertyCriterion.IsEqualTo, 'UserAngle'))
        self.declareProperty('DirectBeamDetectorAngle', -1.,
                             'The detector angle value [Degree] for the corresponding direct beam, '
                             'used if angle option is DetectorAngle')
        self.declareProperty('DirectBeamForegroundCentre', -1.,
                             'Fractional pixel index for the direct beam, used if angle option is DetectorAngle.')
        self.setPropertySettings('DirectBeamDetectorAngle',
                                 EnabledWhenProperty('AngleOption', PropertyCriterion.IsEqualTo, 'DetectorAngle'))
        self.setPropertySettings('DirectBeamForegroundCentre',
                                 EnabledWhenProperty('AngleOption', PropertyCriterion.IsEqualTo, 'DetectorAngle'))
        self.declareProperty(Prop.SUBALG_LOGGING,
                             defaultValue=SubalgLogging.OFF,
                             validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
                             doc='Enable or disable child algorithm logging.')
        self.declareProperty(Prop.CLEANUP,
                             defaultValue=utils.Cleanup.ON,
                             validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
                             doc='Enable or disable intermediate workspace cleanup.')
        self.declareProperty(MatrixWorkspaceProperty(Prop.WATER_REFERENCE,
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     validator=WorkspaceUnitValidator("TOF"),
                                                     optional=PropertyMode.Optional),
                             doc='A (water) calibration workspace (unit TOF).')
        self.declareProperty(Prop.SLIT_NORM,
                             defaultValue=SlitNorm.AUTO,
                             validator=StringListValidator([SlitNorm.AUTO, SlitNorm.OFF, SlitNorm.ON]),
                             doc='Enable or disable slit normalisation.')
        self.declareProperty(Prop.FLUX_NORM_METHOD,
                             defaultValue=FluxNormMethod.TIME,
                             validator=StringListValidator([FluxNormMethod.TIME,
                                                            FluxNormMethod.MONITOR,
                                                            FluxNormMethod.OFF]),
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
                             doc='Distance of flat background region towards smaller detector angles from the '
                                 + 'foreground centre, in pixels.')
        self.declareProperty(Prop.LOW_BKG_WIDTH,
                             defaultValue=5,
                             validator=nonnegativeInt,
                             doc='Width of flat background region towards smaller detector angles from the '
                                 + 'foreground centre, in pixels.')
        self.declareProperty(Prop.HIGH_BKG_OFFSET,
                             defaultValue=7,
                             validator=nonnegativeInt,
                             doc='Distance of flat background region towards larger detector angles from the '
                                 + 'foreground centre, in pixels.')
        self.declareProperty(Prop.HIGH_BKG_WIDTH,
                             defaultValue=5,
                             validator=nonnegativeInt,
                             doc='Width of flat background region towards larger detector angles from the '
                                 + 'foreground centre, in pixels.')
        self.declareProperty(Prop.START_WS_INDEX,
                             validator=wsIndexRange,
                             defaultValue=0,
                             doc='Start workspace index used for peak fitting.')
        self.declareProperty(Prop.END_WS_INDEX,
                             validator=wsIndexRange,
                             defaultValue=255,
                             doc='Last workspace index used for peak fitting.')
        self.declareProperty(Prop.XMIN,
                             defaultValue=-1.,
                             doc='Minimum wavelength [Angstrom] used for peak fitting.')
        self.declareProperty(Prop.XMAX,
                             defaultValue=-1.,
                             doc='Maximum wavelength [Angstrom] used for peak fitting.')

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        if self.getPropertyValue('Measurement') == 'ReflectedBeam':
            angle_option = self.getPropertyValue('AngleOption')
            if angle_option == 'UserAngle' and self.getProperty('BraggAngle').isDefault:
                issues['BraggAngle'] = 'Bragg angle is mandatory if the angle option is UserAngle'
            elif angle_option == 'DetectorAngle':
                if self.getProperty('DirectBeamDetectorAngle').isDefault:
                    issues['DirectBeamDetectorAngle'] = 'Direct beam detector angle is mandatory' \
                                                        ' if the angle option is DetectorAngle'
                if self.getProperty('DirectBeamForegroundCentre').isDefault:
                    issues['DirectBeamForegroundCentre'] = 'Direct beam foreground centre is mandatory' \
                                                           ' if the angle option is DetectorAngle'

        if self.getProperty(Prop.BKG_METHOD).value != BkgMethod.OFF:
            if self.getProperty(Prop.LOW_BKG_WIDTH).value == 0 and self.getProperty(Prop.HIGH_BKG_WIDTH).value == 0:
                issues[Prop.BKG_METHOD] = 'Cannot calculate flat background if both upper and lower background /' \
                                          ' widths are zero.'
        # Early input validation to prevent FindReflectometryLines to fail its validation
        if not self.getProperty(Prop.XMIN).isDefault and not self.getProperty(Prop.XMAX).isDefault:
            xmin = self.getProperty(Prop.XMIN).value
            xmax = self.getProperty(Prop.XMAX).value
            if xmax < xmin:
                issues[Prop.XMIN] = 'Must be smaller than RangeUpper.'
            if xmin < 0.0:
                issues[Prop.XMIN] = 'Must be larger or equal than 0.0.'
            if xmax > 255.0:
                issues[Prop.XMAX] = 'Must be smaller or equal than 255.0.'
        if not self.getProperty(Prop.START_WS_INDEX).isDefault \
                and not self.getProperty(Prop.END_WS_INDEX).isDefault:
            minIndex = self.getProperty(Prop.START_WS_INDEX).value
            maxIndex = self.getProperty(Prop.END_WS_INDEX).value
            if maxIndex < minIndex:
                issues[Prop.START_WS_INDEX] = 'Must be smaller than EndWorkspaceIndex.'
        return issues

    def _convertToWavelength(self, ws):
        """Convert the X units of ws to wavelength."""
        wavelengthWSName = self._names.withSuffix('in_wavelength')
        wavelengthWS = ConvertUnits(
            InputWorkspace=ws,
            OutputWorkspace=wavelengthWSName,
            Target='Wavelength',
            EMode='Elastic',
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(ws)
        return wavelengthWS

    def _extractMonitors(self, ws):
        """Extract monitor spectra from ws to another workspace."""
        detWSName = self._names.withSuffix('detectors')
        monWSName = self._names.withSuffix('monitors')
        ExtractMonitors(
            InputWorkspace=ws,
            DetectorWorkspace=detWSName,
            MonitorWorkspace=monWSName,
            EnableLogging=self._subalgLogging
        )
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
        peakPos = ws.run().getProperty(common.SampleLogs.FOREGROUND_CENTRE).value
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

    def _theta_from_detector_angles(self):
        """Returns the bragg angle as half of detector angle difference"""
        first_run = self.getProperty(Prop.RUN).value[0]
        db_detector_angle = self.getProperty('DirectBeamDetectorAngle').value
        return (common.detector_angle(first_run) - db_detector_angle) / 2.

    def _inputWS(self):
        """Return a raw input workspace."""
        inputFiles = self.getPropertyValue(Prop.RUN)
        inputFiles = inputFiles.replace(',', '+')
        mergedWSName = self._names.withSuffix('merged')
        measurement_type = self.getPropertyValue('Measurement')
        load_options = {
            'Measurement': measurement_type,
            'FitStartWorkspaceIndex': self.getProperty(Prop.START_WS_INDEX).value,
            'FitEndWorkspaceIndex': self.getProperty(Prop.END_WS_INDEX).value,
            'FitRangeLower': self.getProperty(Prop.XMIN).value,
            'FitRangeUpper': self.getProperty(Prop.XMAX).value
        }
        if measurement_type == 'ReflectedBeam':
            bragg_angle = None
            angle_option = self.getPropertyValue('AngleOption')
            first_run = self.getProperty(Prop.RUN).value[0]
            if angle_option == 'SampleAngle':
                bragg_angle = common.sample_angle(first_run)
            elif angle_option == 'DetectorAngle':
                bragg_angle = self._theta_from_detector_angles()
                # in this clause we still need to correct for the difference of foreground
                # centres between direct and reflected beams
                # but we need first to load the reflected beam to be able to do this
            elif angle_option == 'UserAngle':
                bragg_angle = self.getProperty('BraggAngle').value
            load_options['BraggAngle'] = bragg_angle

        # MergeRunsOptions are defined by the parameter files and will not be modified here!
        ws = LoadAndMerge(
            Filename=inputFiles,
            LoaderName='LoadILLReflectometry',
            LoaderOptions=load_options,
            OutputWorkspace=mergedWSName,
            EnableLogging=self._subalgLogging
        )
        return ws

    def _addSampleLogInfo(self, ws):
        """Add foreground indices (start, center, end), names start with reduction."""
        run = ws.run()
        hws = self._foregroundWidths()
        foregroundCentre = run.getProperty(common.SampleLogs.LINE_POSITION).value
        sign = self._workspaceIndexDirection(ws)
        startIndex = foregroundCentre - sign * hws[0]
        endIndex = foregroundCentre + sign * hws[1]
        if startIndex > endIndex:
            endIndex, startIndex = startIndex, endIndex
        # note that those 3 are integers, but he line position is fractional
        run.addProperty(common.SampleLogs.FOREGROUND_START, int(startIndex), True)
        run.addProperty(common.SampleLogs.FOREGROUND_CENTRE, int(foregroundCentre), True)
        run.addProperty(common.SampleLogs.FOREGROUND_END, int(endIndex), True)

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
            normalisedWS = NormaliseToMonitor(
                InputWorkspace=detWS,
                OutputWorkspace=normalisedWSName,
                MonitorWorkspace=monWS,
                MonitorWorkspaceIndex=monIndex,
                IntegrationRangeMin=minX,
                IntegrationRangeMax=maxX,
                EnableLogging=self._subalgLogging
            )
            self._cleanup.cleanup(detWS)
            return normalisedWS
        elif method == FluxNormMethod.TIME:
            t = detWS.run().getProperty('time').value
            normalisedWSName = self._names.withSuffix('normalised_to_time')
            scaledWS = Scale(
                InputWorkspace=detWS,
                OutputWorkspace=normalisedWSName,
                Factor=1.0 / t,
                EnableLogging=self._subalgLogging
            )
            self._cleanup.cleanup(detWS)
            return scaledWS
        return detWS

    def _normaliseToSlits(self, ws):
        """Normalise ws to slit opening and update slit widths."""
        # Update slit width in any case for later re-use.
        common.slitSizes(ws)
        slitNorm = self.getProperty(Prop.SLIT_NORM).value
        if slitNorm == SlitNorm.OFF:
            return ws
        elif slitNorm == SlitNorm.AUTO and self._instrumentName != 'D17':
            return ws
        run = ws.run()
        slit2width = run.get(common.SampleLogs.SLIT2WIDTH).value
        slit3width = run.get(common.SampleLogs.SLIT3WIDTH).value
        if slit2width == '-' or slit3width == '-':
            self.log().warning('Slit information not found in sample logs. Slit normalisation disabled.')
            return ws
        f = slit2width * slit3width
        normalisedWSName = self._names.withSuffix('normalised_to_slits')
        normalisedWS = Scale(
            InputWorkspace=ws,
            OutputWorkspace=normalisedWSName,
            Factor=1.0 / f,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(ws)
        return normalisedWS

    def _subtractFlatBkg(self, ws):
        """Return a workspace where a flat background has been subtracted from ws."""
        method = self.getProperty(Prop.BKG_METHOD).value
        if method == BkgMethod.OFF:
            return ws
        clonedWSName = self._names.withSuffix('cloned_for_flat_bkg')
        clonedWS = CloneWorkspace(
            InputWorkspace=ws,
            OutputWorkspace=clonedWSName,
            EnableLogging=self._subalgLogging
        )
        transposedWSName = self._names.withSuffix('transposed_clone')
        transposedWS = Transpose(
            InputWorkspace=clonedWS,
            OutputWorkspace=transposedWSName,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(clonedWS)
        ranges = self._flatBkgRanges(ws)
        polynomialDegree = 0 if self.getProperty(Prop.BKG_METHOD).value == BkgMethod.CONSTANT else 1
        transposedBkgWSName = self._names.withSuffix('transposed_flat_background')
        transposedBkgWS = CalculatePolynomialBackground(
            InputWorkspace=transposedWS,
            OutputWorkspace=transposedBkgWSName,
            Degree=polynomialDegree,
            XRanges=ranges,
            CostFunction='Unweighted least squares',
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(transposedWS)
        bkgWSName = self._names.withSuffix('flat_background')
        bkgWS = Transpose(
            InputWorkspace=transposedBkgWS,
            OutputWorkspace=bkgWSName,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(transposedBkgWS)
        subtractedWSName = self._names.withSuffix('flat_background_subtracted')
        subtractedWS = Minus(
            LHSWorkspace=ws,
            RHSWorkspace=bkgWS,
            OutputWorkspace=subtractedWSName,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(ws)
        self._cleanup.cleanup(bkgWS)
        return subtractedWS

    def _waterCalibration(self, ws):
        """Divide ws by a (water) reference workspace."""
        if self.getProperty(Prop.WATER_REFERENCE).isDefault:
            return ws
        waterWS = self.getProperty(Prop.WATER_REFERENCE).value
        detWSName = self._names.withSuffix('water_detectors')
        waterWS = ExtractMonitors(
            InputWorkspace=waterWS,
            DetectorWorkspace=detWSName,
            EnableLogging=self._subalgLogging
        )
        if mtd.doesExist(detWSName) is None:
            raise RuntimeError('No detectors in the water reference data.')
        if waterWS.getNumberHistograms() != ws.getNumberHistograms():
            self.log().error('Water workspace and run do not have the same number of histograms.')
        rebinnedWaterWSName = self._names.withSuffix('water_rebinned')
        rebinnedWaterWS = RebinToWorkspace(
            WorkspaceToRebin=waterWS,
            WorkspaceToMatch=ws,
            OutputWorkspace=rebinnedWaterWSName,
            EnableLogging=self._subalgLogging
        )
        calibratedWSName = self._names.withSuffix('water_calibrated')
        calibratedWS = Divide(
            LHSWorkspace=ws,
            RHSWorkspace=rebinnedWaterWS,
            OutputWorkspace=calibratedWSName,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(waterWS)
        self._cleanup.cleanup(rebinnedWaterWS)
        self._cleanup.cleanup(ws)
        return calibratedWS

    def _calibrateDetectorAngleByDirectBeam(self, ws):
        """Perform detector position correction for reflected beams."""
        direct_line = self.getProperty('DirectBeamForegroundCentre').value
        calibratedWSName = self._names.withSuffix('reflected_beam_calibration')
        calibratedWS = SpecularReflectionPositionCorrect(
            InputWorkspace=ws,
            OutputWorkspace=calibratedWSName,
            DetectorComponentName='detector',
            LinePosition=direct_line, # yes, this is the direct line position!
            TwoTheta=2*self._theta_from_detector_angles(),
            PixelSize=common.pixelSize(self._instrumentName),
            DetectorCorrectionType='RotateAroundSample',
            DetectorFacesSample=True,
            EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(ws)
        return calibratedWS

AlgorithmFactory.subscribe(ReflectometryILLPreprocess)
