# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (
    absolute_import,
    division,
    print_function
)
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    FileProperty,
    MatrixWorkspaceProperty,
    MultipleFileProperty,
    Progress
)
from mantid.kernel import (
    CompositeValidator,
    Direction,
    FloatArrayBoundedValidator,
    FloatArrayProperty,
    IntArrayLengthValidator,
    IntArrayBoundedValidator,
    IntArrayProperty,
    Property,
    StringArrayLengthValidator,
    StringArrayProperty,
    StringListValidator
)
from mantid.simpleapi import (
    mtd,
    ReflectometryILLPreprocess,
    ReflectometryILLSumForeground,
    ReflectometryILLConvertToQ,
    ReflectometryILLPolarizationCor,
    RenameWorkspace,
    Scale,
    Stitch1DMany
)
import ILL_utilities as utils
import ReflectometryILL_common as common
from ReflectometryILLPreprocess import BkgMethod, Prop, SubalgLogging
from ReflectometryILLSumForeground import SumType
import numpy


class PropAutoProcess(object):
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'DirectFlatBackground'
    TWO_THETA = 'TwoTheta'
    DB = 'DirectRun'
    EFFICIENCY_FILE = 'EfficiencyFile'
    END_WS_INDEX_DIRECT = 'DirectFitEndWorkspaceIndex'
    END_WS_INDEX = 'FitEndWorkspaceIndex'
    END_OVERLAPS = 'EndOverlaps'
    HIGH_FOREGROUND_HALF_WIDTH = 'HighAngleForegroundHalfWidth'
    HIGH_FOREGROUND_HALF_WIDTH_DIRECT = 'DirectHighAngleForegroundHalfWidth'
    GROUPING_FRACTION = 'GroupingQFraction'
    HIGH_BKG_OFFSET = 'HighAngleBkgOffset'
    HIGH_BKG_OFFSET_DIRECT = 'DirectHighAngleBkgOffset'
    HIGH_BKG_WIDTH = 'HighAngleBkgWidth'
    HIGH_BKG_WIDTH_DIRECT = 'DirectHighAngleBkgWidth'
    LOW_BKG_OFFSET = 'LowAngleBkgOffset'
    LOW_BKG_OFFSET_DIRECT = 'DirectLowAngleBkgOffset'
    LOW_BKG_WIDTH = 'LowAngleBkgWidth'
    LOW_BKG_WIDTH_DIRECT = 'DirectLowAngleBkgWidth'
    LOW_FOREGROUND_HALF_WIDTH = 'LowAngleForegroundHalfWidth'
    LOW_FOREGROUND_HALF_WIDTH_DIRECT = 'DirectLowAngleForegroundHalfWidth'
    MANUAL_SCALE_FACTORS = 'ManualScaleFactors'
    RB = 'Run'
    SCALE_FACTOR = 'ScaleFactor'
    START_WS_INDEX = 'FitStartWorkspaceIndex'
    START_WS_INDEX_DIRECT = 'DirectFitStartWorkspaceIndex'
    START_OVERLAPS = 'StartOverlaps'
    SUM_TYPE = 'SummationType'
    USE_MANUAL_SCALE_FACTORS = 'UseManualScaleFactors'
    WAVELENGTH_UPPER = 'WavelengthUpper'
    WAVELENGTH_LOWER = 'WavelengthLower'
    XMAX = 'FitWavelengthUpper'
    XMAX_DIRECT = 'DirectFitWavelengthUpper'
    XMIN = 'FitWavelengthLower'
    XMIN_DIRECT = 'DirectFitWavelengthLower'


class Angle(object):
    SAN = 'Sample angle'
    DAN = 'Detector angle'


class ReflectometryILLAutoProcess(DataProcessorAlgorithm):

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algrithm."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLAutoProcess'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Reduction of ILL reflectometry data."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return [
            'ReflectometryILLConvertToQ',
            'ReflectometryILLPolarizationCor',
            'ReflectometryILLPreprocess',
            'ReflectometryILLSumForeground'
        ]

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        nonnegativeInts = IntArrayBoundedValidator()
        nonnegativeInts.setLower(0)
        threeNonnegativeInts = CompositeValidator()
        threeNonnegativeInts.add(IntArrayLengthValidator(3))
        threeNonnegativeInts.add(nonnegativeInts)
        maxTwoNonnegativeInts = CompositeValidator()
        maxTwoNonnegativeInts.add(IntArrayLengthValidator(lenmin=0, lenmax=2))
        maxTwoNonnegativeInts.add(nonnegativeInts)
        nonnegativeFloatArray = FloatArrayBoundedValidator()
        nonnegativeFloatArray.setLower(0.)
        stringArrayValidator = StringArrayLengthValidator()
        stringArrayValidator.setLengthMin(1)

        listOrSingleNumber = ': provide either a list or a single value for each angle.'

        self.declareProperty(
            MultipleFileProperty(
                PropAutoProcess.RB,
                action=FileAction.Load,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files.')
        self.setPropertyGroup(PropAutoProcess.RB, '')
        self.declareProperty(
            MultipleFileProperty(
                PropAutoProcess.DB,
                action=FileAction.Load,
                extensions=['nxs']
            ),
            doc='A list of direct run numbers/files.')
        self.setPropertyGroup(PropAutoProcess.DB, '')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.OUTPUT_WS,
                defaultValue='',
                direction=Direction.Output
            ),
            doc='The output workspace (momentum transfer), single histogram.')
        self.setPropertyGroup(Prop.OUTPUT_WS, '')
        self.declareProperty(
            StringArrayProperty(
                PropAutoProcess.ANGLE_OPTION,
                values=[Angle.DAN],
                validator=stringArrayValidator,
                direction=Direction.Input,
            ),
            doc='Angle option used for detector positioning{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.ANGLE_OPTION, '')
        preProcessGen = 'ReflectometryILLPreprocess, common properties'
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.TWO_THETA,
                values=[Property.EMPTY_DBL]
            ),
            doc='A user-defined angle two theta in degree{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.TWO_THETA, preProcessGen)
        self.declareProperty(
            FloatArrayProperty(
                Prop.LINE_POSITION,
                values=[Property.EMPTY_DBL]
            ),
            doc='A fractional workspace index corresponding to the beam centre between 0 and 255 {}'.
                format(listOrSingleNumber)
        )
        self.copyProperties(
            'ReflectometryILLPreprocess',
            [
                Prop.SUBALG_LOGGING,
                Prop.CLEANUP,
                Prop.WATER_REFERENCE,
                Prop.SLIT_NORM,
                Prop.FLUX_NORM_METHOD
            ]
        )
        self.setPropertyGroup(Prop.LINE_POSITION, preProcessGen)
        self.setPropertyGroup(Prop.SUBALG_LOGGING, preProcessGen)
        self.setPropertyGroup(Prop.CLEANUP, preProcessGen)
        self.setPropertyGroup(Prop.WATER_REFERENCE, preProcessGen)
        self.setPropertyGroup(Prop.SLIT_NORM, preProcessGen)
        self.setPropertyGroup(Prop.FLUX_NORM_METHOD, preProcessGen)
        preProcessDirect = 'ReflectometryILLPreprocess for direct runs'
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH_DIRECT,
                values=[0],
                validator=nonnegativeInts
            ),
            doc='Number of foreground pixels at lower angles from the centre pixel.'
        )
        self.setPropertyGroup(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH_DIRECT,
                values=[0],
                validator=nonnegativeInts
            ),
            doc='Number of foreground pixels at higher angles from the centre pixel.'
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            PropAutoProcess.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT,
                 BkgMethod.LINEAR,
                 BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.setPropertyGroup(PropAutoProcess.BKG_METHOD_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_BKG_OFFSET_DIRECT,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.LOW_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.LOW_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_BKG_OFFSET_DIRECT,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.START_WS_INDEX_DIRECT,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.START_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.END_WS_INDEX_DIRECT,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.END_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            PropAutoProcess.XMIN_DIRECT,
            defaultValue=Property.EMPTY_DBL,
            doc='Minimum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(PropAutoProcess.XMIN_DIRECT, preProcessDirect)
        self.declareProperty(
            PropAutoProcess.XMAX_DIRECT,
            defaultValue=Property.EMPTY_DBL,
            doc='Maximum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(PropAutoProcess.XMAX_DIRECT, preProcessDirect)
        preProcessReflected = 'ReflectometryILLPreprocess for reflected runs'
        self.copyProperties(
            'ReflectometryILLPreprocess',
            [
                Prop.BKG_METHOD
            ]
        )
        self.setPropertyGroup(Prop.BKG_METHOD, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Number of foreground pixels at lower angles from the centre pixel.'
        )
        self.setPropertyGroup(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Number of foreground pixels at higher angles from the centre pixel.'
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_BKG_OFFSET,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropAutoProcess.LOW_BKG_OFFSET, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_BKG_WIDTH,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.LOW_BKG_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_BKG_OFFSET,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_BKG_OFFSET, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_BKG_WIDTH,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_BKG_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.START_WS_INDEX,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.START_WS_INDEX, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.END_WS_INDEX,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.END_WS_INDEX, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.XMIN,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Minimum x value (unit wavelength) used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.XMIN, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.XMAX,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Maximum x value (unit wavelength) used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.XMAX, preProcessReflected)
        # For ReflectometryILLSumForeground
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.WAVELENGTH_LOWER,
                values=[0.],
                validator=nonnegativeFloatArray
            ),
            doc='The lower wavelength bound (Angstrom){}'.format(listOrSingleNumber)
        )
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.WAVELENGTH_UPPER,
                values=[35.],
                validator=nonnegativeFloatArray
            ),
            doc='The upper wavelength bound (Angstrom){}'.format(listOrSingleNumber)
        )
        self.declareProperty(
            StringArrayProperty(
                PropAutoProcess.SUM_TYPE,
                values=[SumType.IN_LAMBDA],
                validator=stringArrayValidator,
                direction=Direction.Input,
            ),
            doc='Type of summation to perform{}'.format(listOrSingleNumber)
        )
        # For ReflectometryILLPolarizationCorr -> action is now OptionalLoad
        self.declareProperty(
            FileProperty(
                PropAutoProcess.EFFICIENCY_FILE,
                defaultValue='',
                action=FileAction.OptionalLoad
            ),
            doc='A file containing the polarization efficiency factors.'
        )
        # For ReflectometryILLConvertToQ
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.GROUPING_FRACTION,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='If set, group the output by steps of this fraction multiplied by Q resolution'
        )
        # For Stitch1DMany
        self.copyProperties(
            'Stitch1DMany',
            [
                PropAutoProcess.START_OVERLAPS,
                PropAutoProcess.END_OVERLAPS,
                PropAutoProcess.USE_MANUAL_SCALE_FACTORS,
                PropAutoProcess.MANUAL_SCALE_FACTORS
            ]
        )
        # Final scale factor
        self.declareProperty(
            PropAutoProcess.SCALE_FACTOR,
            defaultValue=1.0,
            doc='Scale factor.'
        )

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        DB = self.getProperty(PropAutoProcess.DB).value
        RB = self.getProperty(PropAutoProcess.RB).value
        if len(DB) != len(RB):
            issues[PropAutoProcess.RB] = "The same number of direct runs and reflected runs must be given."
        return issues

    def _directBeamName(self, string1, string2):
        return self._wsPrefix + '_direct_{}_angle_{}'.format(string1, string2)

    def _getValue(self, propertyName, angle):
        """Return the value of the property at given angle."""
        value = self.getProperty(propertyName).value
        if len(value) == 1:
            return value[0]
        elif angle <= len(value):
            return value[angle]
        else:
            raise RuntimeError(
                'The number of entries for {} must correspond to the number of reflected beams'.format(propertyName)
            )

    def _twoThetaFromSampleAngle(self, run):
        """Return the two theta angle in degrees of the sample angle."""
        import h5py
        # Need to still check whether the unit is degree
        if isinstance(run, list):
            run = run[0]
        with h5py.File(run, "r") as nexus:
            if nexus.get('entry0/instrument/SAN') is not None:
                return 2. * float(numpy.array(nexus.get('entry0/instrument/SAN/value'), dtype='float'))
            elif nexus.get('entry0/instrument/san') is not None:
                return 2. * float(numpy.array(nexus.get('entry0/instrument/san/value'), dtype='float'))
            else:
                raise RuntimeError('Cannot retrieve sample angle from Nexus file {}.'.format(run))

    def _mtdName(self, runName):
        """Return a name suitable to put in the ADS"""
        nameForADS = ''
        if not isinstance(runName, list):
            return runName[-10:-4]
        else:
            for name in runName:
                nameForADS += name[-10:-4]
        return nameForADS

    def _angleOrMergeRuns(self, run):
        """Return the string that will be passed to load the files, i.e. determine if angle is treated or runs get
        merged."""
        beamInput = run
        if isinstance(run, list):
            return '+'.join(beamInput)
        return beamInput

    def _twoTheta(self, run, angle):
        """Return the TwoTheta scattering angle depending on user input options."""
        if numpy.isclose(self._getValue(PropAutoProcess.TWO_THETA, angle), Property.EMPTY_DBL):
            if self._getValue(PropAutoProcess.ANGLE_OPTION, angle) == Angle.DAN:
                self.log().notice('Using DAN angle')
                return Property.EMPTY_DBL
            elif self._getValue(PropAutoProcess.ANGLE_OPTION, angle) == Angle.SAN:
                twoT = self._twoThetaFromSampleAngle(run)
                self.log().notice('Using SAN angle : {} degree'.format(twoT / 2.))
                return twoT
            else:
                raise RuntimeError('{} must be {} or {}.'.format(PropAutoProcess.ANGLE_OPTION, Angle.SAN, Angle.DAN))
        else:
            twoT = self._getValue(PropAutoProcess.TWO_THETA, angle)
            self.log().notice('Using TwoTheta angle : {} degree'.format(twoT))
            return twoT

    def _linePosition(self, angle):
        """Return the value of the line position input property."""
        if numpy.isclose(self._getValue(Prop.LINE_POSITION, angle), Property.EMPTY_DBL):
            return Property.EMPTY_DBL
        else:
            return float(self._getValue(Prop.LINE_POSITION, angle))

    def _isPolarized(self):
        """Return True, if a polarization file is given and False otherwise."""
        if self.getProperty(PropAutoProcess.EFFICIENCY_FILE).value == "":
            return False
        else:
            return True

    def _runReflectometryILLPreprocess(self, run, outputWorkspaceName, angle, linePosition = Property.EMPTY_DBL,
                                       twoTheta = Property.EMPTY_DBL):
        """Run the ReflectometryILLPreprocess, linePosition decides, if reflected beam is present."""
        halfWidths = [int(self._getValue(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH, angle)),
                      int(self._getValue(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH, angle))]
        runDB = self._mtdName(self._db[angle])
        if linePosition != Property.EMPTY_DBL:
            directBeamName = self._directBeamName(runDB, angle)
            direct = '_DIRECT'
        else:
            directBeamName = ''
            direct = ''
        ReflectometryILLPreprocess(
            Run=run,
            OutputWorkspace=outputWorkspaceName,
            TwoTheta=twoTheta,
            LinePosition=linePosition,
            DirectLineWorkspace=directBeamName,
            ForegroundHalfWidth=halfWidths,
            SlitNormalisation=self._slitNorm,
            FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD + direct).value,
            LowAngleBkgOffset=int(self._getValue(PropAutoProcess.LOW_BKG_OFFSET + direct, angle)),
            LowAngleBkgWidth=int(self._getValue(PropAutoProcess.LOW_BKG_WIDTH + direct, angle)),
            HighAngleBkgOffset=int(self._getValue(PropAutoProcess.HIGH_BKG_OFFSET + direct, angle)),
            HighAngleBkgWidth=int(self._getValue(PropAutoProcess.HIGH_BKG_WIDTH + direct, angle)),
            FitStartWorkspaceIndex=int(self._getValue(PropAutoProcess.START_WS_INDEX + direct, angle)),
            FitEndWorkspaceIndex=int(self._getValue(PropAutoProcess.END_WS_INDEX + direct, angle)),
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )
        self._workflowProgress.report()

    def _runReflectometryILLSumForeground(self, inputWorkspaceName, outputWorkspaceName, sumType, angle,
                                          directForegroundName=''):
        """Run the ReflectometryILLSumForeground, empty directForegroundName decides, if reflected beam is present."""
        wavelengthRange = [float(self._getValue(PropAutoProcess.WAVELENGTH_LOWER, angle)),
                           float(self._getValue(PropAutoProcess.WAVELENGTH_UPPER, angle))]
        runDB = self._mtdName(self._db[angle])
        if directForegroundName is not '':
            directBeamName = self._directBeamName(runDB, angle)
        else:
            directBeamName = ''
        ReflectometryILLSumForeground(
            InputWorkspace=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            SummationType=sumType,
            DirectForegroundWorkspace=directForegroundName,
            DirectLineWorkspace=directBeamName,
            WavelengthRange=wavelengthRange,
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )
        self._workflowProgress.report()

    def _runReflectometryILLPolarizationCor(self, inputWorkspaceName, outputWorkspaceName):
        """Run the ReflectometryILLPolarizationCor."""
        ReflectometryILLPolarizationCor(
            InputWorkspaces=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            EfficiencyFile=self.getProperty(PropAutoProcess.EFFICIENCY_FILE).value,
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def _runReflectometryILLConvertToQ(self, inputWorkspaceName, outputWorkspaceName, directForegroundName, angle):
        """Run the ReflectometryILLConvertToQ."""
        ReflectometryILLConvertToQ(
            InputWorkspace=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            DirectForegroundWorkspace=directForegroundName,
            GroupingQFraction=float(self._getValue(PropAutoProcess.GROUPING_FRACTION, angle)),
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def _stitch(self, toStitch):
        """Stitch the workspaces for different angles."""
        if len(toStitch) > 1:
            Stitch1DMany(
                InputWorkspaces=','.join(toStitch),
                OutputWorkspace=self._wsPrefix,
                StartOverlaps=self.getProperty(PropAutoProcess.START_OVERLAPS).value,
                EndOverlaps=self.getProperty(PropAutoProcess.END_OVERLAPS).value,
                UseManualScaleFactors=self.getProperty(PropAutoProcess.USE_MANUAL_SCALE_FACTORS).value,
                ManualScaleFactors=self.getProperty(PropAutoProcess.MANUAL_SCALE_FACTORS).value,
            )
        else:
            RenameWorkspace(
                InputWorkspace=toStitch[0],
                OutputWorkspace=self._wsPrefix,
            )

    def _scale(self):
        """Perform a final scaling of the output workspace."""
        scaleFactor = self.getProperty(PropAutoProcess.SCALE_FACTOR).value
        if scaleFactor != 1.:
            Scale(
                InputWorkspace=self._wsPrefix,
                OutputWorkspace=self._wsPrefix,
                Factor=scaleFactor,
            )

    def _setup(self):
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value
        self._cleanup = self.getProperty(Prop.CLEANUP).value
        self._autoCleanup = utils.Cleanup(self._cleanup, self._subalgLogging == SubalgLogging.ON)
        self._wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(self._wsPrefix, self._cleanup)
        self._db = self.getProperty(PropAutoProcess.DB).value
        self._workflowProgress = Progress(self, start=0.0, end=1.0, nreports=len(self._db)*2)
        self._slitNorm = self.getProperty(Prop.SLIT_NORM).value

    def _processDirectBeam(self, directBeamName, directForegroundName, angle):
        """Combine all algorithm runs of the direct beam processing for an angle."""
        self.log().notice('Direct beam {} not cached in AnalysisDataService.'.format(directBeamName))
        directBeamInput = self._angleOrMergeRuns(self._db[angle])
        self._runReflectometryILLPreprocess(directBeamInput,
                                            directBeamName,
                                            angle)
        self._autoCleanup.protect(directBeamName)
        self._runReflectometryILLSumForeground(directBeamName,
                                               directForegroundName,
                                               SumType.IN_LAMBDA,
                                               angle)
        self._autoCleanup.protect(directForegroundName)
        if self._isPolarized():
            self._polCorDirectName = self._names.withSuffix('{}_polcor'.format(directBeamName))
            self._runReflectometryILLPolarizationCor(directForegroundName,
                                                     self._polCorDirectName)

    def _processReflectedBeam(self, reflectedInput, reflectedBeamName, directForegroundName, angle):
        """Combine all algorithm runs of the reflected beam processing for an angle."""
        twoTheta = self._twoTheta(self._rb[angle], angle)
        linePosition = self._linePosition(angle)
        self._runReflectometryILLPreprocess(reflectedInput,
                                            reflectedBeamName,
                                            angle,
                                            linePosition,
                                            twoTheta)
        foregroundName = '{}foreground'.format(reflectedBeamName)
        self._runReflectometryILLSumForeground(reflectedBeamName,
                                               foregroundName,
                                               self._getValue(PropAutoProcess.SUM_TYPE, angle),
                                               angle,
                                               directForegroundName)
        self._autoCleanup.cleanupLater(reflectedBeamName)
        self._autoCleanup.cleanupLater(foregroundName)
        return foregroundName

    def PyExec(self):
        """Execute the algorithm."""
        self._setup()
        toStitch = []
        for angle in range(len(self._db)):
            runDB = self._mtdName(self._db[angle])
            directBeamName = self._directBeamName(runDB, angle)
            directForegroundName = '{}_foreground'.format(directBeamName)
            # Direct beam already in ADS?
            if directBeamName not in mtd.getObjectNames():
                self._processDirectBeam(directBeamName, directForegroundName, angle)
            # Reflected beam
            self._rb = self.getProperty(PropAutoProcess.RB).value
            reflectedBeamInput = self._angleOrMergeRuns(self._rb[angle])
            if not self._isPolarized():
                runRB = self._mtdName(self._rb[angle])
                reflectedBeamName = self._names.withSuffix('reflected_{}'.format(runRB))
                workspaceNamesForQConversion = self._processReflectedBeam(reflectedBeamInput,
                                                                          reflectedBeamName,
                                                                          directForegroundName,
                                                                          angle)
            else:
                self._forPolCor = self._polCorDirectName
                for run in reflectedBeamInput:
                    reflectedPolWSName = self._names.withSuffix('reflected_{}'.format(run))
                    reflectedPolForegroundWSName = self._processReflectedBeam(run,
                                                                              reflectedPolWSName,
                                                                              directForegroundName,
                                                                              angle)
                    self._forPolCor.append(reflectedPolForegroundWSName)
                polCorWSName = self._names.withSuffix('reflected_polcor')
                self._runReflectometryILLPolarizationCor(','.join(self._forPolCor),
                                                         polCorWSName)
                workspaceNamesForQConversion = mtd[polCorWSName].getNames()
                for workspace in workspaceNamesForQConversion:
                    self._autoCleanup.cleanupLater(workspace)
            convertedToQName = self._names.withSuffix('angle_{}'.format(angle))
            self._runReflectometryILLConvertToQ(workspaceNamesForQConversion,
                                                convertedToQName,
                                                directForegroundName,
                                                angle)
            toStitch.append(convertedToQName)
            self._autoCleanup.cleanupLater(convertedToQName)
            self._workflowProgress.report()
            self._autoCleanup.protect(self._wsPrefix)
        self._stitch(toStitch)
        self._scale()
        self.setProperty(Prop.OUTPUT_WS, self._wsPrefix)
        self._autoCleanup.finalCleanup()

AlgorithmFactory.subscribe(ReflectometryILLAutoProcess)
