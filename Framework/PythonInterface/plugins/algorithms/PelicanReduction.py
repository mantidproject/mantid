# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import re
import math
import numpy as np
from typing import List, Dict

# import debugpy

from mantid import mtd
from mantid.kernel import StringListValidator, Direction, StringArrayMandatoryValidator, StringArrayProperty, CompositeValidator
from mantid.api import PythonAlgorithm, FileProperty, WorkspaceProperty, FileAction, Progress
from mantid.dataobjects import EventList
from mantid.simpleapi import (
    Scale,
    Minus,
    Divide,
    ResetNegatives,
    DeleteWorkspace,
    CloneWorkspace,
    ReplaceSpecialValues,
    GroupWorkspaces,
    FindEPP,
    Integration,
    SaveNXSPE,
    ExtractMonitors,
    SofQW,
    CorrectKiKf,
    SumSpectra,
    Transpose,
    Rebin,
    ConvertUnits,
    LoadPLN,
    AlgorithmFactory,
    logger,
)

from ANSTO.ansto_common import (
    ScratchFolder,
    FilterPixelsTubes,
    IniParameters,
    hdf_files_from_runs,
    expand_directories,
    load_merge,
    range_to_values,
    extract_hdf_params,
    append_ini_params,
    HdfKey,
    LoaderOptions,
)

# some defaults
DEF_MAX_ENERGY_GAIN = 1000.0
MEV_TO_WAVEVECTOR = 0.6947
TWO_THETA = 125.0 * math.pi / 180.0


def beam_monitor_counts(src: str) -> int:
    run = mtd[src].getRun()
    return np.sum(run.getProperty("MonitorCounts").value)


def scale_and_remove_background(
    source_ws: str, source_scale: float, empty_ws: str, empty_scale: float, out_ws: str, floor_negatives: bool
) -> None:
    # Common reduction processing step in the algorithm,
    #   source_scale.source_ws - empty_scale.empty_ws -> out_ws
    alpha_scale = source_scale * 1.0e6 / beam_monitor_counts(source_ws)
    Scale(InputWorkspace=source_ws, Factor=alpha_scale, OutputWorkspace=out_ws)
    if empty_ws:
        factor = empty_scale * 1.0e6 / beam_monitor_counts(empty_ws)
        Scale(InputWorkspace=empty_ws, Factor=factor, OutputWorkspace=empty_ws)
        Minus(LHSWorkspace=out_ws, RHSWorkspace=empty_ws, OutputWorkspace=out_ws)
        if floor_negatives:
            ResetNegatives(InputWorkspace=out_ws, OutputWorkspace=out_ws, AddMinimum=False)


class PelicanReduction(PythonAlgorithm):
    def category(self) -> str:
        return "Workflow\\Inelastic;Inelastic;Inelastic\\Reduction"

    def summary(self) -> str:
        return "Performs an inelastic energy transfer reduction for ANSTO Pelican geometry data."

    def seeAlso(self) -> List:
        return []

    def name(self) -> str:
        return "PelicanReduction"

    def PyInit(self) -> None:
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        self.declareProperty(
            StringArrayProperty("SampleRuns", values=[], validator=mandatoryInputRuns),
            doc="Optional cycle number followed by comma separated range of\nsample runs as [cycle::] n1,n2,..\n eg 123::7333-7341,7345",
        )

        self.declareProperty(
            name="EmptyRuns",
            defaultValue="",
            doc="Optional cycle number followed by comma separated range of\nruns as [cycle::] n1,n2,..\n  eg 123::6300-6308",
        )

        self.declareProperty(name="ScaleEmptyRuns", defaultValue=1.0, doc="Scale the empty runs prior to subtraction")

        self.declareProperty(
            name="CalibrationRuns",
            defaultValue="",
            doc="Optional cycle number followed by comma separated range of\nruns as [cycle::] n1,n2,..\n  eg 123::6350-6365",
        )

        self.declareProperty(
            name="EmptyCalibrationRuns",
            defaultValue="",
            doc="Optional cycle number followed by comma separated range of\nruns as [cycle::] n1,n2,..\n  eg 123::6370-6375",
        )

        self.declareProperty(
            name="EnergyTransfer", defaultValue="0.0, 0.02, 3.0", doc="Energy transfer range in meV expressed as min, step, max"
        )

        self.declareProperty(
            name="MomentumTransfer",
            defaultValue="",
            doc="Momentum transfer range in inverse Angstroms,\n"
            "expressed as min, step, max\n"
            "Default estimates the max range based on energy transfer.",
        )

        self.declareProperty(
            name="Processing",
            defaultValue="SOFQW1-Centre",
            validator=StringListValidator(["SOFQW1-Centre", "SOFQW3-NormalisedPolygon", "NXSPE"]),
            doc="Convert to SOFQW or save file as NXSPE,\nnote SOFQW3 is more accurate but much slower than SOFQW1.",
        )

        self.declareProperty(name="LambdaOnTwoMode", defaultValue=False, doc="Set if instrument running in lambda on two mode.")

        self.declareProperty(name="FrameOverlap", defaultValue=False, doc="Set if the energy transfer extends over a frame.")

        self.declareProperty(name="CalibrateTOF", defaultValue=False, doc="Determine the TOF correction from the elastic peak in the data.")

        self.declareProperty(name="TOFCorrection", defaultValue="", doc="The TOF correction in usec that aligns the elastic peak.")

        self.declareProperty(name="AnalyseTubes", defaultValue="", doc="Detector tubes to be used in the data analysis.")

        self.declareProperty(name="MaxEnergyGain", defaultValue="", doc="Energy gain in meV used to adjust the min TOF with frame overlap.")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Name for the reduced workspace.")

        self.declareProperty(
            FileProperty("ScratchFolder", "", action=FileAction.OptionalDirectory, direction=Direction.Input),
            doc="Path to save and restore merged workspaces.",
        )

        self.declareProperty(
            name="KeepIntermediateWorkspaces",
            defaultValue=False,
            doc="Whether to keep the intermediate sample and calibration\nworkspaces for diagnostic checks.",
        )

        self.declareProperty(
            FileProperty("ConfigurationFile", "", action=FileAction.OptionalLoad, extensions=["ini"]),
            doc="Optional: INI file to override default processing values.",
        )

    def PyExec(self) -> None:
        # add the following to where it should break
        # debugpy.wait_for_client()
        # debugpy.breakpoint()

        # Set up the processing parameters
        self.setUp()

        # remove any remnant intermediate files
        try:
            ws = mtd["intermediate"]
            DeleteWorkspace(ws)
        except KeyError:
            pass

        # Get the list of data files from the runs
        self._search_dirs = expand_directories(self._search_path)

        def pval(tag):
            return self.getPropertyValue(tag)

        # Get the list of data files from the runs
        sample_runs = hdf_files_from_runs(pval("SampleRuns"), self._search_dirs, self._file_prefix, self._file_extn)
        empty_runs = hdf_files_from_runs(pval("EmptyRuns"), self._search_dirs, self._file_prefix, self._file_extn)
        calibration_runs = hdf_files_from_runs(pval("CalibrationRuns"), self._search_dirs, self._file_prefix, self._file_extn)
        empty_calib_runs = hdf_files_from_runs(pval("EmptyCalibrationRuns"), self._search_dirs, self._file_prefix, self._file_extn)
        total_runs = sum([len(x) for x in [sample_runs, empty_runs, calibration_runs, empty_calib_runs]])

        # set up the wavelength from the sample runs - sample runs includes the dataset index
        # which needs to be removed
        sample_file = re.sub(r":[0-9]+$", "", sample_runs[0])
        self.set_efixed(sample_file)
        if not self._q_range:
            self.set_qrange()

        # The progress includes 4 additional status reports on top of incrementing
        # the progress on each loaded file
        self._progress = Progress(self, start=0.0, end=1.0, nreports=total_runs + 4)
        self._progress.report("File selection complete, loading initial file")

        # If the output workspace is not provided use the basename of the first
        # sample file
        output_ws = self.getPropertyValue("OutputWorkspace")

        # load, merge and convert to the app. energy units
        # the calibration uses a fixed energy bin range to ensure consistent
        # integration results with the FindEPP algorithm
        cal_energy_bins = "-1.5, 0.02, 1.5"
        _sample_ws = self._load_and_reduce("_sample", sample_runs)
        _empty_ws = self._load_and_reduce("_empty", empty_runs)
        _calibration_ws = self._load_and_reduce("_calibration", calibration_runs, energy_bins=cal_energy_bins)
        _empty_calib_ws = self._load_and_reduce("_empty_calib", empty_calib_runs, energy_bins=cal_energy_bins)

        self._progress.report("Background removal, normalization and cleanup")

        # append the selected processing option to the output ws name
        output_ws += self._process_suffix[self._processing]

        # Perform background removal and normalization against the integrated calibration data as
        #   red_2D = (alpha.sample_ws - empty_ws) / integrated (beta.calibration_ws - empty_cal_ws)
        red_2D = output_ws + "_2D"
        scale_and_remove_background(_sample_ws, self._sample_scale, _empty_ws, self._scale_empty, red_2D, self._reset_negatives)
        if _calibration_ws:
            denom_ws = "_integ_cal"
            self._integrated_calibration(_calibration_ws, _empty_calib_ws, denom_ws)

            # perform normalization step and add the denom_ws to be cleaned up later
            Divide(LHSWorkspace=red_2D, RHSWorkspace=denom_ws, OutputWorkspace=red_2D)
            ReplaceSpecialValues(InputWorkspace=red_2D, OutputWorkspace=red_2D, NaNValue=0.0, InfinityValue=0.0)
            self._intermediate_ws.append(denom_ws)

        if self._processing == "NXSPE":
            self._progress.report("NXSPE processing")
            self._nxspe_processing(red_2D)
        else:
            self._progress.report("SOFQW-{} processing".format(self._sofqw_mode))
            self._sofqw_processing(red_2D, output_ws)

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

        self._progress.report("Clean up complete")

    def _nxspe_processing(self, reduced_2D: str) -> None:
        if self._scratch is None:
            nxspe_file = reduced_2D + ".nxspe"
        else:
            nxspe_file = os.path.join(self._scratch.folder, reduced_2D + ".nxspe")

        # SaveNXSPE works with the detectors only
        red_det = reduced_2D + "det"
        ExtractMonitors(InputWorkspace=reduced_2D, DetectorWorkspace=red_det)
        SaveNXSPE(InputWorkspace=red_det, Filename=nxspe_file, EFixed=self._efixed, Psi=self._mscor)
        self.setProperty("OutputWorkspace", reduced_2D)
        DeleteWorkspace(Workspace=red_det)

    def _sofqw_processing(self, reduced_2D: str, output_ws: str) -> None:
        # convert to SofQW and KIKf correction and transpose axis
        SofQW(
            InputWorkspace=reduced_2D,
            OutputWorkspace=reduced_2D,
            QAxisBinning=self._q_range,
            EMode="Direct",
            EFixed=self._efixed,
            Method=self._sofqw_mode,
            ReplaceNANs=True,
        )
        CorrectKiKf(InputWorkspace=reduced_2D, OutputWorkspace=reduced_2D, EMode="Direct", EFixed=self._efixed)

        red_1D = output_ws + "_1D_dE"
        SumSpectra(InputWorkspace=reduced_2D, OutputWorkspace=red_1D, RemoveSpecialValues=True)
        Transpose(InputWorkspace=reduced_2D, OutputWorkspace=reduced_2D)

        # generate the 1D results and group with the 2D data and update the axis for the 2D data
        # which was created when the detector grouping was created and add the ini params
        # for completeness
        red_1DQ = output_ws + "_1D_Q"
        # drop exception handling when updated formal Mantid build includes SumSpectra change
        try:
            SumSpectra(InputWorkspace=reduced_2D, OutputWorkspace=red_1DQ, UseFractionalArea=False, RemoveSpecialValues=True)
        except:
            SumSpectra(InputWorkspace=reduced_2D, OutputWorkspace=red_1DQ, RemoveSpecialValues=True)
        options = self._cfg.get_section("processing")
        actuals = self.processing_options()
        append_ini_params(red_1D, options, actuals)
        append_ini_params(red_1DQ, options, actuals)
        append_ini_params(reduced_2D, options, actuals)
        grouped = [red_1D, red_1DQ, reduced_2D]
        GroupWorkspaces(InputWorkspaces=grouped, OutputWorkspace=output_ws)
        self.setProperty("OutputWorkspace", output_ws)

    def _integrated_calibration(self, calibration_ws: str, empty_calib_ws: str, output_ws: str) -> None:
        scale_and_remove_background(
            calibration_ws, self._calibration_scale, empty_calib_ws, self._cal_background_scale, output_ws, self._reset_negatives
        )

        # Scale the data by the calibration after it has been integrated
        if self._cal_peak_intensity:
            self._integrate_over_peak(output_ws, output_ws)
        else:
            Integration(
                InputWorkspace=output_ws, OutputWorkspace=output_ws, RangeLower=self._lo_integ_range, RangeUpper=self._hi_integ_range
            )

        # average the normalization over the tube
        self._average_over_tube(output_ws, output_ws)

    def set_efixed(self, sample_path: str) -> None:
        # the instrument offers a lambda on two mode which effectively
        # halves the neutron wavelength, the captured raw data stores the
        # nominal wavelength so it needs to be divided by 2
        tags = [HdfKey("wavelength", "/entry1/instrument/crystal/wavelength", None), HdfKey("mscor", "/entry1/sample/mscor", [0.0])]
        values, _ = extract_hdf_params(sample_path, tags)
        if self._lambda_on_two:
            wavelength = 0.5 * float(values["wavelength"][0])
        else:
            wavelength = float(values["wavelength"][0])
        # standard conversion factor from wavelength (A) to meV using
        # planck's constant and neutron mass
        ANGSTROMS_TO_MEV = 81.804
        self._efixed = float(ANGSTROMS_TO_MEV / wavelength**2)
        self._mscor = float(values["mscor"][0])

    def set_qrange(self) -> None:
        Ki = MEV_TO_WAVEVECTOR * math.sqrt(self._efixed)
        # get max transfer to neutron from sample
        max_xfer = -range_to_values(self._ev_range)[0]
        max_xfer = max(0.0, max_xfer)
        Kt = MEV_TO_WAVEVECTOR * math.sqrt(self._efixed + max_xfer)
        Qmax_sqrd = Ki**2 + Kt**2 - 2 * Ki * Kt * math.cos(TWO_THETA)
        self._q_range = "0.0, 0.02, {:.1f}".format(math.sqrt(Qmax_sqrd))

    def processing_options(self) -> Dict[str, str]:
        # returns a dict of parameters that are not visible from the UI
        # and affect the processing results
        options = {}
        options["analysis_tubes"] = self._analyse_tubes
        options["integrate_over_peak"] = str(self._cal_peak_intensity)
        options["average_peak_width"] = str(self._average_peak_width)
        options["active_pixels"] = "{}-{}".format(self._pixel_range[0], self._pixel_range[1])
        options["lo_integ_range"] = "{:.3f}".format(self._lo_integ_range)
        options["hi_integ_range"] = "{:.3f}".format(self._hi_integ_range)
        options["reset_negatives"] = str(self._reset_negatives)
        options["sample_scale"] = "{:.3f}".format(self._sample_scale)
        options["calibration_scale"] = "{:.3f}".format(self._calibration_scale)
        options["cal_background_scale"] = "{:.3f}".format(self._cal_background_scale)

        return options

    def setUp(self) -> None:
        self._pixels_per_tube = 64
        self._detector_spectra = 12800  # 200 * 64
        self._file_prefix = "PLN"

        # convert to SOFQW or save as NXSPE
        processing = self.getPropertyValue("Processing").split("-")
        self._processing = processing[0]
        self._sofqw_mode = processing[1] if len(processing) > 1 else ""
        self._process_suffix = {"SOFQW1": "_qw1", "SOFQW3": "_qw3", "NXSPE": "_spe"}

        # from the configuration (ini) file and then
        # from the run properties for the sample
        #
        temp_folder = self.getPropertyValue("ScratchFolder")
        self._scratch = ScratchFolder(temp_folder) if temp_folder else None

        self._cfg = IniParameters(allow_no_value=True)
        ini_file = self.getPropertyValue("ConfigurationFile")
        if ini_file:
            if os.path.isfile(ini_file):
                self._cfg.read(ini_file)
            else:
                raise RuntimeError('Cannot file ini file: "{}"'.format(ini_file))

        self._file_extn = self._cfg.get_param(str, "processing", "file_extn", ".nx.hdf")

        self._analyse_tubes = self._cfg.get_param(str, "processing", "analyse_tubes", "0-180")
        tubes = self.getProperty("AnalyseTubes").value
        if tubes:
            self._analyse_tubes = tubes
        logger.warning("Analysis tubes : {}".format(self._analyse_tubes))

        self._ev_range = self.getProperty("EnergyTransfer").value
        self._q_range = self.getProperty("MomentumTransfer").value
        self._scale_empty = self.getProperty("ScaleEmptyRuns").value

        self._search_path = [x.strip() for x in self._cfg.get_param(str, "processing", "search_path", ".").split(";")]

        self._cal_peak_intensity = self._cfg.get_bool("processing", "integrate_over_peak", True)
        self._average_peak_width = self._cfg.get_bool("processing", "average_peak_width", True)
        pixels = self._cfg.get_param(str, "processing", "active_pixels", "0-63").split("-")
        self._pixel_range = (int(pixels[0]), int(pixels[-1]))

        self._sample_scale = self._cfg.get_param(float, "processing", "sample_scale", 1.0)
        self._calibration_scale = self._cfg.get_param(float, "processing", "calibration_scale", 1.0)
        self._cal_background_scale = self._cfg.get_param(float, "processing", "cal_background_scale", 1.0)

        self._keep_intermediate = self.getProperty("KeepIntermediateWorkspaces").value
        self._lambda_on_two = self.getProperty("LambdaOnTwoMode").value
        self._frame_overlap = self.getProperty("FrameOverlap").value

        self._lo_integ_range = self._cfg.get_param(float, "processing", "lo_integ_range", -0.18)
        self._hi_integ_range = self._cfg.get_param(float, "processing", "hi_integ_range", 0.18)
        self._reset_negatives = self._cfg.get_bool("processing", "reset_negatives", False)

        self._tof_correction = self._cfg.get_param(float, "processing", "tof_correction", -258.0)
        self._calibrate_tof = self.getProperty("CalibrateTOF").value
        tof_value = self.getProperty("TOFCorrection").value.strip()
        if tof_value:
            self._tof_correction = float(tof_value)
        if self._calibrate_tof:
            logger.warning("Calibrate TOF enabled.")
        else:
            logger.warning("TOF correction : {:.1f} usec".format(self._tof_correction))

        self._max_energy_gain = self._cfg.get_param(float, "processing", "max_energy_gain", DEF_MAX_ENERGY_GAIN)
        svalue = self.getProperty("MaxEnergyGain").value.strip()
        if svalue:
            self._max_energy_gain = float(svalue)

        # set up the loader options used in the scan and reduce
        self._analyse_load_opts = {
            "BinaryEventPath": "./hsdata",
            "CalibrateTOFBias": self._calibrate_tof,
            "TimeOfFlightBias": self._tof_correction,
            "LambdaOnTwoMode": self._lambda_on_two,
        }

        self._filter_ws = FilterPixelsTubes(self._analyse_tubes, self._pixel_range, self._pixels_per_tube, 0)

        # keep pre-reduced to avoid rebinning which is slow with the reference
        # data that has a lot of events
        self._intermediate_ws = []

    def _integrate_over_peak(self, input_ws: str, output_ws: str) -> None:
        # performs an 3 sigma integration around a gaussian fitted peak
        iws = mtd[input_ws]
        nhist = iws.getNumberHistograms()

        # get the gaussian fit parameters per spectra
        epps = FindEPP(iws)
        lo_vals = np.empty(nhist)
        hi_vals = np.empty(nhist)
        for i in range(nhist):
            peak = epps.cell("PeakCentre", i)
            sigma = epps.cell("Sigma", i)
            lo_vals[i] = peak - 3 * sigma
            hi_vals[i] = peak + 3 * sigma
        DeleteWorkspace(Workspace=epps)

        if self._average_peak_width:
            lo = lo_vals[np.nonzero(lo_vals)].mean()
            hi = hi_vals[np.nonzero(hi_vals)].mean()
            Integration(InputWorkspace=input_ws, OutputWorkspace=output_ws, RangeLower=lo, RangeUpper=hi)
        else:
            Integration(InputWorkspace=input_ws, OutputWorkspace=output_ws, RangeLowerList=lo_vals, RangeUpperList=hi_vals)

    def _average_over_tube(self, input_ws: str, output_ws: str) -> None:
        # build the vector of tube averaged spectra weighting but ignore the
        # monitors
        ws = mtd[input_ws]
        yv = ws.extractY()
        yd = yv[: self._detector_spectra]
        y2d = yd.reshape(-1, self._pixels_per_tube)
        yk = np.ones_like(y2d).T * np.mean(y2d, axis=1)
        yav = yk.T.reshape(-1)

        # create the output workspace and replace the Y values
        if output_ws != input_ws:
            CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=output_ws)
        ows = mtd[output_ws]
        for i in range(len(yav)):
            ows.dataY(i)[0] = yav[i] if yav[i] > 0 else 1.0

    def _get_minimum_tof(self) -> float:
        """
        Converts the maximum energy transfer to neutron to an equivalent
        minimum tof. The distance from the sample to the detector is 2.4m (fixed) and
        source to sample is 0.695m. The result is the minimum tof from source to detector
        and the result is returned in microseconds.
        """
        nom_velocity = 437.4 * math.sqrt(self._efixed)
        max_meV = self._efixed + self._max_energy_gain
        max_velocity = 437.4 * math.sqrt(max_meV)
        min_tof = 0.695 / nom_velocity + 2.4 / max_velocity
        return min_tof * 1e6

    def _adjust_frame_overlap(self, eventlist: EventList, gate_period: float, min_tof: float) -> None:
        tofs, pulsetimes = eventlist.getTofs(), eventlist.getPulseTimes()

        # shift the fast event to the end of the frame
        cnd = tofs < min_tof
        tofs[cnd] += gate_period

        # clear and read events
        eventlist.clear(False)
        for tof, pt in zip(tofs, pulsetimes):
            eventlist.addEventQuickly(tof, pt)

    def _load_and_reduce(self, output_ws: str, analyse_runs: List[str], convert_dE: bool = True, energy_bins: str = "") -> str:
        # check if no runs or already loaded
        if not analyse_runs:
            return ""

        if self._keep_intermediate:
            # include the pre-converted merged file for analysis if needed
            merged_ws = output_ws + "_merged"
            self._load_merge(analyse_runs, merged_ws, self._analyse_load_opts)
            CloneWorkspace(InputWorkspace=merged_ws, OutputWorkspace=output_ws)
            self._intermediate_ws.append(merged_ws)
        else:
            self._load_merge(analyse_runs, output_ws, self._analyse_load_opts)

        # if frame overlap is enabled then shift the tof by the gate period
        if self._frame_overlap:
            # if max energy gain is not set at the UI display the actual value used
            if not self.getProperty("MaxEnergyGain").value.strip():
                logger.warning("Using default 'MaxEnergyGain' = {:.1f} meV".format(self._max_energy_gain))
            ows = mtd[output_ws]
            try:
                gate_period = ows.getRun().getProperty("GatePeriod").value[0]
            except TypeError:
                gate_period = ows.getRun().getProperty("GatePeriod").value
            minimum_tof = self._get_minimum_tof()
            for i in range(ows.getNumberHistograms()):
                evl = ows.getSpectrum(i)
                self._adjust_frame_overlap(evl, gate_period, minimum_tof)

            # reset the X values
            maxTOF = ows.getTofMax()
            minTOF = ows.getTofMin()
            paramstr = "{}, {}, {}".format(minTOF, maxTOF - minTOF, maxTOF)
            Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws, Params=paramstr, PreserveEvents=True)

        if convert_dE:
            # the energy conversion for analysed data uses the existing
            # unit conversion
            ConvertUnits(
                InputWorkspace=output_ws, OutputWorkspace=output_ws, Target="DeltaE", EMode="Direct", EFixed=self._efixed, AlignBins=True
            )
            use_energy_bins = energy_bins if energy_bins else self._ev_range
            Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws, Params=use_energy_bins, PreserveEvents=False)
        else:
            Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws, Params="0, 1, 6000", PreserveEvents=True)

        self._intermediate_ws.append(output_ws)

        return output_ws

    def _load_merge(self, runs: List[str], output_ws: str, lopts: LoaderOptions):
        params = [("LambdaOnTwoMode", "LambdaOnTwoMode", 0), ("SelectDataset", "SelectDataset", 0.1)]
        if lopts["CalibrateTOFBias"]:
            params.append(("CalibrateTOFBias", "CalibrateTOF", 0.1))
        else:
            params.append(("TimeOfFlightBias", "TOFCorrection", 1.0))

        load_merge(
            LoadPLN,
            runs,
            output_ws,
            lopts,
            event_dirs=self._search_dirs,
            params=params,
            filter=self._filter_ws,
            scratch=self._scratch,
            progress=self._progress,
        )

        # highlight the values determined by the calibration process
        if lopts["CalibrateTOFBias"]:
            run = mtd[output_ws].getRun()
            tof = run.getProperty("TOFCorrection").value
            if len(tof) > 1:
                logger.warning("Calibrated TOF : {:.1f} +- {:.1f} usec for {} runs".format(np.mean(tof), np.std(tof), len(tof)))
            else:
                logger.warning("Calibrated TOF : {:.1f} usec".format(tof[0]))


# Register algorithm with Mantid
AlgorithmFactory.subscribe(PelicanReduction)
