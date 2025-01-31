from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    PropertyMode,
    WorkspaceGroup,
)
from mantid.kernel import (
    Direction,
    StringArrayLengthValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
    CompositeValidator,
    logger,
)


from dataclasses import dataclass
from typing import Any

_TRANS_ALG = "ReflectometryISISCreateTransmission"
_EFF_ALG = "PolarizationEfficienciesWildes"
_JOIN_ALG = "JoinISISPolarizationEfficiencies"


@dataclass
class PropData:
    name: str = ""
    default: Any = None
    alg: str = ""
    mag: bool = None


class Prop:
    NON_MAG_INPUT_RUNS = PropData(name="NonMagInputRuns", default=[], alg=_TRANS_ALG, mag=False)
    BACK_SUB_ROI = PropData(name="BackgroundProcessingInstructions", alg=_TRANS_ALG, mag=False)
    TRANS_ROI = PropData(name="ProcessingInstructions", alg=_TRANS_ALG, mag=False)

    MAG_INPUT_RUNS = PropData(name="MagInputRuns", default=[], alg=_TRANS_ALG, mag=True)
    MAG_BACK_SUB_ROI = PropData(name="MagBackgroundProcessingInstructions", alg=_TRANS_ALG, mag=True)
    MAG_TRANS_ROI = PropData(name="MagProcessingInstructions", alg=_TRANS_ALG, mag=True)

    FLOOD_WS = PropData(name="FloodWorkspace", alg=_TRANS_ALG)
    I0_MON_IDX = PropData(name="I0MonitorIndex", alg=_TRANS_ALG)
    MON_WAV_MIN = PropData(name="MonitorIntegrationWavelengthMin", alg=_TRANS_ALG)
    MON_WAV_MAX = PropData(name="MonitorIntegrationWavelengthMax", alg=_TRANS_ALG)

    FLIPPERS = PropData(name="Flippers", default=["00", "01", "10", "11"], alg=_EFF_ALG)
    INPUT_POL_EFF = PropData(name="InputPolarizerEfficiency", alg=_EFF_ALG)
    INPUT_AN_EFF = PropData(name="InputAnalyserEfficiency", alg=_EFF_ALG)

    OUTPUT_WS = PropData(name="OutputWorkspace")


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
            self._TRANS_ALG,
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
            MatrixWorkspaceProperty(Prop.MAG_BACK_SUB_ROI.name, "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The workspace to be used for the flood correction. If this property is not set then no flood correction is performed.",
        )
        self.declareProperty(
            StringArrayProperty(Prop.FLIPPERS.name, values=Prop.FLIPPERS.default),
            doc="Flipper configurations of the input group workspace",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(Prop.INPUT_POL_EFF.name, "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing the known wavelength-dependent efficiency for the polarizer.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(Prop.INPUT_AN_EFF.name, "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing the known wavelength-dependent efficiency for the analyser.",
        )

    def _populate_args_dict(self, alg_name: str, mag: bool = False) -> dict:
        args = {}
        for prop_str in [a for a in dir(Prop) if not a.startswith("__")]:
            prop = getattr(Prop, prop_str)
            if prop.alg == alg_name and (prop.mag == mag or prop.mag is None):
                args.update({prop.name, self.getProperty(prop.name).value})
        return args

    def PyExec(self):
        input_workspace = self.getProperty(Prop.NON_MAG_INPUT_RUNS.name).value
        trans_output = self._run_algorithm(input_workspace, _TRANS_ALG, Prop.NON_MAG_INPUT_RUNS.name, self._populate_args_dict(_TRANS_ALG))

        input_workspace_mag = self.getProperty(Prop.MAG_INPUT_RUNS.name).value
        eff_args = {}
        if input_workspace_mag:
            trans_output_mag = self._run_algorithm(
                input_workspace_mag, _TRANS_ALG, Prop.MAG_INPUT_RUNS.name, self._populate_args_dict(_TRANS_ALG, mag=True)
            )
            eff_args.update({"InputNonMagWorkspace", trans_output_mag})
        eff_args.update(self._populate_args_dict(_EFF_ALG))
        eff_output = self._run_algorithm(trans_output, _EFF_ALG, Prop.NON_MAG_INPUT_RUNS.name, eff_args)
        print(eff_output)  # to keep ruff happy

        # Join WS

        # self.setProperty(Prop.OUTPUT_WS, ws)
        print("test")

    def _run_algorithm_impl(self, input_ws, input_ws_prop_name, output_ws, alg_name: str, args: dict):
        args.update({input_ws_prop_name: input_ws, "OutputWorkspace": output_ws})
        alg = self.createChildAlgorithm(alg_name, **args)
        alg.setRethrows(True)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _run_algorithm(self, workspace, input_ws_prop_name: str, alg_name: str, args: dict, output_ws_name=None):
        """Run the specified algorithm as a child algorithm using the arguments provided.
        The input and output workspace properties are both set to be the provided workspace,
        unless otherwise specified."""

        # If the input run loads as a workspace group, then using the default workspace group handling of other Mantid algorithms
        # prevents us retrieving an output workspace group from this algorithm when it is run as a child (unless we store things
        # in the ADS). See issue #38473.
        # To avoid this, when we have a workspace group we loop through it, run each algorithm against the child
        # workspaces individually and then collect them into a group again at the end
        if isinstance(workspace, WorkspaceGroup):
            output_grp = WorkspaceGroup()
            for child_ws in workspace:
                if output_ws_name:
                    # enable output workspace name as a list?
                    logger.notice(f"output workspace name {output_ws_name} not used as input was a workspace group")
                output_grp.addWorkspace(self._run_alg_impl(child_ws, input_ws_prop_name, child_ws, alg_name, args))
            return output_grp
        else:
            output_ws_name = workspace if not output_ws_name else output_ws_name
            return self._run_alg_impl(workspace, input_ws_prop_name, output_ws_name, alg_name, args)


AlgorithmFactory.subscribe(CalculateISISPolarizationEfficiencies)
