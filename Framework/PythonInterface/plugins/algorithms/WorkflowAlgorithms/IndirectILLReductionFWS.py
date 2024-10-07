# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import time
from mantid import mtd
from mantid.kernel import StringListValidator, Direction, FloatBoundedValidator
from mantid.api import PythonAlgorithm, MultipleFileProperty, FileProperty, FileAction, WorkspaceGroupProperty, Progress
from mantid.simpleapi import *


class IndirectILLReductionFWS(PythonAlgorithm):
    _SAMPLE = "sample"
    _BACKGROUND = "background"
    _CALIBRATION = "calibration"
    _BACKCALIB = "calibrationBackground"

    _sample_files = None
    _background_files = None
    _calibration_files = None
    _background_calib_files = None
    _observable = None
    _sortX = None
    _red_ws = None
    _back_scaling = None
    _back_calib_scaling = None
    _criteria = None
    _progress = None
    _back_option = None
    _calib_option = None
    _back_calib_option = None
    _common_args = {}
    _all_runs = None
    _discard_sds = None
    _group_detectors = None

    def category(self):
        return "Workflow\\MIDAS;Workflow\\Inelastic;Inelastic\\Indirect;Inelastic\\Reduction;ILL\\Indirect"

    def summary(self):
        return (
            "Performs fixed-window scan (FWS) multiple file reduction (both elastic and inelastic) "
            "for ILL indirect geometry data, instrument IN16B."
        )

    def seeAlso(self):
        return ["IndirectILLReductionQENS", "IndirectILLEnergyTransfer"]

    def name(self):
        return "IndirectILLReductionFWS"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("Run", extensions=["nxs"]), doc="Run number(s) of sample run(s).")

        self.declareProperty(
            MultipleFileProperty("BackgroundRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of background (empty can) run(s).",
        )

        self.declareProperty(
            MultipleFileProperty("CalibrationRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of vanadium calibration run(s).",
        )

        self.declareProperty(
            MultipleFileProperty("CalibrationBackgroundRun", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Run number(s) of background (empty can) run(s) for vanadium run.",
        )

        self.declareProperty(name="Observable", defaultValue="sample.temperature", doc="Scanning observable, a Sample Log entry\n")

        self.declareProperty(name="SortXAxis", defaultValue=False, doc="Whether or not to sort the x-axis\n")

        self.declareProperty(
            name="BackgroundScalingFactor",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Scaling factor for background subtraction",
        )

        self.declareProperty(
            name="CalibrationBackgroundScalingFactor",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Scaling factor for background subtraction for vanadium calibration",
        )

        self.declareProperty(
            name="BackgroundOption",
            defaultValue="Sum",
            validator=StringListValidator(["Sum", "Interpolate"]),
            doc="Whether to sum or interpolate the background runs.",
        )

        self.declareProperty(
            name="CalibrationOption",
            defaultValue="Sum",
            validator=StringListValidator(["Sum", "Interpolate"]),
            doc="Whether to sum or interpolate the calibration runs.",
        )

        self.declareProperty(
            name="CalibrationBackgroundOption",
            defaultValue="Sum",
            validator=StringListValidator(["Sum", "Interpolate"]),
            doc="Whether to sum or interpolate the background run for calibration runs.",
        )

        self.declareProperty(
            FileProperty("MapFile", "", action=FileAction.OptionalLoad, extensions=["map", "xml"]),
            doc="Filename of the detector grouping map file to use. \n"
            "By default all the pixels will be summed per each tube. \n"
            "Use .map or .xml file (see GroupDetectors documentation) "
            "only if different range is needed for each tube.",
        )

        self.declareProperty(
            name="ManualPSDIntegrationRange",
            defaultValue=[1, 128],
            doc="Integration range of vertical pixels in each PSD tube. \n"
            "By default all the pixels will be summed per each tube. \n"
            "Use this option if the same range (other than default) "
            "is needed for all the tubes.",
        )

        self.declareProperty(name="Analyser", defaultValue="silicon", validator=StringListValidator(["silicon"]), doc="Analyser crystal.")

        self.declareProperty(
            name="Reflection", defaultValue="111", validator=StringListValidator(["111", "311"]), doc="Analyser reflection."
        )

        self.declareProperty(WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace group")

        self.declareProperty(
            name="SpectrumAxis",
            defaultValue="SpectrumNumber",
            validator=StringListValidator(["SpectrumNumber", "2Theta", "Q", "Q2"]),
            doc="The spectrum axis conversion target.",
        )

        self.declareProperty(name="DiscardSingleDetectors", defaultValue=False, doc="Whether to discard the spectra of single detectors.")

        self.declareProperty(
            name="ManualInelasticPeakChannels",
            defaultValue=[-1, -1],
            doc="The channel indices for the inelastic peak positions in the beginning "
            "and in the end of the spectra; by default the maxima of the monitor "
            "spectrum will be used for this. The intensities will be integrated symmetrically "
            "around each peak.",
        )

        self.declareProperty(
            name="GroupDetectors",
            defaultValue=True,
            doc="Group the pixels using the range, tube-by-tube (default) or in a custom way; \n"
            "it is not recommended to group the detectors at this stage, \n"
            "in order to get absorption corrections right, \n"
            "however the default value is True for backwards compatibility.",
        )

    def validateInputs(self):
        issues = dict()

        if self.getPropertyValue("CalibrationBackgroundRun") and not self.getPropertyValue("CalibrationRun"):
            issues["CalibrationRun"] = "Calibration runs are required, " "if background for calibration is given."

        if not self.getProperty("ManualInelasticPeakChannels").isDefault:
            peaks = self.getProperty("ManualInelasticPeakChannels").value
            if len(peaks) != 2:
                issues["ManualInelasticPeakChannels"] = "Invalid value for peak channels, " "provide two comma separated positive integers."
            elif peaks[0] >= peaks[1]:
                issues["ManualInelasticPeakChannels"] = "First peak channel must be less than the second"
            elif peaks[0] <= 0:
                issues["ManualInelasticPeakChannels"] = "Non negative integers are required"

        return issues

    def setUp(self):
        self._sample_files = self.getPropertyValue("Run")
        self._background_files = self.getPropertyValue("BackgroundRun")
        self._calibration_files = self.getPropertyValue("CalibrationRun")
        self._background_calib_files = self.getPropertyValue("CalibrationBackgroundRun")
        self._observable = self.getPropertyValue("Observable")
        self._sortX = self.getProperty("SortXAxis").value
        self._back_scaling = self.getProperty("BackgroundScalingFactor").value
        self._back_calib_scaling = self.getProperty("CalibrationBackgroundScalingFactor").value
        self._back_option = self.getPropertyValue("BackgroundOption")
        self._calib_option = self.getPropertyValue("CalibrationOption")
        self._back_calib_option = self.getPropertyValue("CalibrationBackgroundOption")
        self._spectrum_axis = self.getPropertyValue("SpectrumAxis")
        self._discard_sds = self.getProperty("DiscardSingleDetectors").value
        self._group_detectors = self.getProperty("GroupDetectors").value

        # arguments to pass to IndirectILLEnergyTransfer
        self._common_args["MapFile"] = self.getPropertyValue("MapFile")
        self._common_args["Analyser"] = self.getPropertyValue("Analyser")
        self._common_args["Reflection"] = self.getPropertyValue("Reflection")
        self._common_args["ManualPSDIntegrationRange"] = self.getProperty("ManualPSDIntegrationRange").value
        self._common_args["SpectrumAxis"] = self._spectrum_axis
        self._common_args["DiscardSingleDetectors"] = self._discard_sds
        self._common_args["GroupDetectors"] = self._group_detectors

        self._red_ws = self.getPropertyValue("OutputWorkspace")

        suffix = ""
        if self._spectrum_axis == "SpectrumNumber":
            suffix = "_red"
        elif self._spectrum_axis == "2Theta":
            suffix = "_2theta"
        elif self._spectrum_axis == "Q":
            suffix = "_q"
        elif self._spectrum_axis == "Q2":
            suffix = "_q2"

        self._red_ws += suffix

        # Nexus metadata criteria for FWS type of data (both EFWS and IFWS)
        self._criteria = (
            "($/entry0/instrument/Doppler/maximum_delta_energy$ == 0. or " "$/entry0/instrument/Doppler/velocity_profile$ == 1)"
        )

        # force sort x-axis, if interpolation is requested
        if (
            (self._back_option == "Interpolate" and self._background_files)
            or (self._calib_option == "Interpolate" and self._calibration_files)
            or (self._back_calib_option == "Interpolate" and self._background_calib_files)
        ) and not self._sortX:
            self.log().warning("Interpolation option requested, X-axis will be sorted.")
            self._sortX = True

        # empty dictionary to keep track of all runs (ws names)
        self._all_runs = dict()

    def _filter_files(self, files, label):
        """
        Filters the given list of files according to nexus criteria
        @param  files :: list of input files (i.e. , and + separated string)
        @param  label :: label of error message if nothing left after filtering
        @throws RuntimeError :: when nothing left after filtering
        @return :: the list of input files that passsed the criteria
        """

        files = SelectNexusFilesByMetadata(files, self._criteria)

        if not files:
            raise RuntimeError("None of the {0} runs satisfied the FWS and Observable criteria.".format(label))
        else:
            self.log().information("Filtered {0} runs are: {1} \\n".format(label, files.replace(",", "\\n")))

        return files

    def _ifws_peak_bins(self, ws):
        """
        Gives the bin indices of the first and last inelastic peaks
        By default they are taken from the maxima of the monitor spectrum
        Or they can be specified manually as input parameters
        @param ws :: input workspace
        return    :: [imin,imax]
        """
        if not self.getProperty("ManualInelasticPeakChannels").isDefault:
            peak_channels = self.getProperty("ManualInelasticPeakChannels").value
            blocksize = mtd[ws].blocksize()
            if peak_channels[1] >= blocksize:
                raise RuntimeError("Manual peak channel {0} is out of range {1}".format(peak_channels[1], blocksize))
            else:
                AddSampleLogMultiple(
                    Workspace=ws,
                    LogNames=["ManualInelasticLeftPeak", "ManualInelasticRightPeak"],
                    LogValues=str(peak_channels[0]) + "," + str(peak_channels[1]),
                )
                return peak_channels
        run = mtd[ws].getRun()
        if not run.hasProperty("MonitorLeftPeak") or not run.hasProperty("MonitorRightPeak"):
            raise RuntimeError("Unable to retrieve the monitor peak information from the sample logs.")
        else:
            imin = run.getLogData("MonitorLeftPeak").value
            imax = run.getLogData("MonitorRightPeak").value
        return imin, imax

    def _ifws_integrate(self, wsgroup):
        """
        Integrates IFWS over two peaks at the beginning and end
        @param ws :: input workspace group
        """

        for item in mtd[wsgroup]:
            ws = item.name()
            size = item.blocksize()
            imin, imax = self._ifws_peak_bins(ws)
            x_values = item.readX(0)
            int1 = "__int1_" + ws
            int2 = "__int2_" + ws
            Integration(InputWorkspace=ws, OutputWorkspace=int1, RangeLower=x_values[0], RangeUpper=x_values[2 * imin])
            Integration(InputWorkspace=ws, OutputWorkspace=int2, RangeLower=x_values[-2 * (size - imax)], RangeUpper=x_values[-1])
            Plus(LHSWorkspace=int1, RHSWorkspace=int2, OutputWorkspace=ws)
            DeleteWorkspace(int1)
            DeleteWorkspace(int2)

    def _perform_unmirror(self, groupws):
        """
        Sums the integrals of left and right for two wings, or returns the integral of one wing
        @param ws :: group workspace containing one ws for one wing, and two ws for two wing data
        """
        if mtd[groupws].getNumberOfEntries() == 2:  # two wings, sum
            left = mtd[groupws].getItem(0).name()
            right = mtd[groupws].getItem(1).name()
            left_right_sum = "__sum_" + groupws

            left_monitor = mtd[left].getRun().getLogData("MonitorIntegral").value
            right_monitor = mtd[right].getRun().getLogData("MonitorIntegral").value

            if left_monitor != 0.0 and right_monitor != 0.0:
                sum_monitor = left_monitor + right_monitor
                left_factor = left_monitor / sum_monitor
                right_factor = right_monitor / sum_monitor
                Scale(InputWorkspace=left, OutputWorkspace=left, Factor=left_factor)
                Scale(InputWorkspace=right, OutputWorkspace=right, Factor=right_factor)
            else:
                self.log().notice(
                    "Zero monitor integral has been found in one (or both) wings; left: {0}, right: {1}".format(left_monitor, right_monitor)
                )

            Plus(LHSWorkspace=left, RHSWorkspace=right, OutputWorkspace=left_right_sum)

            DeleteWorkspace(left)
            DeleteWorkspace(right)

            RenameWorkspace(InputWorkspace=left_right_sum, OutputWorkspace=groupws)

        else:
            RenameWorkspace(InputWorkspace=mtd[groupws].getItem(0), OutputWorkspace=groupws)

    def PyExec(self):
        self.setUp()

        # total number of (unsummed) runs
        total = self._sample_files.count(",") + self._background_files.count(",") + self._calibration_files.count(",")

        self._progress = Progress(self, start=0.0, end=1.0, nreports=total)

        self._reduce_multiple_runs(self._sample_files, self._SAMPLE)

        if self._background_files:
            self._reduce_multiple_runs(self._background_files, self._BACKGROUND)

            back_ws = self._red_ws + "_" + self._BACKGROUND

            Scale(InputWorkspace=back_ws, Factor=self._back_scaling, OutputWorkspace=back_ws)

            if self._back_option == "Sum":
                self._integrate(self._BACKGROUND, self._SAMPLE)
            else:
                self._interpolate(self._BACKGROUND, self._SAMPLE)

            self._subtract_background(self._BACKGROUND, self._SAMPLE)

            DeleteWorkspace(back_ws)

        if self._calibration_files:
            self._reduce_multiple_runs(self._calibration_files, self._CALIBRATION)

            if self._background_calib_files:
                self._reduce_multiple_runs(self._background_calib_files, self._BACKCALIB)

                back_calib_ws = self._red_ws + "_" + self._BACKCALIB

                Scale(InputWorkspace=back_calib_ws, Factor=self._back_calib_scaling, OutputWorkspace=back_calib_ws)

                if self._back_calib_option == "Sum":
                    self._integrate(self._BACKCALIB, self._CALIBRATION)
                else:
                    self._interpolate(self._BACKCALIB, self._CALIBRATION)

                self._subtract_background(self._BACKCALIB, self._CALIBRATION)

                DeleteWorkspace(back_calib_ws)

            if self._calib_option == "Sum":
                self._integrate(self._CALIBRATION, self._SAMPLE)
            else:
                self._interpolate(self._CALIBRATION, self._SAMPLE)

            self._calibrate()

            DeleteWorkspace(self._red_ws + "_" + self._CALIBRATION)

        self.log().debug("Run files map is :" + str(self._all_runs))

        self.setProperty("OutputWorkspace", self._red_ws)

    def _reduce_multiple_runs(self, files, label):
        """
        Filters and reduces multiple files
        @param files :: list of run paths
        @param label :: output ws name
        """

        files = self._filter_files(files, label)

        for run in files.split(","):
            self._reduce_run(run, label)

        self._create_matrices(label)

    def _reduce_run(self, run, label):
        """
        Reduces the given (single or summed multiple) run
        @param run :: run path
        @param  label :: sample, background or calibration
        """

        runs_list = run.split("+")

        runnumber = os.path.basename(runs_list[0]).split(".")[0]

        ws = "__" + runnumber

        if len(runs_list) > 1:
            ws += "_multiple"

        ws += "_" + label

        self._progress.report("Reducing run #" + runnumber)

        IndirectILLEnergyTransfer(Run=run, OutputWorkspace=ws, **self._common_args)

        energy = round(mtd[ws].getItem(0).getRun().getLogData("Doppler.maximum_delta_energy").value, 2)

        if energy == 0.0:
            # Elastic, integrate over full energy range
            Integration(InputWorkspace=ws, OutputWorkspace=ws)
        else:
            # Inelastic, do something more complex
            self._ifws_integrate(ws)

        ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)

        self._perform_unmirror(ws)

        self._subscribe_run(ws, energy, label)

    def _subscribe_run(self, ws, energy, label):
        """
        Subscribes the given ws name to the map for given energy and label
        @param ws     :: workspace name
        @param energy :: energy value
        @param label  :: sample, calibration or background
        """

        if label in self._all_runs:
            if energy in self._all_runs[label]:
                self._all_runs[label][energy].append(ws)
            else:
                self._all_runs[label][energy] = [ws]
        else:
            self._all_runs[label] = dict()
            self._all_runs[label][energy] = [ws]

    def _integrate(self, label, reference):
        """
        Averages the background or calibration intensities over all observable points at given energy
        @param label :: calibration or background
        @param reference :: sample or calibration
        """

        for energy in self._all_runs[reference]:
            if energy in self._all_runs[label]:
                ws = self._insert_energy_value(self._red_ws + "_" + label, energy, label)
                if mtd[ws].blocksize() > 1:
                    SortXAxis(InputWorkspace=ws, OutputWorkspace=ws)
                    axis = mtd[ws].readX(0)
                    start = axis[0]
                    end = axis[-1]
                    integration_range = end - start
                    params = [start, integration_range, end]
                    Rebin(InputWorkspace=ws, OutputWorkspace=ws, Params=params)

    def _interpolate(self, label, reference):
        """
        Interpolates the background or calibration intensities to
        all observable points existing in sample at a given energy
        @param label  :: calibration or background
        @param reference :: to interpolate to, can be sample or calibration
        """

        for energy in self._all_runs[reference]:
            if energy in self._all_runs[label]:
                ws = self._insert_energy_value(self._red_ws + "_" + label, energy, label)

                if reference == self._SAMPLE:
                    ref = self._insert_energy_value(self._red_ws, energy, reference)
                else:
                    ref = self._insert_energy_value(self._red_ws + "_" + reference, energy, reference)

                if mtd[ws].blocksize() > 1:
                    SplineInterpolation(WorkspaceToInterpolate=ws, WorkspaceToMatch=ref, Linear2Points=True, OutputWorkspace=ws)

    def _subtract_background(self, background, reference):
        """
        Subtracts the background per each energy if background run is available
        @param background :: background to subtract
        @param reference :: to subtract from
        """

        for energy in self._all_runs[reference]:
            if energy in self._all_runs[background]:
                if reference == self._SAMPLE:
                    lhs = self._insert_energy_value(self._red_ws, energy, reference)
                else:
                    lhs = self._insert_energy_value(self._red_ws + "_" + reference, energy, reference)

                rhs = self._insert_energy_value(self._red_ws + "_" + background, energy, background)
                Minus(LHSWorkspace=lhs, RHSWorkspace=rhs, OutputWorkspace=lhs)
            else:
                self.log().warning(
                    "No background subtraction can be performed for doppler energy of {0} microEV, "
                    "since no background run was provided for the same energy value.".format(energy)
                )

    def _calibrate(self):
        """
        Performs calibration per each energy if calibration run is available
        """

        for energy in self._all_runs[self._SAMPLE]:
            if energy in self._all_runs[self._CALIBRATION]:
                sample_ws = self._insert_energy_value(self._red_ws, energy, self._SAMPLE)
                calib_ws = sample_ws + "_" + self._CALIBRATION
                Divide(LHSWorkspace=sample_ws, RHSWorkspace=calib_ws, OutputWorkspace=sample_ws)
                self._scale_calibration(sample_ws, calib_ws)
            else:
                self.log().warning(
                    "No calibration can be performed for doppler energy of {0} microEV, "
                    "since no calibration run was provided for the same energy value.".format(energy)
                )

    def _scale_calibration(self, sample, calib):
        """
        Scales sample workspace after calibration up by the maximum of integral intensity
        in calibration run for each observable point
        @param sample  :: sample workspace after calibration
        @param calib   :: calibration workspace
        """

        if mtd[calib].blocksize() == 1:
            scale = np.max(mtd[calib].extractY()[:, 0])
            Scale(InputWorkspace=sample, Factor=scale, OutputWorkspace=sample, Operation="Multiply")
        else:
            # here calib and sample have the same size already
            for column in range(mtd[sample].blocksize()):
                scale = np.max(mtd[calib].extractY()[:, column])
                for spectrum in range(mtd[sample].getNumberHistograms()):
                    mtd[sample].dataY(spectrum)[column] *= scale
                    mtd[sample].dataE(spectrum)[column] *= scale

    def _get_observable_values(self, ws_list):
        """
        Retrieves the needed sample log values for the given list of workspaces
        @param ws_list :: list of workspaces
        @returns :: array of observable values
        @throws  :: ValueError if the log entry is not a number nor time-stamp
        """

        result = []

        zero_time = 0

        pattern = "%Y-%m-%dT%H:%M:%S"

        for i, ws in enumerate(ws_list):
            log = mtd[ws].getRun().getLogData(self._observable)
            value = log.value

            if log.type == "number":
                value = float(value)
            else:
                try:
                    value = time.mktime(time.strptime(value, pattern))
                except ValueError:
                    raise ValueError(
                        "Invalid observable. "
                        "Provide a numeric (sample.*, run_number, etc.) or time-stamp "
                        "like string (e.g. start_time) log."
                    )
                if i == 0:
                    zero_time = value

                value = value - zero_time

            result.append(value)

        return result

    def _create_matrices(self, label):
        """
        For each reduction type concatenates the workspaces putting the given sample log value as x-axis
        Creates a group workspace for the given label, that contains 2D workspaces for each distinct energy value
        @param label :: sample, background or calibration
        """

        togroup = []

        groupname = self._red_ws

        if label != self._SAMPLE:
            groupname += "_" + label

        for energy in sorted(self._all_runs[label]):
            ws_list = self._all_runs[label][energy]

            wsname = self._insert_energy_value(groupname, energy, label)

            togroup.append(wsname)
            nspectra = mtd[ws_list[0]].getNumberHistograms()
            observable_array = self._get_observable_values(self._all_runs[label][energy])

            ConjoinXRuns(InputWorkspaces=ws_list, OutputWorkspace=wsname)

            mtd[wsname].setDistribution(True)

            run_list = ""  # to set to sample logs

            for ws in ws_list:
                run = mtd[ws].getRun()

                if run.hasProperty("run_number_list"):
                    run_list += run.getLogData("run_number_list").value.replace(", ", "+") + ","
                else:
                    run_list += str(run.getLogData("run_number").value) + ","

            AddSampleLog(Workspace=wsname, LogName="ReducedRunsList", LogText=run_list.rstrip(","))

            for spectrum in range(nspectra):
                mtd[wsname].setX(spectrum, np.array(observable_array))

            if self._sortX:
                SortXAxis(InputWorkspace=wsname, OutputWorkspace=wsname)

            self._set_x_label(wsname)

        for energy, ws_list in self._all_runs[label].items():
            for ws in ws_list:
                DeleteWorkspace(ws)

        GroupWorkspaces(InputWorkspaces=togroup, OutputWorkspace=groupname)

    def _set_x_label(self, ws):
        """
        Sets the x-axis label
        @param ws :: input workspace
        """

        axis = mtd[ws].getAxis(0)
        if self._observable == "sample.temperature":
            axis.setUnit("Label").setLabel("Temperature", "K")
        elif self._observable == "sample.pressure":
            axis.setUnit("Label").setLabel("Pressure", "P")
        elif "time" in self._observable:
            axis.setUnit("Label").setLabel("Time", "seconds")
        else:
            axis.setUnit("Label").setLabel(self._observable, "")

    def _insert_energy_value(self, ws_name, energy, label):
        """
        Inserts the doppler's energy value in the workspace name
        in between the user input and automatic suffix
        @param ws_name : workspace name
        @param energy : energy value
        @param label : sample, background, or calibration
        @return : new name with energy value inside
        Example:
        user_input_2theta > user_input_1.5_2theta
        user_input_red_background > user_input_1.5_red_background
        """
        suffix_pos = ws_name.rfind("_")

        if label != self._SAMPLE:
            # find second to last underscore
            suffix_pos = ws_name.rfind("_", 0, suffix_pos)

        return ws_name[:suffix_pos] + "_" + str(energy) + ws_name[suffix_pos:]


# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectILLReductionFWS)
