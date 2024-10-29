# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import DirectILL_common as common
import ILL_utilities as utils
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    InstrumentValidator,
    ITableWorkspaceProperty,
    MatrixWorkspaceProperty,
    MultipleFileProperty,
    Progress,
    PropertyMode,
    WorkspaceProperty,
    WorkspaceUnitValidator,
)
from mantid.kernel import (
    CompositeValidator,
    Direct,
    Direction,
    FloatBoundedValidator,
    IntBoundedValidator,
    IntMandatoryValidator,
    Property,
    StringListValidator,
    UnitConversion,
)
from mantid.simpleapi import (
    AddSampleLog,
    CalculateFlatBackground,
    CorrectTOFAxis,
    CreateEPP,
    CreateSingleValuedWorkspace,
    CreateWorkspace,
    CropWorkspace,
    DeleteWorkspace,
    ExtractMonitors,
    FindEPP,
    GetEiMonDet,
    GroupDetectors,
    LoadAndMerge,
    Minus,
    mtd,
    NormaliseToMonitor,
    Scale,
    SetInstrumentParameter,
)
import numpy as np

_MONSUM_LIMIT = 100


def _addEfixedInstrumentParameter(ws):
    """Adds the [calibrated] Ei as Efixed instrument parameter.
    This is needed for subsequent QENS analysis routines, if one wishes to do in Mantid."""
    efixed = ws.getRun().getLogData("Ei").value
    SetInstrumentParameter(Workspace=ws, ParameterName="Efixed", ParameterType="Number", Value=str(efixed))


def _applyIncidentEnergyCalibration(ws, eiWS, wsNames, report, algorithmLogging):
    """Update incident energy and wavelength in the sample logs."""
    originalEnergy = ws.getRun().getLogData("Ei").value
    originalWavelength = ws.getRun().getLogData("wavelength").value
    energy = eiWS.readY(0)[0]
    wavelength = UnitConversion.run("Energy", "Wavelength", energy, 0, 0, 0, Direct, 5)
    AddSampleLog(
        Workspace=ws,
        LogName="Ei",
        LogText=str(energy),
        LogType="Number",
        NumberType="Double",
        LogUnit="meV",
        EnableLogging=algorithmLogging,
    )
    AddSampleLog(
        Workspace=ws,
        Logname="wavelength",
        LogText=str(wavelength),
        LogType="Number",
        NumberType="Double",
        LogUnit="Angstrom",
        EnableLogging=algorithmLogging,
    )
    report.notice("Applied Ei calibration to '" + str(ws) + "'.")
    report.notice("Original Ei: {} new Ei: {}.".format(originalEnergy, energy))
    report.notice("Original wavelength: {} new wavelength {}.".format(originalWavelength, wavelength))
    return ws


def _calculateEPP(ws, sigma, wsNames, algorithmLogging):
    eppWSName = wsNames.withSuffix("epp_detectors")
    eppWS = CreateEPP(InputWorkspace=ws, OutputWorkspace=eppWSName, Sigma=sigma, EnableLogging=algorithmLogging)
    return eppWS


def _calibratedIncidentEnergy(detWorkspace, monWorkspace, monEPPWorkspace, eiCalibrationMon, wsNames, log, algorithmLogging):
    """Return the calibrated incident energy."""
    instrument = detWorkspace.getInstrument()
    instrument_name = instrument.getName()
    eiWorkspace = None
    if instrument_name in ["IN4", "IN6", "PANTHER", "SHARP"]:
        run = detWorkspace.run()
        eiCalibrationDets = instrument.getStringParameter("Ei_calibration_detectors")[0]
        maximumEnergy = 10.0
        timeFrame = None
        if instrument_name in ["IN4", "PANTHER"]:
            maximumEnergy = 1000.0
            # This could be changed in real rotation speed...
            fermiChopperSpeed = run.getProperty("FC.rotation_speed").value
            backgroundChopperSpeed = run.getProperty("BC1.rotation_speed").value
            # timeFrame should be calculated according to BC1 to avoid pb in higher order mode
            timeFrame = 60.0e6 / backgroundChopperSpeed / 8
            if abs(fermiChopperSpeed / 4.0 - backgroundChopperSpeed) > 10.0:
                log.warning("Fermi speed not four times the background chopper speed. Omitting incident energy calibration.")
                return None
        elif instrument_name == "IN6":
            suppressorChopperSpeed = run.getProperty("Suppressor.rotation_speed").value
            timeFrame = 60.0e6 / suppressorChopperSpeed / 2
        elif instrument_name == "SHARP":
            fermiChopperSpeed = run.getProperty("Fermi.rotation_speed").value
            timeFrame = 60.0e6 / fermiChopperSpeed / 2
            maximumEnergy = 1000.0
        energy = GetEiMonDet(
            DetectorWorkspace=detWorkspace,
            DetectorWorkspaceIndexType="WorkspaceIndex",
            DetectorWorkspaceIndexSet=eiCalibrationDets,
            MonitorWorkspace=monWorkspace,
            MonitorEPPTable=monEPPWorkspace,
            MonitorIndex=eiCalibrationMon,
            MaximumEnergy=maximumEnergy,
            EnableLogging=algorithmLogging,
            PulseInterval=timeFrame,
        )
        eiWSName = wsNames.withSuffix("incident_energy")
        eiWorkspace = CreateSingleValuedWorkspace(OutputWorkspace=eiWSName, DataValue=energy, EnableLogging=algorithmLogging)
    else:
        log.error("Instrument " + instrument_name + " not supported for incident energy calibration")
    return eiWorkspace


def _createFlatBkg(ws, wsType, windowWidth, wsNames, algorithmLogging):
    """Return a flat background workspace."""
    if wsType == common.WS_CONTENT_DETS:
        bkgWSName = wsNames.withSuffix("flat_bkg_for_detectors")
    else:
        bkgWSName = wsNames.withSuffix("flat_bkg_for_monitors")
    bkgWS = CalculateFlatBackground(
        InputWorkspace=ws,
        OutputWorkspace=bkgWSName,
        Mode="Moving Average",
        OutputMode="Return Background",
        SkipMonitors=False,
        NullifyNegativeValues=False,
        AveragingWindowWidth=windowWidth,
        EnableLogging=algorithmLogging,
    )
    firstBinStart = bkgWS.dataX(0)[0]
    firstBinEnd = bkgWS.dataX(0)[1]
    bkgWS = CropWorkspace(InputWorkspace=bkgWS, OutputWorkspace=bkgWS, XMin=firstBinStart, XMax=firstBinEnd, EnableLogging=algorithmLogging)
    return bkgWS


def _fitElasticChannel(ys, wsNames, wsCleanup, algorithmLogging):
    """Return index to the peak position of ys."""
    xs = np.array([i for i in range(len(ys))])
    l2SumWSName = wsNames.withSuffix("summed_detectors_at_l2")
    l2SumWS = CreateWorkspace(OutputWorkspace=l2SumWSName, DataX=xs, DataY=ys, EnableLogging=algorithmLogging)
    fitWSName = wsNames.withSuffix("summed_detectors_at_l2_fit_results")
    fitWS = FindEPP(InputWorkspace=l2SumWS, OutputWorkspace=fitWSName, EnableLogging=algorithmLogging)
    peakCentre = float(fitWS.cell("PeakCentre", 0))
    wsCleanup.cleanup(l2SumWS)
    wsCleanup.cleanup(fitWS)
    return peakCentre


def _fitEPP(ws, wsType, wsNames, algorithmLogging):
    """Return a fitted EPP table for a workspace."""
    if wsType == common.WS_CONTENT_DETS:
        eppWSName = wsNames.withSuffix("epp_detectors")
    else:
        eppWSName = wsNames.withSuffix("epp_monitors")
    eppWS = FindEPP(InputWorkspace=ws, OutputWorkspace=eppWSName, EnableLogging=algorithmLogging)
    return eppWS


def _monitorCounts(ws):
    """Return the total monitor counts from the sample logs"""
    logs = ws.run()
    instrument = ws.getInstrument()
    if instrument.getName() == "IN6":
        return logs.getProperty("monitor1.monsum").value
    else:
        return logs.getProperty("monitor.monsum").value


def _normalizeToMonitor(ws, monWS, monIndex, integrationBegin, integrationEnd, wsNames, wsCleanup, algorithmLogging):
    """Normalize to monitor counts."""
    normalizedWSName = wsNames.withSuffix("normalized_to_monitor")
    normalizationFactorWsName = wsNames.withSuffix("normalization_factor_monitor")
    normalizedWS, normalizationFactorWS = NormaliseToMonitor(
        InputWorkspace=ws,
        OutputWorkspace=normalizedWSName,
        MonitorWorkspace=monWS,
        MonitorWorkspaceIndex=monIndex,
        IntegrationRangeMin=integrationBegin,
        IntegrationRangeMax=integrationEnd,
        NormFactorWS=normalizationFactorWsName,
        EnableLogging=algorithmLogging,
    )
    wsCleanup.cleanup(normalizationFactorWS)
    return normalizedWS


def _normalizeToTime(ws, wsNames, wsCleanup, algorithmLogging):
    """Normalize to the 'actual_time' sample log."""
    log = ws.run()
    if not log.hasProperty("duration"):
        if not log.hasProperty("actual_time"):
            raise RuntimeError("Cannot normalise to acquisition time: 'duration' missing from sample logs.")
        time = log.getProperty("actual_time").value
    else:
        time = log.getProperty("duration").value
    if time == 0:
        raise RuntimeError("Cannot normalise to acquisition time: time is zero.")
    if time < 0:
        raise RuntimeError("Cannot normalise to acquisition time: time is negative.")
    normalizedWSName = wsNames.withSuffix("normalized_to_time")
    normalizedWS = Scale(InputWorkspace=ws, Factor=1.0 / time, OutputWorkspace=normalizedWSName, EnableLogging=algorithmLogging)
    return normalizedWS


def _scaleAfterMonitorNormalization(ws, wsNames, wsCleanup, algorithmLogging):
    """Scale ws by a factor given in the instrument parameters."""
    SCALING_PARAM = "scaling_after_monitor_normalisation"
    NON_RECURSIVE = False  # Prevent recursive calls.
    instr = ws.getInstrument()
    if not instr.hasParameter(SCALING_PARAM, NON_RECURSIVE):
        return ws
    factor = instr.getNumberParameter(SCALING_PARAM, NON_RECURSIVE)[0]
    scaledWSName = wsNames.withSuffix("scaled_by_monitor_factor")
    scaledWS = Scale(InputWorkspace=ws, OutputWorkspace=scaledWSName, Factor=factor, EnableLogging=algorithmLogging)
    wsCleanup.cleanup(ws)
    return scaledWS


def _subtractFlatBkg(ws, wsType, bkgWorkspace, bkgScaling, wsNames, wsCleanup, algorithmLogging):
    """Subtract a scaled flat background from a workspace."""
    if wsType == common.WS_CONTENT_DETS:
        subtractedWSName = wsNames.withSuffix("flat_bkg_subtracted_detectors")
        scaledBkgWSName = wsNames.withSuffix("flat_bkg_for_detectors_scaled")
    else:
        subtractedWSName = wsNames.withSuffix("flat_bkg_subtracted_monitors")
        scaledBkgWSName = wsNames.withSuffix("flat_bkg_for_monitors_scaled")
    Scale(InputWorkspace=bkgWorkspace, OutputWorkspace=scaledBkgWSName, Factor=bkgScaling, EnableLogging=algorithmLogging)
    subtractedWS = Minus(LHSWorkspace=ws, RHSWorkspace=scaledBkgWSName, OutputWorkspace=subtractedWSName, EnableLogging=algorithmLogging)
    wsCleanup.cleanup(scaledBkgWSName)
    return subtractedWS


def _sumDetectorsAtDistance(ws, distance, tolerance):
    """Return a sum of the Y values of detectors at distance away from the sample."""
    histogramCount = ws.getNumberHistograms()
    ySums = np.zeros(ws.blocksize())
    detectorInfo = ws.detectorInfo()
    sample = ws.getInstrument().getSample()
    for i in range(histogramCount):
        det = ws.getDetector(i)
        sampleToDetector = sample.getDistance(det)
        if abs(distance - sampleToDetector) < tolerance:
            if detectorInfo.isMonitor(i) or detectorInfo.isMasked(i):
                continue
            ySums += ws.readY(i)
    return ySums


class DirectILLCollectData(DataProcessorAlgorithm):
    """A workflow algorithm for the initial sample, vanadium and empty container reductions."""

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the algorithm's category."""
        return common.CATEGORIES

    def seeAlso(self):
        return ["DirectILLReduction"]

    def name(self):
        """Return the algorithm's name."""
        return "DirectILLCollectData"

    def summary(self):
        """Return a summary of the algorithm."""
        return "An initial step of the reduction workflow for the direct geometry TOF spectrometers at ILL."

    def version(self):
        """Return the algorithm's version."""
        return 1

    def PyExec(self):
        """Execute the data collection workflow."""
        progress = Progress(self, 0.0, 1.0, 9)
        self._report = utils.Report()
        self._subalgLogging = self.getProperty(common.PROP_SUBALG_LOGGING).value == common.SUBALG_LOGGING_ON
        namePrefix = self.getProperty(common.PROP_OUTPUT_WS).valueAsStr
        cleanupMode = self.getProperty(common.PROP_CLEANUP_MODE).value
        self._cleanup = utils.Cleanup(cleanupMode, self._subalgLogging)
        self._names = utils.NameSource(namePrefix, cleanupMode)

        # The variables 'mainWS' and 'monWS shall hold the current main
        # data throughout the algorithm.

        # Get input workspace.
        progress.report("Loading inputs")
        mainWS = self._inputWS()

        # Extract monitors to a separate workspace.
        progress.report("Extracting monitors")
        mainWS, monWS = self._separateMons(mainWS)

        # Save the main workspace for later use, if needed.
        rawWS = None
        if not self.getProperty(common.PROP_OUTPUT_RAW_WS).isDefault:
            rawWS = mainWS
            self._cleanup.protect(rawWS)

        # Normalisation to monitor/time, if requested.
        progress.report("Normalising to monitor/time")
        monWS = self._flatBkgMon(monWS)
        monEPPWS = self._createEPPWSMon(monWS)
        mainWS = self._normalize(mainWS, monWS, monEPPWS)

        # Time-independent background.
        progress.report("Calculating backgrounds")
        mainWS = self._flatBkgDet(mainWS)

        # Calibrate incident energy, if requested.
        progress.report("Calibrating incident energy")
        mainWS, monWS = self._calibrateEi(mainWS, monWS, monEPPWS)
        self._cleanup.cleanup(monWS, monEPPWS)

        # Add the Ei as Efixed instrument parameter
        _addEfixedInstrumentParameter(mainWS)

        progress.report("Correcting TOF")
        mainWS = self._correctTOFAxis(mainWS)
        self._outputRaw(mainWS, rawWS)

        # Find elastic peak positions.
        progress.report("Calculating EPPs")
        self._outputDetEPPWS(mainWS)

        self._finalize(mainWS)
        progress.report("Done")

    def PyInit(self):
        """Initialize the algorithm's input and output properties."""
        PROPGROUP_FLAT_BKG = "Flat Time-Independent Background"
        PROPGROUP_INCIDENT_ENERGY_CALIBRATION = "Indicent Energy Calibration"
        PROPGROUP_MON_NORMALISATION = "Neutron Flux Normalisation"
        # Validators.
        mandatoryPositiveInt = CompositeValidator()
        mandatoryPositiveInt.add(IntMandatoryValidator())
        mandatoryPositiveInt.add(IntBoundedValidator(lower=0))
        positiveFloat = FloatBoundedValidator(lower=0)
        positiveInt = IntBoundedValidator(lower=0)
        inputWorkspaceValidator = CompositeValidator()
        inputWorkspaceValidator.add(InstrumentValidator())
        inputWorkspaceValidator.add(WorkspaceUnitValidator("TOF"))

        # Properties.
        self.declareProperty(
            MultipleFileProperty(name=common.PROP_INPUT_FILE, action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="An input run number (or a list thereof) or a filename.",
        )
        self.getProperty(common.PROP_INPUT_FILE).setAutoTrim(False)
        self.declareProperty(
            MatrixWorkspaceProperty(
                name=common.PROP_INPUT_WS,
                defaultValue="",
                validator=inputWorkspaceValidator,
                optional=PropertyMode.Optional,
                direction=Direction.Input,
            ),
            doc="Input workspace if no run is given.",
        )
        self.declareProperty(
            WorkspaceProperty(name=common.PROP_OUTPUT_WS, defaultValue="", direction=Direction.Output),
            doc="A flux normalized and background subtracted workspace.",
        )
        self.declareProperty(
            name=common.PROP_CLEANUP_MODE,
            defaultValue=utils.Cleanup.ON,
            validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
            direction=Direction.Input,
            doc="What to do with intermediate workspaces.",
        )
        self.declareProperty(
            name=common.PROP_SUBALG_LOGGING,
            defaultValue=common.SUBALG_LOGGING_OFF,
            validator=StringListValidator([common.SUBALG_LOGGING_OFF, common.SUBALG_LOGGING_ON]),
            direction=Direction.Input,
            doc="Enable or disable subalgorithms to " + "print in the logs.",
        )
        self.declareProperty(
            name=common.PROP_EPP_METHOD,
            defaultValue=common.EPP_METHOD_AUTO,
            validator=StringListValidator([common.EPP_METHOD_AUTO, common.EPP_METHOD_FIT, common.EPP_METHOD_CALCULATE]),
            direction=Direction.Input,
            doc="Method to create the EPP table for detectors (monitor is awlays fitted).",
        )
        self.declareProperty(
            name=common.PROP_EPP_SIGMA,
            defaultValue=Property.EMPTY_DBL,
            validator=positiveFloat,
            direction=Direction.Input,
            doc="Nominal sigma for the EPP table when "
            + common.PROP_EPP_METHOD
            + " is set to "
            + common.EPP_METHOD_CALCULATE
            + " (default: 10 times the first bin width).",
        )
        self.declareProperty(
            name=common.PROP_ELASTIC_CHANNEL_MODE,
            defaultValue=common.ELASTIC_CHANNEL_AUTO,
            validator=StringListValidator([common.ELASTIC_CHANNEL_AUTO, common.ELASTIC_CHANNEL_SAMPLE_LOG, common.ELASTIC_CHANNEL_FIT]),
            direction=Direction.Input,
            doc="How to acquire the nominal elastic channel.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(
                name=common.PROP_ELASTIC_CHANNEL_WS, defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="A single value workspace containing the nominal elastic channel index(can be floating point). Overrides {}.".format(
                common.PROP_ELASTIC_CHANNEL_MODE
            ),
        )
        self.declareProperty(
            name=common.PROP_MON_INDEX,
            defaultValue=Property.EMPTY_INT,
            validator=positiveInt,
            direction=Direction.Input,
            doc="Index of the incident monitor, if not specified in instrument parameters.",
        )
        self.declareProperty(
            name=common.PROP_INCIDENT_ENERGY_CALIBRATION,
            defaultValue=common.INCIDENT_ENERGY_CALIBRATION_AUTO,
            validator=StringListValidator(
                [common.INCIDENT_ENERGY_CALIBRATION_AUTO, common.INCIDENT_ENERGY_CALIBRATION_ON, common.INCIDENT_ENERGY_CALIBRATION_OFF]
            ),
            direction=Direction.Input,
            doc="Control the incident energy calibration.",
        )
        self.setPropertyGroup(common.PROP_INCIDENT_ENERGY_CALIBRATION, PROPGROUP_INCIDENT_ENERGY_CALIBRATION)
        self.declareProperty(
            MatrixWorkspaceProperty(
                name=common.PROP_INCIDENT_ENERGY_WS, defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="A single-valued workspace holding a previously determined " + "incident energy.",
        )
        self.setPropertyGroup(common.PROP_INCIDENT_ENERGY_WS, PROPGROUP_INCIDENT_ENERGY_CALIBRATION)
        self.declareProperty(
            name=common.PROP_FLAT_BKG,
            defaultValue=common.BKG_AUTO,
            validator=StringListValidator([common.BKG_AUTO, common.BKG_ON, common.BKG_OFF]),
            direction=Direction.Input,
            doc="Control flat background subtraction.",
        )
        self.setPropertyGroup(common.PROP_FLAT_BKG, PROPGROUP_FLAT_BKG)
        self.declareProperty(
            name=common.PROP_FLAT_BKG_SCALING,
            defaultValue=1.0,
            validator=positiveFloat,
            direction=Direction.Input,
            doc="Flat background multiplication factor.",
        )
        self.setPropertyGroup(common.PROP_FLAT_BKG_SCALING, PROPGROUP_FLAT_BKG)
        self.declareProperty(
            name=common.PROP_FLAT_BKG_WINDOW,
            defaultValue=30,
            validator=mandatoryPositiveInt,
            direction=Direction.Input,
            doc="Running average window width (in bins) for flat background.",
        )
        self.setPropertyGroup(common.PROP_FLAT_BKG_WINDOW, PROPGROUP_FLAT_BKG)
        self.declareProperty(
            MatrixWorkspaceProperty(
                name=common.PROP_FLAT_BKG_WS, defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="Workspace with previously determined flat background data.",
        )
        self.setPropertyGroup(common.PROP_FLAT_BKG_WS, PROPGROUP_FLAT_BKG)
        self.declareProperty(
            name=common.PROP_DET_HOR_GROUPING,
            defaultValue=1,
            doc="Step to use when grouping detectors horizontally (between tubes) to increase"
            " the statistics for flat background calculation.",
        )
        self.setPropertyGroup(common.PROP_DET_HOR_GROUPING, PROPGROUP_FLAT_BKG)
        self.declareProperty(
            name=common.PROP_DET_VER_GROUPING,
            defaultValue=1,
            doc="Step to use when grouping detectors vertically (inside the same tube)"
            " to increase the statistics for flat background calculation.",
        )
        self.setPropertyGroup(common.PROP_DET_VER_GROUPING, PROPGROUP_FLAT_BKG)
        self.declareProperty(
            name=common.PROP_NORMALISATION,
            defaultValue=common.NORM_METHOD_MON,
            validator=StringListValidator([common.NORM_METHOD_MON, common.NORM_METHOD_TIME, common.NORM_METHOD_OFF]),
            direction=Direction.Input,
            doc="Normalisation method.",
        )
        self.setPropertyGroup(common.PROP_NORMALISATION, PROPGROUP_MON_NORMALISATION)
        self.declareProperty(
            name=common.PROP_MON_PEAK_SIGMA_MULTIPLIER,
            defaultValue=7.0,
            validator=positiveFloat,
            direction=Direction.Input,
            doc="Width of the monitor peak in multiples " + " of 'Sigma' in monitor's EPP table.",
        )
        self.setPropertyGroup(common.PROP_MON_PEAK_SIGMA_MULTIPLIER, PROPGROUP_MON_NORMALISATION)
        # Rest of the output properties.
        self.declareProperty(
            WorkspaceProperty(name=common.PROP_OUTPUT_RAW_WS, defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Non-normalized and non-background subtracted output workspace for DirectILLDiagnostics.",
        )
        self.setPropertyGroup(common.PROP_OUTPUT_RAW_WS, common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(
            WorkspaceProperty(
                name=common.PROP_OUTPUT_ELASTIC_CHANNEL_WS, defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="Output workspace for elastic channel index.",
        )
        self.setPropertyGroup(common.PROP_OUTPUT_ELASTIC_CHANNEL_WS, common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(
            ITableWorkspaceProperty(
                name=common.PROP_OUTPUT_DET_EPP_WS, defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="Output workspace for elastic peak positions.",
        )
        self.setPropertyGroup(common.PROP_OUTPUT_DET_EPP_WS, common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(
            WorkspaceProperty(
                name=common.PROP_OUTPUT_INCIDENT_ENERGY_WS, defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="Output workspace for calibrated incident energy.",
        )
        self.setPropertyGroup(common.PROP_OUTPUT_INCIDENT_ENERGY_WS, common.PROPGROUP_OPTIONAL_OUTPUT)
        self.declareProperty(
            WorkspaceProperty(
                name=common.PROP_OUTPUT_FLAT_BKG_WS, defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="Output workspace for flat background.",
        )
        self.setPropertyGroup(common.PROP_OUTPUT_FLAT_BKG_WS, common.PROPGROUP_OPTIONAL_OUTPUT)

    def validateInputs(self):
        """Check for issues with user input."""
        issues = dict()

        fileGiven = not self.getProperty(common.PROP_INPUT_FILE).isDefault
        wsGiven = not self.getProperty(common.PROP_INPUT_WS).isDefault
        # Validate that an input exists
        if fileGiven == wsGiven:
            issues[common.PROP_INPUT_FILE] = "Must give either an input file or an input workspace."
        if not wsGiven and self.getProperty(common.PROP_INPUT_WS).value:
            issues[common.PROP_INPUT_WS] = "Input workspace has to be in the ADS."
        if fileGiven and self.getPropertyValue(common.PROP_INPUT_FILE).count(",") > 0:
            issues[common.PROP_INPUT_FILE] = "List of runs is given without summing. Consider giving summed runs (+) or summed ranges (-)."
        return issues

    def _calibrateEi(self, mainWS, monWS, monEPPWS):
        """Perform and apply incident energy calibration."""
        eiCalibrationWS = None
        if self._eiCalibrationEnabled(mainWS):
            if self.getProperty(common.PROP_INCIDENT_ENERGY_WS).isDefault:
                monIndex = self._monitorIndex(monWS)
                eiCalibrationWS = _calibratedIncidentEnergy(mainWS, monWS, monEPPWS, monIndex, self._names, self.log(), self._subalgLogging)
            else:
                eiCalibrationWS = self.getProperty(common.PROP_INCIDENT_ENERGY_WS).value
                self._cleanup.protect(eiCalibrationWS)
            if eiCalibrationWS:
                mainWS = _applyIncidentEnergyCalibration(mainWS, eiCalibrationWS, self._names, self._report, self._subalgLogging)
                monWS = _applyIncidentEnergyCalibration(monWS, eiCalibrationWS, self._names, self._report, self._subalgLogging)
        if not self.getProperty(common.PROP_OUTPUT_INCIDENT_ENERGY_WS).isDefault:
            if eiCalibrationWS is None:
                eiCalibrationWSName = self._names.withSuffix("incident_energy_from_logs")
                Ei = mainWS.run().getProperty("Ei").value
                eiCalibrationWS = CreateSingleValuedWorkspace(
                    OutputWorkspace=eiCalibrationWSName, DataValue=Ei, EnableLogging=self._subalgLogging
                )
            self.setProperty(common.PROP_OUTPUT_INCIDENT_ENERGY_WS, eiCalibrationWS)
        self._cleanup.cleanup(eiCalibrationWS)
        return mainWS, monWS

    def _chooseElasticChannelMode(self, mainWS):
        """Return suitable elastic channel mode."""
        mode = self.getProperty(common.PROP_ELASTIC_CHANNEL_MODE).value
        if mode == common.ELASTIC_CHANNEL_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter("enable_elastic_channel_fitting"):
                if instrument.getBoolParameter("enable_elastic_channel_fitting")[0]:
                    self._report.notice(common.PROP_ELASTIC_CHANNEL_MODE + " set to " + common.ELASTIC_CHANNEL_FIT + " by the IPF.")
                    return common.ELASTIC_CHANNEL_FIT
                else:
                    self._report.notice(common.PROP_ELASTIC_CHANNEL_MODE + " set to " + common.ELASTIC_CHANNEL_SAMPLE_LOG + " by the IPF.")
                    return common.ELASTIC_CHANNEL_SAMPLE_LOG
            else:
                self._report.notice("Defaulted " + common.PROP_ELASTIC_CHANNEL_MODE + " to " + common.ELASTIC_CHANNEL_SAMPLE_LOG + ".")
                return common.ELASTIC_CHANNEL_SAMPLE_LOG
        return mode

    def _chooseEPPMethod(self, mainWS):
        """Return a suitable EPP method."""
        eppMethod = self.getProperty(common.PROP_EPP_METHOD).value
        if eppMethod == common.EPP_METHOD_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter("enable_elastic_peak_fitting"):
                if instrument.getBoolParameter("enable_elastic_peak_fitting")[0]:
                    self._report.notice(common.PROP_EPP_METHOD + " set to " + common.EPP_METHOD_FIT + " by the IPF.")
                    return common.EPP_METHOD_FIT
                else:
                    self._report.notice(common.PROP_EPP_METHOD + " set to " + common.EPP_METHOD_CALCULATE + " by the IPF.")
                    return common.EPP_METHOD_CALCULATE
            else:
                self._report.notice("Defaulted " + common.PROP_EPP_METHOD + " to " + common.EPP_METHOD_FIT + ".")
                return common.EPP_METHOD_FIT
        return eppMethod

    def _correctTOFAxis(self, mainWS):
        """Adjust the TOF axis to get the elastic channel correct."""
        try:
            l2 = float(mainWS.getInstrument().getStringParameter("l2")[0])
        except IndexError:
            self.log().warning("No 'l2' instrument parameter defined. TOF axis will not be adjusted")
            return mainWS
        if not self.getProperty(common.PROP_ELASTIC_CHANNEL_WS).isDefault:
            indexWS = self.getProperty(common.PROP_ELASTIC_CHANNEL_WS).value
            index = indexWS.readY(0)[0]
        else:
            mode = self._chooseElasticChannelMode(mainWS)
            if mode == common.ELASTIC_CHANNEL_SAMPLE_LOG:
                if not mainWS.run().hasProperty("Detector.elasticpeak"):
                    self.log().warning("No " + common.PROP_ELASTIC_CHANNEL_WS + " given. TOF axis will not be adjusted.")
                    return mainWS
                index = mainWS.run().getLogData("Detector.elasticpeak").value
            else:
                ys = _sumDetectorsAtDistance(mainWS, l2, 1e-5)
                index = _fitElasticChannel(ys, self._names, self._cleanup, self._subalgLogging)
                precision = int(mainWS.getInstrument().getIntParameter("elastic_channel_precision")[0])
                index = np.trunc(index * 10**precision) / 10**precision
        correctedWSName = self._names.withSuffix("tof_axis_corrected")
        correctedWS = CorrectTOFAxis(
            InputWorkspace=mainWS,
            OutputWorkspace=correctedWSName,
            IndexType="Workspace Index",
            ElasticBinIndex=index,
            L2=l2,
            EnableLogging=self._subalgLogging,
        )
        self._report.notice("Elastic channel index {0} was used for TOF axis adjustment.".format(index))
        if not self.getProperty(common.PROP_OUTPUT_ELASTIC_CHANNEL_WS).isDefault:
            indexOutputWSName = self._names.withSuffix("elastic_channel_output")
            indexOutputWS = CreateSingleValuedWorkspace(
                OutputWorkspace=indexOutputWSName, DataValue=index, EnableLogging=self._subalgLogging
            )
            self.setProperty(common.PROP_OUTPUT_ELASTIC_CHANNEL_WS, indexOutputWS)
            self._cleanup.cleanup(indexOutputWS)
        self._cleanup.cleanup(mainWS)
        return correctedWS

    def _createEPPWSDet(self, mainWS):
        """Create an EPP table for a detector workspace."""
        eppMethod = self._chooseEPPMethod(mainWS)
        if eppMethod == common.EPP_METHOD_FIT:
            detEPPWS = _fitEPP(mainWS, common.WS_CONTENT_DETS, self._names, self._subalgLogging)
        else:
            sigma = self.getProperty(common.PROP_EPP_SIGMA).value
            if sigma == Property.EMPTY_DBL:
                sigma = 10.0 * (mainWS.readX(0)[1] - mainWS.readX(0)[0])
            detEPPWS = _calculateEPP(mainWS, sigma, self._names, self._subalgLogging)
        self._cleanup.cleanupLater(detEPPWS)
        return detEPPWS

    def _createEPPWSMon(self, monWS):
        """Create an EPP table for a monitor workspace."""
        monEPPWS = _fitEPP(monWS, common.WS_CONTENT_MONS, self._names, self._subalgLogging)
        return monEPPWS

    def _finalize(self, outWS):
        """Do final cleanup and set the output property."""
        self.setProperty(common.PROP_OUTPUT_WS, outWS)
        self._cleanup.cleanup(outWS)
        self._cleanup.finalCleanup()
        self._report.toLog(self.log())

    def _eiCalibrationEnabled(self, mainWS):
        """Return true if incident energy calibration should be perfomed, false if not."""
        calibration = self.getProperty(common.PROP_INCIDENT_ENERGY_CALIBRATION).value
        state = None
        ENABLED_AUTOMATICALLY = 1
        if calibration == common.INCIDENT_ENERGY_CALIBRATION_OFF:
            return False
        elif calibration == common.INCIDENT_ENERGY_CALIBRATION_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter("enable_incident_energy_calibration"):
                enabled = instrument.getBoolParameter("enable_incident_energy_calibration")[0]
                if not enabled:
                    self._report.notice("Incident energy calibration disabled by the IPF.")
                    return False
                else:
                    state = ENABLED_AUTOMATICALLY
        monitorCounts = _monitorCounts(mainWS)
        if monitorCounts < _MONSUM_LIMIT:
            self._report.warning("'monsum' less than {}. Disabling incident energy calibration.".format(_MONSUM_LIMIT))
            return False
        if state == ENABLED_AUTOMATICALLY:
            self._report.notice("Incident energy calibration enabled.")
        return True

    def _flatBkgDet(self, mainWS):
        """Subtract flat background from a detector workspace."""
        if not self._flatBgkEnabled(mainWS):
            return mainWS
        if not self.getProperty(common.PROP_FLAT_BKG_WS).isDefault:
            bkgWS = self.getProperty(common.PROP_FLAT_BKG_WS).value
            self._cleanup.protect(bkgWS)
        else:
            windowWidth = self.getProperty(common.PROP_FLAT_BKG_WINDOW).value
            if not self.getProperty(common.PROP_DET_HOR_GROUPING).isDefault or not self.getProperty(common.PROP_DET_VER_GROUPING).isDefault:
                grouping_pattern = common.get_grouping_pattern(
                    mainWS, self.getProperty(common.PROP_DET_VER_GROUPING).value, self.getProperty(common.PROP_DET_HOR_GROUPING).value
                )
                flatInputWS = self._group_detectors(mainWS, grouping_pattern)
                flatOutputWS = _createFlatBkg(flatInputWS, common.WS_CONTENT_DETS, windowWidth, self._names, self._subalgLogging)
                bkgWS = self._ungroup_detectors(input_ws=flatOutputWS, ws_to_match=mainWS, grouping_pattern=grouping_pattern)
                self._cleanup.cleanup(flatInputWS)
                self._cleanup.cleanup(flatOutputWS)
            else:
                bkgWS = _createFlatBkg(mainWS, common.WS_CONTENT_DETS, windowWidth, self._names, self._subalgLogging)
        if not self.getProperty(common.PROP_OUTPUT_FLAT_BKG_WS).isDefault:
            self.setProperty(common.PROP_OUTPUT_FLAT_BKG_WS, bkgWS)
        bkgScaling = self.getProperty(common.PROP_FLAT_BKG_SCALING).value
        bkgSubtractedWS = _subtractFlatBkg(
            mainWS, common.WS_CONTENT_DETS, bkgWS, bkgScaling, self._names, self._cleanup, self._subalgLogging
        )
        self._cleanup.cleanup(mainWS)
        self._cleanup.cleanup(bkgWS)
        return bkgSubtractedWS

    def _flatBgkEnabled(self, mainWS):
        """Returns true if flat background subtraction is enabled, false otherwise."""
        flatBkgOption = self.getProperty(common.PROP_FLAT_BKG).value
        if flatBkgOption == common.BKG_AUTO:
            instrument = mainWS.getInstrument()
            if instrument.hasParameter("enable_flat_background_subtraction"):
                enabled = instrument.getBoolParameter("enable_flat_background_subtraction")[0]
                if not enabled:
                    self._report.notice("Flat background subtraction disabled by the IPF.")
                    return False
            self._report.notice("Flat background subtraction enabled.")
            return True
        return flatBkgOption != common.BKG_OFF

    def _flatBkgMon(self, monWS):
        """Subtract flat background from a monitor workspace."""
        windowWidth = self.getProperty(common.PROP_FLAT_BKG_WINDOW).value
        monBkgWS = _createFlatBkg(monWS, common.WS_CONTENT_MONS, windowWidth, self._names, self._subalgLogging)
        monBkgScaling = 1
        bkgSubtractedMonWS = _subtractFlatBkg(
            monWS, common.WS_CONTENT_MONS, monBkgWS, monBkgScaling, self._names, self._cleanup, self._subalgLogging
        )
        self._cleanup.cleanup(monBkgWS)
        self._cleanup.cleanup(monWS)
        return bkgSubtractedMonWS

    @staticmethod
    def _group_detectors(input_ws, grouping_pattern):
        """Groups detectors of the input workspace according to the provided pattern and returns
        the name of the grouped workspace."""
        output_ws = f"{input_ws}_grouped"
        GroupDetectors(InputWorkspace=input_ws, OutputWorkspace=output_ws, GroupingPattern=grouping_pattern, Behaviour="Average")
        return output_ws

    @staticmethod
    def _ungroup_detectors(input_ws, ws_to_match, grouping_pattern):
        """Assigns Y-axis values of grouped detectors back to the original (ungrouped) detectors."""
        output_ws = f"{input_ws}ungrouped"
        CreateWorkspace(
            DataX=ws_to_match.readX(0)[0] * ws_to_match.getNumberHistograms(),
            DataY=np.full(shape=(ws_to_match.getNumberHistograms()), fill_value=0.0, dtype="float64"),
            Nspec=ws_to_match.getNumberHistograms(),
            ParentWorkspace=ws_to_match,
            UnitX=ws_to_match.getAxis(0).getUnit().unitID(),
            OutputWorkspace=output_ws,
        )
        for group_no, grouped_detectors in enumerate(grouping_pattern.split(",")):
            det_list = list(map(int, grouped_detectors.split("+")))
            det_grouped_val = input_ws.readY(group_no)
            for det_no in det_list:
                mtd[output_ws].setY(det_no, det_grouped_val)

        return mtd[output_ws]

    def _inputWS(self):
        """Return the raw input workspace."""
        inputFiles = self.getPropertyValue(common.PROP_INPUT_FILE)
        if inputFiles:
            mergedWSName = self._names.withSuffix("merged")
            mainWS = LoadAndMerge(
                Filename=inputFiles,
                OutputWorkspace=mergedWSName,
                LoaderName="LoadILLTOF",
                LoaderOptions={"ConvertToTOF": True},
                EnableLogging=self._subalgLogging,
            )
        else:
            mainWS = self.getProperty(common.PROP_INPUT_WS).value
            self._cleanup.protect(mainWS)
        return mainWS

    def _monitorIndex(self, monWS):
        """Return the workspace index of the main monitor."""
        if self.getProperty(common.PROP_MON_INDEX).isDefault:
            NON_RECURSIVE = False  # Prevent recursive calls in the following.
            if not monWS.getInstrument().hasParameter("default-incident-monitor-spectrum", NON_RECURSIVE):
                raise RuntimeError(
                    "default-incident-monitor-spectrum missing in instrument parameters; " + common.PROP_MON_INDEX + " must be specified."
                )
            monIndex = monWS.getInstrument().getIntParameter("default-incident-monitor-spectrum", NON_RECURSIVE)[0]
            monIndex = common.convertToWorkspaceIndex(monIndex, monWS, common.INDEX_TYPE_SPECTRUM_NUMBER)
        else:
            monIndex = self.getProperty(common.PROP_MON_INDEX).value
            monIndex = common.convertToWorkspaceIndex(monIndex, monWS)
        return monIndex

    def _normalize(self, mainWS, monWS, monEPPWS):
        """Normalize to monitor or measurement time."""
        normalisationMethod = self.getProperty(common.PROP_NORMALISATION).value
        if normalisationMethod == common.NORM_METHOD_OFF:
            return mainWS
        if normalisationMethod == common.NORM_METHOD_MON:
            monitorCounts = _monitorCounts(monWS)
            if monitorCounts < _MONSUM_LIMIT:
                self._report.warning("'monsum' less than {}. Disabling normalization to monitor.".format(_MONSUM_LIMIT))
                normalisationMethod = common.NORM_METHOD_TIME
            else:
                sigmaMultiplier = self.getProperty(common.PROP_MON_PEAK_SIGMA_MULTIPLIER).value
                monIndex = self._monitorIndex(monWS)
                eppRow = monEPPWS.row(monIndex)
                if eppRow["FitStatus"] != "success":
                    self.log().warning(
                        "Fitting to monitor data failed. Integrating the intensity over " + "the entire TOF range for normalisation."
                    )
                    begin = monWS.dataX(monIndex)[0]
                    end = monWS.dataX(monIndex)[-1]
                else:
                    sigma = eppRow["Sigma"]
                    centre = eppRow["PeakCentre"]
                    begin = centre - sigmaMultiplier * sigma
                    end = centre + sigmaMultiplier * sigma
                normalizedWS = _normalizeToMonitor(mainWS, monWS, monIndex, begin, end, self._names, self._cleanup, self._subalgLogging)
                normalizedWS = _scaleAfterMonitorNormalization(normalizedWS, self._names, self._cleanup, self._subalgLogging)
        if normalisationMethod == common.NORM_METHOD_TIME:
            normalizedWS = _normalizeToTime(mainWS, self._names, self._cleanup, self._subalgLogging)
        self._cleanup.cleanup(mainWS)
        return normalizedWS

    def _outputDetEPPWS(self, mainWS):
        """Set the output epp workspace property, if needed."""
        if not self.getProperty(common.PROP_OUTPUT_DET_EPP_WS).isDefault:
            eppWS = self._createEPPWSDet(mainWS)
            self.setProperty(common.PROP_OUTPUT_DET_EPP_WS, eppWS)
            self._cleanup.cleanup(eppWS)

    def _outputRaw(self, mainWS, rawWS):
        """Optionally set mainWS as the raw output workspace."""
        if not self.getProperty(common.PROP_OUTPUT_RAW_WS).isDefault:
            CorrectTOFAxis(InputWorkspace=rawWS, OutputWorkspace=rawWS, ReferenceWorkspace=mainWS, EnableLogging=self._subalgLogging)
            self.setProperty(common.PROP_OUTPUT_RAW_WS, rawWS)
            DeleteWorkspace(Workspace=rawWS, EnableLogging=self._subalgLogging)

    def _separateMons(self, mainWS):
        """Extract monitors to a separate workspace."""
        detWSName = self._names.withSuffix("extracted_detectors")
        monWSName = self._names.withSuffix("extracted_monitors")
        detWS, monWS = ExtractMonitors(
            InputWorkspace=mainWS, DetectorWorkspace=detWSName, MonitorWorkspace=monWSName, EnableLogging=self._subalgLogging
        )
        self._cleanup.cleanup(mainWS)
        return detWS, monWS


AlgorithmFactory.subscribe(DirectILLCollectData)
