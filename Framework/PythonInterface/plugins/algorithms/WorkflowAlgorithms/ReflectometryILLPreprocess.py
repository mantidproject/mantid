# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import (CompositeValidator, Direction, EnabledWhenProperty, IntArrayLengthValidator, IntArrayBoundedValidator,
                           IntArrayProperty, IntBoundedValidator, StringListValidator, PropertyCriterion)
from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspace, MatrixWorkspaceProperty, MultipleFileProperty,
                        PropertyMode, WorkspaceUnitValidator)
from mantid.simpleapi import *
import ReflectometryILL_common as common
import ILL_utilities as utils
import numpy as np
from math import fabs, atan
from scipy.constants import physical_constants


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
    AVERAGE = 'Background Average'
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

    _bragg_angle = None
    _theta_zero = None

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

    @staticmethod
    def _check_angles_match(input_files):
        """
            Checks if the sample and detector angles difference between the loaded data is close enough
            @param  input_files: string containing paths to NeXus files to be checked
        """
        tolerance = 0.1  # degree
        file_list = input_files.split('+')
        san = common.sample_angle(file_list[0])
        dan = common.detector_angle(file_list[0])
        r = file_list[0][file_list[0].rfind('/')+1:-4]
        for file_no in range(1, len(file_list)):
            file_name = file_list[file_no]
            san_test = common.sample_angle(file_name)
            dan_test = common.detector_angle(file_name)
            r_test = file_name[file_list[0].rfind('/')+1:-4]
            if fabs(san - san_test) > tolerance:
                logger.warning('Different sample angles detected! {0}: {1}, {2}: {3}'.format(r, san,
                                                                                             r_test, san_test))
            if fabs(dan - dan_test) > tolerance:
                logger.warning('Different detector angles detected! {0}: {1}, {2}: {3}'.format(r, dan,
                                                                                               r_test, dan_test))

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

        if len((self.getPropertyValue(Prop.RUN)).split('+')) > 1:
            # foreground refitting is needed if there is more than one merged numor
            self._refit_foreground(ws)

        self._recalculate_average_chopper_params(ws)
        self._addSampleLogInfo(ws)

        if self.getPropertyValue('AngleOption') == 'DetectorAngle' and self.getPropertyValue('Measurement') == 'ReflectedBeam':
            # we still have to do this after having loaded and found the foreground centre of the reflected beam
            ws = self._calibrateDetectorAngleByDirectBeam(ws)

        ws = self._waterCalibration(ws)

        ws = self._normaliseToSlits(ws)

        ws = self._normaliseToFlux(ws, monWS)
        self._cleanup.cleanup(monWS)

        ws = self._subtractFlatBkg(ws)

        ws = self._convertToWavelength(ws)

        if self.getProperty('CorrectGravity').value and self._instrumentName == "FIGARO":
            ws = self._gravity_correction(ws)

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
        self.setPropertySettings('AngleOption',
                                 EnabledWhenProperty('Measurement', PropertyCriterion.IsEqualTo, 'ReflectedBeam'))
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
                             defaultValue=BkgMethod.AVERAGE,
                             validator=StringListValidator([BkgMethod.AVERAGE, BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]),
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

        self.declareProperty(
            name="CorrectGravity",
            defaultValue=False,
            doc="Whether to correct for gravity effects (FIGARO only)."
        )

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
            'XUnit': 'TimeOfFlight',
            'FitStartWorkspaceIndex': self.getProperty(Prop.START_WS_INDEX).value,
            'FitEndWorkspaceIndex': self.getProperty(Prop.END_WS_INDEX).value,
            'FitRangeLower': self.getProperty(Prop.XMIN).value,
            'FitRangeUpper': self.getProperty(Prop.XMAX).value
        }
        if measurement_type == 'ReflectedBeam':
            angle_option = self.getPropertyValue('AngleOption')
            first_run = self.getProperty(Prop.RUN).value[0]
            if angle_option == 'SampleAngle':
                self._bragg_angle = common.sample_angle(first_run)
            elif angle_option == 'DetectorAngle':
                self._bragg_angle = 0.0
                # in this case, we still need to correct for the difference of foreground
                # centres between direct and reflected beams, and this can be done most clearly
                # when the centre of the reflected beam is set at the beam axis
            elif angle_option == 'UserAngle':
                self._bragg_angle = self.getProperty('BraggAngle').value
            load_options['BraggAngle'] = self._bragg_angle

        # perform data consistency check
        if len(inputFiles.split('+')) > 1:
            self._check_angles_match(inputFiles)

        # MergeRunsOptions are defined by the parameter files and will not be modified here!
        ws = LoadAndMerge(
            Filename=inputFiles,
            LoaderName='LoadILLReflectometry',
            LoaderOptions=load_options,
            OutputWorkspace=mergedWSName,
            EnableLogging=self._subalgLogging
        )
        return ws

    def _refit_foreground(self, ws: MatrixWorkspace):
        """Refits the foreground position of the reflected beam measurement, and corrects position of the entire
         detector to correspond to the proper place.

        Args:
            ws (MatrixWorkspace): merged workspace that requires foreground refitting
        """
        # first, we need to  integrate the workspace indices
        int_ws = '{}_integrated'.format(ws)
        kwargs = {}
        start_index = 0
        if not self.getProperty(Prop.START_WS_INDEX).isDefault:
            kwargs['StartWorkspaceIndex'] = self.getProperty(Prop.START_WS_INDEX).value
            start_index = self.getProperty(Prop.START_WS_INDEX).value
        if not self.getProperty(Prop.END_WS_INDEX).isDefault:
            kwargs['EndWorkspaceIndex'] = self.getProperty(Prop.END_WS_INDEX).value
        if not self.getProperty(Prop.XMAX).isDefault:
            kwargs['RangeUpper'] = self.getProperty(Prop.XMAX).value
        if not self.getProperty(Prop.XMIN).isDefault:
            kwargs['RangeLower'] = self.getProperty(Prop.XMIN).value
        Integration(
            InputWorkspace=ws,
            OutputWorkspace=int_ws,
            **kwargs
        )
        Transpose(InputWorkspace=int_ws, OutputWorkspace=int_ws)  # the integration output is a bin plot, cannot be fitted
        original_peak_centre = ws.getRun().getLogData('reduction.line_position').value
        # determine the initial fitting parameters
        yAxis = mtd[int_ws].readY(0)
        max_height = np.max(yAxis)
        max_index = start_index + (np.where(yAxis == max_height))[0][0]
        sigma = max_index - (np.where(yAxis > 0.667 * max_height))[0][0]
        sigma = sigma if sigma > 0 else 2
        fit_fun = "name=FlatBackground, A0={};".format(0.3*max_height) \
                  + "name=Gaussian, PeakCentre={0}, Height={1}, Sigma={2}".format(max_index, 0.7*max_height, sigma)
        try:
            fit_name = 'fit_output'
            fit_output = Fit(Function=fit_fun,
                             InputWorkspace=int_ws,
                             StartX=float(max_index - 3 * sigma),
                             EndX=float(max_index + 3 * sigma),
                             CreateOutput=True,
                             IgnoreInvalidData=True,
                             Output='{}_{}'.format(int_ws, fit_name))
            # this table contains 5 rows, with fitted parameters of the magnitude flat background, then height, peakCentre,
            # and Sigma of the Gaussian, and finally the cost function
            param_table = fit_output.OutputParameters
            # grabbing the fitted peak centre, already as a function of spectrum number
            peak_centre = param_table.row(2)['Value']
            # finally the instrument should be rotated to the correct position

            self._cleanup.cleanup('{}_{}_Parameters'.format(int_ws, fit_name))
            self._cleanup.cleanup('{}_{}_Workspace'.format(int_ws, fit_name))
            self._cleanup.cleanup('{}_{}_NormalisedCovarianceMatrix'.format(int_ws, fit_name))
        except RuntimeError:
            self.log().error("Refitting of the foreground position of merge data failed. Using the maximum value instead.")
            peak_centre = max_index
        ws.getRun().addProperty('reduction.line_position', float(peak_centre), True)
        self._rotate_instrument(ws, peak_centre, original_peak_centre)
        self._cleanup.cleanup(int_ws)

    def _rotate_instrument(self, ws: MatrixWorkspace, peak_centre: float, original_centre: float):
        """
        Rotates the instrument of the provided workspace by the given angle around the sample, and then rotates
        the instrument around its own axis to maintain perpendicular orientation versus the incoming beam.

        Args:
        ws (MatrixWorkspace): workspace of which the instrument is going to be rotated
        peak_centre (float): fitted peak centre in fractional tube number
        original_centre (float): peak centre before the refitting
        """
        distance_entry = "det.value" if self._instrumentName == 'D17' else "Distance.Sample_CenterOfDetector_distance"
        distance = 1e-3 * ws.getRun().getLogData(distance_entry).value  # in m

        # the calculation below is reimplementation of the code in the LoadILLReflectometry that performs the same operation
        sign = -1.0 if self._instrumentName == 'D17' else 1.0
        pixel_width = common.pixelSize(self._instrumentName)
        offset_width = (127.5 - peak_centre) * pixel_width
        offset_angle = sign * atan(offset_width / distance)
        angle = offset_angle
        if self._bragg_angle is not None:  # case for a reflected beam
            angle += 2 * self._bragg_angle * np.pi / 180.0

        if self._instrumentName == 'D17':
            x = distance * np.sin(angle)
            y = 0
            z = distance * np.cos(angle)
        else:
            x = 0
            y = distance * np.sin(angle)
            z = distance * np.cos(angle)
        MoveInstrumentComponent(Workspace=ws, ComponentName="detector", x=x, y=y, z=z, RelativePosition=False)

        # after the instrument is moved to its correct new position, it also has to be rotated to face the beam
        original_angle = sign * atan((127.5 - original_centre) * pixel_width / distance) * 180.0 / np.pi
        if self._bragg_angle is not None:  # case for a reflected beam
            original_angle += 2 * self._bragg_angle
        delta_angle = original_angle - angle * 180.0 / np.pi  # in degrees
        x_orientation = 0 if self._instrumentName == 'D17' else -1
        y_orientation = 1 if self._instrumentName == 'D17' else 0
        RotateInstrumentComponent(Workspace=ws, ComponentName="detector", X=x_orientation, Y=y_orientation, Z=0,
                                  Angle=delta_angle, RelativeRotation=True)

    def _recalculate_average_chopper_params(self, ws):

        run = ws.run()
        time = run.getProperty('time').value
        logs_to_average = ['chopper1.speed_average', 'chopper1.phase_average',
                           'chopper2.speed_average', 'chopper2.phase_average']
        for log in logs_to_average:
            if run.hasProperty(log):
                param = run.getProperty(log).value
                if not isinstance(param, np.ndarray):
                    continue
                param = np.sum(param * time) / np.sum(time)
                run.addProperty(log, float(param), True)

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
            t = detWS.run().getProperty('duration').value
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

    def _calculate_average_background(self, transposedWS, transposedBkgWSName, ranges):
        """Calculates mean background in the specified detector region"""
        nspec = transposedWS.getNumberHistograms()
        blocksize = transposedWS.blocksize()
        y = transposedWS.extractY()
        x = transposedWS.extractX()
        transposedBkgWS = CloneWorkspace(InputWorkspace=transposedWS, OutputWorkspace=transposedBkgWSName)
        condition = (x >= ranges[0]) & (x <= ranges[1])
        if len(ranges) == 4:
            condition = (((x >= ranges[0]) & (x <= ranges[1])) | ((x >= ranges[2]) & (x <= ranges[3])))
        bkg_region = np.extract(condition, y)
        bkg_region = bkg_region.reshape((nspec, int(bkg_region.size/nspec)))
        bkg = np.mean(bkg_region, axis=1)
        for channel in range(nspec):
            transposedBkgWS.setE(channel, np.zeros(blocksize))
            transposedBkgWS.setY(channel, bkg[channel] * np.ones(blocksize))
        return transposedBkgWS

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
        self.log().information('Calculating background in the range ' + str(ranges))
        transposedBkgWSName = self._names.withSuffix('transposed_flat_background')
        if method == BkgMethod.CONSTANT or method == BkgMethod.LINEAR:
            # fit with polynomial
            polynomialDegree = 0 if method == BkgMethod.CONSTANT else 1
            transposedBkgWS = CalculatePolynomialBackground(
                InputWorkspace=transposedWS,
                OutputWorkspace=transposedBkgWSName,
                Degree=polynomialDegree,
                XRanges=ranges,
                CostFunction='Unweighted least squares',
                EnableLogging=self._subalgLogging
            )
        elif method == BkgMethod.AVERAGE:
            transposedBkgWS = self._calculate_average_background(transposedWS, transposedBkgWSName, ranges)
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
        calibratedWSName = self._names.withSuffix('reflected_beam_calibration')
        direct_line = self.getProperty('DirectBeamForegroundCentre').value
        run = ws.getRun()
        refl_line = run.getLogData('reduction.line_position').value
        if self._instrumentName == 'FIGARO':
            # Figaro requires an extra correction to its position equal to difference between direct and reflected
            # lines and their instrument centre
            pixel_width = common.pixelSize(self._instrumentName)
            sample_det_distance = run.getLogData('L2').value
            peak_centre = 127.5  # centre of the instrument
            ref_down = run.getLogData('refdown').value
            dir_angle = (180.0 / np.pi) * atan((direct_line - peak_centre) * pixel_width / sample_det_distance)
            refl_angle = (180.0 / np.pi) * atan((refl_line - peak_centre) * pixel_width / sample_det_distance)
            sign = -1.0 if ref_down else 1.0
            delta_angle = sign*(refl_angle - dir_angle)
            line_position = int(refl_line)
        else:
            line_position = direct_line
            delta_angle = 0.0
            sign = 1.0
        self._theta_zero = sign*(2.0 * sign * self._theta_from_detector_angles() + delta_angle)
        # LinePosition parameter below used to be defined as direct_line for both D17 and FIGARO,
        # but COSMOS for FIGARO assumes the foreground centre of the reflected beam to be exactly at the value given
        # by the _theta_zero variable, and not with regards to the direct beam as for D17.
        # For FIGARO, the direct beam position correction is handled by the delta_angle offset.
        calibratedWS = SpecularReflectionPositionCorrect(
            InputWorkspace=ws,
            OutputWorkspace=calibratedWSName,
            DetectorComponentName='detector',
            LinePosition=line_position,  # specular line position
            TwoTheta=self._theta_zero,
            PixelSize=common.pixelSize(self._instrumentName),
            DetectorCorrectionType='RotateAroundSample',
            DetectorFacesSample=True,
            EnableLogging=self._subalgLogging,
        )

        self._cleanup.cleanup(ws)
        return calibratedWS

    def _gravity_correction(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Performs simple gravity correction by replacing the wavelength X-axis and providing the corrected
        grazing angle, which is applied to data at a later stage. The algorithm below is reimplemented from COSMOS.

        Args:
             ws (MatrixWorkspace): reflected or direct beam workspace to be corrected for gravity
        """
        mm2m = 1e-3  # millimetre to metre conversion factor
        us2s = 1e-6  # microsecond to second conversion factor
        deg2rad = np.pi / 180.0
        rad2deg = 1.0 / deg2rad
        g = physical_constants['standard acceleration of gravity'][0]  # 9.80665 m / s^2
        h = physical_constants['Planck constant'][0]  # in J * s
        m_n = physical_constants['neutron mass'][0]  # in kg
        planckperkg = h / m_n  # in m^2 / s

        tsize = len(ws.readX(0))
        xAxis = np.linspace(0.5, tsize-1, tsize, endpoint=False)

        run = ws.getRun()
        channel_width = run.getLogData('FrameOverlap.channel_width').value * us2s
        poff = run.getLogData('MainParameters.pickup_offset').value
        open_off = run.getLogData('MainParameters.open_offset').value
        chop_window = run.getLogData('ChopperWindow').value
        master_phase = run.getLogData('MainParameters.Master_chopper_phase').value
        slave_phase = run.getLogData('MainParameters.Slave_chopper_phase').value
        open_angle = chop_window - (slave_phase - master_phase) - open_off
        delay_angle = master_phase + 0.5 * (poff - open_angle)
        master_speed = run.getLogData('MainParameters.Master_chopper_speed').value
        slave_speed = run.getLogData('MainParameters.Slave_chopper_speed').value
        period = 60. / (0.5 * (master_speed + slave_speed))  # rpm
        edelay = run.getLogData('PSD.time_of_flight_2').value * us2s
        delay = edelay - (delay_angle / 360.0) * period
        ref_total_tofd = ws.spectrumInfo().l1()+ws.spectrumInfo().l2(0)  # total time of flight distance
        wavelength = 1e10 * (planckperkg * (xAxis * channel_width + delay) / ref_total_tofd)
        xchopper = ws.spectrumInfo().l1()  # distance mid-chopper to sample
        offset = run.getLogData('Distance.sample_changer_horizontal_offset').value * mm2m
        xslits2 = run.getLogData('Distance.S2_sample').value * mm2m + offset
        xslits3 = run.getLogData('Distance.S3_sample').value * mm2m + offset
        cr = ws.getInstrument().getNumberParameter('chopper_radius')[0]  # m, Nexus is improperly filled

        if self._theta_zero is not None:
            theta0 = self._theta_zero * deg2rad / 2.0
        else:
            try:
                theta0 = ws.getRun().getLogData('VirtualAxis.SAN_actual_angle').value * deg2rad
            except RuntimeError:
                theta0 = ws.getRun().getLogData('VirtualAxis.DAN_actual_angle').value * deg2rad

        yslits3 = xslits3 * np.tan(theta0)  # x1 * tan(theta) in the paper
        yslits2 = xslits2 * np.tan(theta0)

        # origin is the centre of the sample
        v = planckperkg * 1e10 / wavelength  # neutron velocity in m/s, 1e10 for A to m conversion
        k = g / (2. * v ** 2)  # a characteristic inverse length

        # define parabola y = y0 - k * (x-x0) ^ 2 passing by (xslits3, yslits3) and (xslits2, yslits2)
        x0 = (k * ((xslits2**2) - (xslits3**2)) + yslits2 - yslits3) / (2 * k * (xslits2 - xslits3))
        y0 = yslits3 + k * ((xslits3 - x0)**2)

        delta = x0 - np.sqrt(y0 / k)  # shift in x along sample due to gravity

        grad = (2. * k) * (x0 - delta)  # gradient of parabola (to find true theta at y = 0)
        newTheta = np.arctan(grad)
        name = ws.name()
        shift = 2 if name[0] == '_' else 0
        new_twoTheta_ws = '{}_new_twoTheta'.format(name[shift:shift+name[shift:].find('_')])

        chopz = xchopper * np.tan(theta0) - (y0 - k * (xchopper - x0) ** 2)
        poffoff = (chopz / cr) * rad2deg

        newdelayangle = delay_angle - poffoff / 2.
        newdelay = delay + ((delay_angle - newdelayangle) / 360.) * period
        newlambda = 1e10 * (planckperkg * (xAxis * channel_width + newdelay) / ref_total_tofd)
        if self.getPropertyValue('Measurement') == 'ReflectedBeam':
            correction = np.abs(newTheta / theta0)
            # the y axis needs to be 1 value shorter than the X axis so the workspace is a histogram,
            # which is needed for rebinning at a later stage
            correction = 0.5*(correction[1:] + correction[:-1])
            CreateWorkspace(DataX=newlambda, DataY=correction, OutputWorkspace=new_twoTheta_ws, UnitX='Wavelength',
                            ParentWorkspace=ws)
        for spec_no in range(ws.getNumberHistograms()):
            ws.setX(spec_no, newlambda)
        return ws


AlgorithmFactory.subscribe(ReflectometryILLPreprocess)
