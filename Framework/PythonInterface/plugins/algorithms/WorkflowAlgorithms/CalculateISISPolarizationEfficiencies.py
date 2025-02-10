from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    PropertyMode,
)
from mantid.kernel import (
    Direction,
    StringArrayLengthValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
    CompositeValidator,
    StringPropertyWithValue,
)


from dataclasses import dataclass
from typing import Any
from copy import copy

_TRANS_ALG = "ReflectometryISISCreateTransmission"
_EFF_ALG = "PolarizationEfficienciesWildes"
_JOIN_ALG = "JoinISISPolarizationEfficiencies"


@dataclass(frozen=True)
class DataName:
    name: str = ""
    alias: str = ""


@dataclass(frozen=True)
class PropData(DataName):
    default: Any = None
    alg: str = ""
    mag: bool = None


class Prop:
    NON_MAG_INPUT_RUNS = PropData(name="NonMagInputRuns", alias="InputRuns", default=[], alg=_TRANS_ALG, mag=False)
    BACK_SUB_ROI = PropData(name="BackgroundProcessingInstructions", alg=_TRANS_ALG, mag=False)
    TRANS_ROI = PropData(name="ProcessingInstructions", alg=_TRANS_ALG, mag=False)

    MAG_INPUT_RUNS = PropData(name="MagInputRuns", alias="InputRuns", default=[], alg=_TRANS_ALG, mag=True)
    MAG_BACK_SUB_ROI = PropData(
        name="MagBackgroundProcessingInstructions", alias="BackgroundProcessingInstructions", alg=_TRANS_ALG, mag=True
    )
    MAG_TRANS_ROI = PropData(name="MagProcessingInstructions", alias="ProcessingInstructions", alg=_TRANS_ALG, mag=True)

    FLOOD_WS = PropData(name="FloodWorkspace", alg=_TRANS_ALG)
    I0_MON_IDX = PropData(name="I0MonitorIndex", alg=_TRANS_ALG)
    MON_WAV_MIN = PropData(name="MonitorIntegrationWavelengthMin", alg=_TRANS_ALG)
    MON_WAV_MAX = PropData(name="MonitorIntegrationWavelengthMax", alg=_TRANS_ALG)

    FLIPPERS = PropData(name="Flippers", alg=_EFF_ALG)
    INPUT_POL_EFF = PropData(name="InputPolarizerEfficiency", alg=_EFF_ALG)
    INPUT_AN_EFF = PropData(name="InputAnalyserEfficiency", alg=_EFF_ALG)
    INCLUDE_DIAG_OUT = PropData(name="IncludeDiagnosticOutputs", alg=_EFF_ALG)
    OUT_PHI = PropData(name="OutputPhi", alg=_EFF_ALG)
    OUT_RHO = PropData(name="OutputRho", alg=_EFF_ALG)
    OUT_ALPHA = PropData(name="OutputAlpha", alg=_EFF_ALG)
    OUT_TWO_P_MINUS_ONE = PropData(name="OutputTwoPMinusOne", alg=_EFF_ALG)
    OUT_TWO_A_MINUS_ONE = PropData(name="OutputTwoAMinusOne", alg=_EFF_ALG)

    OUT_WS = PropData(name="OutputWorkspace")


class NonPropData:
    OUT_FP_EFF = DataName(name="OutputFpEfficiency", alias="F1")
    OUT_FA_EFF = DataName(name="OutputFaEfficiency", alias="F2")
    OUT_POL_EFF = DataName(name="OutputPolarizerEfficiency", alias="P1")
    OUT_AN_EFF = DataName(name="OutputAnalyserEfficiency", alias="P2")
    OUT_PHI = DataName(name="OutputPhi", alias="OutputPhi")
    OUT_RHO = DataName(name="OutputRho", alias="OutputRho")
    OUT_AL = DataName(name="OutputAlpha", alias="OutputAlpha")
    OUT_2P = DataName(name="OutputTwoPMinusOne", alias="OutputTwoPMinusOne")
    OUT_2A = DataName(name="OutputTwoAMinusOne", alias="OutputTwoAMinusOne")


_EFF_ALG_OUTPUT = [NonPropData.OUT_FP_EFF, NonPropData.OUT_FA_EFF, NonPropData.OUT_POL_EFF, NonPropData.OUT_AN_EFF]
_EFF_ALG_OUTPUT_DIAG = [NonPropData.OUT_PHI, NonPropData.OUT_RHO, NonPropData.OUT_AL]
_EFF_ALG_OUTPUT_DIAG_MAG = [NonPropData.OUT_2P, NonPropData.OUT_2A]


class CalculateISISPolarizationEfficiencies(DataProcessorAlgorithm):
    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISCreateTransmission"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Create a transmission workspace for ISIS reflectometry data, including optional flood and background corrections."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return [self._FLOOD_ALG, self._BACK_SUB_ALG, self._TRANS_WS_ALG]

    @staticmethod
    def _create_input_run_validator() -> CompositeValidator:
        input_run_validator = CompositeValidator()
        input_run_validator.add(StringArrayMandatoryValidator())
        len_validator = StringArrayLengthValidator()
        len_validator.setLengthMin(1)
        input_run_validator.add(len_validator)
        return input_run_validator

    def PyInit(self):
        input_run_validator = self._create_input_run_validator()

        self.declareProperty(
            StringArrayProperty(Prop.NON_MAG_INPUT_RUNS.name, values=Prop.NON_MAG_INPUT_RUNS.default, validator=input_run_validator),
            doc="A list of input run numbers. Multiple runs will be summed after loading",
        )
        self.declareProperty(
            StringArrayProperty(Prop.MAG_INPUT_RUNS.name, values=Prop.MAG_INPUT_RUNS.default),
            doc="A list of magnetic input run numbers. Multiple runs will be summed after loading",
        )
        self.copyProperties(
            _TRANS_ALG,
            [
                Prop.TRANS_ROI.name,
                Prop.I0_MON_IDX.name,
                Prop.MON_WAV_MIN.name,
                Prop.MON_WAV_MAX.name,
                Prop.FLOOD_WS.name,
                Prop.BACK_SUB_ROI.name,
            ],
        )
        self.declareProperty(
            StringPropertyWithValue(Prop.MAG_TRANS_ROI.name, ""),
            doc="Grouping pattern of magnetic spectrum numbers to yield only the detectors of interest. See group detectors for syntax.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(Prop.MAG_BACK_SUB_ROI.name, "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The workspace to be used for the flood correction. If this property is not set then no flood correction is performed.",
        )
        self.copyProperties(
            _EFF_ALG,
            [
                Prop.FLIPPERS.name,
                Prop.INPUT_POL_EFF.name,
                Prop.INPUT_AN_EFF.name,
                Prop.INCLUDE_DIAG_OUT.name,
                Prop.OUT_PHI.name,
                Prop.OUT_RHO.name,
                Prop.OUT_ALPHA.name,
                Prop.OUT_TWO_A_MINUS_ONE.name,
                Prop.OUT_TWO_P_MINUS_ONE.name,
            ],
        )
        self.declareProperty(
            MatrixWorkspaceProperty(Prop.OUT_WS.name, "calc_pol_eff_out", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the workspace to be output as a result of the algorithm.",
        )

    def _populate_args_dict(self, alg_name: str, mag: bool = False) -> dict:
        args = {}
        for prop_str in [a for a in dir(Prop) if not a.startswith("__")]:
            prop = getattr(Prop, prop_str)
            if prop.alg == alg_name and (prop.mag == mag or prop.mag is None):
                key = prop.name if not prop.alias else prop.alias
                args.update({key: self.getProperty(prop.name).value})
        return args

    def PyExec(self):
        eff_args = {}
        trans_output = self._run_algorithm(_TRANS_ALG, self._populate_args_dict(_TRANS_ALG), ["OutputWorkspace"])
        eff_args.update({"InputNonMagWorkspace": trans_output, "OutputFpEfficiency": "fp_eff", "OutputFaEfficiency": "fa_eff"})
        mag_runs_input = bool(self.getProperty(Prop.MAG_INPUT_RUNS.name).value)
        if mag_runs_input:
            trans_output_mag = self._run_algorithm(_TRANS_ALG, self._populate_args_dict(_TRANS_ALG, mag=True), ["OutputWorkspace"])
            eff_args.update(
                {"InputMagWorkspace": trans_output_mag, "OutputPolarizerEfficiency": "p_eff", "OutputAnalyserEfficiency": "a_eff"}
            )
        eff_args.update(self._populate_args_dict(_EFF_ALG))
        eff_output = {key: None for key in _EFF_ALG_OUTPUT}
        include_diag_out = self.getProperty(Prop.INCLUDE_DIAG_OUT.name).value
        if include_diag_out:
            eff_alg_output_diag = copy(_EFF_ALG_OUTPUT_DIAG)
            if mag_runs_input:
                eff_alg_output_diag.extend(_EFF_ALG_OUTPUT_DIAG_MAG)
            eff_output.update({key: None for key in eff_alg_output_diag})
        eff_output.update(dict(zip([x.alias for x in eff_output], self._run_algorithm(_EFF_ALG, eff_args, [x.name for x in eff_output]))))
        join_output = self._run_algorithm(_JOIN_ALG, {key.alias: eff_output[key.alias] for key in _EFF_ALG_OUTPUT}, ["OutputWorkspace"])
        self.setProperty(Prop.OUT_WS.name, join_output[0])
        if include_diag_out:
            for key in eff_alg_output_diag:
                self.setProperty(key.name, eff_output[key.alias])

    def _run_algorithm(self, alg_name: str, args: dict, output_properties: list[str]):
        alg = self.createChildAlgorithm(alg_name, **args)
        alg.execute()
        result = []
        for key in output_properties:
            result.append(alg.getProperty(key).value)
        return result


AlgorithmFactory.subscribe(CalculateISISPolarizationEfficiencies)
