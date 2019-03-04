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
    StringArrayProperty,
    StringArrayLengthValidator,
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
from ReflectometryILLPreprocess import BkgMethod, Prop
from ReflectometryILLSumForeground import SumType
import numpy


class PropAutoProcess(object):
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'DirectFlatBackground'
    DB = 'DirectRun'
    EFFICIENCY_FILE = 'EfficiencyFile'
    END_WS_INDEX_DIRECT = 'DirectFitEndWorkspaceIndex'
    END_WS_INDEX_REFLECTED = 'FitEndWorkspaceIndex'
    END_OVERLAPS = 'EndOverlaps'
    FOREGROUND_HALF_WIDTH_DIRECT = 'DirectForegroundHalfWidth'
    FOREGROUND_HALF_WIDTH = 'ForegroundHalfWidth'
    GROUPING_FRACTION = 'GroupingQFraction'
    HIGH_BKG_OFFSET_DIRECT = 'DirectHighAngleBkgOffset'
    HIGH_BKG_OFFSET_REFLECTED = 'HighAngleBkgOffset'
    HIGH_BKG_WIDTH_DIRECT = 'DirectHighAngleBkgWidth'
    HIGH_BKG_WIDTH_REFLECTED = 'HighAngleBkgWidth'
    LOW_BKG_OFFSET_DIRECT = 'DirectLowAngleBkgOffset'
    LOW_BKG_OFFSET_REFLECTED = 'LowAngleBkgOffset'
    LOW_BKG_WIDTH_DIRECT = 'DirectLowAngleBkgWidth'
    LOW_BKG_WIDTH_REFLECTED = 'LowAngleBkgWidth'
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
        stringArrayValidator = StringArrayLengthValidator()
        stringArrayValidator.setLengthMin(1)

        listOrSingleNumber = ': provide either a list or a single value for each angle.'

        self.declareProperty(
            MultipleFileProperty(
                PropAutoProcess.RB,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files.')
        self.setPropertyGroup(PropAutoProcess.RB, '')
        self.declareProperty(
            MultipleFileProperty(
                PropAutoProcess.DB,
                action=FileAction.OptionalLoad,
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
            PropAutoProcess.ANGLE_OPTION,
            defaultValue=Angle.DAN,
            validator=StringListValidator(
                [Angle.SAN, Angle.DAN]
            ),
            doc='Angle option used for detector positioning.'
        )
        self.setPropertyGroup(PropAutoProcess.ANGLE_OPTION, '')
        preProcessGen = 'ReflectometryILLPreprocess, common properties'
        self.copyProperties(
            'ReflectometryILLPreprocess',
            [
                Prop.TWO_THETA,
                Prop.LINE_POSITION,
                Prop.SUBALG_LOGGING,
                Prop.CLEANUP,
                Prop.WATER_REFERENCE,
                Prop.SLIT_NORM,
                Prop.FLUX_NORM_METHOD
            ]
        )
        self.setPropertyGroup(Prop.TWO_THETA, preProcessGen)
        self.setPropertyGroup(Prop.LINE_POSITION, preProcessGen)
        self.setPropertyGroup(Prop.SUBALG_LOGGING, preProcessGen)
        self.setPropertyGroup(Prop.CLEANUP, preProcessGen)
        self.setPropertyGroup(Prop.WATER_REFERENCE, preProcessGen)
        self.setPropertyGroup(Prop.SLIT_NORM, preProcessGen)
        self.setPropertyGroup(Prop.FLUX_NORM_METHOD, preProcessGen)
        preProcessDirect = 'ReflectometryILLPreprocess for direct runs'
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.FOREGROUND_HALF_WIDTH_DIRECT,
                validator=maxTwoNonnegativeInts
            ),
            doc='Number of foreground pixels at lower and higher angles from the centre pixel.'
        )
        self.setPropertyGroup(PropAutoProcess.FOREGROUND_HALF_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            PropAutoProcess.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.CONSTANT,
            validator=StringListValidator(
                [BkgMethod.CONSTANT, BkgMethod.LINEAR, BkgMethod.OFF]
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
                validator=nonnegativeInts,
                values=[0],
            ),
            doc='Start histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.START_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.END_WS_INDEX_DIRECT,
                validator=nonnegativeInts,
                values=[255],
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
                Prop.FOREGROUND_HALF_WIDTH,
                Prop.BKG_METHOD
            ]
        )
        self.setPropertyGroup(Prop.FOREGROUND_HALF_WIDTH, preProcessReflected)
        self.setPropertyGroup(Prop.BKG_METHOD, preProcessReflected)
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
                validator=nonnegativeInts,
                values=[0],
            ),
            doc='Start histogram index used for peak fitting{}'.format(listOrSingleNumber)
        )
        self.setPropertyGroup(PropAutoProcess.START_WS_INDEX_REFLECTED, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropAutoProcess.END_WS_INDEX_REFLECTED,
                validator=nonnegativeInts,
                values=[255],
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
                validator=stringArrayValidator,
                direction=Direction.Input,
                values=[SumType.IN_LAMBDA]
            ),
            doc='Type of summation to perform.'
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
        numberDirectRuns = len(self.getProperty(PropAutoProcess.DB).value)
        numberReflectedRuns = len(self.getProperty(PropAutoProcess.RB).value)
        if numberDirectRuns == 0 and numberReflectedRuns == 0:
            issues[PropAutoProcess.RB] = "Nothing to do."
        if numberDirectRuns != numberReflectedRuns:
            issues[PropAutoProcess.RB] = "The same number of direct runs and reflected runs must be given."
        return issues

    def getValue(self, propertyName, angle):
        value = self.getProperty(propertyName).value
        if not self.getProperty(propertyName).isDefault:
            return value[angle]
        else:
            return value[0]

    def getString(self, propertyName, angle):
        stringVal = self.getPropertyValue(propertyName)
        allStrings = stringVal.split(',')
        if len(allStrings) == 1:
            return allStrings[0]
        elif angle <= len(allStrings):
            return allStrings[angle]
        else:
            raise RuntimeError(
                'The number of entries for {} must correspond to the number of reflected beams'.format(Prop.SUM_TYPE)
            )

    def PyExec(self):
        """Execute the algorithm."""
        self._subalgLogging = self.getPropertyValue(Prop.SUBALG_LOGGING)
        self._cleanup = self.getPropertyValue(Prop.CLEANUP)
        self.wsPrefix = self.getPropertyValue(Prop.OUTPUT_WS)

        angleOption = self.getPropertyValue(PropAutoProcess.ANGLE_OPTION)

        rb = self.getProperty(PropAutoProcess.RB).value
        db = self.getProperty(PropAutoProcess.DB).value

        self._progress = Progress(self, start=0.0, end=1.0, nreports=len(rb))

        self.halfWidthsReflected = self.getProperty(Prop.FOREGROUND_HALF_WIDTH).value
        self.wavelengthRange = [float(self.getProperty(PropAutoProcess.WAVELENGTH_LOWER).value),
                                float(self.getProperty(PropAutoProcess.WAVELENGTH_UPPER).value)]
        self.polarizationEffFile = self.getProperty(PropAutoProcess.EFFICIENCY_FILE).value
        self.twoTheta = self.getProperty(Prop.TWO_THETA).value
        self.linePosition = self.getProperty(Prop.LINE_POSITION).value
        self.slitNorm = self.getProperty(Prop.SLIT_NORM).value
        if self.polarizationEffFile == "":
            self.isPolarized = False
        else:
            self.isPolarized = True
        self.scaling = self.getProperty(PropAutoProcess.SCALE_FACTOR).value
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
            sumType = self.getString(PropAutoProcess.SUM_TYPE, angle)
            # Direct beam already in ADS?
            workspaces = mtd.getObjectNames()
            if not 'direct-{}'.format(db[angle]) in workspaces:
                # Direct beam pre-processing
                ReflectometryILLPreprocess(
                    Run=db[angle],
                    LinePosition=self.linePosition,
                    TwoTheta=self.twoTheta,
                    OutputWorkspace='direct-{}'.format(runDB),
                    ForegroundHalfWidth=self.getProperty(PropAutoProcess.FOREGROUND_HALF_WIDTH_DIRECT).value,
                    SlitNormalisation=self.slitNorm,
                    FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD).value,
                    LowAngleBkgOffset=int(self.getValue(PropAutoProcess.LOW_BKG_OFFSET_DIRECT, angle)),
                    LowAngleBkgWidth=int(self.getValue(PropAutoProcess.LOW_BKG_WIDTH_DIRECT, angle)),
                    HighAngleBkgOffset=int(self.getValue(PropAutoProcess.HIGH_BKG_OFFSET_DIRECT, angle)),
                    HighAngleBkgWidth=int(self.getValue(PropAutoProcess.HIGH_BKG_WIDTH_DIRECT, angle)),
                    FitStartWorkspaceIndex=int(self.getValue(PropAutoProcess.START_WS_INDEX_DIRECT, angle)),
                    FitEndWorkspaceIndex=int(self.getValue(PropAutoProcess.END_WS_INDEX_DIRECT, angle)),
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
                # Direct sum foreground
                ReflectometryILLSumForeground(
                    InputWorkspace='direct-{}'.format(runDB),
                    OutputWorkspace='direct-{}-foreground'.format(runDB),
                    SummationType=sumType,
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
                    FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD).value,
                    LowAngleBkgOffset=int(self.getValue(PropAutoProcess.LOW_BKG_OFFSET_REFLECTED, angle)),
                    LowAngleBkgWidth=int(self.getValue(PropAutoProcess.LOW_BKG_WIDTH_REFLECTED, angle)),
                    HighAngleBkgOffset=int(self.getValue(PropAutoProcess.HIGH_BKG_OFFSET_REFLECTED, angle)),
                    HighAngleBkgWidth=int(self.getValue(PropAutoProcess.HIGH_BKG_WIDTH_REFLECTED, angle)),
                    FitStartWorkspaceIndex=int(self.getValue(PropAutoProcess.START_WS_INDEX_REFLECTED, angle)),
                    FitEndWorkspaceIndex=int(self.getValue(PropAutoProcess.END_WS_INDEX_REFLECTED, angle)),
                    SubalgorithmLogging=self._subalgLogging,
                    Cleanup=self._cleanup,
                )
                # Reflected sum foreground
                self._workspaceNamesForQConversion = 'reflected-{}-foreground'.format(runRB)
                ReflectometryILLSumForeground(
                    InputWorkspace='reflected-{}'.format(runRB),
                    OutputWorkspace=self._workspaceNamesForQConversion,
                    #Foreground=self.foregroundReflected,
                    SummationType=sumType,
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
                        SummationType=sumType,
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
            outWS = 'Angle-{}'.format(angle)
            ReflectometryILLConvertToQ(
                InputWorkspace=self._workspaceNamesForQConversion,
                OutputWorkspace=outWS,
                DirectForegroundWorkspace='direct-{}-foreground'.format(runDB),
                GroupingQFraction=float(self.getValue(PropAutoProcess.GROUPING_FRACTION, angle)),
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
                StartOverlaps=self.getProperty(PropAutoProcess.START_OVERLAPS).value,
                EndOverlaps=self.getProperty(PropAutoProcess.END_OVERLAPS).value,
                UseManualScaleFactors=self.getProperty(PropAutoProcess.USE_MANUAL_SCALE_FACTORS).value,
                ManualScaleFactors=self.getProperty(PropAutoProcess.MANUAL_SCALE_FACTORS).value,
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
