# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    FileProperty,
    MultipleFileProperty,
    Progress,
    WorkspaceGroup,
    WorkspaceGroupProperty
)
from mantid.kernel import (
    CompositeValidator,
    Direction,
    EnabledWhenProperty,
    FloatArrayBoundedValidator,
    FloatArrayProperty,
    IntArrayLengthValidator,
    IntArrayBoundedValidator,
    IntArrayProperty,
    PropertyCriterion,
    StringArrayLengthValidator,
    StringArrayProperty,
    StringListValidator
)
import ILL_utilities as utils
from ReflectometryILL_common import SampleLogs
from ReflectometryILLPreprocess import BkgMethod, Prop, SubalgLogging
from ReflectometryILLSumForeground import SumType
from mantid.simpleapi import *
import math


class PropertyNames(object):
    RB = 'Run'
    DB = 'DirectRun'
    WAVELENGTH_LOWER = 'WavelengthLowerBound'
    WAVELENGTH_UPPER = 'WavelengthUpperBound'
    GROUPING_FRACTION = 'DeltaQFractionBinning'
    ANGLE_OPTION = 'AngleOption'
    BKG_METHOD_DIRECT = 'DirectFlatBackground'
    BKG_METHOD = 'ReflFlatBackground'
    THETA = 'Theta'
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

    POLARIZATION_OPTION = 'PolarizationOption'
    RB00 = 'Run00'
    RB01 = 'Run01'
    RB10 = 'Run10'
    RB11 = 'Run11'
    EFFICIENCY_FILE = 'PolarizationEfficiencyFile'

    # all these array properties must have either single value, or
    # as many, as there are reflected beams (i.e. angle configurations)
    PROPETIES_TO_SIZE_MATCH = [DB, ANGLE_OPTION, THETA, SUM_TYPE, GROUPING_FRACTION,
                               HIGH_FRG_HALF_WIDTH, HIGH_FRG_HALF_WIDTH_DIRECT,
                               HIGH_BKG_OFFSET, HIGH_BKG_OFFSET_DIRECT,
                               HIGH_BKG_WIDTH, HIGH_BKG_WIDTH_DIRECT,
                               LOW_FRG_HALF_WIDTH, LOW_FRG_HALF_WIDTH_DIRECT,
                               LOW_BKG_OFFSET, LOW_BKG_OFFSET_DIRECT,
                               LOW_BKG_WIDTH, LOW_BKG_WIDTH_DIRECT,
                               START_WS_INDEX, END_WS_INDEX,
                               START_WS_INDEX_DIRECT, END_WS_INDEX_DIRECT,
                               WAVELENGTH_LOWER, WAVELENGTH_UPPER]

    DAN = 'DetectorAngle'
    SAN = 'SampleAngle'
    UAN = 'UserAngle'
    INCOHERENT = 'Incoherent'
    COHERENT = 'Coherent'

    USE_MANUAL_SCALE_FACTORS = 'UseManualScaleFactors'
    MANUAL_SCALE_FACTORS = 'ManualScaleFactors'
    CACHE_DIRECT_BEAM = 'CacheDirectBeam'


class ReflectometryILLAutoProcess(DataProcessorAlgorithm):

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algrithm."""
        return 'ILL\\Reflectometry;ILL\\Auto;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryILLAutoProcess'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Performs reduction of ILL reflectometry data, instruments D17 and FIGARO."

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

        #======================== Main Properties ========================
        self.declareProperty(PropertyNames.POLARIZATION_OPTION,
                             'NonPolarized',
                             StringListValidator(['NonPolarized', 'Polarized']),
                             'Indicate whether measurements are polarized')

        is_polarized = EnabledWhenProperty(PropertyNames.POLARIZATION_OPTION,
                                           PropertyCriterion.IsEqualTo, 'Polarized')
        is_not_polarized = EnabledWhenProperty(PropertyNames.POLARIZATION_OPTION,
                                               PropertyCriterion.IsEqualTo, 'NonPolarized')
        polarized = 'Inputs for polarized measurements'

        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.RB,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files.')
        self.setPropertySettings(PropertyNames.RB, is_not_polarized)

        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.RB00,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files for 00.')
        self.setPropertySettings(PropertyNames.RB00, is_polarized)
        self.setPropertyGroup(PropertyNames.RB00, polarized)

        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.RB01,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files for 01.')
        self.setPropertySettings(PropertyNames.RB01, is_polarized)
        self.setPropertyGroup(PropertyNames.RB01, polarized)

        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.RB10,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files for 10.')
        self.setPropertySettings(PropertyNames.RB10, is_polarized)
        self.setPropertyGroup(PropertyNames.RB10, polarized)

        self.declareProperty(
            MultipleFileProperty(
                PropertyNames.RB11,
                action=FileAction.OptionalLoad,
                extensions=['nxs']
            ),
            doc='A list of reflected run numbers/files for 11.')
        self.setPropertySettings(PropertyNames.RB11, is_polarized)
        self.setPropertyGroup(PropertyNames.RB11, polarized)

        self.declareProperty(
            FileProperty(
                PropertyNames.EFFICIENCY_FILE,
                defaultValue='',
                action=FileAction.OptionalLoad
            ),
            doc='A file containing the polarization efficiency factors.'
        )
        self.setPropertySettings(PropertyNames.EFFICIENCY_FILE, is_polarized)
        self.setPropertyGroup(PropertyNames.EFFICIENCY_FILE, polarized)

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
            PropertyNames.BKG_METHOD_DIRECT,
            defaultValue=BkgMethod.AVERAGE,
            validator=StringListValidator(
                [BkgMethod.AVERAGE,
                 BkgMethod.CONSTANT,
                 BkgMethod.LINEAR,
                 BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
        )
        self.declareProperty(
            PropertyNames.BKG_METHOD,
            defaultValue=BkgMethod.AVERAGE,
            validator=StringListValidator(
                [BkgMethod.AVERAGE,
                 BkgMethod.CONSTANT,
                 BkgMethod.LINEAR,
                 BkgMethod.OFF]
            ),
            doc='Flat background calculation method for background subtraction.'
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
        self.declareProperty(
            PropertyNames.SCALE_FACTOR,
            defaultValue=1.0,
            doc='Scale factor.'
        )

        self.declareProperty(
            PropertyNames.USE_MANUAL_SCALE_FACTORS,
            defaultValue=False,
            doc='Choose to apply manual scale factors for stitching.'
        )

        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.MANUAL_SCALE_FACTORS,
                values=[]
            ),
            doc='A list of manual scale factors for stitching (number of anlge configurations minus 1)'
        )

        self.setPropertySettings(PropertyNames.MANUAL_SCALE_FACTORS,
                                 EnabledWhenProperty(PropertyNames.USE_MANUAL_SCALE_FACTORS, PropertyCriterion.IsNotDefault))

        self.declareProperty(
            PropertyNames.CACHE_DIRECT_BEAM,
            defaultValue=False,
            doc='Cache the processed direct beam in ADS for ready use with further reflected beams;'
                'saves important execution time, however assumes that the direct beam processing '
                'configuration must be invariant for different reflected beams.'
        )

        # ======================== Common Properties ========================
        commonProp = 'Preprocessing common properties: provide a list or a single value'

        self.declareProperty(
            StringArrayProperty(
                PropertyNames.ANGLE_OPTION,
                values=[PropertyNames.DAN],
                validator=stringArrayValidator,
                direction=Direction.Input,
            ),
            doc='Angle option used for detector positioning'
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.THETA,
                values=[-1.]
            ),
            doc='A user-defined angle theta in degree'
        )
        self.declareProperty(
            StringArrayProperty(
                PropertyNames.SUM_TYPE,
                values=[PropertyNames.INCOHERENT],
                validator=stringArrayValidator,
                direction=Direction.Input,
            ),
            doc='Type of summation to perform'
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.WAVELENGTH_LOWER,
                values=[0.],
                validator=nonnegativeFloatArray
            ),
            doc='The lower wavelength bound (Angstrom)'
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.WAVELENGTH_UPPER,
                values=[35.],
                validator=nonnegativeFloatArray
            ),
            doc='The upper wavelength bound (Angstrom)'
        )
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.GROUPING_FRACTION,
                values=[0.5],
                validator=nonnegativeFloatArray,
            ),
            doc='If set, group the output by steps of this fraction multiplied by Q resolution'
        )
        self.setPropertyGroup(PropertyNames.ANGLE_OPTION, commonProp)
        self.setPropertyGroup(PropertyNames.THETA, commonProp)
        self.setPropertyGroup(PropertyNames.SUM_TYPE, commonProp)
        self.setPropertyGroup(PropertyNames.WAVELENGTH_LOWER, commonProp)
        self.setPropertyGroup(PropertyNames.WAVELENGTH_UPPER, commonProp)
        self.setPropertyGroup(PropertyNames.GROUPING_FRACTION, commonProp)

        # ======================== Direct Run Properties ========================
        preProcessDirect = 'Preprocessing for direct runs: provide a list or a single value'

        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT,
                values=[1],
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
            doc='Distance of flat background region towards smaller detector angles from the '
                + 'foreground centre, in pixels'
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the '
                + 'foreground centre, in pixels'
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT,
                values=[1],
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
            doc='Distance of flat background region towards larger detector angles from the '
                + 'foreground centre, in pixels'
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_OFFSET_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_BKG_WIDTH_DIRECT,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the '
                + 'foreground centre, in pixels'
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_WIDTH_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.START_WS_INDEX_DIRECT,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting'
        )
        self.setPropertyGroup(PropertyNames.START_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.END_WS_INDEX_DIRECT,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting'
        )
        self.setPropertyGroup(PropertyNames.END_WS_INDEX_DIRECT, preProcessDirect)
        self.declareProperty(
            PropertyNames.XMIN_DIRECT,
            defaultValue=-1.,
            doc='Minimum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(PropertyNames.XMIN_DIRECT, preProcessDirect)
        self.declareProperty(
            PropertyNames.XMAX_DIRECT,
            defaultValue=-1.,
            doc='Maximum x value (unit wavelength) used for peak fitting.'
        )
        self.setPropertyGroup(PropertyNames.XMAX_DIRECT, preProcessDirect)

        # ======================== Preprocessing For Reflected Runs ========================
        preProcessReflected = 'Preprocessing for reflected runs: provide a list or a single value'

        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_FRG_HALF_WIDTH,
                values=[1],
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
            doc='Distance of flat background region towards smaller detector angles from the '
                + 'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_OFFSET, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.LOW_BKG_WIDTH,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards smaller detector angles from the '
                + 'foreground centre, in pixels'
        )
        self.setPropertyGroup(PropertyNames.LOW_BKG_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_FRG_HALF_WIDTH,
                values=[1],
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
            doc='Distance of flat background region towards larger detector angles from the '
                + 'foreground centre, in pixels'
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_OFFSET, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.HIGH_BKG_WIDTH,
                values=[5],
                validator=nonnegativeInts,
            ),
            doc='Width of flat background region towards larger detector angles from the '
                + 'foreground centre, in pixels.'
        )
        self.setPropertyGroup(PropertyNames.HIGH_BKG_WIDTH, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.START_WS_INDEX,
                values=[0],
                validator=nonnegativeInts,
            ),
            doc='Start histogram index used for peak fitting'
        )
        self.setPropertyGroup(PropertyNames.START_WS_INDEX, preProcessReflected)
        self.declareProperty(
            IntArrayProperty(
                PropertyNames.END_WS_INDEX,
                values=[255],
                validator=nonnegativeInts,
            ),
            doc='Last histogram index used for peak fitting'
        )
        self.setPropertyGroup(PropertyNames.END_WS_INDEX, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.XMIN,
                values=[-1.]
            ),
            doc='Minimum x value (unit wavelength) used for peak fitting'
        )
        self.setPropertyGroup(PropertyNames.XMIN, preProcessReflected)
        self.declareProperty(
            FloatArrayProperty(
                PropertyNames.XMAX,
                values=[-1.]
            ),
            doc='Maximum x value (unit wavelength) used for peak fitting'
        )
        self.setPropertyGroup(PropertyNames.XMAX, preProcessReflected)

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        RB = self.getProperty(PropertyNames.RB).value
        if not RB:
            RB = self.getProperty(PropertyNames.RB00).value
        dimensionality = len(RB)
        for property_name in PropertyNames.PROPETIES_TO_SIZE_MATCH:
            value = self.getProperty(property_name).value
            if len(value) != dimensionality and len(value) != 1:
                issues[property_name] = 'Must have a single value or as many as there are reflected beams: given {0}, '\
                                        'but there are {1} reflected beams'.format(len(value), dimensionality)
        if self.getProperty(PropertyNames.USE_MANUAL_SCALE_FACTORS).value:
            manual_scale_factors = self.getProperty(PropertyNames.MANUAL_SCALE_FACTORS).value
            if len(manual_scale_factors) != dimensionality-1:
                issues[PropertyNames.MANUAL_SCALE_FACTORS] = \
                    'Provide N-1 manual scale factors, where N is the number of different angle configurations'
        angle_options = self.getProperty(PropertyNames.ANGLE_OPTION).value
        for angle_option in angle_options:
            if angle_option not in [PropertyNames.DAN, PropertyNames.SAN, PropertyNames.UAN]:
                issues[PropertyNames.ANGLE_OPTION] = 'Invalid angle option given: ' + angle_option
                break
        sum_types = self.getProperty(PropertyNames.SUM_TYPE).value
        for sum_type in sum_types:
            if sum_type not in [PropertyNames.INCOHERENT, PropertyNames.COHERENT]:
                issues[PropertyNames.SUM_TYPE] = 'Invalid summation option given: ' + sum_type
                break
        if self.getPropertyValue(PropertyNames.POLARIZATION_OPTION) == 'NotPolarized' and not self.getPropertyValue(PropertyNames.RB):
            issues[PropertyNames.RB] = 'Reflected beam input runs are mandatory'
        if self.getPropertyValue(PropertyNames.POLARIZATION_OPTION) == 'Polarized':
            if not self.getPropertyValue(PropertyNames.RB00):
                issues[PropertyNames.RB00] = 'Reflected beam input runs are mandatory for 00 (or 0)'
            if not self.getPropertyValue(PropertyNames.RB11):
                issues[PropertyNames.RB11] = 'Reflected beam input runs are mandatory for 11 (or 1)'
        return issues

    def setup(self):
        self._subalgLogging = self.getProperty(Prop.SUBALG_LOGGING).value
        self._cleanup = self.getProperty(Prop.CLEANUP).value
        self._autoCleanup = utils.Cleanup(self._cleanup, self._subalgLogging == SubalgLogging.ON)
        self._outWS = self.getPropertyValue(Prop.OUTPUT_WS)
        self._db = self.getProperty(PropertyNames.DB).value
        self._rb = self.getProperty(PropertyNames.RB).value
        self._rb00 = self.getProperty(PropertyNames.RB00).value
        self._rb01 = self.getProperty(PropertyNames.RB01).value
        self._rb10 = self.getProperty(PropertyNames.RB10).value
        self._rb11 = self.getProperty(PropertyNames.RB11).value
        self._dimensionality = len(self._rb) if self._rb else len(self._rb00)

    def get_value(self, propertyName, angle_index):
        """Return the value of the property at given angle index."""
        value = self.getProperty(propertyName).value
        if len(value) == 1:
            return value[0]
        else:
            return value[angle_index]

    def make_name(self, runName):
        """Return a name suitable to put in the ADS: the run number"""
        if not isinstance(runName, list):
            if runName:
                return runName[-10:-4]
            else:
                return ''
        else:
            # for multiple files return the run number of the first run
            if runName[0]:
                return self.make_name(runName[0])
            else:
                return ''

    def compose_run_string(self, run):
        """Return the string that will be passed to load the files."""
        if isinstance(run, list):
            return '+'.join(run)
        else:
            return run

    def is_polarized(self):
        """Return True, if a polarization file is given and False otherwise."""
        return self.getProperty(PropertyNames.POLARIZATION_OPTION).value == 'Polarized'

    def preprocess_direct_beam(self, run, out_ws, angle_index):
        """Runs preprocess for the direct beam"""
        ReflectometryILLPreprocess(
            Run=run,
            Measurement='DirectBeam',
            OutputWorkspace=out_ws,
            SlitNormalisation=self.getProperty(Prop.SLIT_NORM).value,
            FluxNormalisation=self.getProperty(Prop.FLUX_NORM_METHOD).value,
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
            ForegroundHalfWidth=[
                int(self.get_value(PropertyNames.LOW_FRG_HALF_WIDTH_DIRECT, angle_index)),
                int(self.get_value(PropertyNames.HIGH_FRG_HALF_WIDTH_DIRECT, angle_index))],
            LowAngleBkgOffset=int(self.get_value(PropertyNames.LOW_BKG_OFFSET_DIRECT, angle_index)),
            LowAngleBkgWidth=int(self.get_value(PropertyNames.LOW_BKG_WIDTH_DIRECT, angle_index)),
            HighAngleBkgOffset=int(self.get_value(PropertyNames.HIGH_BKG_OFFSET_DIRECT, angle_index)),
            HighAngleBkgWidth=int(self.get_value(PropertyNames.HIGH_BKG_WIDTH_DIRECT, angle_index)),
            FlatBackground=self.getPropertyValue(PropertyNames.BKG_METHOD_DIRECT),
            FitStartWorkspaceIndex=int(self.get_value(PropertyNames.START_WS_INDEX_DIRECT, angle_index)),
            FitEndWorkspaceIndex=int(self.get_value(PropertyNames.END_WS_INDEX_DIRECT, angle_index)),
            FitRangeLower=self.getProperty(PropertyNames.XMIN_DIRECT).value,
            FitRangeUpper=self.getProperty(PropertyNames.XMAX_DIRECT).value
        )

    def detector_angle_from_logs(self, ws):
        """Returns the detector angle from sample logs"""
        run = mtd[ws].run() if not isinstance(mtd[ws], WorkspaceGroup) else mtd[ws][0].run()
        if run.hasProperty('DAN.value'):
            return run.getLogData('DAN.value').value
        elif run.hasProperty('dan.value'):
            return run.getLogData('dan.value').value
        else:
            raise RuntimeError('Unable to retrieve the detector angle from ' + ws)

    def foreground_centre_from_logs(self, ws):
        """Returns the foreground centre from sample logs"""
        run = mtd[ws].run() if not isinstance(mtd[ws], WorkspaceGroup) else mtd[ws][0].run()
        if run.hasProperty(SampleLogs.LINE_POSITION):
            return run.getLogData(SampleLogs.LINE_POSITION).value
        else:
            raise RuntimeError('Unable to retrieve the direct beam foreground centre needed for DAN option.')

    def log_foreground_centres(self, reflectedBeamName, directBeamName):
        """Logs the fractional foreground centres for direct and reflected beams"""
        db = mtd[directBeamName] if not isinstance(mtd[directBeamName], WorkspaceGroup) else mtd[directBeamName][0]
        db_frg_centre = db.run().getLogData(SampleLogs.LINE_POSITION).value
        rb_frg_centre = mtd[reflectedBeamName].run().getLogData(SampleLogs.LINE_POSITION).value
        self.log().accumulate('Direct beam foreground centre [pixel]: {0:.5f}\n'.format(db_frg_centre))
        self.log().accumulate('Reflected beam foreground centre [pixel]: {0:.5f}\n'.format(rb_frg_centre))

    def preprocess_reflected_beam(self, run, out_ws, directBeamName, angle_index):
        """Runs preprocess for the reflected beam"""
        angle_option = self.get_value(PropertyNames.ANGLE_OPTION, angle_index)
        preprocess_args = {
            'Run': run,
            'Measurement': 'ReflectedBeam',
            'OutputWorkspace': out_ws,
            'AngleOption': angle_option,
            'SlitNormalisation': self.getProperty(Prop.SLIT_NORM).value,
            'FluxNormalisation': self.getProperty(Prop.FLUX_NORM_METHOD).value,
            'SubalgorithmLogging': self._subalgLogging,
            'Cleanup': self._cleanup,
            'FlatBackground': self.getPropertyValue(PropertyNames.BKG_METHOD),
            'ForegroundHalfWidth': [int(self.get_value(PropertyNames.LOW_FRG_HALF_WIDTH, angle_index)),
                                    int(self.get_value(PropertyNames.HIGH_FRG_HALF_WIDTH, angle_index))],
            'LowAngleBkgOffset': int(self.get_value(PropertyNames.LOW_BKG_OFFSET, angle_index)),
            'LowAngleBkgWidth': int(self.get_value(PropertyNames.LOW_BKG_WIDTH, angle_index)),
            'HighAngleBkgOffset': int(self.get_value(PropertyNames.HIGH_BKG_OFFSET, angle_index)),
            'HighAngleBkgWidth': int(self.get_value(PropertyNames.HIGH_BKG_WIDTH, angle_index)),
            'FitStartWorkspaceIndex': int(self.get_value(PropertyNames.START_WS_INDEX, angle_index)),
            'FitEndWorkspaceIndex': int(self.get_value(PropertyNames.END_WS_INDEX, angle_index)),
            'FitRangeLower': self.get_value(PropertyNames.XMIN, angle_index),
            'FitRangeUpper': self.get_value(PropertyNames.XMAX, angle_index)
        }
        if angle_option == PropertyNames.UAN:
            preprocess_args['BraggAngle'] = self.get_value(PropertyNames.THETA, angle_index)
        elif angle_option == PropertyNames.DAN:
            preprocess_args['DirectBeamDetectorAngle'] = self.detector_angle_from_logs(directBeamName)
            preprocess_args['DirectBeamForegroundCentre'] = self.foreground_centre_from_logs(directBeamName)
        ReflectometryILLPreprocess(**preprocess_args)
        self.log().accumulate('Angle method: {0}\n'.format(angle_option))


    def sum_foreground(self, inputWorkspaceName, outputWorkspaceName, sumType, angle_index, directForegroundName = ''):
        """Run the ReflectometryILLSumForeground, empty directForegroundName decides, if reflected beam is present."""
        wavelengthRange = [float(self.get_value(PropertyNames.WAVELENGTH_LOWER, angle_index)),
                           float(self.get_value(PropertyNames.WAVELENGTH_UPPER, angle_index))]
        directBeamName = directForegroundName[:-4] if directForegroundName else ''
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
        if directForegroundName:
            self.log().accumulate('Final source (mid chopper) to sample distance [m]: {0:.5f}\n'.
                                  format(mtd[outputWorkspaceName].spectrumInfo().l1()))
            self.log().accumulate('Final reflected foreground centre distance [m]: {0:.5f}\n'.
                                  format(mtd[outputWorkspaceName].spectrumInfo().l2(0)))

    def polarization_correction(self, inputWorkspaceName, outputWorkspaceName):
        """Run the ReflectometryILLPolarizationCor."""
        ReflectometryILLPolarizationCor(
            InputWorkspaces=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            EfficiencyFile=self.getProperty(PropertyNames.EFFICIENCY_FILE).value,
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def convert_to_momentum_transfer(self, inputWorkspaceName, outputWorkspaceName, directForegroundName, angle_index):
        """Run the ReflectometryILLConvertToQ."""
        ReflectometryILLConvertToQ(
            InputWorkspace=inputWorkspaceName,
            OutputWorkspace=outputWorkspaceName,
            DirectForegroundWorkspace=directForegroundName,
            GroupingQFraction=float(self.get_value(PropertyNames.GROUPING_FRACTION, angle_index)),
            SubalgorithmLogging=self._subalgLogging,
            Cleanup=self._cleanup,
        )

    def process_direct_beam(self, directBeamName, directForegroundName, angle_index):
        """Processes the direct beam for the given angle configuration."""
        dbrun = self._db[0]  if len(self._db) == 1 else self._db[angle_index]
        directBeamInput = self.compose_run_string(dbrun)
        self.preprocess_direct_beam(directBeamInput, directBeamName, angle_index)
        self.sum_foreground(directBeamName, directForegroundName, SumType.IN_LAMBDA, angle_index)
        if self.getProperty(PropertyNames.CACHE_DIRECT_BEAM).value:
            self._autoCleanup.protect(directBeamName)
            self._autoCleanup.protect(directForegroundName)
        else:
            self._autoCleanup.cleanupLater(directBeamName)
            self._autoCleanup.cleanupLater(directForegroundName)
        if self.is_polarized():
            self.polarization_correction(directForegroundName, directForegroundName)
            frg_ws_name = mtd[directForegroundName][0].getName()
            RenameWorkspace(InputWorkspace=frg_ws_name, OutputWorkspace=directForegroundName)

    def process_reflected_beam(self, reflectedInput, reflectedBeamName, directBeamName, angle_index):
        """Processes the reflected beam for the given angle."""
        self.preprocess_reflected_beam(reflectedInput, reflectedBeamName, directBeamName, angle_index)
        self.log_foreground_centres(reflectedBeamName, directBeamName)
        foregroundName = reflectedBeamName + '_frg'
        directForegroundName = directBeamName + '_frg'
        sum_type = self.get_value(PropertyNames.SUM_TYPE, angle_index)
        self.log().accumulate('Summation method: {0}\n'.format(sum_type))
        sum_type = 'SumInLambda' if sum_type == PropertyNames.INCOHERENT else 'SumInQ'
        self.sum_foreground(reflectedBeamName, foregroundName, sum_type, angle_index, directForegroundName)
        final_two_theta = mtd[foregroundName].spectrumInfo().twoTheta(0) * 180/math.pi
        self.log().accumulate('Calibrated 2theta of foreground centre [degree]: {0:.5f}\n'.
                              format(final_two_theta))
        if sum_type == 'SumInQ':
            isBent = mtd[foregroundName].run().getProperty('beam_stats.bent_sample').value
            self.log().accumulate('Sample: {0}\n'.format('Bent' if isBent == 1 else 'Flat'))
        self._autoCleanup.cleanupLater(reflectedBeamName)
        self._autoCleanup.cleanupLater(foregroundName)
        return foregroundName

    def PyExec(self):
        """Execute the algorithm."""
        self.log().purge()
        self.setup()
        to_group = []
        scaleFactor = self.getProperty(PropertyNames.SCALE_FACTOR).value
        progress = Progress(self, start=0.0, end=1.0, nreports=self._dimensionality)
        self.log().purge()
        self.log().accumulate('\nNumber of angles treated: {0}\n'.format(self._dimensionality))
        for angle_index in range(self._dimensionality):
            if len(self._db) == 1:
                runDB = self.make_name(self._db[0])
            else:
                runDB = self.make_name(self._db[angle_index])
            self.log().accumulate('Angle {0}:\n'.format(angle_index+1))
            self.log().accumulate('Direct Beam: {0}\n'.format(runDB))
            directBeamName = runDB + '_direct'
            directForegroundName = directBeamName + '_frg'
            # always process direct beam; even if it can be the same for different angles,
            # the foreground and background regions might be different
            self.process_direct_beam(directBeamName, directForegroundName, angle_index)
            if not self.is_polarized():
                runRB = self.make_name(self._rb[angle_index])
                self.log().accumulate('Reflected Beam: {0}\n'.format(runRB))
                reflectedBeamName = runRB + '_reflected'
                reflectedBeamInput = self.compose_run_string(self._rb[angle_index])
                to_convert_to_q = self.process_reflected_beam(reflectedBeamInput, reflectedBeamName, directBeamName, angle_index)
            else:
                run00_name = self.make_name(self._rb00[angle_index]) + '_00'
                run00_input = self.compose_run_string(self._rb00[angle_index])
                run01_name = self.make_name(self._rb01[angle_index]) + '_01'
                run01_input = self.compose_run_string(self._rb01[angle_index])
                run10_name = self.make_name(self._rb10[angle_index]) + '_10'
                run10_input = self.compose_run_string(self._rb10[angle_index])
                run11_name = self.make_name(self._rb11[angle_index]) + '_11'
                run11_input = self.compose_run_string(self._rb11[angle_index])
                run_names = [run00_name, run01_name, run10_name, run11_name]
                run_inputs = [run00_input, run01_input, run10_input, run11_input]
                foreground_names = []
                for (run, name) in zip(run_inputs, run_names):
                    reflectedPolForegroundWSName = self.process_reflected_beam(run, name, directBeamName, angle_index)
                    foreground_names.append(reflectedPolForegroundWSName)
                to_convert_to_q = self._outWS + '_pol_' + str(angle_index)
                self.polarization_correction(','.join(foreground_names), to_convert_to_q)
                for workspace in mtd[to_convert_to_q]:
                    self._autoCleanup.cleanupLater(workspace.getName())

            convertedToQName = self._outWS + '_' + str(angle_index)
            self.convert_to_momentum_transfer(to_convert_to_q, convertedToQName, directForegroundName, angle_index)
            if scaleFactor != 1:
                Scale(InputWorkspace=convertedToQName, OutputWorkspace=convertedToQName, Factor=scaleFactor)
            to_group.append(convertedToQName)
            self._autoCleanup.protect(convertedToQName)
            progress.report()

        if len(to_group) > 1:
            try:
                stitched = self._outWS + '_stitched'
                use_manual = self.getProperty(PropertyNames.USE_MANUAL_SCALE_FACTORS).value
                scale_factors = self.getProperty(PropertyNames.MANUAL_SCALE_FACTORS).value
                Stitch1DMany(InputWorkspaces=to_group, OutputWorkspace=stitched, UseManualScaleFactors=use_manual,
                             ManualScaleFactors=scale_factors)
                to_group.append(stitched)
            except RuntimeError as re:
                self.log().warning('Unable to stitch automatically, consider stitching manually: ' + str(re))

        GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=self._outWS)
        self.setProperty(Prop.OUTPUT_WS, self._outWS)
        self._autoCleanup.finalCleanup()
        self.log().flushNotice()

AlgorithmFactory.subscribe(ReflectometryILLAutoProcess)
