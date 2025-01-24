# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
import time
from scipy import interpolate

# import debugpy

from mantid import mtd
from mantid.kernel import (
    StringListValidator,
    Direction,
    FloatBoundedValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
    CompositeValidator,
)
from mantid.api import PythonAlgorithm, FileProperty, WorkspaceProperty, FileAction, Progress, NumericAxis
from mantid.simpleapi import (
    Scale,
    Minus,
    Divide,
    ResetNegatives,
    DeleteWorkspace,
    CloneWorkspace,
    ReplaceSpecialValues,
    GroupWorkspaces,
    Integration,
    LoadEMUHdf,
    CreateWorkspace,
    RenameWorkspace,
    SumSpectra,
    Transpose,
    AlgorithmFactory,
    LoadEmptyInstrument,
    LoadParameterFile,
)


from ANSTO.ansto_common import (
    ScratchFolder,
    hdf_files_from_runs,
    expand_directories,
    load_merge,
    find_nearest_index,
    extract_hdf_params,
    total_time,
    seq_to_list,
    FractionalAreaDetectorTubes,
    append_ini_params,
    setup_axis,
    HdfKey,
    LoaderOptions,
    NDArray,
)
from ANSTO.emu_common import FilterEmuPixelsTubes, EmuParameters, DopplerSupport

from typing import Any, List, Sequence, Callable, Dict, Tuple

Interpolator = Callable[[NDArray], NDArray]


def remove_hist_edges(ws_tag: str) -> None:
    # after a transpose the y axis includes bin edges rather than
    # the bin values, restore bin values using the bin edge midpoint
    ws = mtd[ws_tag]
    oldAxis = ws.getAxis(1)
    unit = oldAxis.getUnit()
    newAxis = NumericAxis.create(ws.getNumberHistograms())
    newAxis.setUnit("Label").setLabel(unit.caption(), unit.symbol())
    yv = oldAxis.extractValues()
    yv = 0.5 * (yv[:-1] + yv[1:])
    for i, y in enumerate(yv):
        newAxis.setValue(i, y)
    ws.replaceAxis(1, newAxis)


class ElasticEMUauReduction(PythonAlgorithm):
    def category(self) -> str:
        return "Workflow\\Elastic;Workflow\\Indirect"

    def summary(self) -> str:
        return "Performs an elastic reduction for ANSTO EMU indirect geometry data."

    def seeAlso(self) -> List[str]:
        return ["IndirectILLEnergyTransfer"]

    def name(self) -> str:
        return "ElasticEMUauReduction"

    def PyInit(self) -> None:
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        self.declareProperty(
            StringArrayProperty("SampleRuns", values=[], validator=mandatoryInputRuns),
            doc="Comma separated range of sample runs, and optional dataset indexes, e.g. [cycle::] 7333-7341,7345[:0-23]",
        )

        self.declareProperty(
            name="BackgroundRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [cycle::] 6300-6308",
        )

        self.declareProperty(
            name="CalibrationRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [cycle::] 6350-6365",
        )

        self.declareProperty(
            name="BackgroundCalibrationRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [cycle::] 6370-6375",
        )

        self.declareProperty(
            name="SpectrumAxis",
            defaultValue="TubeNumber",
            validator=StringListValidator(["TubeNumber", "2Theta", "Q", "Q2"]),
            doc="The spectrum axis conversion target.",
        )

        self.declareProperty(
            name="SampleTransmissionFactor",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Scaling factor for background subtraction from sample",
        )

        self.declareProperty(
            name="CalibrationTransmissionFactor",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0),
            doc="Scaling factor for background subtraction from calibration",
        )

        self.declareProperty(
            name="ScanParameter",
            defaultValue="",
            doc="Display data for time series environment variable, using optional label and units e.g. T02SP06 [, Temperature, K]",
        )

        self.declareProperty(
            name="ReferenceRange",
            defaultValue="",
            doc="Normalise the sample counts over the environment or time range. Values are in environment units or secs, e.g. 75-100",
        )

        self.declareProperty(
            name="SteppedScanParameter",
            defaultValue=False,
            doc="If the environment variable is set and held during each scan rather than continuously changing.",
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Name for the reduced workspace.")

        self.declareProperty(
            FileProperty("ScratchFolder", "", action=FileAction.OptionalDirectory, direction=Direction.Input),
            doc="Path to save and restore merged workspaces.",
        )

        self.declareProperty(
            name="KeepIntermediateWorkspaces",
            defaultValue=False,
            doc="Whether to keep the intermediate sample and calibration workspaces for diagnostic checks.",
        )

        self.declareProperty(
            FileProperty("ConfigurationFile", "", action=FileAction.OptionalLoad, extensions=["ini"]),
            doc="Optional: INI file to override default processing values.",
        )

        self.declareProperty(name="UseFractionalMap", defaultValue=True, doc="Allocate a fractional of the detector counts area")

    def PyExec(self) -> None:
        # add the following to where it should break
        # debugpy.wait_for_client()
        # debugpy.breakpoint()

        # Set up the processing parameters
        self._set_up()

        # remove any remnant intermediate files
        try:
            ws = mtd["intermediate"]
            DeleteWorkspace(ws)
        except KeyError:
            pass

        # Get the list of data files from the runs
        self._search_dirs = expand_directories(self._cfg._search_path)

        def pval(tag):
            return self.getPropertyValue(tag)

        # Get the list of data files from the runs
        sample_runs = hdf_files_from_runs(pval("SampleRuns"), self._search_dirs, self._file_prefix, self._cfg._file_extn)
        empty_runs = hdf_files_from_runs(pval("BackgroundRuns"), self._search_dirs, self._file_prefix, self._cfg._file_extn)
        calibration_runs = hdf_files_from_runs(pval("CalibrationRuns"), self._search_dirs, self._file_prefix, self._cfg._file_extn)
        empty_calib_runs = hdf_files_from_runs(
            pval("BackgroundCalibrationRuns"), self._search_dirs, self._file_prefix, self._cfg._file_extn
        )

        total_runs = sum([len(x) for x in [sample_runs, empty_runs, calibration_runs, empty_calib_runs]])
        if self._cfg._normalise_incident == "DirectMeasurement":
            total_runs *= 2

        # set up the wavelength from the sample runs - sample runs includes the dataset index
        # which needs to be removed
        split_fname = sample_runs[0].split(".hdf:")
        sample_file = split_fname[0] + ".hdf"
        ds_index = int(split_fname[1]) if len(split_fname) > 1 else 0
        self._complete_doppler_setup(sample_file, ds_index)
        if self._environment_param and not self._environment_units:
            self._set_environment_units(sample_file)

        self._progress = Progress(self, start=0.0, end=1.0, nreports=total_runs + 2)
        self._progress.report("File selection complete, loading files")

        # If the output workspace is not provided use the basename of the first
        # sample file
        output_ws = self.getPropertyValue("OutputWorkspace")

        # now call scan and reduce to accumulate the data and normalise to
        # pseudo incident beam monitor, the sample data is normalised against
        # the direct scatter from itself as the environment parameters are
        # changing which invalidates the incident beam ???? is this true
        _sample_ws = self._scan_and_reduce("_sample", sample_runs)
        _empty_ws = self._scan_and_reduce("_empty", empty_runs, single_bin=True)
        _calibration_ws = self._scan_and_reduce("_calibration", calibration_runs, single_bin=True)
        _empty_calib_ws = self._scan_and_reduce("_empty_calib", empty_calib_runs, single_bin=True)

        self._progress.report("Background removal, normalisation and cleanup")

        # append the spectrum axis and doppler window to the output ws name - should not be done
        # before scan and reduce other it will create additional copies of the loaded data
        base_name = [output_ws, self._spectrum_axis]
        if _empty_ws:
            base_name.append("bkg")
        if _calibration_ws:
            base_name.append("cal")
        if self.getProperty("SteppedScanParameter").value:
            base_name.append("step")
        tags = {"ScanTime": "st", "BeamMonitor": "bm", "DirectMeasurement": "dm"}
        try:
            base_name.append(tags[self._cfg._normalise_incident])
        except KeyError:
            raise RuntimeError("Unknown normalise option: {}".format(self._cfg._normalise_incident))
        if self._cfg._doppler_window:
            base_name.append(self._cfg._doppler_window)
        output_ws = "_".join(base_name)

        # perform the normalisation and background removal
        # red_2D = (alpha.sample - empty) / (beta.cal - empty_cal)
        red_2D = self._remove_background_normalise(_sample_ws, _empty_ws, _calibration_ws, _empty_calib_ws, output_ws)

        # set the units to the environment parameter
        ows = mtd[red_2D]
        axis = ows.getAxis(0)
        if self._environment_units:
            evu = self._environment_units
            axis.setUnit("Label").setLabel(evu[0], evu[1])
        else:
            axis.setUnit("Time")

        # setup the spectrum axis and add the ini parameters to the workspace for completeness
        setup_axis(red_2D, self._fractional_group.y_axis)
        options = self._cfg.get_section("processing")
        actuals = self._cfg.processing_options()
        append_ini_params(red_2D, options, actuals)

        # generate the 1D results: I vs Scan and I vs Q and group with the 2D data
        red_1D = output_ws + "_I_Scan"
        SumSpectra(InputWorkspace=red_2D, OutputWorkspace=red_1D)

        # normalise the output to the nearest environment parameter or indexes
        nlim = seq_to_list(self.getPropertyValue("ReferenceRange").strip())
        if nlim:
            self._normalise_reduction(red_2D, nlim)
            self._normalise_reduction(red_1D, nlim)

        # save the 2D transpose with the group but remove spectrum bin edges after
        # the transpose
        red_2DT = output_ws + "_2DT"
        Transpose(InputWorkspace=red_2D, OutputWorkspace=red_2DT)
        remove_hist_edges(red_2DT)
        grouped = [red_1D, red_2D, red_2DT]
        GroupWorkspaces(InputWorkspaces=grouped, OutputWorkspace=output_ws)
        self.setProperty("OutputWorkspace", output_ws)

        # clean up the intermediate workspaces else group them to keep display clean
        if not self._keep_intermediate:
            for wsn in self._intermediate_ws:
                try:
                    DeleteWorkspace(Workspace=wsn)
                except ValueError:
                    pass
            self._intermediate_ws = []
        else:
            GroupWorkspaces(InputWorkspaces=self._intermediate_ws, OutputWorkspace="intermediate")

    def _remove_background_normalise(self, sample_ws: str, empty_ws: str, calib_ws: str, empty_calib_ws: str, output_ws: str) -> str:
        # perform the normalisation and background removal
        # red_2D = (alpha.sample - empty) / (beta.cal - empty_cal)

        red_2D = output_ws + "_2D"
        Scale(InputWorkspace=sample_ws, Factor=self._sample_scale, OutputWorkspace=red_2D)
        if empty_ws:
            Minus(LHSWorkspace=red_2D, RHSWorkspace=empty_ws, OutputWorkspace=red_2D)
            if self._cfg._reset_negatives:
                ResetNegatives(InputWorkspace=red_2D, OutputWorkspace=red_2D, AddMinimum=False)

        if calib_ws:
            denom_ws = "_tmp_cal"
            Scale(InputWorkspace=calib_ws, Factor=self._calibration_scale, OutputWorkspace=denom_ws)
            if empty_calib_ws:
                Minus(LHSWorkspace=denom_ws, RHSWorkspace=empty_calib_ws, OutputWorkspace=denom_ws)
                if self._cfg._reset_negatives:
                    ResetNegatives(InputWorkspace=denom_ws, OutputWorkspace=denom_ws, AddMinimum=False)

            Integration(InputWorkspace=denom_ws, OutputWorkspace=denom_ws)
            Divide(LHSWorkspace=red_2D, RHSWorkspace=denom_ws, OutputWorkspace=red_2D)
            ReplaceSpecialValues(InputWorkspace=red_2D, OutputWorkspace=red_2D, NaNValue=0.0)
            DeleteWorkspace(Workspace=denom_ws)
        return red_2D

    def _complete_doppler_setup(self, sample_file: str, ds_index: int) -> None:
        # updates the amplitude and speed from the file to complete the doppler
        # settings and update the load options
        self._doppler.set_amplitude_speed(sample_file, ds_index)
        self._analyse_load_opts["OverrideDopplerPhase"] = self._doppler.phase
        self._refn_load_opts["OverrideDopplerPhase"] = self._doppler.phase
        self._analyse_load_opts["OverrideDopplerFrequency"] = self._doppler.frequency
        self._refn_load_opts["OverrideDopplerFrequency"] = self._doppler.frequency

    def _get_environment_ranges(self) -> None:
        # set the environment parameter
        param_opts = self.getPropertyValue("ScanParameter").split(",")
        self._environment_param = ""
        self._environment_units = None
        if param_opts:
            try:
                self._environment_param = param_opts[0].strip()
                self._environment_units = (param_opts[1].strip(), param_opts[2].strip())
            except IndexError:
                pass

        self._environment_range = {}

    def _set_environment_units(self, sample_file: str) -> None:
        # if the env param is used but it missing the units try to
        # recover the unit form the attributes, otherwise the units
        # are specified or the env param is not used in which case
        # there is nothing to do
        if self._environment_param and not self._environment_units:
            tags = [HdfKey("env_param", "/entry1/data/" + self._environment_param, None)]
            _, attrs = extract_hdf_params(sample_file, tags)

            env_label = "Unknown"
            env_unit = "?"
            try:
                # set the default units:
                #   'Txxxxxx' refer to temperature and if units are not provided are
                #       assumed to be in deg K
                #   'Pxxxxx' refer to pressure and if units are not included are assumed
                #       to be in mB
                options = [("T", "Temperature", "K"), ("P", "Pressure", "mB")]
                for prefix, label, unit in options:
                    if self._environment_param.startswith(prefix):
                        env_label = label
                        env_unit = unit
                        break
                env_unit = attrs["env_param"]["units"]
            except (KeyError, ValueError):
                pass
            self._environment_units = (env_label, env_unit)

    def _set_up(self) -> None:
        # instrument parameters
        self._inst_ws = LoadEmptyInstrument(Filename="EMUau_Definition.xml")
        LoadParameterFile(self._inst_ws, "EMUau_Parameters.xml")
        self._instrument = self._inst_ws.getInstrument()
        self._efixed = self._instrument.getNumberParameter("EFixed")[0]
        self._analysed_v2 = self._instrument.getNumberParameter("AnalysedV2")[0]
        self._pixels_per_tube = 64  # load from instrument
        self._file_prefix = "EMU"

        # from the configuration (ini) file and then
        # from the run properties for the sample
        #
        temp_folder = self.getPropertyValue("ScratchFolder")
        self._scratch = ScratchFolder(temp_folder) if temp_folder else None

        self._cfg = EmuParameters(allow_no_value=True)
        ini_file = self.getPropertyValue("ConfigurationFile")
        self._cfg._load_parameters(ini_file)

        self._doppler = DopplerSupport(self._cfg._frequency_option, self._cfg._phase_option, self._cfg._doppler_table)

        self._sample_scale = self.getProperty("SampleTransmissionFactor").value
        self._calibration_scale = self.getProperty("CalibrationTransmissionFactor").value
        self._spectrum_axis = self.getPropertyValue("SpectrumAxis")
        self._keep_intermediate = self.getProperty("KeepIntermediateWorkspaces").value
        self._use_fractional_map = self.getProperty("UseFractionalMap").value

        # set up the loader options used in the scan and reduce
        self._analyse_load_opts = {
            "SelectDetectorTubes": self._cfg._analyse_tubes,
            "CalibrateDopplerPhase": False,
            "BinaryEventPath": "../histserv",
        }

        self._refn_load_opts = self._analyse_load_opts.copy()
        self._refn_load_opts["SelectDetectorTubes"] = self._cfg._refn_tubes

        self._filter_ws = FilterEmuPixelsTubes(
            self._cfg._analyse_tubes, self._cfg._pixel_range, self._pixels_per_tube, 0, self._cfg._doppler_window
        )

        # tube groups are indirect hz, indirect vt, direct hz, direct vt
        panel_tubes = [(0, 16), (16, 51), (51, 67), (67, 102)]
        self._fractional_group = FractionalAreaDetectorTubes(
            self._pixels_per_tube,
            self._cfg._analyse_tubes,
            self._spectrum_axis,
            self._cfg._2theta_range,
            self._cfg._q_range,
            self._cfg._q2_range,
            self._efixed,
            panel_tubes,
        )

        self._get_environment_ranges()

        self._fractional_map = None
        self._fractional_wgts = None

        # keep pre-reduced to avoid rebinning which is slow with the reference
        # data that has a lot of events
        self._intermediate_ws = []
        try:
            DeleteWorkspace(Workspace="intermediate")
        except ValueError:
            pass

    def _pre_reduce(self, output_ws: str, runs: Sequence[str], lopts: LoaderOptions, background: float) -> None:
        # performs a load and merge, subtract background,
        # convert to units and rebinning but first check if the output
        # workspace has already been pre-reduced - if so nothing to do
        if output_ws in self._intermediate_ws:
            return

        params = [
            ("OverrideDopplerPhase", "DopplerPhase", 0.01),
            ("OverrideDopplerFrequency", "DopplerFrequency", 0.0001),
            ("SelectDataset", "SelectDataset", 0.1),
        ]

        load_merge(
            LoadEMUHdf,
            runs,
            output_ws,
            lopts,
            event_dirs=self._search_dirs,
            params=params,
            filter=self._filter_ws,
            scratch=self._scratch,
            check_file=self._doppler.check_doppler_settings,
            progress=self._progress,
        )

        # split the background across the spectra to be consistent with
        # mathematica processing
        if background != 0.0:
            ws = mtd[output_ws]
            nhist = ws.getNumberHistograms()
            if nhist > 1:
                ws -= background / nhist

        # add it to the list of reduction intermediate workspaces
        self._intermediate_ws.append(output_ws)

    def _scan_and_reduce(self, output_ws: str, analyse_runs: Sequence[str], single_bin: bool = False) -> str:
        if not analyse_runs:
            return ""

        # load and merge the analysed runs first as there are diff option for the ref
        anl_ws = output_ws + "_bin"
        self._filter_ws.set(valid_tubes=self._cfg._analyse_tubes)
        self._pre_reduce(anl_ws, analyse_runs, self._analyse_load_opts, self._cfg._inebackground)
        stepped_env = self.getProperty("SteppedScanParameter").value

        # determine the pulse map function and bin parameters
        # - if env parameter need to account for stepped or continuous
        # - if pulse then map to dataset index
        if single_bin:
            tsmap, bin_params = self.get_single_bin_map(anl_ws)
        else:
            etag = "env_" + self._environment_param if self._environment_param else ""
            tsmap, bin_params = self.get_pulse_map_bins(anl_ws, etag, stepped_env)

        # rebin the analysis map
        self.rebin_by_values(anl_ws, anl_ws, tsmap, bin_params)

        # build the 2D detector grouping map and sum the spectra for the reference
        self._fractional_group._convert_spectrum(anl_ws, anl_ws)
        if not self._fractional_group.detector_map:
            if self._use_fractional_map:
                self._fractional_group.build_fractional_map(anl_ws, self._spectrum_axis)
            else:
                self._fractional_group.build_detector_map(anl_ws, self._spectrum_axis)
        self._fractional_group.apply_fractional_grouping(anl_ws, anl_ws)

        # Either normalise the data by the scan time or the neutron count per bin
        if self._cfg._normalise_incident == "ScanTime":
            ref_ws = output_ws + "_ref"
            self.scan_time_to_refn("ReactorPower", anl_ws, ref_ws, tsmap, bin_params)
        elif self._cfg._normalise_incident == "BeamMonitor":
            ref_ws = output_ws + "_ref"
            self.scan_time_to_refn("BeamMonitorRate", anl_ws, ref_ws, tsmap, bin_params)
        else:
            refn_runs = analyse_runs
            ref_ws = output_ws + "_ref"

            self._filter_ws.set(valid_tubes=self._cfg._refn_tubes)
            self._pre_reduce(ref_ws, refn_runs, self._refn_load_opts, self._cfg._totbackground)

            self.rebin_by_values(ref_ws, ref_ws, tsmap, bin_params)

            # Build the normalisation workspace
            SumSpectra(InputWorkspace=ref_ws, OutputWorkspace=ref_ws)

        # normalise the analysed by the refn to account for flux,
        # the reference is applied across all spectrum uniformly
        # and add it to the list of intermediate workspaces
        Divide(LHSWorkspace=anl_ws, RHSWorkspace=ref_ws, OutputWorkspace=output_ws)
        ReplaceSpecialValues(InputWorkspace=output_ws, OutputWorkspace=output_ws, NaNValue=0.0)

        # the analysed and refn dataset are total counts and need to be normalised for
        # total time to convert it to scattered neutrons per sec per flux
        factor = total_time(ref_ws) / total_time(anl_ws)
        Scale(InputWorkspace=output_ws, Factor=factor, OutputWorkspace=output_ws)

        # append to the intermediate list
        for tag in [output_ws, ref_ws]:
            if tag not in self._intermediate_ws:
                self._intermediate_ws.append(tag)

        # set the distribution flag true
        ws = mtd[output_ws]
        ws.setDistribution(True)
        ws.setYUnit("Counts")

        return output_ws

    def _filter_over_tube(self, input_ws: str, output_ws: str) -> None:
        # build the vector of tube averaged spectra weighting but ignore the
        # monitors
        ws = mtd[input_ws]
        yv = ws.extractY()
        yt = yv.T.copy()
        yft = np.convolve(
            yt.reshape(
                -1,
            ),
            self._tube_filter_fir,
            mode="same",
        ).reshape(-1, yv.shape[0])

        # create the output workspace and replace the Y values
        if output_ws != input_ws:
            CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=output_ws)
        ows = mtd[output_ws]
        for i in range(len(yv)):
            ows.dataY(i)[:] = yft[:, i]

    def scan_time_to_refn(self, parameter: str, input_ws: str, output_ws: str, tsmap, bin_params) -> None:
        iws = mtd[input_ws]
        irun = iws.getRun()
        start = irun.startTime()

        # get the period from the frame rate and count, there may be addnl
        # events after the last frame so inc by 1 to catch all
        scan = irun.getLogData("ScanPeriod")
        period = scan.value
        times = (scan.times - start.to_datetime64()) / np.timedelta64(1, "s")  # time in seconds
        pulses = []
        step_size = 0.1
        for t0, dt in zip(times, period):
            pulses.append(np.arange(t0, t0 + dt, step=step_size))
        pulses = np.concatenate(pulses).ravel()
        values = tsmap(pulses)

        # include the reactor power or beam monitor as a factor
        rpwr = irun.getLogData("ReactorPower")
        if len(rpwr.times) == 1:
            txv = [0, period[0]]
            rpfact = [1, 1]
        else:
            txv = (rpwr.times - start.to_datetime64()) / np.timedelta64(1, "s")  # time in seconds
            avg_pwr = np.mean(rpwr.value)
            rpfact = rpwr.value / avg_pwr
        pwrfn = interpolate.interp1d(txv, rpfact, bounds_error=False, fill_value="extrapolate")

        weights = pwrfn(pulses)
        kwargs = bin_params.copy()
        kwargs["weights"] = weights
        hist, bin_edges = np.histogram(values, **kwargs)

        # convert back to seconds
        outputY = (hist * step_size).reshape(1, -1)
        outputE = np.sqrt(outputY)
        CreateWorkspace(
            OutputWorkspace=output_ws,
            DataX=bin_edges,
            DataY=outputY,
            DataE=outputE,
            NSpec=1,
            Distribution=iws.isDistribution(),
            WorkspaceTitle=iws.getTitle(),
            ParentWorkspace=input_ws,
        )

    def get_single_bin_map(self, input_ws: str) -> Tuple[Interpolator, Dict[str, List[float]]]:
        iws = mtd[input_ws]
        irun = iws.getRun()
        start = irun.startTime()

        # get the period from the frame rate and count, there may be addnl
        # events after the last frame so inc by 1 to catch all
        scan = irun.getLogData("ScanPeriod")
        period = scan.value
        times = (scan.times - start.to_datetime64()) / np.timedelta64(1, "s")  # time in seconds
        pulse_limits = [times[0], times[-1] + period[-1]]
        tsmap = interpolate.interp1d(pulse_limits, pulse_limits, bounds_error=False, fill_value="extrapolate")

        bin_params = {"bins": pulse_limits}

        return tsmap, bin_params

    def get_pulse_map_bins(self, input_ws: str, env_tag: str, stepped_env: bool) -> Tuple[Interpolator, Dict[str, Any]]:
        iws = mtd[input_ws]
        irun = iws.getRun()
        start = irun.startTime()

        # get the scan times and period
        scan = irun.getLogData("ScanPeriod")
        period = scan.value
        times = (scan.times - start.to_datetime64()) / np.timedelta64(1, "s")  # time in seconds

        if env_tag:  # sample env scan
            tss = irun.getLogData(env_tag)

            # if single point processing (0,T) -> env,
            # extend the bin edges so the bin width is non-zero
            if len(tss.value) == 1:
                tsmap = interpolate.interp1d(
                    [times[0], times[0] + period[0]], [tss.value[0], tss.value[0]], bounds_error=False, fill_value="extrapolate"
                )
                bin_params = {"bins": [tss.value[0] - 1, tss.value[0] + 1]}

            elif stepped_env:
                txv = np.vstack((times, times + period)).T.reshape(
                    -1,
                )
                tyv = np.vstack((tss.value, tss.value)).T.reshape(
                    -1,
                )
                tsmap = interpolate.interp1d(txv, tyv, bounds_error=False, fill_value="extrapolate")

                # sort the env values if necessary as histogram bins are assumed sorted
                def is_sorted(a):
                    return np.all(a[:-1] <= a[1:])

                txv = tss.value if is_sorted(tss.value) else np.sort(tss.value)

                # take the mid point as the bin edges
                bin_edges = np.zeros(len(txv) + 1)
                bin_edges[1:-1] = 0.5 * (txv[1:] + txv[:-1])
                bin_edges[0] = 2 * txv[0] - bin_edges[1]
                bin_edges[-1] = 2 * txv[-1] - bin_edges[-2]
                bin_params = {"bins": bin_edges}

            else:  # continuously variable param
                times = (tss.times - start.to_datetime64()) / np.timedelta64(1, "s")  # time in seconds
                tsmap = interpolate.interp1d(times, tss.value, bounds_error=False, fill_value="extrapolate")

                # bins are not defined so use the limits of the observation, noting
                # that SICS records the value at the end of the scan for each dataset
                # so add the value at t = 0 as a limit
                ystart = tsmap(0)
                ymin = min(ystart, np.min(tss.value))
                ymax = max(ystart, np.max(tss.value))
                bin_params = {"bins": len(tss.value), "range": (ymin, ymax)}

        else:  # time scan
            bin_range = [times[0], times[-1] + period[-1]]
            tsmap = interpolate.interp1d(bin_range, bin_range, bounds_error=False, fill_value="extrapolate")
            bin_params = {"bins": len(times), "range": bin_range}

        return tsmap, bin_params

    def rebin_by_values(self, input_ws: str, output_ws: str, tsmap: Interpolator, bin_params: Dict[str, Any]) -> None:
        # open the workspace, get the variable timeseries and create the interpolator,
        # note that the env variables are captured at the end dataset acquisition
        iws = mtd[input_ws]
        irun = iws.getRun()
        start = irun.startTime()

        # get the histogram arguments
        if "range" in bin_params:
            nsteps = bin_params["bins"]
        else:
            nsteps = len(bin_params["bins"]) - 1

        nspec = iws.getNumberHistograms()
        outputY = np.zeros((nspec, nsteps))
        outputE = np.zeros_like(outputY)
        bin_edges = np.histogram_bin_edges([], **bin_params)

        # for each spectrum map pulse times to xval and histogram the vector
        count = 0
        time_start = time.time()
        for ix in range(nspec):
            evl = iws.getSpectrum(ix)
            tv = evl.getPulseTimesAsNumpy()
            count += len(tv)
            if len(tv) > 0:
                tvs = (tv - start.to_datetime64()) / np.timedelta64(1, "s")  # time in seconds
                # convert to env value
                tvs = tsmap(tvs)
                hist, _ = np.histogram(tvs, bins=bin_edges)
                outputY[ix, :] = hist[:]
        outputE = np.sqrt(outputY)
        time_end = time.time()

        self.log().information("total events = {}, hist = {}, {:.1f} sec".format(count, np.sum(outputY), time_end - time_start))

        # create the 2D workspace and add the label
        target_ws = output_ws + "_tmp_" if input_ws == output_ws else output_ws
        CreateWorkspace(
            OutputWorkspace=target_ws,
            DataX=bin_edges,
            DataY=outputY,
            DataE=outputE,
            NSpec=nspec,
            Distribution=iws.isDistribution(),
            WorkspaceTitle=iws.getTitle(),
            ParentWorkspace=input_ws,
        )

        if input_ws == output_ws:
            DeleteWorkspace(input_ws)
            RenameWorkspace(InputWorkspace=target_ws, OutputWorkspace=output_ws)

    def _normalise_reduction(self, ws_tag: str, nlim: Sequence[float]) -> None:
        if len(nlim) > 2:
            self.log().warning("Got more than 2 values for the normalisation range, using {} to {}".format(nlim[0], nlim[-1]))

        # extract the y and x values for the reduced data
        ws = mtd[ws_tag]
        xv_ = ws.extractX()[0, :]
        xv = 0.5 * (xv_[:-1] + xv_[1:])
        ym = ws.extractY()

        # if the environment param is set then determine the columns from the
        # limits, otherwise use these as the indexes
        lo_index = find_nearest_index(xv, nlim[0])
        hi_index = find_nearest_index(xv, nlim[-1]) + 1

        ref_counts = np.mean(ym[:, lo_index:hi_index])
        Scale(InputWorkspace=ws_tag, Factor=1.0 / ref_counts, OutputWorkspace=ws_tag)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(ElasticEMUauReduction)
