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
    MultipleFileProperty,
    Progress,
    WorkspaceGroupProperty
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
from mantid.simpleapi import *
import ILL_utilities as utils
from ReflectometryILLPreprocess import BkgMethod, Prop, SubalgLogging
from ReflectometryILLSumForeground import SumType
import numpy


class PropertyNames(object):
    RB = 'Run'
    DB = 'DirectRun'
    WAVELENGTH_LOWER = 'WavelengthLowerBound'
    WAVELENGTH_UPPER = 'WavelengthUpperBound'
    EFFICIENCY_FILE = 'EfficiencyFile'
    GROUPING_FRACTION = 'DeltaQFractionBinning'
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'DirectFlatBackground'
    BKG_METHOD = 'ReflFlatBackgroun'
    TWO_THETA = 'TwoTheta'
    SCALE_FACTOR = 'GlobalScaleFactor'
    SUM_TYPE = 'SummationType'

    HIGH_FRG_HALF_WIDTH = 'ReflHighAngleFrgHalfWidth'
    HIGH_FRG_HALF_WIDTH_DIRECT = 'DirectHighAngleFrgHalfWidth'
    HIGH_BKG_OFFSET = 'ReflHighAngleBkgOffset'
    HIGH_BKG_OFFSET_DIRECT = 'DirectHighAngleBkgOffset'
    HIGH_BKG_WIDTH = 'ReflHighAngleBkgWidth'
    HIGH_BKG_WIDTH_DIRECT = 'DirectHighAngleBkgWidth'

    LOW_FRG_HALF_WIDTH = 'ReflLowAngleFrgHalfWidth'
    LOW_FRG_HALF_WIDTH_DIRECT = 'DirectLowAngleFrgHalfWidth'
    LOW_BKG_OFFSET = 'ReflLowAngleBkgOffset'
    LOW_BKG_OFFSET_DIRECT = 'DirectLowAngleBkgOffset'
    LOW_BKG_WIDTH = 'ReflLowAngleBkgWidth'
    LOW_BKG_WIDTH_DIRECT = 'DirectLowAngleBkgWidth'

    START_WS_INDEX = 'ReflFitStartWorkspaceIndex'
    END_WS_INDEX = 'ReflFitEndWorkspaceIndex'
    START_WS_INDEX_DIRECT = 'DirectFitStartWorkspaceIndex'
    END_WS_INDEX_DIRECT = 'DirectFitEndWorkspaceIndex'
    XMAX = 'ReflFitWavelengthUpperBound'
    XMAX_DIRECT = 'DirectFitWavelengthUpperBound'
    XMIN = 'ReflFitWavelengthLowerBound'
    XMIN_DIRECT = 'DirectFitWavelengthLowerBound'

    # all these array properties must have either single value, or
    # as many, as there are reflected beams (i.e. angle configurations)
    PROPETIES_TO_SIZE_MATCH = [DB, ANGLE_OPTION, TWO_THETA,
                               SUM_TYPE, HIGH_FRG_HALF_WIDTH, HIGH_FRG_HALF_WIDTH_DIRECT,
                               HIGH_BKG_OFFSET, HIGH_BKG_OFFSET_DIRECT, HIGH_BKG_WIDTH, HIGH_BKG_WIDTH_DIRECT,
                               LOW_FRG_HALF_WIDTH, LOW_FRG_HALF_WIDTH_DIRECT, LOW_BKG_OFFSET, LOW_BKG_OFFSET_DIRECT,
                               LOW_BKG_WIDTH, LOW_BKG_WIDTH_DIRECT, START_WS_INDEX, END_WS_INDEX, START_WS_INDEX_DIRECT,
                               END_WS_INDEX_DIRECT]

    DAN = 'DetectorAngle'
    SAN = 'SampleAngle'


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
        maxTwoNonnegativeInts = CompositeValidator()
        maxTwoNonnegativeInts.add(IntArrayLengthValidator(lenmin=0, lenmax=2))
        maxTwoNonnegativeInts.add(nonnegativeInts)
        nonnegativeFloatArray = FloatArrayBoundedValidator()
        nonnegativeFloatArray.setLower(0.)
        stringArrayValidator = StringArrayLengthValidator()
        stringArrayValidator.setLengthMin(1)
        listOrSingleNumber = ': provide either a list or a single value.'

        #======================== Main Properties ========================
        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.RB,
                action=FileAction.Load,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files.')
        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.DB,
                action=FileAction.Load,
                extensions=['nxs']
            ),
            doc='A list of direct run numbers/files.')
        self.declareProperty(
            WorkspaceGroupProperty(
                Prop.OUTPUT_WS,
                defaultValue='',
                direction=Direction.Output
            ),
            doc='The output workspace group.')
        self.declareProperty(
            StringArrayProperty(
                PropertyNames.ANGLE_OPTION,
                values=[PropertyNames.DAN],
                validator=stringArrayValidator,
                direction=Direction.Input,
            ),
            doc='Angle option used for detector positioning' + listOrSingleNumber
        )
        self.declareProperty(
            StringArrayProperty(
                PropertyNames.SUM_TYPE,
                values=['Incoherent'],
                validator=stringArrayValidator,
                direction=Direction.Input,
            ),
            doc='Type of summation to perform' + listOrSingleNumber
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.WAVELENGTH_LOWER,
                values=[0.],
                validator=nonnegativeFloatArray
            ),
            doc='The lower wavelength bound (Angstrom)' + listOrSingleNumber
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.WAVELENGTH_UPPER,
                values=[35.],
                validator=nonnegativeFloatArray
            ),
            doc='The upper wavelength bound (Angstrom)' + listOrSingleNumber
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.GROUPING_FRACTION,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='If set, group the output by steps of this fraction multiplied by Q resolution'
        )
        self.declareProperty(
            PropertyNames.SCALE_FACTOR,
            defaultValue=1.0,
            doc='Scale factor.'
        )
        self.declareProperty(
            FileProperty(
                PropertyNames.EFFICIENCY_FILE,
                defaultValue='',
                action=FileAction.OptionalLoad
            ),
            doc='A file containing the polarization efficiency factors.'
        )

        # ======================== Preprocessing Common Properties ========================
        preProcessGen = 'Preprocessing common properties'
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.TWO_THETA,
                values=[Property.EMPTY_DBL]
            ),
            doc='A user-defined angle two theta in degree' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.TWO_THETA, preProcessGen)
        self.declareProperty(
            FloatArrayProperty(
                Prop.LINE_POSITION,
                values=[Property.EMPTY_DBL]
            ),
            doc='A fractional workspace index corresponding to the beam centre between 0 and 255' + listOrSingleNumber
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

        # ======================== Preprocessing For Direct Run Properties ========================
        preProcessDirect = 'Preprocessing for direct runs'
        self.declareProperty(
            PropertyNames.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT,
                 BkgMethod.LINEAR,
                 BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.setPropertyGroup(PropertyNames.BKG_METHOD_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT,
                values=[0],
                validator=nonnegativeInts
            ),
            doc='Number of foreground pixels at lower angles from the centre pixel.'
        )
        self.setPropertyGroup(PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_BKG_OFFSET_DIRECT,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT,
                values=[0],
                validator=nonnegativeInts
            ),
            doc='Number of foreground pixels at higher angles from the centre pixel.'
        )
        self.setPropertyGroup(PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_BKG_OFFSET_DIRECT,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.START_WS_INDEX_DIRECT,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.START_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.END_WS_INDEX_DIRECT,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.END_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            PropertyNames.XMIN_DIRECT,
            defaultValue=Property.EMPTY_DBL,
            doc='Minimum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(PropertyNames.XMIN_DIRECT, preProcessDirect)
        self.declareProperty(
            PropertyNames.XMAX_DIRECT,
            defaultValue=Property.EMPTY_DBL,
            doc='Maximum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(PropertyNames.XMAX_DIRECT, preProcessDirect)

        # ======================== Preprocessing For Reflected Run Properties ========================
        preProcessReflected = 'Preprocessing for reflected runs'
        self.declareProperty(
            PropertyNames.BKG_METHOD,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT,
                 BkgMethod.LINEAR,
                 BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.setPropertyGroup(PropertyNames.BKG_METHOD, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_FRG_HALF_WIDTH,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Number of foreground pixels at lower angles from the centre pixel.'
        )
        self.setPropertyGroup(PropertyNames.LOW_FRG_HALF_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_BKG_OFFSET,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_OFFSET, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_BKG_WIDTH,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_FRG_HALF_WIDTH,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Number of foreground pixels at higher angles from the centre pixel.'
        )
        self.setPropertyGroup(PropertyNames.HIGH_FRG_HALF_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_BKG_OFFSET,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels'+listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_OFFSET, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_BKG_WIDTH,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.START_WS_INDEX,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.START_WS_INDEX, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.END_WS_INDEX,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.END_WS_INDEX, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.XMIN,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Minimum x value (unit wavelength) used for peak fitting' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.XMIN, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.XMAX,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Maximum x value (unit wavelength) used for peak fitting' + listOrSingleNumber
        )
        self.setPropertyGroup(PropertyNames.XMAX, preProcessReflected)

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        RB = self.getProperty(PropertyNames.RB).value
        dimensionality = len(RB)
        for property_name in PropertyNames.PROPETIES_TO_SIZE_MATCH:
            value = self.getProperty(property_name).value
            if len(value) != dimensionality and len(value) != 1:
                issues[property_name] = 'Must have a single value or as many as there are reflected beams: given {0}, ' \
                                        'but there are {1} reflected beams'.format(len(value), dimensionality)
        angle_options = self.getProperty(PropertyNames.ANGLE_OPTION).value
        for angle_option in angle_options:
            if angle_option not in [PropertyNames.DAN, PropertyNames.SAN]:
                issues[PropertyNames.ANGLE_OPTION] = 'Invalid angle option given: ' + angle_option
                break
        sum_types = self.getProperty(PropertyNames.SUM_TYPE).value
        for sum_type in sum_types:
            if sum_type not in ['Incoherent', 'Coherent']:
                issues[PropertyNames.SUM_TYPE] = 'Invalid summation option given: ' + sum_type
                break
        return issues

    def _setup(self):
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value
        self._cleanup = self.getProperty(Prop.CLEANUP).value
        self._autoCleanup = utils.Cleanup(self._cleanup, self._subalgLogging == SubalgLogging.ON)
        self._outWS = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(self._outWS, self._cleanup)
        self._db = self.getProperty(PropertyNames.DB).value
        self._rb = self.getProperty(PropertyNames.RB).value
        self._dimensionality = len(self._rb)
        self._slitNorm = self.getProperty(Prop.SLIT_NORM).value

    def _getValue(self, propertyName, angle_index):
        """Return the value of the property at given angle index."""
        value = self.getProperty(propertyName).value
        if len(value) == 1:
            return value[0]
        else:
            return value[angle_index]

    def _mtdName(self, runName):
        """Return a name suitable to put in the ADS: the run number"""
        if not isinstance(runName, list):
            return runName[-10:-4]
        else:
            # for multiple files return the run number of the first run
            return runName[0][-10:-4]

    def _composeRunString(self, run):
        """Return the string that will be passed to load the files."""
        if isinstance(run, list):
            return '+'.join(run)
        else:
            return run

    def _isPolarized(self):
        """Return True, if a polarization file is given and False otherwise."""
        return self.getProperty(PropertyNames.EFFICIENCY_FILE).value != ""

    def _twoThetaFromSampleAngle(self, run):
        """Return the two theta angle in degrees of the sample angle."""
        import h5py
        # Need to still check whether the unit is degree
        if isinstance(run, list):
            first_run = run[0]
        else:
            first_run = run
        with h5py.File(first_run, "r") as nexus:
            if nexus.get('entry0/instrument/SAN') is not None:
                return 2. * float(numpy.array(nexus.get('entry0/instrument/SAN/value'), dtype='float'))
            elif nexus.get('entry0/instrument/san') is not None:
                return 2. * float(numpy.array(nexus.get('entry0/instrument/san/value'), dtype='float'))
            else:
                raise RuntimeError('Cannot retrieve sample angle from Nexus file {}.'.format(first_run))

    def _twoTheta(self, angle_index):
        """Return the TwoTheta scattering angle depending on user input options."""
        two_theta = self._getValue(PropertyNames.TWO_THETA, angle_index)
        angle_option = self._getValue(PropertyNames.ANGLE_OPTION, angle_index)
        if numpy.isclose(two_theta, Property.EMPTY_DBL):
            if  angle_option == PropertyNames.DAN:
                self.log().information('Using DAN angle')
                return Property.EMPTY_DBL
            elif angle_option == PropertyNames.SAN:
                two_theta = self._twoThetaFromSampleAngle(self._rb[angle_index])
                self.log().information('Using SAN angle : {} degree'.format(two_theta / 2.))
                return two_theta
        else:
            self.log().information('Using TwoTheta angle : {} degree'.format(two_theta))
            return two_theta

    def _linePosition(self, angle_index):
        """Return the value of the line position input property."""
        line_pos = self._getValue(Prop.LINE_POSITION, angle_index)
        if numpy.isclose(line_pos, Property.EMPTY_DBL):
            return Property.EMPTY_DBL
        else:
            return float(line_pos)

    def _runReflectometryILLPreprocess(self, run, outputWorkspaceName, angle_index, linePosition = Property.EMPTY_DBL,
                                       twoTheta = Property.EMPTY_DBL):
        """Run the ReflectometryILLPreprocess, linePosition decides, if reflected beam is present."""
        halfWidths = [int(self._getValue(PropertyNames.LOW_FRG_HALF_WIDTH, angle_index)),
                      int(self._getValue(PropertyNames.HIGH_FRG_HALF_WIDTH, angle_index))]
        if linePosition != Property.EMPTY_DBL:
            directBeamName = self._mtdName(self._db[angle_index]) + '_direct'
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
            LowAngleBkgOffset=int(self._getValue(PropertyNames.LOW_BKG_OFFSET + direct, angle_index)),
            LowAngleBkgWidth=int(self._getValue(PropertyNames.LOW_BKG_WIDTH + direct, angle_index)),
            HighAngleBkgOffset=int(self._getValue(PropertyNames.HIGH_BKG_OFFSET + direct, angle_index)),
            HighAngleBkgWidth=int(self._getValue(PropertyNames.HIGH_BKG_WIDTH + direct, angle_index)),
            FitStartWorkspaceIndex=int(self._getValue(PropertyNames.START_WS_INDEX + direct, angle_index)),
            FitEndWorkspaceIndex=int(self._getValue(PropertyNames.END_WS_INDEX + direct, angle_index)),
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def _runReflectometryILLSumForeground(self, inputWorkspaceName, outputWorkspaceName, sumType, angle_index,
                                          directForegroundName = ''):
        """Run the ReflectometryILLSumForeground, empty directForegroundName decides, if reflected beam is present."""
        wavelengthRange = [float(self._getValue(PropertyNames.WAVELENGTH_LOWER, angle_index)),
                           float(self._getValue(PropertyNames.WAVELENGTH_UPPER, angle_index))]
        directBeamName = directForegroundName[:-4] if directForegroundName else ''
        # we need to cache both direct line workspace and its foreground, as they are both needed for reflected beam
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

    def _runReflectometryILLPolarizationCor(self, inputWorkspaceName, outputWorkspaceName):
        """Run the ReflectometryILLPolarizationCor."""
        ReflectometryILLPolarizationCor(
            InputWorkspaces=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            EfficiencyFile=self.getProperty(PropertyNames.EFFICIENCY_FILE).value,
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def _runReflectometryILLConvertToQ(self, inputWorkspaceName, outputWorkspaceName, directForegroundName, angle_index):
        """Run the ReflectometryILLConvertToQ."""
        ReflectometryILLConvertToQ(
            InputWorkspace=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            DirectForegroundWorkspace=directForegroundName,
            GroupingQFraction=float(self._getValue(PropertyNames.GROUPING_FRACTION, angle_index)),
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def _processDirectBeam(self, directBeamName, directForegroundName, angle_index):
        """Combine all algorithm runs of the direct beam processing for an angle."""
        self.log().information('Direct beam {} not cached in AnalysisDataService.'.format(directBeamName))
        directBeamInput = self._composeRunString(self._db[angle_index])
        self._runReflectometryILLPreprocess(directBeamInput,
                                            directBeamName,
                                            angle_index)
        self._autoCleanup.protect(directBeamName)
        self._runReflectometryILLSumForeground(directBeamName,
                                               directForegroundName,
                                               SumType.IN_LAMBDA,
                                               angle_index)
        self._autoCleanup.protect(directForegroundName)
        if self._isPolarized():
            self._polCorDirectName = self._names.withSuffix('{}_polcor'.format(directBeamName))
            self._runReflectometryILLPolarizationCor(directForegroundName,
                                                     self._polCorDirectName)

    def _processReflectedBeam(self, reflectedInput, reflectedBeamName, directForegroundName, angle_index):
        """Combine all algorithm runs of the reflected beam processing for an angle."""
        twoTheta = self._twoTheta(angle_index)
        linePosition = self._linePosition(angle_index)
        self._runReflectometryILLPreprocess(reflectedInput,
                                            reflectedBeamName,
                                            angle_index,
                                            linePosition,
                                            twoTheta)
        foregroundName = reflectedBeamName + '_frg'
        sum_type = self._getValue(PropertyNames.SUM_TYPE, angle_index)
        sum_type = 'SumInLambda' if sum_type == 'Incoherent' else 'SumInQ'
        self._runReflectometryILLSumForeground(reflectedBeamName,
                                               foregroundName,
                                               sum_type,
                                               angle_index,
                                               directForegroundName)
        self._autoCleanup.cleanupLater(reflectedBeamName)
        self._autoCleanup.cleanupLater(foregroundName)
        return foregroundName

    def PyExec(self):
        """Execute the algorithm."""
        self._setup()
        to_group = []
        scaleFactor = self.getProperty(PropertyNames.SCALE_FACTOR).value
        progress = Progress(self, start=0.0, end=1.0, nreports=self._dimensionality)
        for angle_index in range(self._dimensionality):
            runDB = self._mtdName(self._db[angle_index])
            directBeamName = runDB + '_direct'
            directForegroundName = directBeamName + '_frg'
            if directBeamName not in mtd.getObjectNames():
                self._processDirectBeam(directBeamName, directForegroundName, angle_index)
            reflectedBeamInput = self._composeRunString(self._rb[angle_index])
            if not self._isPolarized():
                runRB = self._mtdName(self._rb[angle_index])
                reflectedBeamName = runRB + '_reflected'
                to_convert_to_q = self._processReflectedBeam(reflectedBeamInput,
                                                             reflectedBeamName,
                                                             directForegroundName,
                                                             angle_index)
            else:
                self._forPolCor = self._polCorDirectName
                for run in reflectedBeamInput:
                    reflectedPolWSName = self._names.withSuffix('reflected_{}'.format(run))
                    reflectedPolForegroundWSName = self._processReflectedBeam(run,
                                                                              reflectedPolWSName,
                                                                              directForegroundName,
                                                                              angle_index)
                    self._forPolCor.append(reflectedPolForegroundWSName)
                polCorWSName = self._names.withSuffix('reflected_polcor')
                self._runReflectometryILLPolarizationCor(','.join(self._forPolCor),
                                                         polCorWSName)
                to_convert_to_q = mtd[polCorWSName].getNames()
                for workspace in to_convert_to_q:
                    self._autoCleanup.cleanupLater(workspace)

            convertedToQName = self._outWS + '_' + str(angle_index)
            self._runReflectometryILLConvertToQ(to_convert_to_q,
                                                convertedToQName,
                                                directForegroundName,
                                                angle_index)
            if scaleFactor != 1:
                Scale(InputWorkspace=convertedToQName, OutputWorkspace=convertedToQName, Factor=scaleFactor)
            to_group.append(convertedToQName)
            self._autoCleanup.protect(convertedToQName)
            progress.report()

        try:
            stitched = self._outWS + '_stitched'
            Stitch1DMany(InputWorkspaces=to_group, OutputWorkspace=stitched)
            to_group.append(stitched)
        except RuntimeError as re:
            self.log().warning('Unable to stitch automatically, consider stitching manually: ' + re.message)

        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=self._outWS)
        self.setProperty(Prop.OUTPUT_WS, self._outWS)
        self._autoCleanup.finalCleanup()

AlgorithmFactory.subscribe(ReflectometryILLAutoProcess)
