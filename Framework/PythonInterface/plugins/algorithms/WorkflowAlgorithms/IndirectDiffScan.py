# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from mantid import config

from IndirectCommon import formatRuns


class IndirectDiffScan(DataProcessorAlgorithm):
    _data_files = None
    _sum_files = None
    _load_logs = None
    _calibration_ws = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _spectra_range = None
    _background_range = None
    _elastic_range = None
    _inelastic_range = None
    _rebin_string = None
    _detailed_balance = None
    _grouping_method = None
    _grouping_ws = None
    _grouping_map_file = None
    _output_ws = None
    _output_x_units = None
    _plot_type = None
    _save_formats = None
    _ipf_filename = None
    _sample_log_name = None
    _sample_log_value = None
    _workspace_names = None
    _scan_ws = None

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS"

    def summary(self):
        return "Runs an energy transfer reduction for an inelastic indirect geometry instrument."

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma separated list of input files")

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Load log files when loading runs")

        # Instrument configuration properties
        self.declareProperty(
            name="Instrument", defaultValue="", validator=StringListValidator(["IRIS", "OSIRIS"]), doc="Instrument used during run."
        )

        int_arr_valid = IntArrayBoundedValidator(lower=0)

        self.declareProperty(
            IntArrayProperty(name="SpectraRange", values=[0, 1], validator=int_arr_valid),
            doc="Comma separated range of spectra number to use.",
        )

        self.declareProperty(name="SampleEnvironmentLogName", defaultValue="sample", doc="Name of the sample environment log entry")

        sampEnvLogVal_type = ["last_value", "average"]
        self.declareProperty(
            "SampleEnvironmentLogValue",
            "last_value",
            StringListValidator(sampEnvLogVal_type),
            doc="Value selection of the sample environment log entry",
        )

        # Output properties
        self.declareProperty(name="OutputWorkspace", defaultValue="Output", doc="Workspace group for the resulting workspaces.")

    def PyExec(self):
        self._setup()

        process_prog = Progress(self, start=0.1, end=0.9, nreports=len(self._workspace_names))
        process_prog.report("Running diffraction")
        scan_alg = self.createChildAlgorithm("ISISIndirectDiffractionReduction", 0.05, 0.95)
        scan_alg.setProperty("InputFiles", formatRuns(self._data_files, self._instrument_name))
        scan_alg.setProperty("ContainerFiles", self._can_files)
        scan_alg.setProperty("ContainerScaleFactor", self._can_scale)
        scan_alg.setProperty("CalFile", self._calib_file)
        scan_alg.setProperty("SumFiles", self._sum_files)
        scan_alg.setProperty("LoadLogFiles", self._load_logs)
        scan_alg.setProperty("Instrument", self._instrument_name)
        scan_alg.setProperty("Mode", self._mode)
        scan_alg.setProperty("SpectraRange", self._spectra_range)
        scan_alg.setProperty("RebinParam", self._rebin_paras)
        scan_alg.setProperty("GroupingMethod", self._grouping_method)
        scan_alg.setProperty("OutputWorkspace", self._output_ws)
        scan_alg.execute()
        logger.information("OutputWorkspace : %s" % self._output_ws)

        self.setProperty("OutputWorkspace", scan_alg.getPropertyValue("OutputWorkspace"))

        workspace_names = mtd[self._output_ws].getNames()
        scan_workspace = self._output_ws + "_scan"
        temperatures = list()
        run_numbers = []
        for input_ws in workspace_names:
            temp = self._get_temperature(input_ws)
            if temp is not None:
                temperatures.append(temp)
            # Get the run number
            run_no = self._get_InstrRun(input_ws)[1]
            run_numbers.append(run_no)

        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=False)
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=False)
        for idx in range(len(workspace_names)):
            if idx == 0:
                clone_alg.setProperty("InputWorkspace", workspace_names[0])
                clone_alg.setProperty("OutputWorkspace", scan_workspace)
                clone_alg.execute()
                scan_workspace = clone_alg.getProperty("OutputWorkspace").value
            else:
                append_alg.setProperty("InputWorkspace1", scan_workspace)
                append_alg.setProperty("InputWorkspace2", workspace_names[idx])
                append_alg.setProperty("OutputWorkspace", scan_workspace)
                append_alg.execute()
                scan_workspace = append_alg.getProperty("OutputWorkspace").value

        # Set the vertical axis units
        num_hist = scan_workspace.getNumberHistograms()
        v_axis_is_temp = num_hist == len(temperatures)

        if v_axis_is_temp:
            logger.notice("Vertical axis is in temperature")
            unit = ("Temperature", "K")
        else:
            logger.notice("Vertical axis is in run number")
            unit = ("Run No", " last 3 digits")

        # Create a new vertical axis for the workspaces
        y_ws_axis = NumericAxis.create(len(run_numbers))
        y_ws_axis.setUnit("Label").setLabel(unit[0], unit[1])

        # Set the vertical axis values
        for idx in range(num_hist):
            if v_axis_is_temp:
                y_ws_axis.setValue(idx, float(temperatures[idx]))
            else:
                y_ws_axis.setValue(idx, float(run_numbers[idx][-3:]))

        # Add the new vertical axis to each workspace
        scan_workspace.replaceAxis(1, y_ws_axis)

        mtd.addOrReplace(self._output_ws + "_scan", scan_workspace)

    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate spectra range
        spectra_range = self.getProperty("SpectraRange").value
        if len(spectra_range) != 2:
            issues["SpectraRange"] = "Range must contain exactly two items"
        elif spectra_range[0] > spectra_range[1]:
            issues["SpectraRange"] = "Range must be in format: lower,upper"

        return issues

    def _get_temperature(self, ws_name):
        """
        Gets the sample temperature for a given workspace.

        @param ws_name Name of workspace
        @returns Temperature in Kelvin or None if not found
        """
        instr, run_number = self._get_InstrRun(ws_name)

        pad_num = config.getInstrument(instr).zeroPadding(int(run_number))
        zero_padding = "0" * (pad_num - len(run_number))

        run_name = instr + zero_padding + run_number
        log_filename = run_name.upper() + ".log"

        run = mtd[ws_name].getRun()

        if self._sample_log_name in run:
            # Look for temperature in logs in workspace
            tmp = run[self._sample_log_name].value
            value_action = {"last_value": lambda x: x[-1], "average": lambda x: x.mean()}
            temp = value_action[self._sample_log_value](tmp)
            logger.debug("Temperature %d K found for run: %s" % (temp, run_name))
            return temp

        else:
            # Logs not in workspace, try loading from file
            logger.information("Log parameter not found in workspace. Searching for log file.")
            log_path = FileFinder.getFullPath(log_filename)

            if log_path != "":
                # Get temperature from log file
                LoadLog(Workspace=ws_name, Filename=log_path)
                run_logs = mtd[ws_name].getRun()
                if self._sample_log_name in run_logs:
                    tmp = run_logs[self._sample_log_name].value
                    temp = tmp[-1]
                    logger.debug("Temperature %d K found for run: %s" % (temp, run_name))
                    return temp
                else:
                    logger.warning("Log entry %s for run %s not found" % (self._sample_log_name, run_name))
            else:
                logger.warning("Log file for run %s not found" % run_name)

        # Can't find log file
        logger.warning("No temperature found for run: %s" % run_name)
        return None

    def _get_InstrRun(self, ws_name):
        """
        Get the instrument name and run number from a workspace.

        @param ws_name - name of the workspace
        @return tuple of form (instrument, run number)
        """

        run_number = str(mtd[ws_name].getRunNumber())
        if run_number == "0":
            # Attempt to parse run number off of name
            match = re.match(r"([a-zA-Z]+)([0-9]+)", ws_name)
            if match:
                run_number = match.group(2)
            else:
                raise RuntimeError("Could not find run number associated with workspace.")

        instrument = mtd[ws_name].getInstrument().getName()
        if instrument != "":
            for facility in config.getFacilities():
                try:
                    instrument = facility.instrument(instrument).filePrefix(int(run_number))
                    instrument = instrument.lower()
                    break
                except RuntimeError:
                    continue

        return instrument, run_number

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty("InputFiles").value
        self._can_files = ""
        self._can_scale = 1.0
        self._calib_file = ""
        self._sum_files = False
        self._load_logs = self.getProperty("LoadLogFiles").value

        self._instrument_name = self.getPropertyValue("Instrument")
        self._mode = "diffspec"

        self._spectra_range = self.getProperty("SpectraRange").value
        self._rebin_paras = ""

        self._grouping_method = "All"

        self._sample_log_name = self.getPropertyValue("SampleEnvironmentLogName")
        self._sample_log_value = self.getPropertyValue("SampleEnvironmentLogValue")

        self._output_ws = self.getPropertyValue("OutputWorkspace")

        # Disable sum files if there is only one file
        if len(self._data_files) == 1:
            if self._sum_files:
                logger.warning("SumFiles disabled when only one input file is provided.")
            self._sum_files = False

        # The list of workspaces being processed
        self._workspace_names = []


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectDiffScan)
