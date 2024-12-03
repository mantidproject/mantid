# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    PropertyMode,
    WorkspaceGroup,
    WorkspaceProperty,
)
from mantid.kernel import Direction, StringArrayLengthValidator, StringArrayMandatoryValidator, StringArrayProperty, CompositeValidator


class Prop:
    INPUT_RUNS = "InputRuns"
    OUTPUT_WS = "OutputWorkspace"
    FLOOD_WS = "FloodWorkspace"
    BACK_SUB_ROI = "BackgroundProcessingInstructions"
    TRANS_ROI = "ProcessingInstructions"
    I0_MON_IDX = "I0MonitorIndex"
    MON_WAV_MIN = "MonitorIntegrationWavelengthMin"
    MON_WAV_MAX = "MonitorIntegrationWavelengthMax"


class ReflectometryISISCreateTransmission(DataProcessorAlgorithm):
    _LOAD_ALG = "LoadAndMerge"
    _FLOOD_ALG = "ApplyFloodWorkspace"
    _BACK_SUB_ALG = "ReflectometryBackgroundSubtraction"
    _TRANS_WS_ALG = "CreateTransmissionWorkspaceAuto"

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

    def PyInit(self):
        mandatory_runs = CompositeValidator()
        mandatory_runs.add(StringArrayMandatoryValidator())
        len_validator = StringArrayLengthValidator()
        len_validator.setLengthMin(1)
        mandatory_runs.add(len_validator)
        self.declareProperty(
            StringArrayProperty(Prop.INPUT_RUNS, values=[], validator=mandatory_runs),
            doc="A list of input run numbers. Multiple runs will be summed after loading",
        )

        self.copyProperties(self._TRANS_WS_ALG, [Prop.TRANS_ROI, Prop.I0_MON_IDX, Prop.MON_WAV_MIN, Prop.MON_WAV_MAX])

        self.declareProperty(
            MatrixWorkspaceProperty(Prop.FLOOD_WS, "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The workspace to be used for the flood correction. If this property is not set then no flood correction is performed.",
        )

        self.declareProperty(
            Prop.BACK_SUB_ROI,
            "",
            doc=f"The set of workspace indices to be passed to the ProcessingInstructions property of the {self._BACK_SUB_ALG} algorithm."
            " If this property is not set then no background subtraction is performed.",
        )

        self.declareProperty(
            WorkspaceProperty(Prop.OUTPUT_WS, "", direction=Direction.Output),
            doc="The output transmission workspace",
        )

    def PyExec(self):
        ws = self._load_and_sum_runs(self.getProperty(Prop.INPUT_RUNS).value, self.getPropertyValue(Prop.OUTPUT_WS))

        ws = self._apply_flood_correction(ws)
        ws = self._apply_background_subtraction(ws)
        ws = self._create_transmission_ws(ws)

        self.setProperty(Prop.OUTPUT_WS, ws)

    def _load_and_sum_runs(self, runs: list[str], output_name: str):
        """Load and sum the input runs"""
        self.log().information("Loading and summing the run files")
        alg = self.createChildAlgorithm(self._LOAD_ALG, Filename="+".join(runs), LoaderName="LoadNexus", OutputWorkspace=output_name)
        alg.setRethrows(True)
        alg.setProperty("MergeRunsOptions", {"FailBehaviour": "Stop"})
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _apply_flood_correction(self, workspace):
        """If a flood workspace has been provided then apply a flood correction"""
        flood_ws = self.getProperty(Prop.FLOOD_WS).value
        if flood_ws is None:
            return workspace

        self.log().information("Performing flood correction")
        args = {"FloodWorkspace": flood_ws}
        corrected_ws = self._run_algorithm(workspace, self._FLOOD_ALG, "InputWorkspace", args)

        # The flood correction performs a divide, which can result in the workspace being set as a distribution
        # if the flood and input workspaces have the same Y units. We need to set the resulting workspace back to
        # a non-distribution as it may be dimensionless after the divide, but it shouldn't be a distribution.
        if isinstance(corrected_ws, WorkspaceGroup):
            for child_ws in corrected_ws:
                child_ws.setDistribution(False)
        else:
            corrected_ws.setDistribution(False)

        return corrected_ws

    def _apply_background_subtraction(self, workspace):
        """If background processing instructions have been provided then perform a background subtraction"""
        if self.getProperty(Prop.BACK_SUB_ROI).isDefault:
            return workspace

        self.log().information("Performing background subtraction")
        args = {
            "InputWorkspaceIndexType": "WorkspaceIndex",
            "ProcessingInstructions": self.getPropertyValue(Prop.BACK_SUB_ROI),
            "BackgroundCalculationMethod": "PerDetectorAverage",
        }

        try:
            return self._run_algorithm(workspace, self._BACK_SUB_ALG, "InputWorkspace", args)
        except Exception as ex:
            # The error that's printed can be confusing if we don't mention the background subtraction algorithm
            self.log().error(f"Error running {self._BACK_SUB_ALG}")
            raise RuntimeError(ex)

    def _create_transmission_ws(self, workspace):
        """Create the transmission workspace"""
        self.log().information("Creating the transmission workspace")
        args = {
            Prop.TRANS_ROI: self.getPropertyValue(Prop.TRANS_ROI),
            Prop.I0_MON_IDX: self.getPropertyValue(Prop.I0_MON_IDX),
            Prop.MON_WAV_MIN: self.getPropertyValue(Prop.MON_WAV_MIN),
            Prop.MON_WAV_MAX: self.getPropertyValue(Prop.MON_WAV_MAX),
        }
        return self._run_algorithm(workspace, self._TRANS_WS_ALG, "FirstTransmissionRun", args)

    def _run_algorithm(self, workspace, alg_name: str, input_ws_prop_name: str, args: dict):
        """Run the specified algorithm as a child algorithm using the arguments provided.
        The input and output workspace properties are both set to be the provided workspace."""

        def run_alg(ws):
            args.update({input_ws_prop_name: ws, "OutputWorkspace": ws})
            alg = self.createChildAlgorithm(alg_name, **args)
            alg.setRethrows(True)
            alg.execute()
            return alg.getProperty("OutputWorkspace").value

        # If the input run loads as a workspace group, then using the default workspace group handling of other Mantid algorithms
        # prevents us retrieving an output workspace group from this algorithm when it is run as a child (unless we store things
        # in the ADS). To avoid this, when we have a workspace group we loop through it, run each algorithm against the child
        # workspaces individually and then collect them into a group again at the end
        if isinstance(workspace, WorkspaceGroup):
            output_grp = WorkspaceGroup()
            for child_ws in workspace:
                output_grp.addWorkspace(run_alg(child_ws))
            return output_grp

        return run_alg(workspace)


AlgorithmFactory.subscribe(ReflectometryISISCreateTransmission)
