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

import os
import numpy as np

from IndirectCommon import format_runs


class SofQWMomentsScan(DataProcessorAlgorithm):
    _data_files = None
    _sum_files = None
    _load_logs = None
    _calibration_ws = None
    _instrument_name = None
    _analyser = None
    _reflection = None
    _efixed = None
    _resolution = None
    _spectra_range = None
    _background_range = None
    _rebin_string = None
    _detailed_balance = None
    _grouping_method = None
    _grouping_ws = None
    _grouping_map_file = None
    _output_x_units = None
    _ipf_filename = None
    _sample_log_name = None
    _sample_log_value = None
    _workspace_names = None

    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect;Workflow\\MIDAS"

    def summary(self):
        return "Runs an energy transfer reduction for an inelastic indirect geometry instrument."

    def PyInit(self):
        # Input properties
        self.declareProperty(StringArrayProperty(name="InputFiles"), doc="Comma separated list of input files")

        self.declareProperty(name="LoadLogFiles", defaultValue=True, doc="Load log files when loading runs")

        self.declareProperty(
            WorkspaceProperty("CalibrationWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing calibration data",
        )

        # Instrument configuration properties
        self.declareProperty(
            name="Instrument", defaultValue="", validator=StringListValidator(["IRIS", "OSIRIS"]), doc="Instrument used during run."
        )
        self.declareProperty(
            name="Analyser",
            defaultValue="",
            validator=StringListValidator(["graphite", "mica", "fmica"]),
            doc="Analyser bank used during run.",
        )
        self.declareProperty(
            name="Reflection",
            defaultValue="",
            validator=StringListValidator(["002", "004", "006"]),
            doc="Reflection number for instrument setup during run.",
        )
        self.declareProperty(
            IntArrayProperty(name="SpectraRange", values=[0, 1], validator=IntArrayMandatoryValidator()),
            doc="Comma separated range of spectra number to use.",
        )

        range_val = FloatArrayLengthValidator(3)

        self.declareProperty(
            FloatArrayProperty(name="QRange", validator=range_val),
            doc="Range of background to subtract from raw data in time of flight. Start, Width, End",
        )
        self.declareProperty(
            FloatArrayProperty(name="EnergyRange", validator=range_val),
            doc="Range of background to subtract from raw data in time of flight. Start, Width, End",
        )
        self.declareProperty(name="DetailedBalance", defaultValue=Property.EMPTY_DBL, doc="Apply the detailed balance correction")

        # Spectra grouping options
        self.declareProperty(
            name="GroupingMethod",
            defaultValue="Individual",
            validator=StringListValidator(["Individual", "All", "File", "Workspace", "IPF"]),
            doc="Method used to group spectra.",
        )
        self.declareProperty(
            WorkspaceProperty("GroupingWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace containing spectra grouping.",
        )
        self.declareProperty(
            FileProperty("MapFile", "", action=FileAction.OptionalLoad, extensions=[".map"]), doc="File containing spectra grouping."
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
        self.declareProperty("ReducedWorkspace", defaultValue="Reduced", doc="The output reduced workspace.")
        self.declareProperty("SqwWorkspace", defaultValue="Sqw", doc="The output Sqw workspace.")
        self.declareProperty(name="MomentWorkspace", defaultValue="Moment", doc="The output Moment workspace.")

    def PyExec(self):
        self._setup()
        progress = Progress(self, 0.0, 0.05, 3)

        progress.report("Energy transfer")
        scan_alg = self.createChildAlgorithm("ISISIndirectEnergyTransfer", 0.05, 0.95)
        scan_alg.setProperty("InputFiles", format_runs(self._data_files, self._instrument_name))
        scan_alg.setProperty("SumFiles", self._sum_files)
        scan_alg.setProperty("LoadLogFiles", self._load_logs)
        scan_alg.setProperty("CalibrationWorkspace", self._calibration_ws)
        scan_alg.setProperty("Instrument", self._instrument_name)
        scan_alg.setProperty("Analyser", self._analyser)
        scan_alg.setProperty("Reflection", self._reflection)
        scan_alg.setProperty("Efixed", self._efixed)
        scan_alg.setProperty("SpectraRange", self._spectra_range)
        scan_alg.setProperty("BackgroundRange", self._background_range)
        scan_alg.setProperty("RebinString", self._rebin_string)
        scan_alg.setProperty("DetailedBalance", self._detailed_balance)
        scan_alg.setProperty("ScaleFactor", self._scale_factor)
        scan_alg.setProperty("FoldMultipleFrames", self._fold_multiple_frames)
        scan_alg.setProperty("GroupingMethod", self._grouping_method)
        scan_alg.setProperty("GroupingWorkspace", self._grouping_ws)
        scan_alg.setProperty("GroupingFile", self._grouping_map_file)
        scan_alg.setProperty("UnitX", self._output_x_units)
        scan_alg.setProperty("OutputWorkspace", self._red_ws)
        scan_alg.execute()
        logger.information("ReducedWorkspace : %s" % self._red_ws)

        Rebin(InputWorkspace=self._red_ws, OutputWorkspace=self._red_ws, Params=self._energy_range, EnableLogging=False)

        input_workspace_names = mtd[self._red_ws].getNames()

        inst = mtd[input_workspace_names[0]].getInstrument()
        if inst.hasParameter("analyser"):
            analyser_name = inst.getStringParameter("analyser")[0]
            analyser_comp = inst.getComponentByName(analyser_name)
            if analyser_comp is not None and analyser_comp.hasParameter("resolution"):
                self._resolution = float(analyser_comp.getNumberParameter("resolution")[0])
            else:
                self._resolution = 0.01
        logger.information("Resolution = %d" % self._resolution)

        output_workspaces = list()
        temperatures = list()
        run_numbers = list()
        sofqw_alg = self.createChildAlgorithm("SofQW", enableLogging=False)
        group_alg = self.createChildAlgorithm("GroupWorkspaces", enableLogging=False)

        for input_ws in input_workspace_names:
            progress.report("SofQW for workspace: %s" % input_ws)
            sofqw_alg.setProperty("InputWorkspace", input_ws)
            sofqw_alg.setProperty("QAxisBinning", self._q_range)
            sofqw_alg.setProperty("EMode", "Indirect")
            sofqw_alg.setProperty("ReplaceNaNs", True)
            sofqw_alg.setProperty("Method", "NormalisedPolygon")
            sofqw_alg.setProperty("OutputWorkspace", input_ws + "_sqw")
            sofqw_alg.execute()
            mtd.addOrReplace(input_ws + "_sqw", sofqw_alg.getProperty("OutputWorkspace").value)
            output_workspaces.append(input_ws + "_sqw")

            # Get the sample temperature
            temp = self._get_temperature(input_ws + "_sqw")
            if temp is not None:
                temperatures.append(temp)
            else:
                # Get the run number
                run_no = self._get_InstrRun(input_ws)[1]
                run_numbers.append(run_no)

        y_axis = mtd[input_workspace_names[0] + "_sqw"].getAxis(1)
        y_values = y_axis.extractValues()
        q = list()
        for idx in range(len(y_values)):
            q.append(y_values[idx])
        group_alg.setProperty("InputWorkspaces", output_workspaces)
        group_alg.setProperty("OutputWorkspace", self._sqw_ws)
        group_alg.execute()
        mtd.addOrReplace(self._sqw_ws, group_alg.getProperty("OutputWorkspace").value)
        logger.information("Sqw Workspace : %s" % self._sqw_ws)

        # Get input workspaces
        input_workspace_names = mtd[self._sqw_ws].getNames()
        output_workspaces = list()
        width_workspaces = list()

        delete_alg = self.createChildAlgorithm("DeleteWorkspace", enableLogging=False)
        create_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
        for input_ws in input_workspace_names:
            progress.report("SofQWMoments for workspace: %s" % input_ws)
            SofQWMoments(
                InputWorkspace=input_ws,
                EnergyMin=self._energy_range[0],
                EnergyMax=self._energy_range[2],
                Scale=self._scale_factor,
                OutputWorkspace=input_ws + "_mom",
                EnableLogging=False,
            )
            output_workspaces.append(input_ws + "_mom")
            progress.report("Fitting workspace: %s" % input_ws)
            num_hist = mtd[input_ws].getNumberHistograms()
            result = "__result"
            params_table = "__result_Parameters"
            dataX = list()
            dataY = list()
            dataE = list()
            func = "name=Lorentzian,Amplitude=1.0,PeakCentre=0.0,FWHM=0.01"
            func += ",constraint=(Amplitude>0.0,FWHM>0.0)"
            for idx in range(num_hist):
                Fit(
                    InputWorkspace=input_ws,
                    Function=func,
                    DomainType="Simple",
                    WorkspaceIndex=idx,
                    Minimizer="Levenberg-Marquardt",
                    MaxIterations=500,
                    CreateOutput=True,
                    Output=result,
                    OutputParametersOnly=False,
                    EnableLogging=False,
                )
                dataX.append(q[idx])
                para_y = np.asarray(mtd[params_table].column("Value"))
                dataY.append(para_y[2])
                para_e = np.asarray(mtd[params_table].column("Error"))
                dataE.append(para_e[2])
            progress.report("Creating width workspace")
            width_ws = input_ws + "_width"
            create_alg.setProperty("OutputWorkspace", width_ws)
            create_alg.setProperty("DataX", dataX)
            create_alg.setProperty("DataY", dataY)
            create_alg.setProperty("DataE", dataE)
            create_alg.setProperty("NSpec", 1)
            create_alg.setProperty("UnitX", "MomentumTransfer")
            create_alg.setProperty("YUnitLabel", "FWHM")
            create_alg.execute()
            mtd.addOrReplace(width_ws, create_alg.getProperty("OutputWorkspace").value)
            width_workspaces.append(width_ws)
            delete_alg.setProperty("Workspace", params_table)
            delete_alg.execute()
            delete_alg.setProperty("Workspace", result + "_NormalisedCovarianceMatrix")
            delete_alg.execute()
            delete_alg.setProperty("Workspace", result + "_Workspace")
            delete_alg.execute()
        logger.information("Moment Workspace : %s" % self._moment_ws)

        width_workspace = self._sqw_ws + "_width"
        clone_alg = self.createChildAlgorithm("CloneWorkspace", enableLogging=True)
        append_alg = self.createChildAlgorithm("AppendSpectra", enableLogging=True)
        for idx in range(len(width_workspaces)):
            if idx == 0:
                clone_alg.setProperty("InputWorkspace", width_workspaces[0])
                clone_alg.setProperty("OutputWorkspace", width_workspace)
                clone_alg.execute()
                mtd.addOrReplace(width_workspace, clone_alg.getProperty("OutputWorkspace").value)
            else:
                append_alg.setProperty("InputWorkspace1", width_workspace)
                append_alg.setProperty("InputWorkspace2", width_workspaces[idx])
                append_alg.setProperty("OutputWorkspace", width_workspace)
                append_alg.execute()
                mtd.addOrReplace(width_workspace, append_alg.getProperty("OutputWorkspace").value)
        logger.information("Width Workspace : %s" % width_workspace)

        numb_temp = len(temperatures)
        x_axis_is_temp = len(input_workspace_names) == numb_temp

        if x_axis_is_temp:
            logger.information("X axis is in temperature")
            unit = ("Temperature", "K")
        else:
            logger.information("X axis is in run number")
            unit = ("Run No", " last 3 digits")

        xdat = list()
        ydat = list()
        edat = list()
        for idx in range(len(temperatures)):
            x = mtd[width_workspace].readX(idx)
            y = mtd[width_workspace].readY(idx)
            e = mtd[width_workspace].readE(idx)
            if x_axis_is_temp:
                xdat.append(float(temperatures[idx]))
            else:
                xdat.append(float(run_numbers[idx][-3:]))
            ydat.append(y[5] / x[5])
            edat.append(e[5] / x[5])
        diffusion_workspace = self._sqw_ws + "_diffusion"
        create_alg = self.createChildAlgorithm("CreateWorkspace", enableLogging=False)
        create_alg.setProperty("OutputWorkspace", diffusion_workspace)
        create_alg.setProperty("DataX", xdat)
        create_alg.setProperty("DataY", ydat)
        create_alg.setProperty("DataE", edat)
        create_alg.setProperty("NSpec", 1)
        create_alg.setProperty("YUnitLabel", "Diffusion")
        create_alg.execute()
        mtd.addOrReplace(diffusion_workspace, create_alg.getProperty("OutputWorkspace").value)
        unitx = mtd[diffusion_workspace].getAxis(0).setUnit("Label")
        unitx.setLabel(unit[0], unit[1])
        logger.information("Diffusion Workspace : %s" % diffusion_workspace)

    def validateInputs(self):
        """
        Validates algorithm properties.
        """
        issues = dict()

        # Validate the instrument configuration by checking if a parameter file exists
        instrument_name = self.getPropertyValue("Instrument")
        analyser = self.getPropertyValue("Analyser")
        reflection = self.getPropertyValue("Reflection")

        ipf_filename = os.path.join(
            config["instrumentDefinition.directory"], instrument_name + "_" + analyser + "_" + reflection + "_Parameters.xml"
        )

        if not os.path.exists(ipf_filename):
            error_message = "Invalid instrument configuration"
            issues["Instrument"] = error_message
            issues["Analyser"] = error_message
            issues["Reflection"] = error_message

        # Validate spectra range
        spectra_range = self.getProperty("SpectraRange").value
        if len(spectra_range) != 2:
            issues["SpectraRange"] = "Range must contain exactly two items"
        elif spectra_range[0] > spectra_range[1]:
            issues["SpectraRange"] = "Range must be in format: lower,upper"

        return issues

    def _setup(self):
        """
        Gets algorithm properties.
        """

        # Get properties
        self._data_files = self.getProperty("InputFiles").value
        self._sum_files = False
        self._load_logs = self.getProperty("LoadLogFiles").value
        self._calibration_ws = ""

        self._instrument_name = self.getPropertyValue("Instrument")
        self._analyser = self.getPropertyValue("Analyser")
        self._reflection = self.getPropertyValue("Reflection")
        self._efixed = Property.EMPTY_DBL

        self._spectra_range = self.getProperty("SpectraRange").value
        self._background_range = ""
        self._rebin_string = ""
        self._detailed_balance = self.getProperty("DetailedBalance").value
        self._scale_factor = 1.0
        self._fold_multiple_frames = False
        self._q_range = self.getProperty("QRange").value
        self._energy_range = self.getProperty("EnergyRange").value

        self._grouping_method = self.getPropertyValue("GroupingMethod")
        self._grouping_ws = ""
        self._grouping_map_file = ""

        self._output_x_units = "DeltaE"

        self._sample_log_name = self.getPropertyValue("SampleEnvironmentLogName")
        self._sample_log_value = self.getPropertyValue("SampleEnvironmentLogValue")

        self._red_ws = self.getPropertyValue("ReducedWorkspace")
        self._sqw_ws = self.getPropertyValue("SqwWorkspace")
        self._moment_ws = self.getProperty("MomentWorkspace").value

        # Disable sum files if there is only one file
        if len(self._data_files) == 1:
            if self._sum_files:
                logger.warning("SumFiles disabled when only one input file is provided.")
            self._sum_files = False

        # Get the IPF filename
        self._ipf_filename = os.path.join(
            config["instrumentDefinition.directory"],
            self._instrument_name + "_" + self._analyser + "_" + self._reflection + "_Parameters.xml",
        )
        logger.information("Instrument parameter file: %s" % self._ipf_filename)

        # Warn when grouping options are to be ignored
        if self._grouping_method != "Workspace" and self._grouping_ws is not None:
            logger.warning("GroupingWorkspace will be ignored by selected GroupingMethod")

        if self._grouping_method != "File" and self._grouping_map_file is not None:
            logger.warning("MapFile will be ignored by selected GroupingMethod")

        # The list of workspaces being processed
        self._workspace_names = []

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
            value_action = {"last_value": lambda x: x[len(x) - 1], "average": lambda x: x.mean()}
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
                    temp = tmp[len(tmp) - 1]
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


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SofQWMomentsScan)
