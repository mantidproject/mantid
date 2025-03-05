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
    IntArrayProperty,
)

from dataclasses import dataclass
from typing import Any, Callable, Optional

_ALGS = {
    "TRANS_ALG": "ReflectometryISISCreateTransmission",
    "EFF_ALG": "PolarizationEfficienciesWildes",
    "JOIN_ALG": "JoinISISPolarizationEfficiencies",
}


@dataclass(frozen=True)
class DataName:
    name: str = ""
    alias: str = ""


@dataclass(frozen=True)
class PropData(DataName):
    default: Optional[Any] = None
    alg: str = ""
    mag: Optional[bool] = None
    get_value: Optional[Callable] = None


def _int_array_to_string(raw_value):
    if len(raw_value) == 0:
        return ""
    min_val = min(raw_value)
    max_val = max(raw_value)
    return str(min_val) if (min_val == max_val) else f"{min_val}-{max_val}"


_PROP_DATA = {
    "NON_MAG_INPUT_RUNS": PropData(name="NonMagInputRuns", alias="InputRuns", default=[], alg=_ALGS["TRANS_ALG"], mag=False),
    "BACK_SUB_ROI": PropData(
        name="BackgroundProcessingInstructions", default=[], alg=_ALGS["TRANS_ALG"], mag=False, get_value=_int_array_to_string
    ),
    "TRANS_ROI": PropData(name="ProcessingInstructions", default=[], alg=_ALGS["TRANS_ALG"], mag=False, get_value=_int_array_to_string),
    "MAG_INPUT_RUNS": PropData(name="MagInputRuns", alias="InputRuns", default=[], alg=_ALGS["TRANS_ALG"], mag=True),
    "MAG_BACK_SUB_ROI": PropData(
        name="MagBackgroundProcessingInstructions",
        alias="BackgroundProcessingInstructions",
        default=[],
        alg=_ALGS["TRANS_ALG"],
        mag=True,
        get_value=_int_array_to_string,
    ),
    "MAG_TRANS_ROI": PropData(
        name="MagProcessingInstructions",
        default=[],
        alias="ProcessingInstructions",
        alg=_ALGS["TRANS_ALG"],
        mag=True,
        get_value=_int_array_to_string,
    ),
    "FLOOD_WS": PropData(name="FloodWorkspace", alg=_ALGS["TRANS_ALG"]),
    "I0_MON_IDX": PropData(name="I0MonitorIndex", alg=_ALGS["TRANS_ALG"]),
    "MON_WAV_MIN": PropData(name="MonitorIntegrationWavelengthMin", alg=_ALGS["TRANS_ALG"]),
    "MON_WAV_MAX": PropData(name="MonitorIntegrationWavelengthMax", alg=_ALGS["TRANS_ALG"]),
    "FLIPPERS": PropData(name="Flippers", alg=_ALGS["EFF_ALG"]),
    "INPUT_POL_EFF": PropData(name="InputPolarizerEfficiency", alg=_ALGS["EFF_ALG"]),
    "INPUT_AN_EFF": PropData(name="InputAnalyserEfficiency", alg=_ALGS["EFF_ALG"]),
    "INCLUDE_DIAG_OUT": PropData(name="IncludeDiagnosticOutputs", alg=_ALGS["EFF_ALG"]),
    "OUT_PHI": PropData(name="OutputPhi", alg=_ALGS["EFF_ALG"]),
    "OUT_RHO": PropData(name="OutputRho", alg=_ALGS["EFF_ALG"]),
    "OUT_ALPHA": PropData(name="OutputAlpha", alg=_ALGS["EFF_ALG"]),
    "OUT_TWO_P_MINUS_ONE": PropData(name="OutputTwoPMinusOne", alg=_ALGS["EFF_ALG"]),
    "OUT_TWO_A_MINUS_ONE": PropData(name="OutputTwoAMinusOne", alg=_ALGS["EFF_ALG"]),
    "OUT_WS": PropData(name="OutputWorkspace"),
}

_NON_PROP_DATA = {
    "OUT_FP_EFF": DataName(name="OutputFpEfficiency", alias="F1"),
    "OUT_FA_EFF": DataName(name="OutputFaEfficiency", alias="F2"),
    "OUT_POL_EFF": DataName(name="OutputPolarizerEfficiency", alias="P1"),
    "OUT_AN_EFF": DataName(name="OutputAnalyserEfficiency", alias="P2"),
    "OUT_PHI": DataName(name="OutputPhi", alias="OutputPhi"),
    "OUT_RHO": DataName(name="OutputRho", alias="OutputRho"),
    "OUT_AL": DataName(name="OutputAlpha", alias="OutputAlpha"),
    "OUT_2P": DataName(name="OutputTwoPMinusOne", alias="OutputTwoPMinusOne"),
    "OUT_2A": DataName(name="OutputTwoAMinusOne", alias="OutputTwoAMinusOne"),
    "IN_NON_MAG_WS": DataName(name="InputNonMagWorkspace"),
    "IN_MAG_WS": DataName(name="InputMagWorkspace"),
}

_EFF_ALG_OUTPUT = [_NON_PROP_DATA["OUT_FP_EFF"], _NON_PROP_DATA["OUT_FA_EFF"], _NON_PROP_DATA["OUT_POL_EFF"], _NON_PROP_DATA["OUT_AN_EFF"]]
_EFF_ALG_OUTPUT_DIAG = [_NON_PROP_DATA["OUT_PHI"], _NON_PROP_DATA["OUT_RHO"], _NON_PROP_DATA["OUT_AL"]]
_EFF_ALG_OUTPUT_DIAG_MAG = [_NON_PROP_DATA["OUT_2P"], _NON_PROP_DATA["OUT_2A"]]


class ReflectometryISISCalculatePolEff(DataProcessorAlgorithm):
    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISCalculatePolEff"

    def summary(self):
        """Return a summary of the algorithm."""
        return (
            "A wrapper algorithm around `ReflectometryISISCreateTransmission`, `PolarizationEfficienciesWildes`"
            " and `JoinISISPolarizationEfficiencies`."
        )

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return [_ALGS["TRANS_ALG"], _ALGS["EFF_ALG"], _ALGS["JOIN_ALG"]]

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
            StringArrayProperty(
                _PROP_DATA["NON_MAG_INPUT_RUNS"].name, values=_PROP_DATA["NON_MAG_INPUT_RUNS"].default, validator=input_run_validator
            ),
            doc="A list of input run numbers. Multiple runs will be summed after loading",
        )
        self.declareProperty(
            IntArrayProperty(_PROP_DATA["TRANS_ROI"].name, values=_PROP_DATA["TRANS_ROI"].default),
            doc="Grouping pattern of spectrum numbers to yield only the detectors of interest. See group detectors for syntax.",
        )
        self.declareProperty(
            IntArrayProperty(_PROP_DATA["BACK_SUB_ROI"].name, values=_PROP_DATA["BACK_SUB_ROI"].default),
            doc="A set of workspace indices to be passed as the ProcessingInstructions property when calculating the transmission"
            " workspace. If this property is not set then no background subtraction is performed.",
        )
        self.declareProperty(
            StringArrayProperty(_PROP_DATA["MAG_INPUT_RUNS"].name, values=_PROP_DATA["MAG_INPUT_RUNS"].default),
            doc="A list of magnetic input run numbers. Multiple runs will be summed after loading",
        )
        self.declareProperty(
            IntArrayProperty(_PROP_DATA["MAG_TRANS_ROI"].name, values=_PROP_DATA["MAG_TRANS_ROI"].default),
            doc="Grouping pattern of magnetic spectrum numbers to yield only the detectors of interest. See group detectors for syntax.",
        )
        self.declareProperty(
            IntArrayProperty(_PROP_DATA["MAG_BACK_SUB_ROI"].name, values=_PROP_DATA["MAG_BACK_SUB_ROI"].default),
            doc="A set of workspace indices to be passed as the ProcessingInstructions property when calculating the magnetic transmission"
            " workspace. If this property is not set then no background subtraction is performed.",
        )
        for alg, props in {
            _ALGS["TRANS_ALG"]: ["I0_MON_IDX", "MON_WAV_MIN", "MON_WAV_MAX", "FLOOD_WS"],
            _ALGS["EFF_ALG"]: [
                "FLIPPERS",
                "INPUT_POL_EFF",
                "INPUT_AN_EFF",
                "INCLUDE_DIAG_OUT",
                "OUT_PHI",
                "OUT_RHO",
                "OUT_ALPHA",
                "OUT_TWO_A_MINUS_ONE",
                "OUT_TWO_P_MINUS_ONE",
            ],
        }.items():
            self.copyProperties(alg, [_PROP_DATA[prop].name for prop in props])
        self.declareProperty(
            MatrixWorkspaceProperty(
                _PROP_DATA["OUT_WS"].name, "calc_pol_eff_out", direction=Direction.Output, optional=PropertyMode.Optional
            ),
            doc="The name of the workspace to be output as a result of the algorithm.",
        )

    def PyExec(self):
        # Set class member variables and perform post-start validation
        self._initialize()
        trans_output, trans_output_mag = self._create_transmission_workspaces()
        eff_output = self._calculate_wildes_efficiencies(trans_output, trans_output_mag)
        join_output = self._run_algorithm(
            _ALGS["JOIN_ALG"], {key.alias: eff_output[key.alias] for key in _EFF_ALG_OUTPUT}, ["OutputWorkspace"]
        )
        self._set_output_properties(join_output[0], eff_output)

    def _create_transmission_workspaces(self) -> (list, list):
        trans_output = self._run_algorithm(_ALGS["TRANS_ALG"], self._populate_args_dict(_ALGS["TRANS_ALG"]), ["OutputWorkspace"])
        trans_output_mag = (
            self._run_algorithm(_ALGS["TRANS_ALG"], self._populate_args_dict(_ALGS["TRANS_ALG"], mag=True), ["OutputWorkspace"])
            if self.m_mag_runs_input
            else None
        )
        return trans_output, trans_output_mag

    def _calculate_wildes_efficiencies(self, trans_output: list, trans_output_mag: list) -> dict[str, Any]:
        eff_args = self._generate_eff_args(trans_output, trans_output_mag)
        eff_output_list = _EFF_ALG_OUTPUT + self.m_eff_alg_output_diag
        eff_output = dict(
            zip([x.alias for x in eff_output_list], self._run_algorithm(_ALGS["EFF_ALG"], eff_args, [x.name for x in eff_output_list]))
        )
        return eff_output

    def _initialize(self):
        self.m_mag_runs_input = bool(self.getProperty(_PROP_DATA["MAG_INPUT_RUNS"].name).value)
        self.m_eff_alg_output_diag = []
        if self.getProperty(_PROP_DATA["INCLUDE_DIAG_OUT"].name).value:
            self.m_eff_alg_output_diag = _EFF_ALG_OUTPUT_DIAG + (_EFF_ALG_OUTPUT_DIAG_MAG if self.m_mag_runs_input else [])
        self._validate_processing_instructions()

    def _validate_processing_instructions(self):
        if self.m_mag_runs_input:
            if not (
                len(self.getProperty(_PROP_DATA["TRANS_ROI"].name).value) == len(self.getProperty(_PROP_DATA["MAG_TRANS_ROI"].name).value)
            ):
                raise ValueError("The number of spectra specified in both magnetic and non-magnetic processing properties must be equal")

    def _populate_args_dict(self, alg_name: str, mag: bool = False) -> dict[str:Any]:
        args = {}
        for prop in _PROP_DATA.values():
            if prop.alg == alg_name and (prop.mag == mag or prop.mag is None):
                key = prop.name if not prop.alias else prop.alias
                raw_value = self.getProperty(prop.name).value
                args.update({key: raw_value if not prop.get_value else prop.get_value(raw_value)})
        return args

    def _run_algorithm(self, alg_name: str, args: dict, output_properties: list[str]) -> list:
        alg = self.createChildAlgorithm(alg_name, **args)
        try:
            alg.execute()
        except Exception as e:
            raise RuntimeError(f"""Error thrown during execution of child algorithm: {alg_name}.\nArguments: {args}\nException: {str(e)}""")
        result = []
        for key in output_properties:
            result.append(alg.getProperty(key).value)
        return result

    def _generate_eff_args(self, trans_output: list, trans_output_mag: list) -> dict[str:Any]:
        eff_args = {
            _NON_PROP_DATA["IN_NON_MAG_WS"].name: trans_output,
            _NON_PROP_DATA["OUT_FP_EFF"].name: _NON_PROP_DATA["OUT_FP_EFF"].alias,
            _NON_PROP_DATA["OUT_FA_EFF"].name: _NON_PROP_DATA["OUT_FA_EFF"].alias,
        }
        if trans_output_mag:
            eff_args.update(
                {
                    _NON_PROP_DATA["IN_MAG_WS"].name: trans_output_mag,
                    _NON_PROP_DATA["OUT_POL_EFF"].name: _NON_PROP_DATA["OUT_POL_EFF"].alias,
                    _NON_PROP_DATA["OUT_AN_EFF"].name: _NON_PROP_DATA["OUT_AN_EFF"].alias,
                }
            )
        eff_args.update(self._populate_args_dict(_ALGS["EFF_ALG"]))
        return eff_args

    def _set_output_properties(self, join_ws, eff_output):
        self.setProperty(_PROP_DATA["OUT_WS"].name, join_ws)
        for key in self.m_eff_alg_output_diag:
            self.setProperty(key.name, eff_output[key.alias])


AlgorithmFactory.subscribe(ReflectometryISISCalculatePolEff)
