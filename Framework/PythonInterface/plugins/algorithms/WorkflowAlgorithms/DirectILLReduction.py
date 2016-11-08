# -*- coding: utf-8 -*-

from mantid.api import AlgorithmFactory, AnalysisDataServiceImpl, DataProcessorAlgorithm, FileAction, FileProperty, ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd, PropertyMode,  WorkspaceProperty
from mantid.kernel import Direct, Direction, IntArrayProperty, StringListValidator, UnitConversion
from mantid.simpleapi import AddSampleLog, CalculateFlatBackground,\
                             CloneWorkspace, ComputeCalibrationCoefVan,\
                             ConvertUnits, CorrectKiKf, CreateSingleValuedWorkspace, CreateWorkspace, DeleteWorkspace, DetectorEfficiencyCorUser, Divide, ExtractMonitors, ExtractSpectra, \
                             FindDetectorsOutsideLimits, FindEPP, GetEiMonDet, GroupWorkspaces, Integration, Load,\
                             MaskDetectors, MedianDetectorTest, MergeRuns, Minus, NormaliseToMonitor, Regroup, Scale
import numpy

CLEANUP_DELETE = 'DeleteIntermediateWorkspaces'
CLEANUP_KEEP   = 'KeepIntermediateWorkspaces'

INDEX_TYPE_DETECTOR_ID     = 'DetectorID'
INDEX_TYPE_SPECTRUM_NUMBER = 'SpectrumNumber'
INDEX_TYPE_WORKSPACE_INDEX = 'WorkspaceIndex'

NORM_METHOD_MONITOR = 'Monitor'
NORM_METHOD_TIME    = 'AcquisitionTime'

PROP_BINNING_Q                        = 'QBinning'
PROP_BINNING_W                        = 'WBinning'
PROP_CD_WORKSPACE                     = 'CadmiumWorkspace'
PROP_CLEANUP_MODE                     = 'Cleanup'
PROP_DIAGNOSTICS_WORKSPACE            = 'DetectorDiagnosticsWorkspace'
PROP_DETECTORS_FOR_EI_CALIBRATION     = 'IncidentEnergyCalibrationDetectors'
PROP_DO_DETECTOR_DIAGNOSTICS          = 'UseDetectorDiagnostics'
PROP_EC_WORKSPACE                     = 'EmptyCanWorkspace'
PROP_EPP_WORKSPACE                    = 'EPPWorkspace'
PROP_FLAT_BACKGROUND_SCALING          = 'FlatBackgroundScaling'
PROP_FLAT_BACKGROUND_WINDOW           = 'FlatBackgroundAveragingWindow'
PROP_FLAT_BACKGROUND_WORKSPACE        = 'FlatBackgroundWorkspace'
PROP_INDEX_TYPE                       = 'IndexType'
PROP_INPUT_FILE                       = 'InputFile'
PROP_INPUT_WORKSPACE                  = 'InputWorkspace'
PROP_MONITOR_EPP_WORKSPACE            = 'MonitorEPPWorkspace'
PROP_MONITOR_INDEX                    = 'MonitorIndex'
PROP_NORMALISATION                    = 'Normalisation'
PROP_OUTPUT_DIAGNOSTICS_WORKSPACE     = 'OutputDetectorDiagnosticsWorkspace'
PROP_OUTPUT_EPP_WORKSPACE             = 'OutputEPPWorkspace'
PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE = 'OutputFlatBackgroundWorkspace'
PROP_OUTPUT_MONITOR_EPP_WORKSPACE     = 'OutputMonitorEPPWorkspace'
PROP_OUTPUT_PREFIX                    = 'OutputPrefix'
PROP_OUTPUT_WORKSPACE                 = 'OutputWorkspace'
PROP_REDUCTION_TYPE                   = 'ReductionType'
PROP_TRANSMISSION                     = 'Transmission'
PROP_USER_MASK                        = 'MaskedDetectors'
PROP_VANADIUM_WORKSPACE               = 'VanadiumWorkspace'

REDUCTION_TYPE_CD = 'Empty can/cadmium'
REDUCTION_TYPE_EC = REDUCTION_TYPE_CD
REDUCTION_TYPE_SAMPLE = 'Sample'
REDUCTION_TYPE_VANADIUM = 'Vanadium'

temp_workspaces = []

def namelogging(method):
    '''
    Decorator used within the NameSource class.
    '''
    def wrapper(self):
        name = method(self)
        self._names.add(name)
    return wrapper

# Name generator for temporary workspaces.
# TODO We may want to hide these and delete afterwards.
class NameSource:

    def __init__(self, prefix):
        self._names = set()
        self._prefix = prefix

    @namelogging
    def background(self):
        return self._prefix + '_bkg'

    @namelogging
    def backgroundSubtracted(self):
        return self._prefix + '_bkgsubtr'

    @namelogging
    def badDetectors(self):
        return self._prefix + '_alldiagn'

    @namelogging
    def badDetectorSpuriousBkg(self):
        return self._prefix + '_bkgdiang'

    @namelogging
    def badDetectorEPPFails(self):
        return self._prefix + '_eppfaildiagn'

    @namelogging
    def badDetectorZeroCounts(self):
        return self._prefix + '_zerocountdiagn'

    @namelogging
    def detectorEfficiencyCorrected(self):
        return self._prefix + '_deteff'

    @namelogging
    def ecSubtracted(self):
        return self._prefix + '_ecsubtr'

    @namelogging
    def eiCalibrated(self):
        return self._prefix + '_ecalib'

    @namelogging
    def energyConverted(self):
        return self._prefix +'_econv'

    @namelogging
    def epp(self):
        return self._prefix + '_epp'

    def getNames(self):
        '''
        Returns a set of all names already generated.
        '''
        return self._names

    @namelogging
    def incident(self):
        return self._prefix + '_ie'

    @namelogging
    def kikf(self):
        return self._prefix + '_kikf'

    @namelogging
    def masked(self):
        return self._prefix + '_masked'

    @namelogging
    def monitor(self):
        return self._prefix + '_monepp'

    @namelogging
    def monitor(self):
        return self._prefix + '_monitors'

    @namelogging
    def normalised(self):
        return self._prefix + '_norm'

    @namelogging
    def raw(self):
        return self._prefix + '_raw'

    @namelogging
    def rebinned(self):
        return self._prefix + '_rebinned'

    @namelogging
    def vanadium(self):
        return self._prefix + '_vnorm'

def guessIncidentEnergyWorkspaceName(eppWorkspace):
    # This can be considered a bit of a hack.
    splits = eppWorkspace.getName().split('_')
    return ''.join(splits[:-1]) + '_ie'

def setAsBad(ws, index):
    ws.dataY(index)[0] += 1

class DirectILLReduction(DataProcessorAlgorithm):

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        return 'Workflow\\Inelastic'

    def name(self):
        return 'DirectILLReduction'

    def summary(self):
        return 'Data reduction workflow for the direct geometry time-of-flight spectrometers at ILL'

    def version(self):
        return 1

    def PyExec(self):
        reductionType = self.getProperty(PROP_REDUCTION_TYPE).value
        workspaceNamePrefix = self.getProperty(PROP_OUTPUT_PREFIX).value
        cleanupMode = self.getProperty(PROP_CLEANUP_MODE).value
        if cleanupMode == CLEANUP_DELETE:
            workspaceNamePrefix = '__' + workspaceNamePrefix
        workspaceNames = NameSource(workspaceNamePrefix)
        indexType = self.getProperty(PROP_INDEX_TYPE).value

        if self.getProperty(PROP_INPUT_FILE).value:
            # Load data
            inputFilename = self.getProperty(PROP_INPUT_FILE).value
            outWs = workspaceNames.raw()
            # The variable 'workspace' shall hold the current 'main' data
            # throughout the algorithm.
            workspace = Load(Filename=inputFilename,
                             OutputWorkspace=outWs)

            # Now merge the loaded files (if required)
            workspace = MergeRuns(workspace)

        elif self.getProperty(PROP_INPUT_WORKSPACE).value:
            workspace = self.getProperty(PROP_INPUT_WORKSPACE).value

        # Extract monitors to a separate workspace
        workspace, monitorWorkspace = ExtractMonitors(workspace)

        monitorIndex = self.getProperty(PROP_MONITOR_INDEX).value
        monitorIndex = self._convertToWorkspaceIndex(monitorIndex, monitorWorkspace)

        # Fit time-independent background
        # ATM monitor background is ignored
        bkgInWs = self.getProperty(PROP_FLAT_BACKGROUND_WORKSPACE).value
        bkgOutWs = self.getPropertyValue(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE)
        # Fit background regardless of where it actually comes from.
        if bkgOutWs or not bkgInWs:
            if not bkgOutWs:
                bkgOutWs = workspaceNames.background()
            bkgWindow = self.getProperty(PROP_FLAT_BACKGROUND_WINDOW).value
            bkgScaling = self.getProperty(PROP_FLAT_BACKGROUND_SCALING).value
            bkgWorkspace = CalculateFlatBackground(InputWorkspace=workspace,
                                                   OutputWorkspace=bkgOutWs,
                                                   Mode='Moving Average',
                                                   OutputMode='ReturnBackground',
                                                   SkipMonitors=False,
                                                   NullifyNegativeValues=False,
                                                   AveragingWindowWidth=bkgWindow)
            bkgWorkspace = Scale(InputWorkspace = bkgWorkspace,
                                 OutputWorkspace = bkgWorkspace,
                                 Factor = bkgScaling)
            if self.getProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE).value:
                self.setProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE, bkgOutWs)
        if bkgInWs:
            bkgWorkspace = self.getProperty(PROP_FLAT_BACKGROUND_WORKSPACE).value
            if bkgWorkspace and self.getProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE).value:
                self.setProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE, bkgOutWs)
        # Subtract time-independent background
        outWs = workspaceNames.backgroundSubtracted()
        workspace = Minus(LHSWorkspace=workspace,
                          RHSWorkspace=bkgWorkspace,
                          OutputWorkspace=outWs)

        # Find elastic peak positions
        eppInWs = self.getProperty(PROP_EPP_WORKSPACE).value
        monitorEppInWs = self.getProperty(PROP_MONITOR_EPP_WORKSPACE).value
        eppOutWs = self.getPropertyValue(PROP_OUTPUT_EPP_WORKSPACE)
        monitorEppOutWs = self.getPropertyValue(PROP_OUTPUT_MONITOR_EPP_WORKSPACE)
        # Same as with time-independent backgrounds: the EPP table may
        # came from outside. Otherwise make our own.
        if eppOutWs or not eppInWs:
            if not eppOutWs:
                eppOutWs = workspaceNames.epp()
            if not monitorEppOutWs:
                monitorEppOutWs = workspaceNames.monitorEpp()
            eppWorkspace = FindEPP(InputWorkspace = workspace,
                                   OutputWorkspace = eppOutWs)
            monitorEppWorkspace = FindEPP(InputWorkspace = monitorWorkspace,
                                          OutputWorkspace = monitorEppOutWs)
            if self.getPropertyValue(PROP_OUTPUT_EPP_WORKSPACE):
                self.setProperty(PROP_OUTPUT_EPP_WORKSPACE, eppWorkspace)
            if self.getPropertyValue(PROP_OUTPUT_MONITOR_EPP_WORKSPACE):
                self.setProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE, monitorEppWorkspace)
        if eppInWs:
            eppWorkspace = self.getProperty(PROP_EPP_WORKSPACE).value
            monitorEppWorkspace = self.getProperty(PROP_MONITOR_EPP_WORKSPACE).value
            if not eppOutWs:
                # In any case, some output is required.
                eppOutWs = "dummy_output"
                emptyOutput = CreateSingleValuedWorkspace(OutputWorkspace=bkgOutWs,
                                                          DataValue = 0)
                self.setProperty(PROP_OUTPUT_EPP_WORKSPACE, eppOutWs)
                self.setProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE, eppOutWs)

        # Detector diagnostics, if requested.
        if self.getProperty(PROP_DO_DETECTOR_DIAGNOSTICS).value:
            diagnosticsWs = self.getProperty(PROP_DIAGNOSTICS_WORKSPACE).value
            # No input diagnostics workspace? Diagnose!
            if not diagnosticsWs:
                # 1. Detectors with zero counts.
                outWsName = workspaceNames.badDetector()
                diagnosticsWs, nFailures = FindDetectorsOutsideLimits(InputWorkspace=workspace,
                                                                      OutputWorkspace=outWsName)
                # 2. Detectors where FindEPP failed.
                # TODO Should this be stored in a separate temp ws for debuggin?
                for i in range(diagnosticsWs.getNumberHistograms()):
                    if eppWorkspace.cell('FitStatus', i) != 'success':
                        setAsBad(diagnosticsWs, i)
                # 3. Detectors with high background
                outWsName = workspaceNames.badDetectorSpuriousBkg()
                bkgDiagnostics, nFailures = MedianDetectorTest(InputWorkspace=bkgWorkspace,
                                                               OutputWorkspace=outWsName)
                for i in range(diagnosticsWs.getNumberHistograms()):
                    if bkgDiagnostics.readY(i)[0] == 0:
                        setAsBad(diagnosticsWs, i)
            # Mask detectors identified as bad.
            outWsName = workspaceNames.masked()
            workspace = CloneWorkspace(InputWorkspace=workspace,
                                       OutputWorkspace=outWsName)
            MaskDetectors(Workspace=workspace,
                          MaskedWorkspace=badDetWorkspace)
            # Save output if desired.
            diagnosticsOutWs = self.getProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE).value
            if diagnosticsOutWs:
                self.setProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE, diagnosticsWs)
        # Apply user mask.
        userMask = self.getProperty(PROP_USER_MASK).value
        MaskDetectors(Workspace=workspace,
                      DetectorList=userMask)


        # Get calibrated incident energy from somewhere
        # It should come from the same place as the epp workspace.
        # Or should it?
        instrument = workspace.getInstrument().getName()
        eiCalibrationDets = self.getProperty(PROP_DETECTORS_FOR_EI_CALIBRATION).value
        if instrument in ['IN4', 'IN6']  and eiCalibrationDets:
            eiWsName = guessIncidentEnergyWorkspaceName(eppWorkspace)
            if not AnalysisDataServiceImpl.Instance().doesExist(eiWsName):
                # TODO this should go into the LoadILL algorithm.
                instrument = workspace.getInstrument().getName()
                if instrument == 'IN4':
                    fermiChopperSpeed = workspace.run().getLogData('FC.rotation_speed').value
                    backgroundChopper1Speed = workspace.run().getLogData('BC1.rotation_speed').value
                    backgroundChopper2Speed = workspace.run().getLogData('BC2.rotation_speed').value
                    if abs(backgroundChopper1Speed - backgroundChopper2Speed) > 1:
                        raise RuntimeError('background choppers 1 and 2 have different speeds')
                    n = fermiChopperSpeed / backgroundChopper1Speed / 4
                    pulseInterval = 60.0 / (2 * fermiChopperSpeed) * n
                elif instrument == 'IN6':
                    fermiChopperSpeed = workspace.run().getLogData('Fermi.rotation_speed').value
                    suppressorSpeed = workspace.run().getLogData('Suppressor.rotation_speed').value
                    n = fermiChopperSpeed / suppressorSpeed
                    pulseInterval = 60.0 / (2 * fermiChopperSpeed) * n
                energy = GetEiMonDet(DetectorWorkspace=workspace,
                                     DetectorEPPTable=eppWorkspace,
                                     IndexType=indexType,
                                     Detectors=eiCalibrationDets,
                                     MonitorWorkspace=monitorWorkspace,
                                     MonitorEppTable=monitorEppWorkspace,
                                     Monitor=self.getProperty(PROP_MONITOR_INDEX).value,
                                     PulseInterval=pulseInterval)
                eiWsName = workspaceNames.incidentEnergy()
                CreateSingleValuedWorkspace(OutputWorkspace=eiWsName,
                                            DataValue=energy)
            # Update incident energy
            energy = mtd[eiWsName].readY(0)[0]
            outWs = workspaceNames.eiCalibrated()
            workspace = CloneWorkspace(InputWorkspace=workspace,
                                       OutputWorkspace=outWs)
            AddSampleLog(Workspace=workspace,
                         LogName='Ei',
                         LogText=str(energy),
                         LogType='Number',
                         NumberType='Double',
                         LogUnit='meV')
            wavelength = UnitConversion.run('Energy', 'Wavelength', energy, 0, 0, 0, Direct, 5)
            AddSampleLog(Workspace=workspace,
                         Logname='wavelength',
                         LogText=str(wavelength),
                         LogType='Number',
                         NumberType='Double',
                         LogUnit='Ångström')
        else:
            self.log().information('Skipping incident energy calibration for ' + instrument)

        # Normalisation to monitor/time
        normalisationMethod = self.getProperty(PROP_NORMALISATION).value
        if normalisationMethod:
            if normalisationMethod == NORM_METHOD_MONITOR:
                outWs = workspaceNames.normalised()
                eppRow = monitorEppWorkspace.row(monitorIndex)
                sigma = eppRow['Sigma']
                centre = eppRow['PeakCentre']
                begin = centre - 3 * sigma
                end = centre + 3 * sigma
                workspace, normFactor = NormaliseToMonitor(InputWorkspace=workspace,
                                                           OutputWorkspace=outWs,
                                                           MonitorWorkspace=monitorWorkspace,
                                                           MonitorWorkspaceIndex=monitorIndex,
                                                           IntegrationRangeMin=begin,
                                                           IntegrationRangeMax=end)
            elif normalisationMethod == NORM_METHOD_TIME:
                outWs = workspaceNames.normalised()
                tempWsName = '__actual_time_for_' + outWs
                time = CreateSingleValuedWorkspace(OutputWorkspace=tempWsName,
                                                   DataValue=inWs.getLogData('actual_time').value)
                workspace = Divide(LHSWorkspace=workspace,
                                   RHSWorkspace=tempWsName,
                                   OutputWorkspace=outWs)
                DeleteWorkspace(Workspace=time)
            else:
                raise RuntimeError('Unknonwn normalisation method ' + normalisationMethod)

        # Reduction for empty can and cadmium ends here.
        if reductionType == REDUCTION_TYPE_CD or reductionType == REDUCTION_TYPE_EC:
            self._finalize(workspace, workspaceNames)
            return

        # Continuing with vanadium and sample reductions.

        # Empty can subtraction
        ecWs = self.getProperty(PROP_EC_WORKSPACE).value
        if ecWs:
            outWs = workspaceNames.ecSubtracted()
            cdWs = self.getProperty(PROP_CD_WORKSPACE).value
            transmission = self.getProperty(PROP_TRANSMISSION).value
            tempWsName1 = '__transmission_for_' + outWs
            transmission = CreateSingleValuedWorkspace(OutputWorkspace=tempWsName1,
                                                       DataValue=transmission)
            tempWsName2 = '__input_minus_Cd_for_' + outWs
            tempWsName3 = '__EC_minus_Cd_for_' + outWs
            # If Cd, calculate
            # out = (in - Cd) / transmission - (EC - Cd)
            if cdWs:
                workspace = Minus(LHSWorkspace=workspace,
                                  RHSWorkspace=cdWs,
                                  OutputWorkspace=tempWsName2)
                ecWs = Minus(LHSWorkspace=ecWs,
                             RHSWorkspace=cdWs,
                             OutputWorkspace=tempWsNamw3)
                workspace = Divide(LHSWorkspace=workspace,
                                   RHSWorkspace=transmission,
                                   OutputWorkspace=outWs)
                workspace = Minus(LHSWorkspace=workspace,
                                  RHSWorkspace=ecWs,
                                  OutputWorkspace=workspace)
                # Cleanup
                DeleteWorkspace(Workspace=tempWsName1)
                DeleteWorkspace(Workspace=tempWsName2)
                DeleteWorkspace(Workspace=tempWsName3)
            # If no Cd, calculate
            # out = in - transmission * EC
            else:
                ecWs = Multiply(LHSWorkspace=ecWs,
                                RHSWorkspace=transmission,
                                OutputWorkspace=ecWs)
                workspace = Minus(LHSWorkspace=workspace,
                                  RHSWorkspace=inEC,
                                  OutputWorkspace=workspace)

        # Reduction for vanadium ends here.
        if reductionType == REDUCTION_TYPE_VANADIUM:
            # We output an integrated vanadium, ready to be used for
            # normalization.
            outWs = self.getPropertyValue(PROP_OUTPUT_WORKSPACE)
            # TODO For the time being, we may just want to integrate
            # the vanadium data as `ComputeCalibrationCoef` does not do
            # the best possible Debye-Waller correction.
            workspace = ComputeCalibrationCoefVan(VanadiumWorkspace=workspace,
                                                  EPPTable=eppWorkspace,
                                                  OutputWorkspace=outWs)
            self._finalize(workspace, workspaceNames)
            return

        # Continuing with sample reduction.

        # Vanadium normalisation
        vanadiumNormFactors = self.getProperty(PROP_VANADIUM_WORKSPACE).value
        if vanadiumNormFactors:
            outWs = workspaceNames.vanadiumNormalised()
            workspace = Divide(LHSWorkspace=workspace,
                               RHSWorkspace=vanadiumNormFactors,
                               OutputWorkspace=outWs)

        # Convert units from TOF to energy
        outWs = workspaceNames.energyConverted()
        workspace = ConvertUnits(InputWorkspace = workspace,
                                 OutputWorkspace = outWs,
                                 Target = 'DeltaE',
                                 EMode = 'Direct')

        # KiKf conversion
        outWs = workspaceNames.kikfConverted()
        workspace = CorrectKiKf(InputWorkspace = workspace,
                                OutputWorkspace = outWs)

        # Regrouping
        # TODO automatize binning in w. Do we need regrouping in q as well?
        params = self.getProperty(PROP_BINNING_W).value
        if params:
            outWs = workspaceNames.rebinned()
            workspace = Regroup(InputWorkspace = workspace,
                                OutputWorkspace = outWs,
                                Params = params)

        # Detector efficiency correction
        outWs = workspaceNames.detectorEfficiencyCorrected()
        workspace = DetectorEfficiencyCorUser(InputWorkspace = workspace,
                                              OutputWorkspace = outWs)

        # TODO Self-shielding corrections

        self._finalize(workspace, workspaceNames)

    def PyInit(self):
        # TODO Property validation.
        # Inputs
        self.declareProperty(FileProperty(PROP_INPUT_FILE,
                                          '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['nxs']))
        self.declareProperty(MatrixWorkspaceProperty(PROP_INPUT_WORKSPACE,
                                                     '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input))
        self.declareProperty(PROP_OUTPUT_PREFIX,
                             '',
                             direction=Direction.Input,
                             doc='String to use as prefix in output workspace names')
        
        self.declareProperty(PROP_CLEANUP_MODE,
                             CLEANUP_DELETE,
                             validator=StringListValidator([CLEANUP_DELETE, CLEANUP_KEEP]),
                             direction=Direction.Input,
                             doc='Whether or not to clean up intermediate workspaces')
        self.declareProperty(PROP_REDUCTION_TYPE,
                             REDUCTION_TYPE_SAMPLE,
                             validator=StringListValidator([REDUCTION_TYPE_SAMPLE, REDUCTION_TYPE_VANADIUM, REDUCTION_TYPE_CD, REDUCTION_TYPE_EC]),
                             direction=Direction.Input,
                             doc='Type of reduction workflow to be run on ' + PROP_INPUT_FILE)
        self.declareProperty(PROP_NORMALISATION,
                             NORM_METHOD_MONITOR,
                             validator=StringListValidator([NORM_METHOD_MONITOR, NORM_METHOD_TIME]),
                             direction=Direction.Input,
                             doc='Normalisation method')
        self.declareProperty(MatrixWorkspaceProperty(PROP_VANADIUM_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Reduced vanadium workspace')
        self.declareProperty(MatrixWorkspaceProperty(PROP_EC_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Reduced empty can workspace')
        self.declareProperty(MatrixWorkspaceProperty(PROP_CD_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Reduced cadmium workspace')
        self.declareProperty(ITableWorkspaceProperty(PROP_EPP_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Table workspace containing results from the FindEPP algorithm')
        self.declareProperty(ITableWorkspaceProperty(PROP_MONITOR_EPP_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Table workspace containing results from the FindEPP algorithm for the monitor workspace')
        self.declareProperty(PROP_FLAT_BACKGROUND_SCALING,
                             1.0,
                             direction=Direction.Input,
                             doc='Flat background scaling constant')
        self.declareProperty(PROP_FLAT_BACKGROUND_WINDOW,
                             30,
                             direction=Direction.Input,
                             doc='Running average window width (in bins) for flat background')
        self.declareProperty(MatrixWorkspaceProperty(PROP_FLAT_BACKGROUND_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Workspace from which to get flat background data')
        self.declareProperty(PROP_MONITOR_INDEX,
                             0,
                             direction=Direction.Input,
                             doc='Index of the main monitor spectrum.')
        self.declareProperty(IntArrayProperty(PROP_USER_MASK,
                                              '',
                                              direction=Direction.Input),
                             doc='List of spectra to mask')
        self.declareProperty(PROP_DO_DETECTOR_DIAGNOSTICS,
                             True,
                             direction=Direction.Input,
                             doc='If true, run detector diagnostics or apply ' + PROP_DIAGNOSTICS_WORKSPACE)
        self.declareProperty(MatrixWorkspaceProperty(PROP_DIAGNOSTICS_WORKSPACE,
                                                     '',
                                                     Direction.Input,
                                                     PropertyMode.Optional),
                             doc='Detector diagnostics workspace obtained from another reduction run.')
        self.declareProperty(PROP_TRANSMISSION,
                             1.0,
                             direction=Direction.Input,
                             doc='Sample transmission for empty can subtraction')
        self.declareProperty(PROP_BINNING_Q,
                             '',
                             direction=Direction.Input,
                             doc='Rebinning in q')
        self.declareProperty(PROP_BINNING_W,
                             '',
                             direction=Direction.Input,
                             doc='Rebinning in w')
        self.declareProperty(PROP_DETECTORS_FOR_EI_CALIBRATION,
                             '',
                             direction=Direction.Input,
                             doc='List of detectors used for the incident energy calibration')
        self.declareProperty(PROP_INDEX_TYPE,
                             INDEX_TYPE_WORKSPACE_INDEX,
                             direction=Direction.Input,
                             doc='Type of numbers in ' + PROP_MONITOR_INDEX + ' and ' + PROP_DETECTORS_FOR_EI_CALIBRATION + ' properties')
        # Output
        self.declareProperty(ITableWorkspaceProperty(PROP_OUTPUT_EPP_WORKSPACE,
                             '',
                             direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace for elastic peak positions')
        self.declareProperty(ITableWorkspaceProperty(PROP_OUTPUT_MONITOR_EPP_WORKSPACE,
                             '',
                             direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace for elastic peak positions')
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE,
                             '',
                             direction=Direction.Output,
                             optional=PropertyMode.Optional),
                             doc='Output workspace for flat background')
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_DIAGNOSTICS_WORKSPACE,
                                               '',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output workspace for detector diagnostics')
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_WORKSPACE,
                             '',
                             direction=Direction.Output),
                             doc='The output of the algorithm')

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        fileGiven = not self.getProperty(PROP_INPUT_FILE).isDefault()
        wsGiven = not self.getProperty(PROP_INPUT_WORKSPACE).isDefault()
        # Validate an input exists
        if fileGiven == wsGiven:
            issues[PROP_INPUT_FILE] = 'Must give either an input file or an input workspace.'

        return issues

    def _convertToWorkspaceIndex(self, i, workspace):
        indexType = self.getProperty(PROP_INDEX_TYPE).value
        if indexType == INDEX_TYPE_WORKSPACE_INDEX:
            return i
        elif indexType == INDEX_TYPE_SPECTRUM_NUMBER:
            return workspace.getIndexFromSpectrumNumber(i)
        else: # INDEX_TYPE_DETECTOR_ID
            for j in len(workspace.getNumberHistograms()):
                if workspace.getSpectrum(j).hasDetectorID(i):
                    return j
            raise RuntimeError('No workspace index found for detector id ' + i)

    def _finalize(self, outputWorkspace, workspaceNames):
        self.setProperty(PROP_OUTPUT_WORKSPACE, outputWorkspace)

        if self.getProperty(PROP_CLEANUP_MODE).value == CLEANUP_DELETE:
            # TODO: what can we delete earlier from this set?
            for ws in workspaceNames.getNames():
                if mtd.doesExist(ws):
                    DeleteWorkspace(Workspace = ws)

        self.log().debug('Finished')

AlgorithmFactory.subscribe(DirectILLReduction)
