# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class LiveValue:
    """Hold the value and unit of a live instrument block value. Also hold an
    alternative block name - this is not ideal but is required because
    instruments are switching between instrument control software and could
    have different block names. Longer term it would be good to configure these
    so they don't need to be hard coded but we may end up removing this
    approach altogether in the long run so this is sufficient for now.
    """

    LOG_TYPE_NUM_SERIES = "Number Series"

    def __init__(self, value, unit, alternative_name, log_type):
        self.value = value
        self.unit = unit
        self.alternative_name = alternative_name
        self.log_type = log_type


class ReflectometryReductionOneLiveData(DataProcessorAlgorithm):
    def category(self):
        return "Reflectometry"

    def summary(self):
        return "Run the reflectometry reduction algorithm on live data"

    def seeAlso(self):
        return ["ReflectometryISISLoadAndProcess", "StartLiveData"]

    def PyInit(self):
        instruments = ["CRISP", "INTER", "OFFSPEC", "POLREF", "SURF"]
        defaultInstrument = str(config.getInstrument())
        defaultInstrument = defaultInstrument if defaultInstrument in instruments else instruments[0]
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input))
        self.declareProperty(
            name="Instrument",
            defaultValue=defaultInstrument,
            direction=Direction.Input,
            validator=StringListValidator(instruments),
            doc="Instrument to find live value for.",
        )
        self.declareProperty(
            name="GetLiveValueAlgorithm",
            defaultValue="GetLiveInstrumentValue",
            direction=Direction.Input,
            doc="The algorithm to use to get live values from the instrument",
        )

        self._child_properties = [
            "FirstTransmissionRunList",
            "SecondTransmissionRunList",
            "SliceWorkspace",
            "NumberOfSlices",
            "SummationType",
            "ReductionType",
            "IncludePartialBins",
            "AnalysisMode",
            "ProcessingInstructions",
            "CorrectDetectors",
            "DetectorCorrectionType",
            "WavelengthMin",
            "WavelengthMax",
            "I0MonitorIndex",
            "MonitorBackgroundWavelengthMin",
            "MonitorBackgroundWavelengthMax",
            "MonitorIntegrationWavelengthMin",
            "MonitorIntegrationWavelengthMax",
            "NormalizeByIntegratedMonitors",
            "SubtractBackground",
            "BackgroundProcessingInstructions",
            "BackgroundCalculationMethod",
            "DegreeOfPolynomial",
            "CostFunction",
            "Params",
            "StartOverlap",
            "EndOverlap",
            "ScaleRHSWorkspace",
            "TransmissionProcessingInstructions",
            "CorrectionAlgorithm",
            "Polynomial",
            "C0",
            "C1",
            "MomentumTransferMin",
            "MomentumTransferStep",
            "MomentumTransferMax",
            "ScaleFactor",
            "PolarizationAnalysis",
            "FloodCorrection",
            "FloodWorkspace",
            "Debug",
            "TimeInterval",
            "LogValueInterval",
            "LogName",
            "UseNewFilterAlgorithm",
            "ReloadInvalidWorkspaces",
            "GroupTOFWorkspaces",
            "CalibrationFile",
            "OutputWorkspace",
            "PolarizationEfficiencies",
            "ROIDetectorIDs",
        ]
        self.copyProperties("ReflectometryISISLoadAndProcess", self._child_properties)

    def PyExec(self):
        self._setup_workspace_for_reduction()
        alg = self._setup_reduction_algorithm()
        self._run_reduction_algorithm(alg)

    def _setup_workspace_for_reduction(self):
        """Set up the workspace ready for the reduction"""
        in_ws_name = self.getPropertyValue("InputWorkspace")
        self._temp_ws_name = "__" + self.getPropertyValue("OutputWorkspace")
        self._instrument = self.getProperty("Instrument").value
        # Set up a clone for the output because we need to do some in-place manipulations
        CloneWorkspace(InputWorkspace=in_ws_name, OutputWorkspace=self._temp_ws_name)
        liveValues = self._get_live_values_from_instrument()
        self._setup_sample_logs(liveValues)
        # Set up the instrument after adding the sample logs in case the IDF uses any log values
        self._setup_instrument()
        self._setup_slits(liveValues)

    def _setup_reduction_algorithm(self):
        """Set up the reduction algorithm"""
        alg = self.createChildAlgorithm("ReflectometryISISLoadAndProcess")
        self._copy_property_values_to(alg)
        alg.setProperty("InputRunList", self._temp_ws_name)
        alg.setProperty("ThetaLogName", "Theta")
        alg.setProperty("GroupTOFWorkspaces", False)
        alg.setProperty("ReloadInvalidWorkspaces", False)
        alg.setProperty("OutputWorkspaceBinned", self._temp_ws_name)
        return alg

    def _run_reduction_algorithm(self, alg):
        """Run the reduction"""
        alg.execute()
        out_ws = alg.getProperty("OutputWorkspaceBinned").value
        self.setProperty("OutputWorkspace", out_ws)
        AnalysisDataService.remove(self._temp_ws_name)

    def _setup_instrument(self):
        """Sets the instrument name and loads the instrument on the workspace"""
        LoadInstrument(Workspace=self._temp_ws_name, RewriteSpectraMap=True, InstrumentName=self._instrument)

    def _setup_sample_logs(self, liveValues):
        """Set up the sample logs based on live values from the instrument"""
        log_names = []
        log_values = []
        log_units = []
        log_types = []
        for key, live_values in liveValues.items():
            log_names.append(key)
            log_values.append(live_values.value)
            log_units.append(live_values.unit)
            log_types.append(live_values.log_type)

        AddSampleLogMultiple(
            Workspace=self._temp_ws_name, LogNames=log_names, LogValues=log_values, LogUnits=log_units, LogTypes=log_types, ParseType=False
        )

    def _setup_slits(self, liveValues):
        """Set up instrument parameters for the slits"""
        s1 = liveValues[self._s1vg_name()].value
        s2 = liveValues[self._s2vg_name()].value
        SetInstrumentParameter(
            Workspace=self._temp_ws_name, ParameterName="vertical gap", ParameterType="Number", ComponentName="slit1", Value=str(s1)
        )
        SetInstrumentParameter(
            Workspace=self._temp_ws_name, ParameterName="vertical gap", ParameterType="Number", ComponentName="slit2", Value=str(s2)
        )

    def _copy_property_values_to(self, alg):
        for prop in self._child_properties:
            value = self.getPropertyValue(prop)
            alg.setPropertyValue(prop, value)

    def _get_live_values_from_instrument(self):
        # get values from instrument
        liveValues = self._live_value_list()
        for name, liveValue in liveValues.items():
            if liveValue.value is None:
                try:
                    liveValue.value = self._get_block_value_from_instrument(name)
                except:
                    self.log().information("Failed to get value " + name + " from the instrument; trying " + liveValue.alternative_name)
                    liveValue.value = self._get_block_value_from_instrument(liveValue.alternative_name)
        self._fixup_zero_theta(liveValues)
        # check we have all we need
        self._validate_live_values(liveValues)
        return liveValues

    def _live_value_list(self):
        """Get the list of required live value names and their unit type"""
        liveValues = {
            self._theta_name(): LiveValue(None, "deg", self._alternative_theta_name(), LiveValue.LOG_TYPE_NUM_SERIES),
            self._s1vg_name(): LiveValue(None, "m", self._alternative_s1vg_name(), LiveValue.LOG_TYPE_NUM_SERIES),
            self._s2vg_name(): LiveValue(None, "m", self._alternative_s2vg_name(), LiveValue.LOG_TYPE_NUM_SERIES),
        }
        return liveValues

    def _theta_name(self):
        return "THETA"

    def _alternative_theta_name(self):
        return "Theta"

    def _s1vg_name(self):
        return "S1VG"

    def _alternative_s1vg_name(self):
        return "s1vg"

    def _s2vg_name(self):
        return "S2VG"

    def _alternative_s2vg_name(self):
        return "s2vg"

    def _get_double_or_none(self, propertyName):
        value = self.getProperty(propertyName)
        if value == Property.EMPTY_DBL:
            return None
        return value.value

    def _get_block_value_from_instrument(self, logName):
        algName = self.getProperty("GetLiveValueAlgorithm").value
        alg = self.createChildAlgorithm(algName)
        alg.setProperty("Instrument", self._instrument)
        alg.setProperty("PropertyType", "Block")
        alg.setProperty("PropertyName", logName)
        alg.execute()
        return alg.getProperty("Value").value

    def _fixup_zero_theta(self, liveValues):
        """If theta is zero the reduction will fail. For a consistent output plot while
        monitoring live data, users would like to still see an IvsQ plot with similar axes,
        so instead of failing we give a false angle of 0.7 (it doesn't really matter what this
        is as long as it is in a typical region for ISIS runs) and run the reduction anyway. This
        will give a plot that is close to a straight line at zero intensity, so it is obvious that
        it is not a normal run, but the plot axes do not change much compared to a normal run."""
        if float(liveValues[self._theta_name()].value) <= 1e-06:
            liveValues[self._theta_name()].value = 0.7

    def _validate_live_values(self, liveValues):
        for key in liveValues:
            if liveValues[key].value is None:
                raise RuntimeError("Required value " + key + " was not found for instrument")


AlgorithmFactory.subscribe(ReflectometryReductionOneLiveData)
