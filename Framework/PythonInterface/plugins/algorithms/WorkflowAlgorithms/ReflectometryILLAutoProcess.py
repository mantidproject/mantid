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
    AnalysisDataService,
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
import ReflectometryILL_common as common
from ReflectometryILLPreprocess import BkgMethod, Prop
from ReflectometryILLSumForeground import SumType
import numpy


class PropAutoProcess(object):
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'DirectFlatBackground'
    BRAGG_ANGLE = 'BraggAngle'
    DB = 'DirectRun'
    EFFICIENCY_FILE = 'EfficiencyFile'
    END_WS_INDEX_DIRECT = 'DirectFitEndWorkspaceIndex'
    END_WS_INDEX_REFLECTED = 'FitEndWorkspaceIndex'
    END_OVERLAPS = 'EndOverlaps'
    HIGH_FOREGROUND_HALF_WIDTH = 'HighAngleForegroundHalfWidth'
    HIGH_FOREGROUND_HALF_WIDTH_DIRECT = 'DirectHighAngleForegroundHalfWidth'
    GROUPING_FRACTION = 'GroupingQFraction'
    HIGH_BKG_OFFSET_DIRECT = 'DirectHighAngleBkgOffset'
    HIGH_BKG_OFFSET_REFLECTED = 'HighAngleBkgOffset'
    HIGH_BKG_WIDTH_DIRECT = 'DirectHighAngleBkgWidth'
    HIGH_BKG_WIDTH_REFLECTED = 'HighAngleBkgWidth'
    LOW_BKG_OFFSET_DIRECT = 'DirectLowAngleBkgOffset'
    LOW_BKG_OFFSET_REFLECTED = 'LowAngleBkgOffset'
    LOW_BKG_WIDTH_DIRECT = 'DirectLowAngleBkgWidth'
    LOW_BKG_WIDTH_REFLECTED = 'LowAngleBkgWidth'
    LOW_FOREGROUND_HALF_WIDTH = 'LowAngleForegroundHalfWidth'
    LOW_FOREGROUND_HALF_WIDTH_DIRECT = 'DirectLowAngleForegroundHalfWidth'
    MANUAL_SCALE_FACTORS = 'ManualScaleFactors'
    RB = 'Run'
    SCALE_FACTOR = 'ScaleFactor'
    START_WS_INDEX_DIRECT = 'DirectFitStartWorkspaceIndex'
    START_WS_INDEX_REFLECTED = 'FitStartWorkspaceIndex'
    START_OVERLAPS = 'StartOverlaps'
    SUM_TYPE = 'SummationType'
    USE_MANUAL_SCALE_FACTORS = 'UseManualScaleFactors'
    WAVELENGTH_UPPER = 'WavelengthUpper'
    WAVELENGTH_LOWER = 'WavelengthLower'
    XMAX_DIRECT = 'DirectFitWavelengthUpper'
    XMAX_REFLECTED = 'FitWavelengthUpper'
    XMIN_DIRECT = 'DirectFitWavelengthLower'
    XMIN_REFLECTED = 'FitWavelengthLower'


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
                PropAutoProcess.BRAGG_ANGLE,
                values=[Property.EMPTY_DBL]
            ),
            doc='A user-defined Bragg angle in degree{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.BRAGG_ANGLE, preProcessGen)
        self.copyProperties(
            'ReflectometryILLPreprocess',
            [
                Prop.LINE_POSITION,
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
                PropAutoProcess.LOW_BKG_OFFSET_REFLECTED,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropAutoProcess.LOW_BKG_OFFSET_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.LOW_BKG_WIDTH_REFLECTED,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.LOW_BKG_WIDTH_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_BKG_OFFSET_REFLECTED,
                values=[7],
                validator=nonnegativeInts,
            ),
            doc='Distance of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_BKG_OFFSET_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.HIGH_BKG_WIDTH_REFLECTED,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the ' +
                'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropAutoProcess.HIGH_BKG_WIDTH_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.START_WS_INDEX_REFLECTED,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.START_WS_INDEX_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.END_WS_INDEX_REFLECTED,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.END_WS_INDEX_REFLECTED, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.XMIN_REFLECTED,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Minimum x value (unit wavelength) used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.XMIN_REFLECTED, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropAutoProcess.XMAX_REFLECTED,
                values=[Property.EMPTY_DBL],
                validator=nonnegativeFloatArray,
            ),
            doc='Maximum x value (unit wavelength) used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.XMAX_REFLECTED, preProcessReflected)
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

    def getValue(self, propertyName, angle):
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

    def twoThetaFromSampleAngle(self, run):
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

    def mtdName(self, runName):
        """Return a name suitable to put in the ADS"""
        nameForADS = ''
        if not isinstance(runName, list):
            return runName[-10:-4]
        else:
            for name in runName:
                nameForADS += name[-10:-4]
        return nameForADS

    def angleOrMergeRuns(self, run):
        """Return the string that will be passed to load the files, i.e. determine if angle is treated or runs get
        merged."""
        beamInput = run
        if isinstance(run, list):
            return '+'.join(beamInput)
        return beamInput

    def twoTheta(self, run, angle):
        """Return the TwoTheta scattering angle depending on user input options."""
        if numpy.isclose(self.getValue(PropAutoProcess.BRAGG_ANGLE, angle), Property.EMPTY_DBL):
            if self.getValue(PropAutoProcess.ANGLE_OPTION, angle) == Angle.DAN:
                self.log().notice('Using DAN angle')
                return Property.EMPTY_DBL
            elif self.getValue(PropAutoProcess.ANGLE_OPTION, angle) == Angle.SAN:
                twoT = self.twoThetaFromSampleAngle(run)
                self.log().notice('Using SAN angle: {} degree'.format(twoT / 2.))
                return twoT
            else:
                raise RuntimeError('{} must be {} or {}.'.format(PropAutoProcess.ANGLE_OPTION, Angle.SAN, Angle.DAN))
        else:
            twoT = 2. * self.getValue(PropAutoProcess.BRAGG_ANGLE, angle)
            self.log().notice('Using Bragg angle : {} degree'.format(twoT / 2.))
            return twoT

    def finalize(self):
        """Remove all intermediate workspace, if required."""
        if self.getPropertyValue(Prop.CLEANUP) == common.WSCleanup.ON:
            for workspace in mtd.getObjectNames():
                if '__' in workspace:
                    AnalysisDataService.remove(workspace)

    def isPolarized(self):
        """Return True, if a polarization file is given and False otherwise."""
        if self.getProperty(PropAutoProcess.EFFICIENCY_FILE).value == "":
            return False
        else:
            return True

    def PyExec(self):
        """Execute the algorithm."""
        subalgLogging = self.getPropertyValue(Prop.SUBALG_LOGGING)
        cleanup = self.getPropertyValue(Prop.CLEANUP)

        rb = self.getProperty(PropAutoProcess.RB).value
        db = self.getProperty(PropAutoProcess.DB).value

        workflowProgress = Progress(self, start=0.0, end=1.0, nreports=len(rb))

        linePosition = self.getProperty(Prop.LINE_POSITION).value
        slitNorm = self.getProperty(Prop.SLIT_NORM).value

        toStitch = []

        for angle in range(len(rb)):

            twoTheta = self.twoTheta(rb[angle], angle)

            runDB = self.mtdName(db[angle])
            runRB = self.mtdName(rb[angle])

            halfWidthsReflected = [int(self.getValue(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH, angle)),
                                   int(self.getValue(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH, angle))]
            halfWidthsDirect = [int(self.getValue(PropAutoProcess.LOW_FOREGROUND_HALF_WIDTH_DIRECT, angle)),
                                int(self.getValue(PropAutoProcess.HIGH_FOREGROUND_HALF_WIDTH_DIRECT, angle))]
            wavelengthRange = [float(self.getValue(PropAutoProcess.WAVELENGTH_LOWER, angle)),
                               float(self.getValue(PropAutoProcess.WAVELENGTH_UPPER, angle))]

            directBeamInput = self.angleOrMergeRuns(db[angle])
            reflectedBeamInput = self.angleOrMergeRuns(rb[angle])

            # Direct beam already in ADS?
            workspaces = mtd.getObjectNames()
            if '__direct-{}-angle-{}'.format(runDB, angle) not in workspaces:
                self.log().notice('Direct beam __direct-{}-angle-{} not cached in AnalysisDataService.'.format(runDB,
                                                                                                               angle))
                # The cached value could be used, if the parameters of the angles correspond

                # Direct beam pre-processing
                ReflectometryILLPreprocess(
                    Run=directBeamInput,
                    OutputWorkspace='__direct-{}-angle-{}'.format(runDB, angle),
                    ForegroundHalfWidth=halfWidthsDirect,
                    SlitNormalisation=slitNorm,
                    FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD).value,
                    LowAngleBkgOffset=int(self.getValue(PropAutoProcess.LOW_BKG_OFFSET_DIRECT, angle)),
                    LowAngleBkgWidth=int(self.getValue(PropAutoProcess.LOW_BKG_WIDTH_DIRECT, angle)),
                    HighAngleBkgOffset=int(self.getValue(PropAutoProcess.HIGH_BKG_OFFSET_DIRECT, angle)),
                    HighAngleBkgWidth=int(self.getValue(PropAutoProcess.HIGH_BKG_WIDTH_DIRECT, angle)),
                    FitStartWorkspaceIndex=int(self.getValue(PropAutoProcess.START_WS_INDEX_DIRECT, angle)),
                    FitEndWorkspaceIndex=int(self.getValue(PropAutoProcess.END_WS_INDEX_DIRECT, angle)),
                    SubalgorithmLogging=subalgLogging,
                    Cleanup=cleanup,
                )
                # Direct sum foreground
                ReflectometryILLSumForeground(
                    InputWorkspace='__direct-{}-angle-{}'.format(runDB, angle),
                    OutputWorkspace='__direct-{}-angle-{}-foreground'.format(runDB, angle),
                    SummationType=SumType.IN_LAMBDA,
                    WavelengthRange=wavelengthRange,
                    SubalgorithmLogging=subalgLogging,
                    Cleanup=cleanup,
                )
                # Direct beam polarization correction
                if self.isPolarized():
                    ReflectometryILLPolarizationCor(
                        InputWorkspaces='__direct-{}-angle-{}-foreground'.format(runDB, angle),
                        OutputWorkspace='__direct-{}-angle-{}-polcor'.format(runDB, angle),
                        EfficiencyFile=self.getProperty(PropAutoProcess.EFFICIENCY_FILE).value,
                        SubalgorithmLogging=subalgLogging,
                        Cleanup=cleanup,
                    )
            # Reflected beam
            if not self.isPolarized():
                ReflectometryILLPreprocess(
                    Run=reflectedBeamInput,
                    OutputWorkspace='__reflected-{}'.format(runRB),
                    TwoTheta=twoTheta,
                    LinePosition=linePosition,
                    DirectLineWorkspace='__direct-{}-angle-{}'.format(runDB, angle),
                    ForegroundHalfWidth=halfWidthsReflected,
                    SlitNormalisation=slitNorm,
                    FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD).value,
                    LowAngleBkgOffset=int(self.getValue(PropAutoProcess.LOW_BKG_OFFSET_REFLECTED, angle)),
                    LowAngleBkgWidth=int(self.getValue(PropAutoProcess.LOW_BKG_WIDTH_REFLECTED, angle)),
                    HighAngleBkgOffset=int(self.getValue(PropAutoProcess.HIGH_BKG_OFFSET_REFLECTED, angle)),
                    HighAngleBkgWidth=int(self.getValue(PropAutoProcess.HIGH_BKG_WIDTH_REFLECTED, angle)),
                    FitStartWorkspaceIndex=int(self.getValue(PropAutoProcess.START_WS_INDEX_REFLECTED, angle)),
                    FitEndWorkspaceIndex=int(self.getValue(PropAutoProcess.END_WS_INDEX_REFLECTED, angle)),
                    SubalgorithmLogging=subalgLogging,
                    Cleanup=cleanup,
                )
                # Reflected sum foreground
                workspaceNamesForQConversion = '__reflected-{}-foreground'.format(runRB)
                ReflectometryILLSumForeground(
                    InputWorkspace='__reflected-{}'.format(runRB),
                    OutputWorkspace=workspaceNamesForQConversion,
                    SummationType=self.getValue(PropAutoProcess.SUM_TYPE, angle),
                    DirectForegroundWorkspace='__direct-{}-angle-{}-foreground'.format(runDB, angle),
                    DirectLineWorkspace='__direct-{}-angle-{}'.format(runDB, angle),
                    WavelengthRange=wavelengthRange,
                    SubalgorithmLogging=subalgLogging,
                    Cleanup=cleanup,
                )
            else:
                for run in reflectedBeamInput:
                    ReflectometryILLPreprocess(
                        Run=run,
                        TwoTheta=twoTheta,
                        LinePosition=linePosition,
                        OutputWorkspace='__{}'.format(run),
                        DirectLineWorkspace='__direct-{}- angle-{}'.format(runDB, angle),
                        ForegroundHalfWidth=halfWidthsReflected,
                        SlitNormalisation=slitNorm,
                        LowAngleBkgOffset=int(self.getValue(PropAutoProcess.LOW_BKG_OFFSET_REFLECTED, angle)),
                        LowAngleBkgWidth=int(self.getValue(PropAutoProcess.LOW_BKG_WIDTH_REFLECTED, angle)),
                        HighAngleBkgOffset=int(self.getValue(PropAutoProcess.HIGH_BKG_OFFSET_REFLECTED, angle)),
                        HighAngleBkgWidth=int(self.getValue(PropAutoProcess.HIGH_BKG_WIDTH_REFLECTED, angle)),
                        FitStartWorkspaceIndex=int(self.getValue(PropAutoProcess.START_WS_INDEX_REFLECTED, angle)),
                        FitEndWorkspaceIndex=int(self.getValue(PropAutoProcess.END_WS_INDEX_REFLECTED, angle)),
                        SubalgorithmLogging=subalgLogging,
                        Cleanup=cleanup,
                    )
                    # Reflected sum foreground
                    ReflectometryILLSumForeground(
                        InputWorkspace='__{}'.format(run),
                        OutputWorkspace='__reflected-{}-foreground'.format(run),
                        SummationType=self.getValue(PropAutoProcess.SUM_TYPE, angle),
                        DirectForegroundWorkspace='__direct-{}-angle-{}-foreground'.format(runDB, angle),
                        DirectLineWorkspace='__direct-{}-angle-{}'.format(runDB, angle),
                        WavelengthRange=wavelengthRange,
                        SubalgorithmLogging=subalgLogging,
                        Cleanup=cleanup,
                    )
                # Reflected polarization correction
                inputWorkspaces = '__reflected-{}-foreground'.format(runRB)
                if len(reflectedBeamInput) > 1:
                    for r in self._reflecteds:
                        inputWorkspaces = ',{}'.join(rb)
                        inputWorkspaces += ',' + '__reflected-{}-foreground'.format(r)
                ReflectometryILLPolarizationCor(
                    InputWorkspaces=inputWorkspaces,
                    OutputWorkspace='__reflected-polcor'.format(),
                    EfficiencyFile=self.getProperty(PropAutoProcess.EFFICIENCY_FILE).value,
                    SubalgorithmLogging=subalgLogging,
                    Cleanup=cleanup,
                )
                polCorGroup = mtd['__reflected-polcor']
                workspaceNamesForQConversion = polCorGroup.getNames()
            # Conversion to Q
            outWS = '__Angle-{}'.format(angle)
            ReflectometryILLConvertToQ(
                InputWorkspace=workspaceNamesForQConversion,
                OutputWorkspace=outWS,
                DirectForegroundWorkspace='__direct-{}-angle-{}-foreground'.format(runDB, angle),
                GroupingQFraction=float(self.getValue(PropAutoProcess.GROUPING_FRACTION, angle)),
                SubalgorithmLogging=subalgLogging,
                Cleanup=cleanup,
            )
            toStitch.append(outWS)
            workflowProgress.report()

        wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)
        if len(rb) > 100:
            ','.join(toStitch)
            # need to defined reduction.two_theta for stitched angles, list
            Stitch1DMany(
                InputWorkspaces=toStitch,
                OutputWorkspace='{}'.format(wsPrefix),
                StartOverlaps=self.getProperty(PropAutoProcess.START_OVERLAPS).value,
                EndOverlaps=self.getProperty(PropAutoProcess.END_OVERLAPS).value,
                UseManualScaleFactors=self.getProperty(PropAutoProcess.USE_MANUAL_SCALE_FACTORS).value,
                ManualScaleFactors=self.getProperty(PropAutoProcess.MANUAL_SCALE_FACTORS).value,
            )
        else:
            RenameWorkspace(
                InputWorkspace='{}'.format(toStitch[0]),
                OutputWorkspace='{}'.format(wsPrefix),
            )
        scaleFactor = self.getProperty(PropAutoProcess.SCALE_FACTOR).value
        if scaleFactor != 1.:
            Scale(
                InputWorkspace='{}'.format(wsPrefix),
                OutputWorkspace='{}'.format(wsPrefix),
                Factor=scaleFactor,
            )
        self.setProperty(Prop.OUTPUT_WS, wsPrefix)
        self.finalize()

AlgorithmFactory.subscribe(ReflectometryILLAutoProcess)
