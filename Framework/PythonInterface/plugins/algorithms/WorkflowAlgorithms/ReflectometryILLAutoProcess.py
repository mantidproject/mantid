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
import numpy


class Prop(object):
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'FlatBackgroundDirect'
    BKG_METHOD_REFLECTED = 'FlatBackgroundReflected'
    CLEANUP = 'Cleanup'
    DB = 'DirectRun'
    EFFICIENCY_FILE = 'EfficiencyFile'
    END_WS_INDEX_DIRECT = 'FitEndWorkspaceIndexDirect'
    END_WS_INDEX_REFLECTED = 'FitEndWorkspaceIndexReflected'
    END_OVERLAPS = 'EndOverlaps'
    FLUX_NORM_METHOD_DIRECT = 'FluxNormalisationDirect'
    FLUX_NORM_METHOD_REFLECTED = 'FluxNormalisationReflected'
    FOREGROUND_HALF_WIDTH_DIRECT = 'ForegroundHalfWidthDirect'
    FOREGROUND_HALF_WIDTH_REFLECTED = 'ForegroundHalfWidthReflected'
    FOREGROUND_INDICES_DIRECT = 'ForegroundDirect'
    FOREGROUND_INDICES_REFLECTED = 'ForegroundReflected'
    GROUPING_FRACTION = 'GroupingQFraction'
    HIGH_BKG_OFFSET_DIRECT = 'HighAngleBkgOffsetDirect'
    HIGH_BKG_OFFSET_REFLECTED = 'HighAngleBkgOffsetReflected'
    HIGH_BKG_WIDTH_DIRECT = 'HighAngleBkgWidthDirect'
    HIGH_BKG_WIDTH_REFLECTED = 'HighAngleBkgWidthReflected'
    LINE_POSITION = 'LinePosition'
    LOW_BKG_OFFSET_DIRECT = 'LowAngleBkgOffsetDirect'
    LOW_BKG_OFFSET_REFLECTED = 'LowAngleBkgOffsetReflected'
    LOW_BKG_WIDTH_DIRECT = 'LowAngleBkgWidthDirect'
    LOW_BKG_WIDTH_REFLECTED = 'LowAngleBkgWidthReflected'
    MANUAL_SCALE_FACTORS = 'ManualScaleFactors'
    RB = 'ReflectedRun'
    SCALE_FACTOR = 'ScaleFactor'
    SLIT_NORM = 'SlitNormalisation'
    START_WS_INDEX_DIRECT = 'FitStartWorkspaceIndexDirect'
    START_WS_INDEX_REFLECTED = 'FitStartWorkspaceIndexReflected'
    START_OVERLAPS = 'StartOverlaps'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    SUM_TYPE = 'SummationType'
    OUTPUT_WS = 'OutputWorkspace'
    TWO_THETA = 'TwoTheta'
    USE_MANUAL_SCALE_FACTORS = 'UseManualScaleFactors'
    WATER_REFERENCE = 'WaterWorkspace'
    WAVELENGTH_RANGE = 'WavelengthRange'
    XMAX_DIRECT = 'RangeUpperDirect'
    XMAX_REFLECTED = 'RangeUpperReflected'
    XMIN_DIRECT = 'RangeLowerDirect'
    XMIN_REFLECTED = 'RangeLowerReflected'


class Angle(object):
    SAN = 'Sample angle'
    DAN = 'Detector angle'


class BkgMethod(object):
    CONSTANT = 'Background Constant Fit'
    LINEAR = 'Background Linear Fit'
    OFF = 'Background OFF'


class FluxNormMethod(object):
    MONITOR = 'Normalise To Monitor'
    TIME = 'Normalise To Time'
    OFF = 'Normalisation OFF'


class SlitNorm(object):
    OFF = 'Slit Normalisation OFF'
    ON = 'Slit Normalisation ON'


class SubalgLogging(object):
    OFF = 'Logging OFF'
    ON = 'Logging ON'


class SumType(object):
    IN_LAMBDA = 'SumInLambda'
    IN_Q = 'SumInQ'


class ReflectometryILLAutoProcess(DataProcessorAlgorithm):

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algrithm."""
        return 'ILL\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLAutoReduction'

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

        self.declareProperty(
            MultipleFileProperty(
                Prop.RB,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files.')
        self.setPropertyGroup(Prop.RB, '')
        self.declareProperty(
            MultipleFileProperty(
                Prop.DB,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of direct run numbers/files.')
        self.setPropertyGroup(Prop.DB, '')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.OUTPUT_WS,
                defaultValue='',
                direction=Direction.Output
            ),
            doc='The output workspace (momentum transfer), single histogram.')
        self.setPropertyGroup(Prop.OUTPUT_WS, '')
        self.declareProperty(
            Prop.ANGLE_OPTION,
            defaultValue=Angle.DAN,
            validator=StringListValidator(
                [Angle.SAN, Angle.DAN]
            ),
            doc='Angle option used for detector positioning.'
        )
        self.setPropertyGroup(Prop.ANGLE_OPTION, '')
        preProcessGen = 'ReflectometryILLPreprocess, common properties'
        self.copyProperties(
            'ReflectometryILLPreprocess',
            [
                Prop.TWO_THETA,
                Prop.LINE_POSITION,
                Prop.SUBALG_LOGGING,
                Prop.CLEANUP,
                Prop.WATER_REFERENCE,
                Prop.SLIT_NORM
            ]
        )
        self.setPropertyGroup(Prop.TWO_THETA, preProcessGen)
        self.setPropertyGroup(Prop.LINE_POSITION, preProcessGen)
        self.setPropertyGroup(Prop.SUBALG_LOGGING, preProcessGen)
        self.setPropertyGroup(Prop.CLEANUP, preProcessGen)
        self.setPropertyGroup(Prop.WATER_REFERENCE, preProcessGen)
        self.setPropertyGroup(Prop.SLIT_NORM, preProcessGen)
        preProcessDirect = 'ReflectometryILLPreprocess for direct runs'
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_HALF_WIDTH_DIRECT,
                validator=maxTwoNonnegativeInts
            ),
            doc='Number of foreground pixels at lower and higher angles from the centre pixel.'
        )
        self.setPropertyGroup(Prop.FOREGROUND_HALF_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_INDICES_DIRECT,
                values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                validator=threeNonnegativeInts
            ),
            doc='A three element array of foreground start, centre and end workspace indices.')
        self.setPropertyGroup(Prop.FOREGROUND_INDICES_DIRECT, preProcessDirect)
        self.declareProperty(
            Prop.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.setPropertyGroup(Prop.BKG_METHOD_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.LOW_BKG_OFFSET_DIRECT,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.LOW_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.LOW_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.LOW_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.HIGH_BKG_OFFSET_DIRECT,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.HIGH_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.HIGH_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.HIGH_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            Prop.FLUX_NORM_METHOD_DIRECT,
            defaultValue=FluxNormMethod.TIME,
            validator=StringListValidator(
                [FluxNormMethod.TIME, FluxNormMethod.MONITOR, FluxNormMethod.OFF]
            ),
            doc='Neutron flux normalisation method.'
        )
        self.setPropertyGroup(Prop.FLUX_NORM_METHOD_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.START_WS_INDEX_DIRECT,
                validator=nonnegativeInts,
                values=[0],
            ),
            doc='Start histogram index used for peak fitting.'
        )
        self.setPropertyGroup(Prop.START_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                Prop.END_WS_INDEX_DIRECT,
                validator=nonnegativeInts,
                values=[255],
            ),
            doc='Last histogram index used for peak fitting.'
        )
        self.setPropertyGroup(Prop.END_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            Prop.XMIN_DIRECT,
            defaultValue=Property.EMPTY_DBL,
            doc='Minimum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(Prop.XMIN_DIRECT, preProcessDirect)
        self.declareProperty(
            Prop.XMAX_DIRECT,
            defaultValue=Property.EMPTY_DBL,
            doc='Maximum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(Prop.XMAX_DIRECT, preProcessDirect)
        preProcessReflected = 'ReflectometryILLPreprocess for reflected runs'
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_HALF_WIDTH_REFLECTED,
                validator=maxTwoNonnegativeInts
            ),
            doc='Number of foreground pixels at lower and higher angles from the centre pixel.'
        )
        self.setPropertyGroup(Prop.FOREGROUND_HALF_WIDTH_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_INDICES_REFLECTED,
                values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                validator=threeNonnegativeInts
            ),
            doc='A three element array of foreground start, centre and end workspace indices.'
        )
        self.setPropertyGroup(Prop.FOREGROUND_INDICES_REFLECTED, preProcessReflected)
        self.declareProperty(
            Prop.BKG_METHOD_REFLECTED,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.setPropertyGroup(Prop.BKG_METHOD_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.LOW_BKG_OFFSET_REFLECTED,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.LOW_BKG_OFFSET_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.LOW_BKG_WIDTH_REFLECTED,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.LOW_BKG_WIDTH_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.HIGH_BKG_OFFSET_REFLECTED,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.HIGH_BKG_OFFSET_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.HIGH_BKG_WIDTH_REFLECTED,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(Prop.HIGH_BKG_WIDTH_REFLECTED, preProcessReflected)
        self.declareProperty(
            Prop.FLUX_NORM_METHOD_REFLECTED,
            defaultValue=FluxNormMethod.TIME,
            validator=StringListValidator(
                [FluxNormMethod.TIME, FluxNormMethod.MONITOR, FluxNormMethod.OFF]
            ),
            doc='Neutron flux normalisation method.'
        )
        self.setPropertyGroup(Prop.FLUX_NORM_METHOD_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.START_WS_INDEX_REFLECTED,
                validator=nonnegativeInts,
                values=[0],
            ),
            doc='Start histogram index used for peak fitting.'
        )
        self.setPropertyGroup(Prop.START_WS_INDEX_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                Prop.END_WS_INDEX_REFLECTED,
                validator=nonnegativeInts,
                values=[255],
            ),
            doc='Last histogram index used for peak fitting.'
        )
        self.setPropertyGroup(Prop.END_WS_INDEX_REFLECTED, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                Prop.XMIN_REFLECTED,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Minimum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(Prop.XMIN_REFLECTED, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                Prop.XMAX_REFLECTED,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Maximum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(Prop.XMAX_REFLECTED, preProcessReflected)
        # For ReflectometryILLSumForeground
        self.copyProperties(
            'ReflectometryILLSumForeground',
            [
                Prop.SUM_TYPE,
                Prop.WAVELENGTH_RANGE
            ]
        )
        # For ReflectometryILLPolarizationCorr -> action is now OptionalLoad
        self.declareProperty(
            FileProperty(
                Prop.EFFICIENCY_FILE,
                defaultValue='',
                action=FileAction.OptionalLoad
            ),
            doc='A file containing the polarization efficiency factors.'
        )
        # For ReflectometryILLConvertToQ
        self.declareProperty(
            FloatArrayProperty(
                Prop.GROUPING_FRACTION,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='If set, group the output by steps of this fraction multiplied by Q resolution'
        )
        # For Stitch1DMany
        self.copyProperties(
            'Stitch1DMany',
            [
                Prop.START_OVERLAPS,
                Prop.END_OVERLAPS,
                Prop.USE_MANUAL_SCALE_FACTORS,
                Prop.MANUAL_SCALE_FACTORS
            ]
        )
        # Final scale factor
        self.declareProperty(
            Prop.SCALE_FACTOR,
            defaultValue=1.0,
            doc='Scale factor.'
        )

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        numberDirectRuns = len(self.getProperty(Prop.DB).value)
        numberReflectedRuns = len(self.getProperty(Prop.RB).value)
        if numberDirectRuns == 0 and numberReflectedRuns == 0:
            issues[Prop.RB] = "Nothing to do."
        if numberDirectRuns != numberReflectedRuns:
            issues[Prop.RB] = "The same number of direct runs and reflected runs must be given."
        return issues

    def getValue(self, propertyName, angle):
        value = self.getProperty(propertyName).value
        if not self.getProperty(propertyName).isDefault:
            return value[angle]
        else:
            return value[0]

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getPropertyValue(Prop.SUBALG_LOGGING)
        self._cleanup = self.getPropertyValue(Prop.CLEANUP)
        self.wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)

        angleOption = self.getPropertyValue(Prop.ANGLE_OPTION)

        rb = self.getProperty(Prop.RB).value
        db = self.getProperty(Prop.DB).value

        self._progress = Progress(self, start=0.0, end=1.0, nreports=len(rb))

        # Foreground reflected beam
        self.foregroundReflected = self.getProperty(Prop.FOREGROUND_INDICES_REFLECTED).value
        self.halfWidthsReflected = self.getProperty(Prop.FOREGROUND_HALF_WIDTH_REFLECTED).value

        self.wavelengthRange = self.getProperty(Prop.WAVELENGTH_RANGE).value
        self.polarizationEffFile = self.getProperty(Prop.EFFICIENCY_FILE).value
        self.twoTheta = self.getProperty(Prop.TWO_THETA).value
        self.linePosition = self.getProperty(Prop.LINE_POSITION).value
        self._sumType = self.getProperty(Prop.SUM_TYPE).value
        self.slitNorm = self.getProperty(Prop.SLIT_NORM).value
        if self.polarizationEffFile == "":
            self.isPolarized = False
        else:
            self.isPolarized = True
        self.scaling = self.getProperty(Prop.SCALE_FACTOR).value
        toStitch = []

        for angle in range(len(rb)):
            if angleOption == Angle.SAN and self.getProperty(Prop.TWO_THETA).isDefault:
                import h5py
                with h5py.File(rb[angle], "r") as nexus:
                    # Need to check whether the unit is degree
                    try:
                        san = numpy.array(nexus.get('entry0/instrument/SAN'), dtype='float')
                    except:
                        try:
                            san = numpy.array(nexus.get('entry0/instrument/san'), dtype='float')
                        except:
                            raise RuntimeError('Cannot find sample angle entry in file {}.'.format(rb[angle]))
                    self.twoTheta = 2. * san

            runDB = format(db[angle][-10:-4])
            runRB = format(rb[angle][-10:-4])
            # Direct beam already in ADS?
            workspaces = mtd.getObjectNames()
            if not 'direct-{}'.format(db[angle]) in workspaces:
                # Direct beam pre-processing
                ReflectometryILLPreprocess(
                    Run=db[angle],
                    LinePosition=self.linePosition,
                    TwoTheta=self.twoTheta,
                    OutputWorkspace='direct-{}'.format(runDB),
                    ForegroundHalfWidth=self.getProperty(Prop.FOREGROUND_HALF_WIDTH_DIRECT).value,
                    SlitNormalisation=self.slitNorm,
                    FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD_DIRECT).value,
                    LowAngleBkgOffset=self.getValue(Prop.LOW_BKG_OFFSET_DIRECT, angle),
                    LowAngleBkgWidth=self.getValue(Prop.LOW_BKG_WIDTH_DIRECT, angle),
                    HighAngleBkgOffset=self.getValue(Prop.HIGH_BKG_OFFSET_DIRECT, angle),
                    HighAngleBkgWidth=self.getValue(Prop.HIGH_BKG_WIDTH_DIRECT, angle),
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
                # Direct sum foreground
                ReflectometryILLSumForeground(
                    InputWorkspace='direct-{}'.format(runDB),
                    OutputWorkspace='direct-{}-foreground'.format(runDB),
                    Foreground=self.getProperty(Prop.FOREGROUND_INDICES_DIRECT).value,
                    SummationType=self._sumType,
                    WavelengthRange=self.wavelengthRange,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
                # Direct beam polarization correction
                if self.isPolarized:
                    ReflectometryILLPolarizationCor(
                        InputWorkspaces='direct-{}-foreground'.format(runDB),
                        OutputWorkspace='direct-{}-polcor'.format(runDB),
                        EfficiencyFile=self.polarizationEffFile,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup,
                    )
            # Reflected beam
            self._workspaceNamesForQConversion = ''
            if not self.isPolarized:
                ReflectometryILLPreprocess(
                    Run=rb[angle],
                    OutputWorkspace='reflected-{}'.format(runRB),
                    LinePosition=self.linePosition,
                    TwoTheta=self.twoTheta,
                    DirectLineWorkspace='direct-{}'.format(runDB),
                    ForegroundHalfWidth=self.halfWidthsReflected,
                    SlitNormalisation=self.slitNorm,
                    FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD_REFLECTED).value,
                    LowAngleBkgOffset=self.getValue(Prop.LOW_BKG_OFFSET_REFLECTED, angle),
                    LowAngleBkgWidth=self.getValue(Prop.LOW_BKG_WIDTH_REFLECTED, angle),
                    HighAngleBkgOffset=self.getValue(Prop.HIGH_BKG_OFFSET_REFLECTED, angle),
                    HighAngleBkgWidth=self.getValue(Prop.HIGH_BKG_WIDTH_REFLECTED, angle),
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
                # Reflected sum foreground
                self._workspaceNamesForQConversion = 'reflected-{}-foreground'.format(runRB)
                ReflectometryILLSumForeground(
                    InputWorkspace='reflected-{}'.format(runRB),
                    OutputWorkspace=self._workspaceNamesForQConversion,
                    Foreground=self.foregroundReflected,
                    SummationType=self._sumType,
                    DirectForegroundWorkspace='direct-{}-foreground'.format(runDB),
                    DirectLineWorkspace='direct-{}'.format(runDB),
                    WavelengthRange=self.wavelengthRange,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
            else:
                for run in rb[angle]:
                    ReflectometryILLPreprocess(
                        Run=run,
                        LinePosition=self.linePosition,
                        TwoTheta=self.twoTheta,
                        DirectLineWorkspace='direct-{}'.format(runDB),
                        ForegroundHalfWidth=self.halfWidthsReflected,
                        SlitNormalisation=self.slitNorm,
                        LowAngleBkgOffset=self.lowOffsetReflected,
                        LowAngleBkgWidth=self.lowWidthReflected,
                        HighAngleBkgOffset=self.highOffsetReflected,
                        HighAngleBkgWidth=self.highWidthsReflected,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup,
                    )
                    # Reflected sum foreground
                    ReflectometryILLSumForeground(
                        InputWorkspace='{}'.format(run),
                        OutputWorkspace='reflected-{}-foreground'.format(run),
                        Foreground=self.foregroundReflected,
                        SummationType=self._sumType,
                        DirectForegroundWorkspace='direct-{}-foreground'.format(runDB),
                        DirectLineWorkspace='direct-{}'.format(runDB),
                        WavelengthRange=self.wavelengthRange,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup,
                    )
                # Reflected polarization correction
                inputWorkspaces = 'reflected-{}-foreground'.format(runRB)
                if len(rb[angle]) > 1:
                    for r in self._reflecteds:
                        inputWorkspaces = ',{}'.join(rb)
                        inputWorkspaces += ',' + 'reflected-{}-foreground'.format(r)
                ReflectometryILLPolarizationCor(
                    InputWorkspaces=inputWorkspaces,
                    OutputWorkspace='reflected-polcor'.format(),
                    EfficiencyFile=self.polarizationEffFile,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
                polCorGroup = mtd['reflected-polcor']
                self._workspaceNamesForQConversion = polCorGroup.getNames()
            # Conversion to Q
            outWS = '{}-{}-reflectivity'.format(self.wsPrefix, runRB)
            ReflectometryILLConvertToQ(
                InputWorkspace=self._workspaceNamesForQConversion,
                OutputWorkspace=outWS,
                DirectForegroundWorkspace='direct-{}-foreground'.format(runDB),
                GroupingQFraction=self.getValue(Prop.GROUPING_FRACTION, angle),
                SubalgorithmLogging=self._subalgLogging,
                Cleanup=self._cleanup,
            )
            toStitch.append(outWS)
            self._progress.report()

        if len(rb) > 1:
            ','.join(toStitch)
            Stitch1DMany(
                InputWorkspaces=toStitch,
                OutputWorkspace='{}'.format(self.wsPrefix),
                StartOverlaps=self.getProperty(Prop.START_OVERLAPS).value,
                EndOverlaps=self.getProperty(Prop.END_OVERLAPS).value,
                UseManualScaleFactors=self.getProperty(Prop.USE_MANUAL_SCALE_FACTORS).value,
                ManualScaleFactors=self.getProperty(Prop.MANUAL_SCALE_FACTORS).value,
            )
        else:
            RenameWorkspace(
                InputWorkspace='{}'.format(toStitch[0]),
                OutputWorkspace='{}'.format(self.wsPrefix),
            )
        if self.scaling != 1.:
            Scale(
                InputWorkspace='{}'.format(self.wsPrefix),
                OutputWorkspace='{}'.format(self.wsPrefix),
                Factor=self.scaling,
            )
        self.setProperty(Prop.OUTPUT_WS, self.wsPrefix)

AlgorithmFactory.subscribe(ReflectometryILLAutoProcess)
