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
from mantid.kernel import (
    CompositeValidator,
    StringArrayLengthValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
    Direction,
    StringListValidator,
    Property,
)

import math


class ReflectometryISISPreprocess(DataProcessorAlgorithm):
    _RUNS = "InputRunList"
    _GROUP_TOF = "GroupTOFWorkspaces"
    _OUTPUT_WS = "OutputWorkspace"
    _MONITOR_WS = "MonitorWorkspace"
    _EVENT_MODE = "EventMode"
    _CALIBRATION_FILE = "CalibrationFile"
    _THETA_IN = "ThetaIn"
    _THETA_LOG_NAME = "ThetaLogName"
    _CALIBRATION_FILE_LOG = "reflectometry_calibration_file"
    _POLREF = "POLREF"
    _POLREF_START_WS_INDEX = 4
    _HANDLE_METHOD_ALREADY_CALIBRATED = "IfAlreadyCalibrated"

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

        self._experiment_angle = None

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
        self.declareProperty(
            self._HANDLE_METHOD_ALREADY_CALIBRATED,
            "NONE",
            validator=StringListValidator(["NONE", "WARN", "THROW"]),
            doc="How to handle loaded files that have already been calibrated, if calibration file specified.",
        )
        self.declareProperty(
            name="AdjustedTheta",
            defaultValue=Property.EMPTY_DBL,
            direction=Direction.Output,
            doc="The value of theta following angle correction",
        )

    def PyExec(self):
        workspace, monitor_ws = self._loadRun(self.getPropertyValue(self._RUNS))

        calibration_file = self.getPropertyValue(self._CALIBRATION_FILE)
        if calibration_file:
            self.handle_if_already_calibrated(workspace)
            workspace = self._applyCalibration(workspace, calibration_file)

        self.setProperty(self._OUTPUT_WS, workspace)
        if monitor_ws:
            self.setProperty(self._MONITOR_WS, monitor_ws)

    def handle_if_already_calibrated(self, workspace):
        handle_method = self.getPropertyValue(self._HANDLE_METHOD_ALREADY_CALIBRATED)
        if handle_method == "NONE":
            return
        if isinstance(workspace, WorkspaceGroup):
            for ws in workspace:
                self.handle_if_already_calibrated(ws)
            return
        if workspace.run().hasProperty(self._CALIBRATION_FILE_LOG):
            if handle_method == "WARN":
                self.log().warning(
                    f"Workspace with run no. {workspace.getRunNumber()} already has a calibration file log. "
                    "The calibration algorithm will be rerun, which may produce erroneous results."
                )
            else:  # handle_method == "THROW"
                raise RuntimeError(f"Workspace with run no. {workspace.getRunNumber()} already has a calibration file log.")

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

    def _apply_calibration_impl(
        self, ws: MatrixWorkspace, calibration_filepath: str, specular_pixel_spectrum_no: float | None = None
    ) -> MatrixWorkspace:
        alg = self.createChildAlgorithm("ReflectometryISISCalibration")
        alg.setProperty("InputWorkspace", ws)
        alg.setProperty("CalibrationFile", calibration_filepath)

        if specular_pixel_spectrum_no is not None:  # Only present for POLREF workflow
            alg.setProperty("InstrumentWorkflow", self._POLREF)
            alg.setProperty("SpecularPixelSpectrumNo", specular_pixel_spectrum_no)
            alg.setProperty("ExperimentAngle", self._get_experiment_angle(ws))

        alg.execute()
        calibrated_ws = alg.getProperty("OutputWorkspace").value
        calibrated_ws.run().addProperty(self._CALIBRATION_FILE_LOG, calibration_filepath, True)
        self.log().information(f"Calibrated workspace {ws.getName()}")
        return calibrated_ws

    def _applyCalibration(self, ws: MatrixWorkspace, calibration_filepath: str) -> MatrixWorkspace:
        is_group = isinstance(ws, WorkspaceGroup)
        ws1 = ws[0] if is_group else ws
        specular_pixel_spectrum_no = None
        input_theta = self._get_experiment_angle(ws1)
        adjusted_theta = None
        if ws1.getInstrument().getName() == self._POLREF:
            specular_pixel_spectrum_no = self._find_specular_pixel_spectrum_no(ws1, self._POLREF_START_WS_INDEX)
            adjusted_theta = self._adjust_input_theta(input_theta, specular_pixel_spectrum_no, calibration_filepath)
        self.setProperty("AdjustedTheta", adjusted_theta or input_theta)

        if is_group:
            calibrated_group = WorkspaceGroup()
            for member in ws:
                calibrated_group.addWorkspace(self._apply_calibration_impl(member, calibration_filepath, specular_pixel_spectrum_no))
            return calibrated_group
        return self._apply_calibration_impl(ws, calibration_filepath, specular_pixel_spectrum_no)

    def _find_specular_pixel_spectrum_no(self, ws: MatrixWorkspace, start_index: int) -> float:
        lines_alg = self.createChildAlgorithm("FindReflectometryLines")
        lines_alg.setProperty("InputWorkspace", ws)
        lines_alg.setProperty("StartWorkspaceIndex", start_index)
        lines_alg.execute()
        line_centre = lines_alg.getProperty("LineCentre").value
        return self._spectrum_number_for_workspace_index(ws, line_centre)

    def _adjust_input_theta(self, input_theta: float, specular_pixel_spectrum_no: float, calibration_filepath: str) -> float:
        TEMP_POLREF_SPEC_PIXEL = 280
        frac_spec_pixel_diff = specular_pixel_spectrum_no - TEMP_POLREF_SPEC_PIXEL
        # TODO: check if we're handling spectrum number or index here
        round_fn = math.ceil if frac_spec_pixel_diff > 0 else math.floor
        theta_values = self._get_pixel_positions_from_calibration_map(
            calibration_filepath, [TEMP_POLREF_SPEC_PIXEL, round_fn(specular_pixel_spectrum_no)]
        )
        adjusted_theta = input_theta + (frac_spec_pixel_diff * (theta_values[-1] - theta_values[0]))
        self._experiment_angle = adjusted_theta
        return adjusted_theta

    def _get_pixel_positions_from_calibration_map(self, calibration_filepath: str, pixel_index_list: list):
        with open(calibration_filepath, "r") as file:
            file_entries = self._file_entries(file)
            # TODO: Would be nice to have the validation from `parse_offset_calibration_file` here from `ReflectometryISISCalibraiton`
            next(file_entries)
            idx_angle_map = {int(entries[0]): float(entries[1]) for entries in file_entries}
            try:
                positions = [idx_angle_map[pixel] for pixel in pixel_index_list]
            except KeyError:
                raise RuntimeError("Specular pixel value provided is not present in pixel map")
        return positions

    def _get_experiment_angle(self, ws: MatrixWorkspace) -> float:
        if self._experiment_angle:
            return self._experiment_angle

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

    # TODO: This is pretty much copied from ReflectometryISISCalibration
    # Can we extract to a helper?
    def _file_entries(self, file):
        import csv

        file_reader = csv.reader(file)
        for row in file_reader:
            if len(row) == 0:
                # Ignore any blank lines
                continue

            entries = row[0].split()
            if not entries:
                # Ignore whitespace-only lines
                continue

            if entries[0][0] == "#":
                # Ignore any lines that begin with a #
                # This allows the user to add any metadata they would like
                continue

            yield entries

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
