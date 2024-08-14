# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import AppendSpectra, CloneWorkspace, ElasticWindow, LoadLog, Logarithm, SortXAxis, Transpose
from mantid.kernel import *
from mantid.api import *

import numpy as np


def workspaces_have_same_size(workspaces):
    first_size = len(workspaces[0].readY(0))
    differently_sized_workspaces = [workspace for workspace in workspaces[1:] if len(workspace.readY(0)) != first_size]
    return len(differently_sized_workspaces) == 0


def _normalize_by_index(workspace, index):
    """
    Normalize each spectra of the specified workspace by the
    y-value at the specified index in that spectra.

    @param workspace    The workspace to normalize.
    @param index        The index of the y-value to normalize by.
    """
    number_of_histograms = workspace.getNumberHistograms()

    for idx in range(0, number_of_histograms):
        y_values = workspace.readY(idx)
        y_errors = workspace.readE(idx)

        # Avoid divide by zero
        if y_values[index] == 0.0:
            scale = np.reciprocal(1.0e-8)
        else:
            scale = np.reciprocal(y_values[index])

        # Normalise y values
        y_values_normalised = scale * y_values

        # Propagate y errors: C = A / B ; dC = sqrt( (dA/B)^2 + (A*dB/B^2)^2 )
        a = y_errors * scale
        b = y_values * y_errors[index] * (scale**2)
        y_errors_propagated = np.sqrt(a**2 + b**2)

        workspace.setY(idx, y_values_normalised)
        workspace.setE(idx, y_errors_propagated)


class ElasticWindowMultiple(DataProcessorAlgorithm):
    _sample_log_name = None
    _sample_log_value = None
    _input_workspaces = None
    _q_workspace = None
    _q2_workspace = None
    _elf_workspace = None
    _elt_workspace = None
    _integration_range_start = None
    _integration_range_end = None
    _background_range_start = None
    _background_range_end = None

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect"

    def summary(self):
        return "Performs the ElasticWindow algorithm over multiple input workspaces"

    def PyInit(self):
        self.declareProperty(WorkspaceGroupProperty("InputWorkspaces", "", Direction.Input), doc="Grouped input workspaces")

        self.declareProperty(name="IntegrationRangeStart", defaultValue=0.0, doc="Start of integration range in time of flight")
        self.declareProperty(name="IntegrationRangeEnd", defaultValue=0.0, doc="End of integration range in time of flight")

        self.declareProperty(
            name="BackgroundRangeStart", defaultValue=Property.EMPTY_DBL, doc="Start of background range in time of flight"
        )
        self.declareProperty(name="BackgroundRangeEnd", defaultValue=Property.EMPTY_DBL, doc="End of background range in time of flight")

        self.declareProperty(name="SampleEnvironmentLogName", defaultValue="sample", doc="Name of the sample environment log entry")

        sample_environment_log_values = ["last_value", "average"]
        self.declareProperty(
            "SampleEnvironmentLogValue",
            "last_value",
            StringListValidator(sample_environment_log_values),
            doc="Value selection of the sample environment log entry",
        )

        self.declareProperty(WorkspaceProperty("OutputInQ", "", Direction.Output), doc="Output workspace in Q")

        self.declareProperty(WorkspaceProperty("OutputInQSquared", "", Direction.Output), doc="Output workspace in Q Squared")

        self.declareProperty(WorkspaceProperty("OutputELF", "", Direction.Output, PropertyMode.Optional), doc="Output workspace ELF")

        self.declareProperty(WorkspaceProperty("OutputELT", "", Direction.Output, PropertyMode.Optional), doc="Output workspace ELT")

    def validateInputs(self):
        issues = dict()

        background_range_start = self.getProperty("BackgroundRangeStart").value
        background_range_end = self.getProperty("BackgroundRangeEnd").value

        if background_range_start != Property.EMPTY_DBL and background_range_end == Property.EMPTY_DBL:
            issues["BackgroundRangeEnd"] = "If background range start was given and " "background range end must also be provided."

        if background_range_start == Property.EMPTY_DBL and background_range_end != Property.EMPTY_DBL:
            issues["BackgroundRangeStart"] = "If background range end was given and background " "range start must also be provided."

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._sample_log_name = self.getPropertyValue("SampleEnvironmentLogName")
        self._sample_log_value = self.getPropertyValue("SampleEnvironmentLogValue")

        self._input_workspaces = self.getProperty("InputWorkspaces").value
        self._input_size = len(self._input_workspaces)
        self._elf_ws_name = self.getPropertyValue("OutputELF")
        self._elt_ws_name = self.getPropertyValue("OutputELT")

        self._integration_range_start = self.getProperty("IntegrationRangeStart").value
        self._integration_range_end = self.getProperty("IntegrationRangeEnd").value

        self._background_range_start = self.getProperty("BackgroundRangeStart").value
        self._background_range_end = self.getProperty("BackgroundRangeEnd").value

    def PyExec(self):
        from IndirectCommon import get_instrument_and_run

        # Do setup
        self._setup()

        # Lists of input and output workspaces
        q_workspaces = list()
        q2_workspaces = list()
        elf_workspace = None  # initializing for logs
        elt_workspace = None
        run_numbers = list()
        sample_param = list()

        progress = Progress(self, 0.0, 0.05, 3)

        # Perform the ElasticWindow algorithms
        for input_ws in self._input_workspaces:
            logger.information("Running ElasticWindow for workspace: {}".format(input_ws.name()))
            progress.report("ElasticWindow for workspace: {}".format(input_ws.name()))

            q_workspace, q2_workspace = ElasticWindow(
                InputWorkspace=input_ws,
                IntegrationRangeStart=self._integration_range_start,
                IntegrationRangeEnd=self._integration_range_end,
                BackgroundRangeStart=self._background_range_start,
                BackgroundRangeEnd=self._background_range_end,
                OutputInQ="__q",
                OutputInQSquared="__q2",
                StoreInADS=False,
                EnableLogging=False,
            )

            q2_workspace = Logarithm(InputWorkspace=q2_workspace, OutputWorkspace="__q2", StoreInADS=False, EnableLogging=False)

            q_workspaces.append(q_workspace)
            q2_workspaces.append(q2_workspace)

            # Get the run number
            run_no = get_instrument_and_run(input_ws.name())[1]
            run_numbers.append(run_no)

            # Get the sample environment unit
            sample, unit = self._get_sample_units(input_ws)
            if sample is not None:
                sample_param.append(sample)
            else:
                # No need to output a temperature workspace if there are no temperatures
                self._elt_ws_name = ""

        logger.information("Creating Q and Q^2 workspaces")
        progress.report("Creating Q workspaces")

        if self._input_size == 1:
            q_workspace = q_workspaces[0]
            q2_workspace = q2_workspaces[0]
        else:
            if not workspaces_have_same_size(q_workspaces) or not workspaces_have_same_size(q2_workspaces):
                raise RuntimeError(
                    "The ElasticWindow algorithm produced differently sized workspaces. Please check " "the input files are compatible."
                )
            q_workspace = _append_all(q_workspaces)
            q2_workspace = _append_all(q2_workspaces)

        # Set the vertical axis units
        v_axis_is_sample = self._input_size == len(sample_param)

        if v_axis_is_sample:
            logger.information("Vertical axis is in units of {}".format(unit))
            unit = (self._sample_log_name, unit)

            def axis_value(index):
                return float(sample_param[index])

        else:
            logger.information("Vertical axis is in run number")
            unit = ("Run No", " last 3 digits")

            def axis_value(index):
                return float(run_numbers[index][-3:])

        # Create and set new vertical axis for the Q and Q**2 workspaces
        _set_numeric_y_axis(q_workspace, self._input_size, unit, axis_value)
        _set_numeric_y_axis(q2_workspace, self._input_size, unit, axis_value)

        progress.report("Creating ELF workspaces")

        # Process the ELF workspace
        if self._elf_ws_name != "":
            logger.information("Creating ELF workspace")
            elf_workspace = _sort_x_axis(_transpose(q_workspace))
            self.setProperty("OutputELF", elf_workspace)

        # Do temperature normalisation
        if self._elt_ws_name != "":
            logger.information("Creating ELT workspace")

            # If the ELF workspace was not created, create the ELT workspace
            # from the Q workspace. Else, clone the ELF workspace.
            if self._elf_ws_name == "":
                elt_workspace = _sort_x_axis(_transpose(q_workspace))
            else:
                elt_workspace = CloneWorkspace(
                    InputWorkspace=elf_workspace, OutputWorkspace="__cloned", StoreInADS=False, EnableLogging=False
                )

            _normalize_by_index(elt_workspace, 0)

            self.setProperty("OutputELT", elt_workspace)
        # Add sample logs
        self._add_sample_logs_to_output_workspaces([q_workspace, q2_workspace, elf_workspace, elt_workspace])
        # Set the output workspace
        self.setProperty("OutputInQ", q_workspace)
        self.setProperty("OutputInQSquared", q2_workspace)

    def _get_sample_units(self, workspace):
        """
        Gets the sample environment units for a given workspace.

        @param workspace The workspace
        @returns sample in given units or None if not found
        """
        from IndirectCommon import get_instrument_and_run

        instr, run_number = get_instrument_and_run(workspace.name())

        pad_num = config.getInstrument(instr).zeroPadding(int(run_number))
        zero_padding = "0" * (pad_num - len(run_number))

        run_name = instr + zero_padding + run_number
        log_filename = run_name.upper() + ".log"

        run = workspace.getRun()

        position_logs = ["position", "samp_posn"]
        if self._sample_log_name.lower() in position_logs:
            self._sample_log_name = _extract_sensor_name(self._sample_log_name, run, workspace.getInstrument())

        if self._sample_log_name in run:
            # Look for sample unit in logs in workspace
            if self._sample_log_value == "last_value":
                sample = run[self._sample_log_name].value[-1]
            else:
                sample = run[self._sample_log_name].value.mean()

            unit = run[self._sample_log_name].units
        else:
            # Logs not in workspace, try loading from file
            logger.information("Log parameter not found in workspace. Searching for log file.")
            sample, unit = _extract_temperature_from_log(workspace, self._sample_log_name, log_filename, run_name)

        if sample is not None and unit is not None:
            logger.debug("{0} {1} found for run: {2}".format(sample, unit, run_name))
        else:
            logger.warning("No sample units found for run: {}".format(run_name))

        if unit is not None and unit.isspace():
            unit = ""

        return sample, unit

    def _add_sample_logs_to_output_workspaces(self, out_ws):
        names = ["integration_range_start", "integration_range_end"]
        values = [self._integration_range_start, self._integration_range_end]
        for ws in out_ws:
            if ws is not None:
                self._add_sample_log(ws, names, values)

    def _add_sample_log(self, workspace, names, values):
        add_log = self.createChildAlgorithm("AddSampleLogMultiple", enableLogging=False)
        add_log.setProperty("Workspace", workspace)
        add_log.setProperty("LogNames", names)
        add_log.setProperty("LogValues", values)
        add_log.setProperty("ParseType", True)
        add_log.execute()


def _extract_temperature_from_log(workspace, sample_log_name, log_filename, run_name):
    log_path = FileFinder.getFullPath(log_filename)

    if not log_path:
        logger.warning("Log file for run {} not found".format(run_name))
        return None, None

    LoadLog(Workspace=workspace, Filename=log_filename, EnableLogging=False)
    run = workspace.getRun()

    if sample_log_name in run:
        temperature = run[sample_log_name].value[-1]
        unit = run[sample_log_name].units
        return temperature, unit

    logger.warning("Log entry {0} for run {1} not found".format(sample_log_name, run_name))
    return None, None


def _extract_sensor_name(sample_log_name, run, instrument):
    position = _extract_position_from_run(sample_log_name, run, instrument)
    if position is not None:
        default_names = ["Bot_Can_Top", "Middle_Can_Top", "Top_Can_Top"]
        sensor_names = instrument.getStringParameter("Workflow.TemperatureSensorNames")[0].split(",")

        if position < len(sensor_names) and sensor_names[position] in run:
            return sensor_names[position]
        elif position < len(default_names):
            logger.warning(
                "Position {0} not found within the instrument parameters, " "using default '{1}'.".format(position, default_names[position])
            )
            return default_names[position]
        else:
            logger.warning("Invalid position ({}) found in workspace.".format(position))
    else:
        logger.information("Position not found in sample logs, when using log name {}.".format(sample_log_name))
    return ""


def _extract_position_from_run(sample_log_name, run, instrument):
    if sample_log_name in run:
        if sample_log_name.lower() == "position":
            return _index_of_position(run[sample_log_name].value[-1])
        elif sample_log_name.lower() == "samp_posn":
            return _index_of_samp_posn(run[sample_log_name].value[-1], instrument)
    return None


def _index_of_position(position_log_value):
    if isinstance(position_log_value, str):
        return {"B": 0, "M": 1, "T": 2}.get(position_log_value[0], None)
    return int(position_log_value)


def _index_of_samp_posn(samp_posn_log_value, instrument):
    if instrument.hasParameter("Workflow.SamplePositions"):
        sample_positions = instrument.getStringParameter("Workflow.SamplePositions")[0].split(",")
        if samp_posn_log_value in sample_positions:
            return sample_positions.index(samp_posn_log_value)
    return 0


def _set_numeric_y_axis(workspace, length, unit, get_axis_value):
    workspace_axis = NumericAxis.create(length)
    workspace_axis.setUnit("Label").setLabel(unit[0], unit[1])

    for index in range(length):
        workspace_axis.setValue(index, get_axis_value(index))
    workspace.replaceAxis(1, workspace_axis)


def _append_all(workspaces):
    initial_workspace = workspaces[0]

    for workspace in workspaces[1:]:
        initial_workspace = _append_spectra(initial_workspace, workspace)
    return initial_workspace


def _append_spectra(workspace1, workspace2):
    return AppendSpectra(
        InputWorkspace1=workspace1, InputWorkspace2=workspace2, OutputWorkspace="__appended", StoreInADS=False, EnableLogging=False
    )


def _transpose(workspace):
    return Transpose(InputWorkspace=workspace, OutputWorkspace="__transposed", StoreInADS=False, EnableLogging=False)


def _sort_x_axis(workspace):
    return SortXAxis(InputWorkspace=workspace, OutputWorkspace="__sorted", StoreInADS=False, EnableLogging=False)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ElasticWindowMultiple)
