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
    PropertyMode,
    WorkspaceUnitValidator
)
from mantid.kernel import (
    CompositeValidator,
    Direction,
    FloatArrayProperty,
    FloatArrayBoundedValidator,
    IntArrayLengthValidator,
    IntArrayBoundedValidator,
    IntArrayProperty,
    IntBoundedValidator,
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
import h5py
import ReflectometryILL_common as common
import re


class Prop(object):
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'FlatBackgroundDirect'
    BKG_METHOD_REFLECTED = 'FlatBackgroundReflected'
    CLEANUP = 'Cleanup'
    DB = 'DirectRun'
    EFFICIENCY_FILE = 'EfficiencyFile'
    END_WS_INDEX = 'FitEndWorkspaceIndex'
    END_OVERLAPS = 'EndOverlaps'
    FLUX_NORM_METHOD = 'FluxNormalisation'
    FLUX_NORM_METHOD_DIRECT = 'FluxNormalisationDirect'
    FOREGROUND_HALF_WIDTH_DIRECT = 'ForegroundHalfWidthDirect'
    FOREGROUND_HALF_WIDTH_REFLECTED = 'ForegroundHalfWidthReflected'
    FOREGROUND_INDICES_DIRECT = 'ForegroundDirect'
    FOREGROUND_INDICES_REFLECTED = 'ForegroundReflected'
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
    SLIT_NORM = 'SlitNormalisation'
    START_WS_INDEX = 'FitStartWorkspaceIndex'
    START_OVERLAPS = 'StartOverlaps'
    SUBALG_LOGGING = 'SubalgorithmLogging'
    OUTPUT_WS = 'OutputWorkspace'
    TWO_THETA = 'TwoTheta'
    USE_MANUAL_SCALE_FACTORS = 'UseManualScaleFactors'
    WATER_REFERENCE = 'WaterWorkspace'
    WAVELENGTH_RANGE = 'WavelengthRange'
    XMAX = 'RangeUpper'
    XMIN = 'RangeLower'


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


class ReflectometryILLAutoReduction(DataProcessorAlgorithm):

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
        nonnegativeInt = IntBoundedValidator(lower=0)
        wsIndexRange = IntBoundedValidator(lower=0, upper=255)
        nonnegativeIntArray = IntArrayBoundedValidator()
        nonnegativeIntArray.setLower(0)
        maxTwoNonnegativeInts = CompositeValidator()
        maxTwoNonnegativeInts.add(IntArrayLengthValidator(lenmin=0, lenmax=2))
        maxTwoNonnegativeInts.add(nonnegativeIntArray)
        threeNonnegativeInts = CompositeValidator()
        threeNonnegativeInts.add(IntArrayLengthValidator(3))
        nonnegativeInts = IntArrayBoundedValidator()
        nonnegativeInts.setLower(0)
        threeNonnegativeInts.add(nonnegativeInts)
        nonnegativeFloatArray = FloatArrayBoundedValidator()
        nonnegativeFloatArray.setLower(0.)
        # Purely ReflectometryILLAutoReduction
        self.declareProperty(
            MultipleFileProperty(
                Prop.RB,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files.')
        self.declareProperty(
            MultipleFileProperty(
                Prop.DB,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of direct run numbers/files.')
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.OUTPUT_WS,
                defaultValue='',
                direction=Direction.Output
            ),
            doc='The output workspace (momentum transfer), single histogram.')
        self.declareProperty(
            Prop.ANGLE_OPTION,
            defaultValue=Angle.SAN,
            validator=StringListValidator(
                [Angle.SAN, Angle.DAN]
            ),
            doc='Angle option used for detector positioning.'
        )
        # For ReflectometryILLPreprocess
        self.declareProperty(
            Prop.TWO_THETA,
            defaultValue=Property.EMPTY_DBL,
            doc='A user-defined scattering angle 2 theta (unit degrees).'
        )
        self.declareProperty(
            name=Prop.LINE_POSITION,
            defaultValue=Property.EMPTY_DBL,
            doc='A workspace index corresponding to the beam centre between 0.0 and 255.0.'
        )
        self.declareProperty(
            Prop.SUBALG_LOGGING,
            defaultValue=SubalgLogging.OFF,
            validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
            doc='Enable or disable child algorithm logging.'
        )
        self.declareProperty(
            Prop.CLEANUP,
            defaultValue=common.WSCleanup.ON,
            validator=StringListValidator(
                [common.WSCleanup.ON, common.WSCleanup.OFF]
            ),
            doc='Enable or disable intermediate workspace cleanup.'
        )
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.WATER_REFERENCE,
                defaultValue='',
                direction=Direction.Input,
                validator=WorkspaceUnitValidator("TOF"),
                optional=PropertyMode.Optional
            ),
            doc='A (water) calibration workspace (unit TOF).')
        self.declareProperty(
            Prop.SLIT_NORM,
            defaultValue=SlitNorm.OFF,
            validator=StringListValidator([SlitNorm.OFF, SlitNorm.ON]),
            doc='Enable or disable slit normalisation.'
        )
        self.declareProperty(
            Prop.FLUX_NORM_METHOD,
            defaultValue=FluxNormMethod.TIME,
            validator=StringListValidator(
                [FluxNormMethod.TIME,FluxNormMethod.MONITOR,FluxNormMethod.OFF]
            ),
            doc='Neutron flux normalisation method.'
        )
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_HALF_WIDTH_DIRECT,
                validator=maxTwoNonnegativeInts
            ),
            doc='Number of foreground pixels at lower and higher angles from the centre pixel.'
        )
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_INDICES_DIRECT,
                values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                validator=threeNonnegativeInts
            ),
            doc='A three element array of foreground start, centre and end workspace indices.')
        self.declareProperty(
            Prop.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT,BkgMethod.LINEAR,BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.declareProperty(
            Prop.LOW_BKG_OFFSET_DIRECT,
            defaultValue=7,
            validator=nonnegativeInt,
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.LOW_BKG_WIDTH_DIRECT,
            defaultValue=5,
            validator=nonnegativeInt,
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.HIGH_BKG_OFFSET_DIRECT,
            defaultValue=7,
            validator=nonnegativeInt,
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.HIGH_BKG_WIDTH_DIRECT,
            defaultValue=5,
            validator=nonnegativeInt,
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.FLUX_NORM_METHOD_DIRECT,
            defaultValue=FluxNormMethod.TIME,
            validator=StringListValidator(
                [FluxNormMethod.TIME,FluxNormMethod.MONITOR,FluxNormMethod.OFF]
            ),
            doc='Neutron flux normalisation method.'
        )
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_HALF_WIDTH_REFLECTED,
                validator=maxTwoNonnegativeInts
            ),
            doc='Number of foreground pixels at lower and higher angles from the centre pixel.'
        )
        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_INDICES_REFLECTED,
                values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                validator=threeNonnegativeInts
            ),
            doc='A three element array of foreground start, centre and end workspace indices.'
        )
        self.declareProperty(
            Prop.BKG_METHOD_REFLECTED,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT,BkgMethod.LINEAR,BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.declareProperty(
            Prop.LOW_BKG_OFFSET_REFLECTED,
            defaultValue=7,
            validator=nonnegativeInt,
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.LOW_BKG_WIDTH_REFLECTED,
            defaultValue=5,
            validator=nonnegativeInt,
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.HIGH_BKG_OFFSET_REFLECTED,
            defaultValue=7,
            validator=nonnegativeInt,
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.declareProperty(
            Prop.HIGH_BKG_WIDTH_REFLECTED,
            defaultValue=5,
            validator=nonnegativeInt,
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        # For ReflectometryILLPreprocess (FindReflectometryLines)
        self.declareProperty(
            Prop.START_WS_INDEX,
            validator=wsIndexRange,
            defaultValue=0,
            doc='Start histogram index used for peak fitting.'
        )
        self.declareProperty(
            Prop.END_WS_INDEX,
            validator=wsIndexRange,
            defaultValue=255,
            doc='Last histogram index used for peak fitting.'
        )
        self.declareProperty(
            Prop.XMIN,
            defaultValue=Property.EMPTY_DBL,
            doc='Minimum x value (unit wavelength) used for peak fitting.'
        )
        self.declareProperty(
            Prop.XMAX,
            defaultValue=Property.EMPTY_DBL,
            doc='Maximum x value (unit wavelength) used for peak fitting.'
        )
        # For ReflectometryILLSumForeground
        self.declareProperty(
            FloatArrayProperty(
                Prop.WAVELENGTH_RANGE,
                values=[0.],
                validator=nonnegativeFloatArray
            ),
            doc='The wavelength bounds.'
        )
        # For ReflectometryILLPolarisationCorr
        self.declareProperty(
            FileProperty(
                Prop.EFFICIENCY_FILE,
                defaultValue='',
                action=FileAction.Load
            ),
            doc='A file containing the polarization efficiency factors.'
        )
        # For Stitch1DMany
        self.declareProperty(
            IntArrayProperty(
                Prop.START_OVERLAPS,
                defaultValue=Property.EMPTY_INT
            ),
            doc='Start overlaps for stitched workspaces (number of input workspaces minus one).'
        )
        self.declareProperty(
            IntArrayProperty(
                Prop.END_OVERLAPS,
                defaultValue=Property.EMPTY_INT
            ),
            doc='End overlaps for stitched workspaces (number of input workspaces minus one).'
        )
        self.declareProperty(
            Prop.USE_MANUAL_SCALE_FACTORS,
            defaultValue=False,
            doc='True to use provided values for the scale factor.'
        )
        self.declareProperty(
            Prop.MANUAL_SCALE_FACTORS,
            defaultValue=Property.EMPTY_DBL,
            doc='Either a single scale factor which will be applied to all input workspaces or ' +
                'individual scale factors (number of input workspaces minus one).'
        )

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanupMode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = common.WSCleanup(cleanupMode, self._subalgLogging)
        self.wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = common.WSNameSource(self.wsPrefix, cleanupMode)

        #angleOption = self.getProperty(Prop.ANGLE_OPTION).value
        #if san: read angle from NeXus file, multiply by two and use as TWO_THETA input -> new NeXus, same name
        # with h5py.File(Prop., "r") as f:

        rb = self.getProperty(Prop.RB).value
        db = self.getProperty(Prop.DB).value

        # Background direct beam
        self.lowWidthDirect = self.getProperty(Prop.LOW_BKG_WIDTH_DIRECT)
        self.lowOffsetDirect = self.getProperty(Prop.LOW_BKG_OFFSET_DIRECT)
        self.highWidthsDirect = self.getProperty(Prop.HIGH_BKG_WIDTH_DIRECT)
        self.highOffsetDirect = self.getProperty(Prop.HIGH_BKG_OFFSET_DIRECT)
        # Background reflected beam
        self.lowWidthReflected = self.getProperty(Prop.LOW_BKG_WIDTH_REFLECTED)
        self.lowOffsetReflected = self.getProperty(Prop.LOW_BKG_OFFSET_REFLECTED)
        self.highWidthsReflected = self.getProperty(Prop.HIGH_BKG_WIDTH_REFLECTED)
        self.highOffsetReflected = self.getProperty(Prop.HIGH_BKG_OFFSET_REFLECTED)
        # Foreground direct beam
        self.foregroundDirect = self.getProperty(Prop.FOREGROUND_INDICES_DIRECT)
        # Foreground reflected beam
        self.foregroundReflected = self.getProperty(Prop.FOREGROUND_INDICES_REFLECTED)
        self.halfWidthsDirect = self.getProperty(Prop.FOREGROUND_HALF_WIDTH_DIRECT)
        self.halfWidthsReflected = self.getProperty(Prop.FOREGROUND_HALF_WIDTH_REFLECTED)

        self.wavelengthRange = self.getProperty(Prop.WAVELENGTH_RANGE)

        self.polarizationEffFile = self.getProperty(Prop.EFFICIENCY_FILE)

        for angle in range(len(rb)):
            # Direct beam already in ADS?
            workspaces = mtd.getObjectNames()
            if not 'direct-{}'.format(db[angle]) in workspaces:
                # Direct beam pre-processing
                directBeam = ReflectometryILLPreprocess(
                    Run=db[angle],
                    LinePosition=self.getProperty(Prop.LINE_POSITION),
                    TwoTheta=self.getProperty(Prop.TWO_THETA),
                    OutputWorkspace='direct-{}'.format(db[angle]),
                    ForegroundHalfWidth=self.halfWidthsDirect,
                    SlitNormalisation=SlitNorm.ON,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                    LowAngleBkgOffset=self.lowOffsetDirect,
                    LowAngleBkgWidth=self.lowWidthDirect,
                    HighAngleBkgOffset=self.highOffsetDirect,
                    HighAngleBkgWidth=self.highWidthsDirect
                )
                # Get Bragg angle and beam centre for reflected beam pre-processing
                twoTheta = directBeam.run().getProperty(common.TWO_THETA).value
                linePosition = directBeam.run().getProperty(common.LINE_POSITION).value
                # Direct sum foreground
                ReflectometryILLSumForeground(
                    InputWorkspace='direct-{}'.format(db[angle]),
                    OutputWorkspace='direct-{}-foreground'.format(db[angle]),
                    Foreground=self.foregroundDirect,
                    SummationType=self._sumType,
                    WavelengthRange=self.wavelengthRange,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup
                )
                # Direct beam polarization correction
                if self.isPolarized():
                    ReflectometryILLPolarizationCor(
                        InputWorkspaces='direct-{}-foreground'.format(db[angle]),
                        OutputWorkspace='direct-{}-polcor'.format(self._direct),
                        EfficiencyFile=self.polarizationEffFile,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup
                    )
            # Reflected beam
            self._workspaceNamesForQConversion = ''
            if not self.isPolarized():
                self.rbName = rb[0]
                for i in range(1, len(rb)):
                    self.rbName = '+'.join(rb[i])
                    ReflectometryILLPreprocess(
                        Run=rb,
                        OutputWorkspace='{}'.format(self.rbName),
                        LinePostion=linePosition,
                        TwoTheta=twoTheta,
                        ForegroundHalfWidth=self.halfWidthsReflected,
                        SlitNormalisation=SlitNorm.ON,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup,
                        LowAngleBkgOffset=self.lowOffsetReflected,
                        LowAngleBkgWidth=self.lowWidthReflected,
                        HighAngleBkgOffset=self.highOffsetReflected,
                        HighAngleBkgWidth=self.highWidthsReflected
                    )
                # Reflected sum foreground
                self._workspaceNamesForQConversion = 'reflected-{}-foreground'.format(self.rbName)
                ReflectometryILLSumForeground(
                    InputWorkspace='{}'.format(self.rbName),
                    OutputWorkspace=self._workspaceNamesForQConversion,
                    Foreground=self.foregroundReflected,
                    SummationType=self._sumType,
                    DirectForegroundWorkspace='direct-{}-foreground'.format(db[angle]),
                    DirectLineWorkspace='direct-{}'.format(db[angle]),
                    WavelengthRange=self.wavelengthRange,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup
                )
            else:
                for run in rb[angle]:
                    ReflectometryILLPreprocess(
                        Run=run,
                        OutputWorkspace='{}'.format(run),
                        LinePosition=linePosition,
                        TwoTheta=twoTheta,
                        ForegroundHalfWidth=self.halfWidthsReflected,
                        SlitNormalisation=SlitNorm.ON,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup,
                        LowAngleBkgOffset=self.lowOffsetReflected,
                        LowAngleBkgWidth=self.lowWidthReflected,
                        HighAngleBkgOffset=self.highOffsetReflected,
                        HighAngleBkgWidth=self.highWidthsReflected
                    )
                    # Reflected sum foreground
                    ReflectometryILLSumForeground(
                        InputWorkspace='{}'.format(run),
                        OutputWorkspace='reflected-{}-foreground'.format(run),
                        Foreground=self.foregroundReflected,
                        SummationType=self._sumType,
                        DirectForegroundWorkspace='direct-{}-foreground'.format(db[angle]),
                        DirectLineWorkspace='direct-{}'.format(db[angle]),
                        WavelengthRange=self.wavelengthRange,
                        SubalgorithmLogging=self._subalgLogging,
                        Cleanup=self._cleanup
                    )
                # Reflected polarization correction
                inputWorkspaces = 'reflected-{}-foreground'.format(rb[0])
                if len(rb[angle]) > 1:
                    for r in self._reflecteds:
                        inputWorkspaces = ',{}'.join(rb)
                        inputWorkspaces += ',' + 'reflected-{}-foreground'.format(r)
                ReflectometryILLPolarizationCor(
                    InputWorkspaces=inputWorkspaces,
                    OutputWorkspace='reflected-polcor'.format(),
                    EfficiencyFile=self.polarizationEffFile,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup
                )
                polCorGroup = mtd['reflected-polcor']
                self._workspaceNamesForQConversion = polCorGroup.getNames()
            # Conversion to Q
            for wsName in self._workspaceNamesForQConversion:
                ReflectometryILLConvertToQ(
                    InputWorkspace=wsName,
                    OutputWorkspace='{}-reflectivity'.format(wsName),
                    DirectForegroundWorkspace='direct-{}-foreground'.format(self._direct),
                    GroupingQFraction=self._groupingQFraction,
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self.cleanup
                )
        # If more than one angle, stitch them
        argsStitch = {'OutputWorkspace': '{}'.format(self.wsPrefix)}
        # List all {}-reflectivity workspaces
        toStitch = [name for name in mtd.getObjectNames() if re.findall('-reflectivity', -13, -1)]
        inputWorkspaces = ''
        separator = ''
        suffix = ''
        for names in toStitch:
            for name in names:
                inputWorkspaces += separator + name
                separator = ','
                suffix = [suffix in ['_--', '_-+', '_+-', '_++'] if suffix in name else '']
        if len(toStitch) > 1:
            argsStitch.update({'InputWorkspaces': inputWorkspaces})
            argsStitch.update({Prop.START_OVERLAPS: self.getProperty(Prop.START_OVERLAPS).value})
            argsStitch.update({Prop.END_OVERLAPS: self.getProperty(Prop.END_OVERLAPS).value})
            argsStitch.update({Prop.USE_MANUAL_SCALE_FACTOR: self.getProperty(Prop.USE_MANUAL_SCALE_FACTOR).value})
            argsStitch.update({Prop.MANUAL_SCALE_FACTOR: self.getProperty(Prop.MANUAL_SCALE_FACTOR)})
            Stitch1DMany(**argsStitch)
        else:
            RenameWorkspace(
                InputWorkspace='{}'.format(toStitch[0]),
                OutputWorkspace='{}'.format(self.wsPrefix) + suffix
            )
        if self.scaling != 1.:
            Scale(
                InputWorkspace='{}'.format(self.wsPrefix) + suffix,
                OutputWorkspace='{}'.format(self.wsPrefix) + suffix,
                Factor=self.scaling
            )
        self._finalize(ws)

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        if len(self.getProperty(Prop.DB).value) != len(self.getProperty(Prop.RB).value):
            issues[Prop.RB] = "The number of reflected runs must be equal to the number of direct runs."
        return issues

AlgorithmFactory.subscribe(ReflectometryILLAutoReduction)
