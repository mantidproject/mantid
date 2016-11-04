# -*- coding: utf-8 -*-

from mantid.api import AlgorithmFactory, AnalysisDataServiceImpl, DataProcessorAlgorithm, FileAction, FileProperty, ITableWorkspaceProperty, MatrixWorkspaceProperty, mtd, PropertyMode,  WorkspaceProperty
from mantid.kernel import Direct, Direction, IntArrayProperty, StringListValidator, UnitConversion
from mantid.simpleapi import AddSampleLog, CalculateFlatBackground,\
                             CloneWorkspace, ComputeCalibrationCoefVan,\
                             ConvertUnits, CorrectKiKf, CreateSingleValuedWorkspace, CreateWorkspace, DeleteWorkspace, DetectorEfficiencyCorUser, Divide, ExtractMonitors, ExtractSpectra, \
                             FindDetectorsOutsideLimits, FindEPP, GetEiMonDet, GroupWorkspaces, Integration, Load,\
                             MaskDetectors, MergeRuns, Minus, NormaliseToMonitor, Rebin, Scale
import numpy

INDEX_TYPE_DETECTOR_ID     = 'DetectorID'
INDEX_TYPE_SPECTRUM_NUMBER = 'SpectrumNumber'
INDEX_TYPE_WORKSPACE_INDEX = 'WorkspaceIndex'

NORM_METHOD_MONITOR = 'Monitor'
NORM_METHOD_TIME    = 'AcquisitionTime'

PROP_BINNING_Q                        = 'QBinning'
PROP_BINNING_W                        = 'WBinning'
PROP_CD_WORKSPACE                     = 'CadmiumWorkspace'
PROP_CONTROL_MODE                     = 'ControlMode'
PROP_DETECTORS_FOR_EI_CALIBRATION     = 'IncidentEnergyCalibrationDetectors'
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
PROP_OUTPUT_EPP_WORKSPACE             = 'OutputEPPWorkspace'
PROP_OUTPUT_FLAT_BACKGROUND_WORKSPACE = 'OutputFlatBackgroundWorkspace'
PROP_OUTPUT_MONITOR_EPP_WORKSPACE     = 'OutputMonitorEPPWorkspace'
PROP_OUTPUT_SUFFIX                    = 'OutputPrefix'
PROP_OUTPUT_WORKSPACE                 = 'OutputWorkspace'
PROP_REDUCTION_TYPE                   = 'ReductionType'
PROP_TRANSMISSION                     = 'Transmission'
PROP_USER_MASK                        = 'SpectrumMask'
PROP_VANADIUM_WORKSPACE               = 'VanadiumWorkspace'

REDUCTION_TYPE_CD = 'Empty can/cadmium'
REDUCTION_TYPE_EC = REDUCTION_TYPE_CD
REDUCTION_TYPE_SAMPLE = 'Sample'
REDUCTION_TYPE_VANADIUM = 'Vanadium'

temp_workspaces = []

# Name generators for temporary workspaces.
# TODO We may want to hide these and delete afterwards.
def backgroundWorkspaceName(token):
    return token + '_bkg'

def backgroundSubtractedWorkspaceName(token):
    return token + '_bkgsubtr'

def badDetectorWorkspaceName(token):
    return token + '_mask'

def detectorEfficiencyCorrectedWorkspaceName(token):
    return token + '_deteff'

def ecSubtractedWorkspaceName(token):
    return token + '_ecsubtr'

def eiCalibratedWorkspaceName(token):
    return token + '_ecalib'

def energyConvertedWorkspaceName(token):
    return token +'_econv'

def eppWorkspaceName(token):
    return token + '_epp'

def incidentEnergyWorkspaceName(token):
    return token + '_ie'

def kikfConvertedWorkspaceName(token):
    return token + '_kikf'

def maskedWorkspaceName(token):
    return token + '_masked'

def monitorEppWorkspaceName(token):
    return token + '_monepp'

def monitorWorkspaceName(token):
    return token + '_monitors'

def normalisedWorkspaceName(token):
    return token + '_norm'

def rawWorkspaceName(token):
    return token + '_raw'

def rebinnedWorkspaceName(token):
    return token + '_rebinned'

def vanadiumNormalisedWorkspaceName(token):
    return token + '_vnorm'

def guessIncidentEnergyWorkspaceName(eppWorkspace):
    # This can be considered a bit of a hack.
    splits = eppWorkspace.getName().split('_')
    return ''.join(splits[:-1]) + '_ie'

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
        identifier = self.getProperty(PROP_OUTPUT_SUFFIX).value
        indexType = self.getProperty(PROP_INDEX_TYPE).value

        if self.getProperty(PROP_INPUT_FILE).value:
            # Load data
            inputFilename = self.getProperty(PROP_INPUT_FILE).value
            outWs = rawWorkspaceName(identifier)
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
                bkgOutWs = backgroundWorkspaceName(identifier)
            bkgWindow = self.getProperty(PROP_FLAT_BACKGROUND_WINDOW).value
            bkgScaling = self.getProperty(PROP_FLAT_BACKGROUND_SCALING).value
            bkgWorkspace = self._calculateFlatBackground(workspace, bkgOutWs, bkgWindow)
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
        outWs = backgroundSubtractedWorkspaceName(identifier)
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
                eppOutWs = eppWorkspaceName(identifier)
            if not monitorEppOutWs:
                monitorEppOutWs = monitorEppWorkspaceName(identifier)
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

        # Identify bad detectors & include user mask
        userMask = self.getProperty(PROP_USER_MASK).value
        outWs = badDetectorWorkspaceName(identifier)
        badDetWorkspace, nFailures = FindDetectorsOutsideLimits(InputWorkspace=workspace,
                                                                OutputWorkspace=outWs)
        def mask(maskWs, i):
            maskWs.setY(i, numpy.array([maskWs.readY(i)[0] + 1.0]))
        for i in range(badDetWorkspace.getNumberHistograms()):
            if eppWorkspace.cell('FitStatus', i) != 'success':
                mask(badDetWorkspace, i)
            if i in userMask:
                mask(badDetWorkspace, i)
        # Mask detectors
        outWs = maskedWorkspaceName(identifier)
        workspace = CloneWorkspace(InputWorkspace=workspace,
                                   OutputWorkspace=outWs)
        MaskDetectors(Workspace=workspace,
                      MaskedWorkspace=badDetWorkspace)

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
                eiWsName = incidentEnergyWorkspaceName(identifier)
                CreateSingleValuedWorkspace(OutputWorkspace=eiWsName,
                                            DataValue=energy)
            # Update incident energy
            energy = mtd[eiWsName].readY(0)[0]
            outWs = eiCalibratedWorkspaceName(identifier)
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
                outWs = normalisedWorkspaceName(identifier)
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
                outWs = normalisedWorkspaceName(identifier)
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
            self._finalize(workspace)
            return

        # Continuing with vanadium and sample reductions.

        # Empty can subtraction
        ecWs = self.getProperty(PROP_EC_WORKSPACE).value
        if ecWs:
            outWs = ecSubtractedWorkspaceName(identifier)
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
            self._finalize(workspace)
            return

        # Continuing with sample reduction.

        # Vanadium normalisation
        vanadiumNormFactors = self.getProperty(PROP_VANADIUM_WORKSPACE).value
        if vanadiumNormFactors:
            outWs = vanadiumNormalisedWorkspaceName(identifier)
            workspace = Divide(LHSWorkspace=workspace,
                               RHSWorkspace=vanadiumNormFactors,
                               OutputWorkspace=outWs)

        # Convert units from TOF to energy
        outWs = energyConvertedWorkspaceName(identifier)
        workspace = ConvertUnits(InputWorkspace = workspace,
                                 OutputWorkspace = outWs,
                                 Target = 'DeltaE',
                                 EMode = 'Direct')

        # KiKf conversion
        outWs = kikfConvertedWorkspaceName(identifier)
        workspace = CorrectKiKf(InputWorkspace = workspace,
                                OutputWorkspace = outWs)

        # Rebinning
        # TODO automatize binning in w. Do we need rebinning in q as well?
        params = self.getProperty(PROP_BINNING_W).value
        if params:
            outWs = rebinnedWorkspaceName(identifier)
            workspace = Rebin(InputWorkspace = workspace,
                              OutputWorkspace = outWs,
                              Params = params)

        # Detector efficiency correction
        outWs = detectorEfficiencyCorrectedWorkspaceName(identifier)
        workspace = DetectorEfficiencyCorUser(InputWorkspace = workspace,
                                              OutputWorkspace = outWs)

        # TODO Self-shielding corrections

        self._finalize(workspace)

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
        self.declareProperty(PROP_OUTPUT_SUFFIX,
                             '',
                             direction=Direction.Input,
                             doc='String to use as postfix in output workspace names')
        self.declareProperty(PROP_CONTROL_MODE,
                             False,
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
        self.declareProperty(WorkspaceProperty(PROP_OUTPUT_WORKSPACE,
                             '',
                             direction=Direction.Output),
                             doc='The output of the algorithm')

    def validateInputs(self):
        """
        Checks for issues with user input.
        """
        issues = dict()

        # Validate an input exists
        if not self.getProperty(PROP_INPUT_FILE).value and not self.getProperty(PROP_INPUT_WORKSPACE).value:
            issues[PROP_INPUT_FILE] = 'Must give either an input file or an input workspace.'

        return issues

    def _calculateFlatBackground(self, ws, bkgWsName, window):
        # TODO this functionality should be moved to CalculateFlatBackground
        # algorithm some day
        bkgWs = CloneWorkspace(InputWorkspace=ws,OutputWorkspace=bkgWsName)
        for i in range(ws.getNumberHistograms()):
            bkgWs.dataY(i).fill(self._runningMeanMinimum(ws.readY(i), window))
        return bkgWs

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

    def _finalize(self, outputWorkspace):
        self.setProperty(PROP_OUTPUT_WORKSPACE, outputWorkspace)

        if not self.getProperty(PROP_CONTROL_MODE).value:
            token = self.getProperty(PROP_OUTPUT_SUFFIX).value
            # TODO: what can we delete earlier from this list?
            for ws in [backgroundWorkspaceName(token),
                       backgroundSubtractedWorkspaceName(token),
                       badDetectorWorkspaceName(token),
                       detectorEfficiencyCorrectedWorkspaceName(token),
                       ecSubtractedWorkspaceName(token),
                       eiCalibratedWorkspaceName(token),
                       energyConvertedWorkspaceName(token),
                       eppWorkspaceName(token),
                       incidentEnergyWorkspaceName(token),
                       kikfConvertedWorkspaceName(token),
                       maskedWorkspaceName(token),
                       monitorEppWorkspaceName(token),
                       monitorWorkspaceName(token),
                       normalisedWorkspaceName(token),
                       rawWorkspaceName(token),
                       rebinnedWorkspaceName(token),
                       vanadiumNormalisedWorkspaceName(token),
                       'EPPfit_NormalisedCovarianceMatrix',
                       'EPPfit_Parameters',
                       'normFactor',
                       'workspace',
                       'dectectorWorkspace',
                       'monitorWorkspace']:
                if mtd.doesExist(ws):
                    DeleteWorkspace(Workspace = ws)

        self.log().debug('Finished')


    def _runningMeanMinimum(self, x, window):
        # TODO this functionality should be moved to CalculateFlatBackground
        # algorithm some day
        # Does not handle NaNs.
        dx = window / 2 + 1
        minimum = numpy.min(x[:dx])
        for i in range(len(x) - window):
            minimum = min(minimum, numpy.mean(x[i:i+window]))
        minEnd = numpy.min(x[-dx:])
        return min(minimum, minEnd)

AlgorithmFactory.subscribe(DirectILLReduction)
