# -*- coding: utf-8 -*-

from mantid.api import AlgorithmFactory, DataProcessorAlgorithm, ITableWorkspaceProperty, WorkspaceGroupProperty
from mantid.kernel import Direction
from mantid.simpleapi import AddSampleLog, CalculateFlatBackground,\
                             CloneWorkspace, ComputeCalibrationCoefVan,\
                             ConvertUnits, CorrectKiKf, CreateSingleValuedWorkspace, DetectorEfficiencyCorUser, Divide,\
                             FindDetectorsOutsideLimits, FindEPP, GetEiMonDet, GroupWorkspaces, Integration, Load,\
                             MaskDetectors, Minus, NormaliseToMonitor, Rebin
import numpy

#-------------------------------------------------------------------
# Reduction table utilities
#-------------------------------------------------------------------
BACKGROUND_FIT_BEGIN     = 'BackgroundFitBegin'
BACKGROUND_FIT_END       = 'BackgroundFitEnd'
BACKGROUND_FROM          = 'BackgroundFrom'
CD_FROM                  = 'CdFrom'
EC_FROM                  = 'ECFrom'
EI_CALIBRATION_DETECTORS = 'EiCalibrationDetectors'
EPP_FROM                 = 'EppFrom'
FILENAME                 = 'Filename'
HARD_MASK                = 'HardMask'
ID                       = 'Id'
MONITOR_INDEX            = 'MonitorIndex'
NORMALISATION            = 'Normalisation'
TRANSMISSION             = 'Transmission'
TYPE                     = 'Type'
VANADIUM_FROM            = 'VanadiumFrom'
X_REBINNING_PARAMS       = 'wRebinning'
Y_REBINNING_PARAMS       = 'qRebinning'

def createReductionTable():
    STRING = 'str'
    DOUBLE = 'double'
    INT    = 'int'
    t = WorkspaceFactoryImpl.Instance().createTable()
    t.addColumn(STRING, FILENAME)
    t.addColumn(STRING, ID)
    t.addColumn(STRING, TYPE)
    t.addColumn(INT, MONITOR_INDEX)
    t.addColumn(STRING, BACKGROUND_FROM)
    t.addColumn(DOUBLE, BACKGROUND_FIT_BEGIN)
    t.addColumn(DOUBLE, BACKGROUND_FIT_END)
    t.addColumn(STRING, HARD_MASK)
    t.addColumn(STRING, EPP_FROM)
    t.addColumn(STRING, EI_CALIBRATION_DETECTORS)
    t.addColumn(STRING, NORMALISATION)
    t.addColumn(STRING, EC_FROM)
    t.addColumn(STRING, CD_FROM)
    t.addColumn(DOUBLE, TRANSMISSION)
    t.addColumn(STRING, VANADIUM_FROM)
    t.addColumn(STRING, X_REBINNING_PARAMS)
    t.addColumn(STRING, Y_REBINNING_PARAMS)
    return t


#-----------------------------------------------------------------------
# Reduction step method decorations
#-----------------------------------------------------------------------

def cachedResult(method):
    def wrapper(self):
        if not hasattr(self, '_out'):
            self._out = method(self)
        return self._out
    return wrapper

def raiseIfUnitialised(method):
    def wrapper(self):
        if hasattr(self, '_initialised') and self._initialised:
            return method(self)
        raise RuntimeError('call to a method in an uninitialised reduction step')
    return wrapper

#-------------------------------------------------------------------
# Reduction step classes
#-------------------------------------------------------------------

class BadDetectorIdentification:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        out, nFailures = FindDetectorsOutsideLimits(InputWorkspace=inWs,
                                                    OutputWorkspace=self._outWs)
        eppWs = self._inEppStep.process()
        def mask(maskWs, i):
            maskWs.setY(i, numpy.array([maskWs.readY(i)[0] + 1.0]))
        for i in range(out.getNumberHistograms()):
            if eppWs.cell('FitStatus', i) != 'success':
                mask(out, i)
            if i in self._hardMaskIndices:
                mask(out,i)
        return out
    def setup(self,
              inputWorkspaceStep,
              inputEppStep,
              hardMaskWorkspaceIndices,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._inEppStep = inputEppStep
        self._hardMaskIndices = hardMaskWorkspaceIndices
        self._outWs = outputWorkspace
        self._initialised = True

class DetectorEfficiencyCorrection:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        return DetectorEfficiencyCorUser(InputWorkspace = inWs,
                                         OutputWorkspace = self._outWs)
    def setup(self,
              inputWorkspaceStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._outWs = outputWorkspace
        self._initialised = True

class EmptyCanSubtraction:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        inEC = self._inECStep.process()
        tempWsName1 = '__input_minus_Cd_for_' + self._outWs
        tempWsName2 = '__EC_minus_Cd_for_' + self._outWs
        # If no Cd, calculate
        # out = in / transmission - EC
        # If Cd, calculate
        # out = (in - Cd) / transmission - (EC - Cd)
        if inputCdStep:
            inCd = self._inCdStep.process()
            inWs = Minus(LHSWorkspace=inWs,
                        RHSWorkspace=inCd,
                        OutputWorkspace=tempWsName1)
            inEC = Minus(LHSWorkspace=inEC,
                         RHSWorkspace=inCd,
                         OutputWorkspace=tempWsNamw2)
        out = Divide(LHSWorkspace=inWs,
                     RHSWorkspace=self._transmission,
                     OutputWorkspace=self._outWs)
        Minus(LHSWorkspace=out,
              RHSWorkspace=inEC,
              OutputWorkspace=out)
        if inputCdStep:
            # Cleanup
            DeleteWorkspace(Workspace=tempWsName1)
            DeleteWorkspace(Workspace=tempWsName2)
        return out
    def setup(self, 
              inputWorkspaceStep,
              inputECStep,
              inputCdStep,
              transmission,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._inECStep = inputECStep
        self._inCdStep = inputCdStep
        self._transmission = transmission
        self._outWs = outputWorkspace
        self._initialised = True

class EppFitting:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        return FindEPP(InputWorkspace = inWs, 
                       OutputWorkspace = self._outWs)
    def referenceWorkspace(self):
        return self._inStep.process()
    def setup(self,
              inputWorkspaceStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._outWs = outputWorkspace
        self._initialised = True

class FlatBackgroundFitting:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWorkspace = self._inStep.process()
        return CalculateFlatBackground(InputWorkspace=inWorkspace,
                                       OutputWorkspace=self._outWs,
                                       StartX=self._fitBegin,
                                       EndX=self._fitEnd,
                                       OutputMode='Return Background',
                                       NullifyNegativeValues=False)
    def setup(self,
              inputWorkspaceStep,
              outputWorkspace,
              fitRangeBegin,
              fitRangeEnd):
        self._inStep = inputWorkspaceStep
        self._outWs = outputWorkspace
        self._fitBegin = fitRangeBegin
        self._fitEnd = fitRangeEnd
        self._initialised = True

class FlatBackgroundSubtraction:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        background = self._backgroundStep.process()
        out = Minus(LHSWorkspace=inWs,
                    RHSWorkspace=background,
                    OutputWorkspace=self._outWs)
        return out
    def setup(self, 
              inputWorkspaceStep,
              inputBackgroundStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._backgroundStep = inputBackgroundStep
        self._outWs = outputWorkspace
        self._initialised = True

class IncidentEnergyCalibration:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        energy = self._energyStep.process()
        inWs = self._inStep.process()
        out = CloneWorkspace(InputWorkspace=inWs,
                             OutputWorkspace=self._outWs)
        AddSampleLog(Workspace=out,
                     LogName='Ei',
                     LogText=str(energy))
        return out
    def setup(self,
              inputWorkspaceStep,
              recalculationStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._energyStep = recalculationStep
        self._outWs = outputWorkspace
        self._initialised = True

class IncidentEnergyRecalculation:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        detectorEppTable = self._detectorEppStep.process()
        monitorEppTable = self._monitorEppStep.process()
        detectorWs = self._detectorEppStep.referenceWorkspace()
        instrument = detectorWs.getInstrument().getName()
        if instrument == 'IN4':
            fermiChopperSpeed = detectorWs.run().getLogData('FC.rotation_speed').value
            pulseInterval = 60.0 / (2 * fermiChopperSpeed)
        elif instrument == 'IN6':
            raise RuntimeError('pulse interval not implemented for IN6 yet')
        else:
            raise RuntimeError('unknown instrument: ' + instrument)
        return GetEiMonDet(DetectorWorkspace=detectorWs,
                           DetectorEPPTable=detectorEppTable,
                           IndexType='SpectrumNumber',
                           Detectors=self._detectorIndices,
                           MonitorWorkspace=self._monitorEppStep.referenceWorkspace(),
                           MonitorEppTable=monitorEppTable,
                           Monitor=self._monitorIndex,
                           PulseInterval=pulseInterval)
    def setup(self,
              inputDetectorEppStep,
              inputMonitorEppStep,
              detectorWorkspaceIndices,
              monitorWorkspaceIndex):
        self._detectorEppStep = inputDetectorEppStep
        self._detectorIndices = detectorWorkspaceIndices
        self._monitorEppStep = inputMonitorEppStep
        self._monitorIndex = monitorWorkspaceIndex
        self._initialised = True

class KiKfConversion:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        return CorrectKiKf(InputWorkspace = inWs,
                           OutputWorkspace = self._outWs)
    def setup(self, inputWorkspaceStep, outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._outWs = outputWorkspace
        self._initialised = True

class LoadRun:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        return Load(Filename=self._filename,
                    OutputWorkspace=self._outWs)
    def setup(self, filename, outputWorkspace):
        self._filename = filename
        self._outWs = outputWorkspace
        self._initialised = True

class MaskBadDetectors:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        maskWs = self._inMask.process()
        out = CloneWorkspace(InputWorkspace=inWs,
                             OutputWorkspace=self._outWs)
        MaskDetectors(Workspace=out,
                      MaskedWorkspace=maskWs)
        return out
    def setup(self,
              inputWorkspaceStep,
              inputMaskStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._inMask = inputMaskStep
        self._outWs = outputWorkspace
        self._initialised = True

class NormalisationToMonitor:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        eppTable = self._eppStep.process()
        eppRow = eppTable.row(self._monitorIndex)
        sigma = eppRow['Sigma']
        centre = eppRow['PeakCentre']
        integrationBegin = centre - 2 * sigma
        integrationEnd = centre + 2 * sigma
        inWs = self._inStep.process()
        out, normFactor = NormaliseToMonitor(InputWorkspace=inWs,
                                             OutputWorkspace=self._outWs,
                                             MonitorWorkspace=self._eppStep.referenceWorkspace(),
                                             MonitorWorkspaceIndex=self._monitorIndex,
                                             IntegrationRangeMin=integrationBegin,
                                             IntegrationRangeMax=integrationEnd)
        return out
    def setup(self, 
              inputWorkspaceStep,
              inputMonitorEppTableStep,
              monitorWorkspaceIndex,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._eppStep = inputMonitorEppTableStep
        self._monitorIndex = monitorWorkspaceIndex
        self._outWs = outputWorkspace
        self._initialised = True

class NormalisationToTime:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        tempWsName = '__actual_time_for_' + self._outWs
        time = CreateSingleValuedWorkspace(OutputWorkspace=tempWsName,
                                           DataValue=inWs.getLogData('actual_time').value)
        out = Divide(LHSWorkspace=inWs,
                     RHSWorkspace=tempWsName,
                     OutputWorkspace=self._outWs)
        DeleteWorkspace(Workspace=time)
        return out
    def setup(self,
              inputWorkspaceStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._outWs = outputWorkspace
        self._initialised = True

class NormalisationToVanadium:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inNormFactors = self._inNormFactorsStep.process()
        inWs = self._inStep.process()
        out = Divide(LHSWorkspace=inWs,
                     RHSWorkspace=inNormFactors,
                     OutputWorkspace=self._outWs)
        return out
    def setup(self, 
              inputWorkspaceStep,
              inputVanadiumNormalisationFactors,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._inNormFactorsStep = inputVanadiumNormalisationFactors
        self._outWs = outputWorkspace
        self._initialised = True

class Noop:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        return self._inStep.process()
    def setup(self, InputWorkspaceStep):
        self._inStep = InputWorkspaceStep
        self._initialised = True

class RebinningInQandW:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        return Rebin(InputWorkspace = inWs,
                     OutputWorkspace = self._outWs,
                     Params = self._wBinning)
    def setup(self, 
              inputWorkspaceStep,
              qAxisBinning,
              wAxisBinning,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._qBinning = qAxisBinning
        self._wBinning = wAxisBinning
        self._outWs = outputWorkspace
        self._initialised = True

class TOFToEnergyConversion:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        #energy = inWs.run().getLogData('Ei').value
        return ConvertUnits(InputWorkspace = inWs,
                            OutputWorkspace = self._outWs,
                            Target = 'DeltaE',
                            EMode = 'Direct')
    def setup(self, inputWorkspaceStep, outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._outWs = outputWorkspace
        self._initialised = True

class VanadiumIntegration:
    @raiseIfUnitialised
    @cachedResult
    def process(self):
        inWs = self._inStep.process()
        eppTable = self._inEppStep.process()
        out = ComputeCalibrationCoefVan(VanadiumWorkspace=inWs,
                                        EPPTable=eppTable,
                                        OutputWorkspace=self._outWs)
        return out
    def setup(self,
              inputWorkspaceStep,
              inputEppStep,
              outputWorkspace):
        self._inStep = inputWorkspaceStep
        self._inEppStep = inputEppStep
        self._outWs = outputWorkspace
        self._initialised = True

TYPE_ECCD     = 'EC/Cd'
TYPE_SAMPLE   = 'Sample'
TYPE_VANADIUM = 'Vanadium'

STEP_DETECTOR_EFFICIENCY_CORRECTION = 'DetEffCorrection'
STEP_EC_SUBTRACTION                 = 'ecSubtraction'
STEP_EI_CALIBRATION                 = 'eiCalibration'
STEP_EI_RECALCULATION               = 'eiRecalculation'
STEP_EPP                            = 'epp'
STEP_FLAT_BACKGROUND_DATA           = 'flatBkgData'
STEP_FLAT_BACKGORUND_SUBTRACTION    = 'flatBkgSubtraction'
STEP_FINAL                          = 'finalStep'
STEP_IDENTIFY_BAD_DETECTORS         = 'identifyBadDets'
STEP_KIKF_CONVERSION                = 'kikfConversion'
STEP_MASK_BAD_DETECTORS             = 'maskBadDets'
STEP_NORMALISATION                  = 'normalisation'
STEP_RAW                            = 'rawLoading'
STEP_REBINNING                      = 'rebinning'
STEP_TOF_TO_E_CONVERSION            = 'conversionToEnergy'
STEP_VANADIUM_INTEGRATION           = 'vanaIntegration'
STEP_VANADIUM_NORMALISAION          = 'vanaNormalisation'

# Normalisation methods
MONITOR = 'Monitor'
TIME    = 'Time'

def backgroundWorkspaceName(token):
    return 'bkg_' + token

def backgroundSubtractedWorkspaceName(token):
    return 'bkgSubtr_' + token

def badDetectorWorkspaceName(token):
    return 'mask_' + token

def detectorEfficiencyCorrectedWorkspaceName(token):
    return 'detEff_' + token

def ecSubtractedWorkspaceName(token):
    return 'ECSubtr_' + token

def eiCalibratedWorkspaceName(token):
    return 'calibratedEi_' + token

def energyConvertedWorkspaceName(token):
    return 'energyConv_' + token

def eppWorkspaceName(token):
    return 'eppTable_' + token

def integratedVanadiumWorkspaceName(token):
    return 'vanaNormFactors_' + token

def kikfConvertedWorkspaceName(token):
    return 'kikf_' + token

def maskedWorkspaceName(token):
    return 'masked_' + token

def normalisedWorkspaceName(token):
    return 'norm_' + token

def rawWorkspaceName(token):
    return 'raw_' + token

def rebinnedWorkspaceName(token):
    return 'rebinned_' + token

def vanadiumNormalisedWorkspaceName(token):
    return 'vanaNorm_' + token

class Reduction:
    def __init__(self, setupTableRow):
        self._setup = setupTableRow
    def getSetup(self):
        return self._setup
    def getType(self):
        return self._setup[TYPE]
    def getWorkflow(self, reductions):
        if not hasattr(self, '_workflow'):
            myType = self.getType()
            workflowId = self._setup[ID]
            if myType == TYPE_ECCD:
                self._workflow = uninitialisedECCdWorkflow(self._setup[NORMALISATION])
                initialiseECCdWorkflow(self._setup[ID], self._workflow, reductions)
            elif myType == TYPE_SAMPLE:
                self._workflow = uninitialisedSampleWorkflow(self._setup[NORMALISATION], self._setup[EC_FROM])
                initialiseSampleWorkflow(self._setup[ID], self._workflow, reductions)
            elif myType == TYPE_VANADIUM:
                self._workflow = uninitialisedVanadiumWorkflow(self._setup[NORMALISATION], self._setup[EC_FROM])
                initialiseVanadiumWorkflow(self._setup[ID], self._workflow, reductions)
        return self._workflow

def uninitialisedECCdWorkflow(normalisationMethod):
    wf = dict()
    wf[STEP_RAW] = LoadRun()
    wf[STEP_FLAT_BACKGROUND_DATA] = FlatBackgroundFitting()
    wf[STEP_FLAT_BACKGORUND_SUBTRACTION] = FlatBackgroundSubtraction()
    wf[STEP_EPP] = EppFitting()
    wf[STEP_IDENTIFY_BAD_DETECTORS] = BadDetectorIdentification()
    wf[STEP_MASK_BAD_DETECTORS] = MaskBadDetectors()
    wf[STEP_EI_RECALCULATION] = IncidentEnergyRecalculation()
    wf[STEP_EI_CALIBRATION] = IncidentEnergyCalibration()
    if normalisationMethod == MONITOR:
        wf[STEP_NORMALISATION] = NormalisationToMonitor()
    elif normalisationMethod == TIME:
        wf[STEP_NORMALISATION] = NormalisationToTime()
    else:
        raise RuntimeError('Unknown normalisation method: ' + normalisationMethod)
    wf[STEP_FINAL] = wf[STEP_NORMALISATION]
    return wf

def stringListToArray(string):
    return eval('[' + string + ']')

def initialiseECCdWorkflow(reductionId, workflow, reductions):
    setup = reductions[reductionId].getSetup()
    if reductionId != setup[ID]:
        raise RuntimeError('reduction id does not match the setup id')
    # Load data
    wsName = rawWorkspaceName(setup[ID])
    workflow[STEP_RAW].setup(setup[FILENAME], wsName)
    # Fit background
    wsName = backgroundWorkspaceName(setup[ID])
    fitBegin = setup[BACKGROUND_FIT_BEGIN]
    fitEnd = setup[BACKGROUND_FIT_END]
    workflow[STEP_FLAT_BACKGROUND_DATA].setup(workflow[STEP_RAW],
                                              wsName,
                                              fitBegin,
                                              fitEnd)
    # Subtract background. Fit may come from another reduction
    fromId = setup[BACKGROUND_FROM]
    backgroundFitStep = reductions[fromId].getWorkflow(reductions)[STEP_FLAT_BACKGROUND_DATA]
    wsName = backgroundSubtractedWorkspaceName(setup[ID])
    workflow[STEP_FLAT_BACKGORUND_SUBTRACTION].setup(workflow[STEP_RAW], 
                                                     backgroundFitStep,
                                                     wsName)
    # Find elastic peak positions
    wsName = eppWorkspaceName(setup[ID])
    workflow[STEP_EPP].setup(workflow[STEP_FLAT_BACKGORUND_SUBTRACTION],
                             wsName)
    # Identify bad detectors. Input data may come from other reductions
    fromId = setup[EPP_FROM]
    inputEppStep = reductions[fromId].getWorkflow(reductions)[STEP_EPP]
    wsName = badDetectorWorkspaceName(setup[ID])
    hardMask = stringListToArray(setup[HARD_MASK])
    workflow[STEP_IDENTIFY_BAD_DETECTORS].setup(workflow[STEP_FLAT_BACKGORUND_SUBTRACTION],
                                                inputEppStep,
                                                hardMask,
                                                wsName)
    # Mask bad detectors.
    wsName = maskedWorkspaceName(setup[ID])
    workflow[STEP_MASK_BAD_DETECTORS].setup(workflow[STEP_FLAT_BACKGORUND_SUBTRACTION],
                                            workflow[STEP_IDENTIFY_BAD_DETECTORS],
                                            wsName)
    # Incident energy recalculation.
    eiCalibrationDetectors = setup[EI_CALIBRATION_DETECTORS]
    monitorIndex = setup[MONITOR_INDEX]
    workflow[STEP_EI_RECALCULATION].setup(workflow[STEP_EPP],
                                          workflow[STEP_EPP], # Monitors are still not in a separate ws.
                                          eiCalibrationDetectors,
                                          monitorIndex)
    # Incident energy calibration may actually depend on EPP info coming
    # from elsewhere.
    fromId = setup[EPP_FROM]
    inputEiRecalculationStep = reductions[fromId].getWorkflow(reductions)[STEP_EI_RECALCULATION]
    wsName = eiCalibratedWorkspaceName(setup[ID])
    workflow[STEP_EI_CALIBRATION].setup(workflow[STEP_MASK_BAD_DETECTORS],
                                        inputEiRecalculationStep,
                                        wsName)
    # Normalisation to monitor or time
    normalisationMethod = setup[NORMALISATION]
    wsName = normalisedWorkspaceName(setup[ID])
    if normalisationMethod == MONITOR:
        workflow[STEP_NORMALISATION].setup(workflow[STEP_EI_CALIBRATION],
                                           inputEppStep,
                                           monitorIndex,
                                           wsName)
    elif normalisationMethod == TIME:
        workflow[STEP_NORMALISATION].setup(workflow[STEP_EI_CALIBRATION],
                                           wsName)
    else:
        raise RuntimeError('Unknown normalisation method: ' + normalisationMethod)

def uninitialisedVanadiumWorkflow(normalisationMethod, ecFrom):
    # Initial steps same as with EC/Cd.
    wf = uninitialisedECCdWorkflow(normalisationMethod)
    if ecFrom:
        wf[STEP_EC_SUBTRACTION] = EmptyCanSubtraction()
    else:
        wf[STEP_EC_SUBTRACTION] = Noop()
    wf[STEP_VANADIUM_INTEGRATION] = VanadiumIntegration()
    wf[STEP_FINAL] = wf[STEP_EC_SUBTRACTION]
    return wf

def initialiseVanadiumWorkflow(reductionId, workflow, reductions):
    # Initial steps same as with EC/Cd.
    initialiseECCdWorkflow(reductionId, workflow, reductions)

    setup = reductions[reductionId].getSetup()
    # Empy can subtraction is optional.
    ecFrom = setup[EC_FROM]
    if ecFrom:
        if reductions[ecFrom].getSetup()[TYPE] != TYPE_ECCD:
            raise RuntimeError('EC reduction not marked as such')
        ecInputStep = reductions[ecFrom].getWorkflow(reductions)[STEP_FINAL]
        cdFrom = setup[CD_FROM]
        cdInputStep = None
        if cdFrom:
            if reductions[ecFrom].getSetup()[TYPE] != TYPE_ECCD:
                raise RuntimeError('Cd reduction not marked as such')
            cdInputStep = reductions[cdFrom].getWorkflow(reductions)[STEP_FINAL]
        transmission = setup[TRANSMISSION]
        wsName = ecSubtractedWorkspaceName(setup[ID])
        workflow[STEP_EC_SUBTRACTION].setup(workflow[STEP_NORMALISATION],
                                            ecInputStep,
                                            cdInputStep,
                                            transmission,
                                            wsName)
    else:
        workflow[STEP_EC_SUBTRACTION].setup(workflow[STEP_NORMALISATION])
    # Vanadium integration step
    eppFrom = setup[EPP_FROM]
    eppStep = reductions[eppFrom].getWorkflow(reductions)[STEP_EPP]
    wsName = integratedVanadiumWorkspaceName(setup[ID])
    workflow[STEP_VANADIUM_INTEGRATION].setup(workflow[STEP_EC_SUBTRACTION],
                                              eppStep,
                                              wsName)

def uninitialisedSampleWorkflow(normalisationMethod, ecFrom):
    # Initial steps same as with vanadium.
    wf = uninitialisedVanadiumWorkflow(normalisationMethod, ecFrom)
    wf[STEP_VANADIUM_NORMALISAION] = NormalisationToVanadium()
    wf[STEP_TOF_TO_E_CONVERSION] = TOFToEnergyConversion()
    wf[STEP_KIKF_CONVERSION] = KiKfConversion()
    wf[STEP_REBINNING] = RebinningInQandW()
    wf[STEP_DETECTOR_EFFICIENCY_CORRECTION] = DetectorEfficiencyCorrection()
    
    wf[STEP_FINAL] = wf[STEP_DETECTOR_EFFICIENCY_CORRECTION]
    return wf

def initialiseSampleWorkflow(reductionId, workflow, reductions):
    # Initial steps same as with vanadium.
    # As an added extra, we'll get a STEP_VANADIUM_INTEGRATION.
    initialiseVanadiumWorkflow(reductionId, workflow, reductions)

    setup = reductions[reductionId].getSetup()
    vanaFrom = setup[VANADIUM_FROM]
    vanaType = reductions[vanaFrom].getSetup()[TYPE]
    if vanaType != TYPE_VANADIUM:
        raise RuntimeError(vanaFrom +' as ' + VANADIUM_FROM + ' for ' + setup[ID] + ' but the type is ' + vanaType)
    integratedVanadiumStep = reductions[vanaFrom].getWorkflow(reductions)[STEP_VANADIUM_INTEGRATION]
    wsName = vanadiumNormalisedWorkspaceName(setup[ID])
    workflow[STEP_VANADIUM_NORMALISAION].setup(workflow[STEP_EC_SUBTRACTION],
                                               integratedVanadiumStep,
                                               wsName)
    wsName = energyConvertedWorkspaceName(setup[ID])
    workflow[STEP_TOF_TO_E_CONVERSION].setup(workflow[STEP_VANADIUM_NORMALISAION],
                                             wsName)
    wsName = kikfConvertedWorkspaceName(setup[ID])
    workflow[STEP_KIKF_CONVERSION].setup(workflow[STEP_TOF_TO_E_CONVERSION],
                                         wsName)
    wsName = rebinnedWorkspaceName(setup[ID])
    workflow[STEP_REBINNING].setup(workflow[STEP_KIKF_CONVERSION],
                                   setup[Y_REBINNING_PARAMS],
                                   setup[X_REBINNING_PARAMS],
                                   wsName)
    wsName = detectorEfficiencyCorrectedWorkspaceName(setup[ID])
    workflow[STEP_DETECTOR_EFFICIENCY_CORRECTION].setup(workflow[STEP_REBINNING],
                                                        wsName)

class DGSReductionILL(DataProcessorAlgorithm):

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        return 'Workflow\\Inelastic'

    def name(self):
        return 'DGSReductionILL'

    def summary(self):
        return 'Data reduction workflow for the direct geometry time-of-flight spectrometers at ILL'

    def version(self):
        return 1

    def PyExec(self):
        rTable = self.getProperty('ReductionTable').value
        reductions = dict()
        for row in range(rTable.rowCount()):
            rowId = rTable.cell(ID, row)
            if rowId in reductions:
                raise RuntimeError('duplicate id ' + rowID + ' in reduction table at row ' + row)
            reductions[rowId] = Reduction(rTable.row(row))
        outputWorkspaces = list()
        for rId, reduction in reductions.items():
            if reduction.getType() == TYPE_SAMPLE:
                workflow = reduction.getWorkflow(reductions)
                outputWorkspaces.append(workflow[STEP_FINAL].process().getName())
        outputWorkspace = self.getProperty('OutputWorkspace').value
        outputGroup = GroupWorkspaces(outputWorkspaces, outputWorkspace)
        self.setProperty('OutputWorkspace', outputGroup)

    def PyInit(self):
        self.declareProperty(ITableWorkspaceProperty('ReductionTable', '', Direction.Input), doc='Reduction setup table workspace.')
        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '', Direction.Output), doc='The output of the algorithm.')
        
AlgorithmFactory.subscribe(DGSReductionILL)
