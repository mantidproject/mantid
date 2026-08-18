# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    MatrixWorkspace,
    PropertyMode,
    WorkspaceGroup,
    WorkspaceProperty,
)
from mantid.kernel import CompositeValidator, StringArrayLengthValidator, StringArrayMandatoryValidator, StringArrayProperty, Direction


class ReflectometryISISPreprocess(DataProcessorAlgorithm):
    _RUNS = "InputRunList"
    _GROUP_TOF = "GroupTOFWorkspaces"
    _OUTPUT_WS = "OutputWorkspace"
    _MONITOR_WS = "MonitorWorkspace"
    _EVENT_MODE = "EventMode"
    _CALIBRATION_FILE = "CalibrationFile"
    _THETA_IN = "ThetaIn"
    _THETA_LOG_NAME = "ThetaLogName"

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISPreprocess"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Preprocess ISIS reflectometry data, including optional loading and summing of the input runs."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["ReflectometryISISLoadAndProcess", "ReflectometryReductionOneAuto"]

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty(self._RUNS, values=[], validator=self._get_input_runs_validator()),
            doc="A list of run numbers or workspace names to load and preprocess",
        )
        self.declareProperty(self._EVENT_MODE, False, direction=Direction.Input, doc="If true, load the input workspaces as event data")
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WS, "", direction=Direction.Output),
            doc="The preprocessed output workspace. If multiple input runs are specified "
            "they will be summed into a single output workspace.",
        )
        self.declareProperty(
            MatrixWorkspaceProperty(self._MONITOR_WS, "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The loaded monitors workspace. This is only output in event mode.",
        )
        self.copyProperties("ReflectometryISISCalibration", [self._CALIBRATION_FILE])
        self.copyProperties("ReflectometryReductionOneAuto", [self._THETA_IN, self._THETA_LOG_NAME])

    def PyExec(self):
        workspace, monitor_ws = self._loadRun(self.getPropertyValue(self._RUNS))

        calibration_file = self.getPropertyValue(self._CALIBRATION_FILE)
        if calibration_file:
            workspace = self._applyCalibration(workspace, calibration_file)

        self.setProperty(self._OUTPUT_WS, workspace)
        if monitor_ws:
            self.setProperty(self._MONITOR_WS, monitor_ws)

    @staticmethod
    def _get_input_runs_validator():
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        lenValidator = StringArrayLengthValidator()
        lenValidator.setLengthMin(1)
        mandatoryInputRuns.add(lenValidator)
        return mandatoryInputRuns

    def _loadRun(self, run: str) -> MatrixWorkspace:
        """Load a run as an event workspace if slicing is requested, or a histogram
        workspace otherwise. Transmission runs are always loaded as histogram workspaces."""
        event_mode = self.getProperty(self._EVENT_MODE).value
        monitor_ws = None
        if event_mode:
            alg = self.createChildAlgorithm("LoadEventNexus", Filename=run, LoadMonitors=True)
            alg.execute()
            ws = alg.getProperty("OutputWorkspace").value
            monitor_ws = alg.getProperty("MonitorWorkspace").value
            self._validate_event_ws(ws)
            self.log().information("Loaded event workspace")
        else:
            alg = self.createChildAlgorithm("LoadNexus", Filename=run)
            alg.execute()
            ws = alg.getProperty("OutputWorkspace").value
            self.log().information("Loaded workspace ")
        return ws, monitor_ws

    def _applyCalibration(self, ws: MatrixWorkspace, calibration_filepath: str) -> MatrixWorkspace:
        if isinstance(ws, WorkspaceGroup):
            raise RuntimeError("Calibrating a Workspace Group as part of pre-processing is not currently supported")

        alg = self.createChildAlgorithm("ReflectometryISISCalibration")
        alg.setProperty("InputWorkspace", ws)
        alg.setProperty("CalibrationFile", calibration_filepath)

        if ws.getInstrument().getName() == "POLREF":
            alg.setProperty("InstrumentWorkflow", "POLREF")

            lines_alg = self.createChildAlgorithm("FindReflectometryLines")
            lines_alg.setProperty("InputWorkspace", ws)
            lines_alg.execute()
            line_centre = lines_alg.getProperty("LineCentre").value
            specular_pixel_spectrum_no = self._spectrum_number_for_workspace_index(ws, line_centre)

            alg.setProperty("SpecularPixelSpectrumNo", specular_pixel_spectrum_no)
            alg.setProperty("ExperimentAngle", self._experiment_angle(ws))

        alg.execute()
        calibrated_ws = alg.getProperty("OutputWorkspace").value
        self.log().information("Calibrated workspace")
        return calibrated_ws

    def _experiment_angle(self, ws: MatrixWorkspace) -> float:
        theta = self.getProperty(self._THETA_IN)
        if not theta.isDefault:
            return theta.value

        theta_log_name = self.getPropertyValue(self._THETA_LOG_NAME)
        if theta_log_name:
            theta_log = ws.run().getProperty(theta_log_name)
            if hasattr(theta_log, "lastValue"):
                return theta_log.lastValue()
            return float(theta_log.value)

        raise RuntimeError("ThetaIn or ThetaLogName must be provided when calibrating POLREF data")

    @staticmethod
    def _spectrum_number_for_workspace_index(ws: MatrixWorkspace, workspace_index: float) -> float:
        lower_index = int(workspace_index)
        fraction = workspace_index - lower_index
        lower_spectrum_no = ws.getSpectrum(lower_index).getSpectrumNo()
        if fraction == 0.0:
            return float(lower_spectrum_no)

        upper_spectrum_no = ws.getSpectrum(lower_index + 1).getSpectrumNo()
        return lower_spectrum_no + fraction * (upper_spectrum_no - lower_spectrum_no)

    @staticmethod
    def _validate_event_ws(workspace):
        if isinstance(workspace, WorkspaceGroup):
            # Our reduction algorithm doesn't currently support this due to slicing
            # (which would result in a group of groups)
            raise RuntimeError("Loading Workspace Groups in event mode is not supported currently.")
        if not workspace.run().hasProperty("proton_charge"):
            # Reduction algorithm requires proton_charge
            raise RuntimeError("Event workspaces must contain proton_charge")


AlgorithmFactory.subscribe(ReflectometryISISPreprocess)
