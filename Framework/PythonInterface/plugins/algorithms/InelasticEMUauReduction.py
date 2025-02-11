# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os

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
from mantid.api import PythonAlgorithm, FileProperty, WorkspaceProperty, FileAction, Progress
from mantid.simpleapi import (
    Scale,
    Minus,
    Divide,
    Rebin,
    ResetNegatives,
    DeleteWorkspace,
    ReplaceSpecialValues,
    GroupWorkspaces,
    Integration,
    LoadEMU,
    LoadEMUHdf,
    SumSpectra,
    AlgorithmFactory,
    LoadEmptyInstrument,
    LoadParameterFile,
    NormaliseSpectra,
    ConvertUnits,
    ChangeBinOffset,
)

from ANSTO.ansto_common import (
    ScratchFolder,
    hdf_files_from_runs,
    expand_directories,
    load_merge,
    integrate_over_peak,
    total_time,
    FractionalAreaDetectorTubes,
    append_ini_params,
    setup_axis,
)
from ANSTO.emu_common import FilterEmuPixelsTubes, EmuParameters, DopplerSupport


class InelasticEMUauReduction(PythonAlgorithm):
    def category(self):
        return "Workflow\\Inelastic;Inelastic\\Indirect;Inelastic\\Reduction"

    def summary(self):
        return "Performs an inelastic energy transfer reduction for ANSTO EMU indirect geometry data."

    def seeAlso(self):
        return ["IndirectILLEnergyTransfer"]

    def name(self):
        return "InelasticEMUauReduction"

    def PyInit(self):
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        self.declareProperty(
            StringArrayProperty("SampleRuns", values=[], validator=mandatoryInputRuns),
            doc="Comma separated range of sample runs,\n eg [cycle::] 7333-7341,7345",
        )

        self.declareProperty(
            name="BackgroundRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [path::] 6300-6308",
        )

        self.declareProperty(
            name="CalibrationRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [path::] 6350-6365",
        )

        self.declareProperty(
            name="BackgroundCalibrationRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [path::] 6370-6375",
        )

        self.declareProperty(
            name="IncidentSpectrumRuns",
            defaultValue="",
            doc="Optional path followed by comma separated range of runs,\n"
            "looking for runs in the sample folder if path not included,\n"
            "  eg [path::] 6375-6385",
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

    def PyExec(self):
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
        incident_beam_runs = hdf_files_from_runs(pval("IncidentSpectrumRuns"), self._search_dirs, self._file_prefix, self._cfg._file_extn)

        total_runs = sum([len(x) for x in [sample_runs, empty_runs, calibration_runs, empty_calib_runs]])
        if len(incident_beam_runs) == 0:
            total_runs *= 2
        else:
            total_runs += len(incident_beam_runs)

        # set up the wavelength from the sample runs - sample runs includes the dataset index
        # which needs to be removed
        split_fname = sample_runs[0].split(self._cfg._file_extn + ":")
        sample_file = split_fname[0] + self._cfg._file_extn
        ds_index = int(split_fname[1]) if len(split_fname) > 1 else 0
        self._complete_doppler_setup(sample_file, ds_index)
        self._cfg.set_energy_transfer_bins(self._doppler, self._efixed, self._analysed_v2)

        self._progress = Progress(self, start=0.0, end=1.0, nreports=total_runs + 2)
        self._progress.report("File selection complete, loading files")

        # If the output workspace is not provided use the basename of the first
        # sample file
        output_ws = self.getPropertyValue("OutputWorkspace")

        # now call scan and reduce to accumulate the data and normalize to
        # pseudo incident beam monitor
        _sample_ws = self._scan_and_reduce("_sample", sample_runs, incident_beam_runs)
        _empty_ws = self._scan_and_reduce("_empty", empty_runs, incident_beam_runs)
        _calibration_ws = self._scan_and_reduce("_calibration", calibration_runs, incident_beam_runs)
        _empty_calib_ws = self._scan_and_reduce("_empty_calib", empty_calib_runs, incident_beam_runs)

        self._progress.report("Background removal, normalization and cleanup")

        # append the spectrum axis and doppler window to the output ws name - should not be done
        # before scan and reduce other it will create additional copies of the loaded data
        base_name = [output_ws, self._spectrum_axis]
        if _empty_ws is not None:
            base_name.append("bkg")
        if _calibration_ws is not None:
            base_name.append("cal")
        if self._cfg._doppler_window:
            base_name.append(self._cfg._doppler_window)
        output_ws = "_".join(base_name)

        # perform the normalization and background removal
        # red_2D = (alpha.sample - empty) / (beta.cal - empty_cal)

        red_2D = output_ws + "_2D"
        Scale(InputWorkspace=_sample_ws, Factor=self._sample_scale, OutputWorkspace=red_2D)
        if _empty_ws is not None:
            Minus(LHSWorkspace=red_2D, RHSWorkspace=_empty_ws, OutputWorkspace=red_2D)
            if self._cfg._reset_negatives:
                ResetNegatives(InputWorkspace=red_2D, OutputWorkspace=red_2D, AddMinimum=False)

        if _calibration_ws is not None:
            denom_ws = "_tmp_cal"
            Scale(InputWorkspace=_calibration_ws, Factor=self._calibration_scale, OutputWorkspace=denom_ws)
            if _empty_calib_ws is not None:
                Minus(LHSWorkspace=denom_ws, RHSWorkspace=_empty_calib_ws, OutputWorkspace=denom_ws)
                if self._cfg._reset_negatives:
                    ResetNegatives(InputWorkspace=denom_ws, OutputWorkspace=denom_ws, AddMinimum=False)

            # Scale the data by the calibration which has been integrated or fitted (clarification with NDS)
            if self._cfg._cal_peak_intensity:
                integrate_over_peak(denom_ws, denom_ws, self._cfg._average_peak_width)
            else:
                Integration(
                    InputWorkspace=denom_ws,
                    OutputWorkspace=denom_ws,
                    RangeLower=self._cfg._integration_range[0],
                    RangeUpper=self._cfg._integration_range[1],
                )
            Divide(LHSWorkspace=red_2D, RHSWorkspace=denom_ws, OutputWorkspace=red_2D)
            ReplaceSpecialValues(InputWorkspace=red_2D, OutputWorkspace=red_2D, NaNValue=0.0)
            DeleteWorkspace(Workspace=denom_ws)

        # generate the 1D results and group with the 2D data and update the axis for the 2D data
        # which was created when the detector grouping was created and add the ini params
        # for completeness
        red_1D = output_ws + "_1D"
        SumSpectra(InputWorkspace=red_2D, OutputWorkspace=red_1D)
        if self._cfg._normalise_spectra:
            NormaliseSpectra(InputWorkspace=red_1D, OutputWorkspace=red_1D)
        setup_axis(red_2D, self._fractional_group.y_axis)

        options = self._cfg.get_section("processing")
        actuals = self._cfg.processing_options()
        append_ini_params(red_1D, options, actuals)
        append_ini_params(red_2D, options, actuals)
        grouped = [red_1D, red_2D]
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

    def _complete_doppler_setup(self, sample_file, ds_index):
        # updates the amplitude and speed from the file to complete the doppler
        # settings and update the load options
        self._doppler.set_amplitude_speed(sample_file, ds_index)
        self._analyse_load_opts["OverrideDopplerPhase"] = self._doppler.phase
        self._refn_load_opts["OverrideDopplerPhase"] = self._doppler.phase
        self._analyse_load_opts["OverrideDopplerFrequency"] = self._doppler.frequency
        self._refn_load_opts["OverrideDopplerFrequency"] = self._doppler.frequency

    def setUp(self):
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
        if ini_file and not os.path.isfile(ini_file):
            raise RuntimeError('Cannot file ini file: "{}"'.format(ini_file))
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
        # IHx = 0
        # IVx = 16
        # DHx = 51
        # DVx = 67
        # TUBES = 102
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

        self._x_units = "DeltaE"
        self._e_mode = "Indirect"

        # keep pre-reduced to avoid rebinning which is slow with the reference
        # data that has a lot of events
        self._intermediate_ws = []
        try:
            DeleteWorkspace(Workspace="intermediate")
        except ValueError:
            pass

    def _pre_reduce(self, basename, runs, lopts, background, direct, sum_spectra):
        # performs a load and merge, subtract background,
        # convert to units and rebinning but first check if the output
        # workspace has already been pre-reduced - if so nothing to do
        output_ws = basename + "_dE"
        if output_ws in self._intermediate_ws:
            return output_ws

        params = [
            ("OverrideDopplerPhase", "DopplerPhase", 0.01),
            ("OverrideDopplerFrequency", "DopplerFrequency", 0.0001),
            ("SelectDataset", "SelectDataset", 0.1),
        ]

        emu_loader = LoadEMU if self._cfg._file_extn == ".tar" else LoadEMUHdf
        load_merge(
            emu_loader,
            runs,
            basename,
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
            ws = mtd[basename]
            nhist = ws.getNumberHistograms()
            if nhist > 1:
                ws -= background / nhist

        # the energy conversion for analysed data uses the existing
        # unit conversion
        # direct measurements that estimated the counts in the energy
        # band converts to energy as an elastic conversion and subtract
        # the nominal energy
        if direct:
            ConvertUnits(
                InputWorkspace=basename, OutputWorkspace=output_ws, Target="Energy", EMode="Elastic", EFixed=self._efixed, AlignBins=True
            )
            ChangeBinOffset(InputWorkspace=output_ws, OutputWorkspace=output_ws, Offset=-self._efixed)
            # convert the axis units to ensure compatibility for division
            axis = mtd[output_ws].getAxis(0)
            axis.setUnit("DeltaE")
        else:
            ConvertUnits(
                InputWorkspace=basename, OutputWorkspace=output_ws, Target="DeltaE", EMode="Indirect", EFixed=self._efixed, AlignBins=True
            )

        if self._cfg._centre_offset != 0.0:
            ChangeBinOffset(InputWorkspace=output_ws, OutputWorkspace=output_ws, Offset=self._cfg._centre_offset)

        Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws, Params=self._cfg._ev_range, PreserveEvents=False)

        if sum_spectra:
            SumSpectra(InputWorkspace=output_ws, OutputWorkspace=output_ws)

        # add the basename to the list of reduction intermediate workspaces
        # the output workspace is managed by the caller
        self._intermediate_ws.append(basename)

        return output_ws

    def _scan_and_reduce(self, basename, analyse_runs, _refn_runs):
        if not analyse_runs:
            return None

        # load and merge the analysed and refn runs
        # if the refn run is empty use the direct data for the analysed runs
        self._filter_ws.set(valid_tubes=self._cfg._analyse_tubes)
        output_ws = self._pre_reduce(
            basename, analyse_runs, self._analyse_load_opts, self._cfg._inebackground, direct=False, sum_spectra=False
        )
        if len(_refn_runs) > 0:
            refn_runs = _refn_runs
            ref_ws = "_incident_beam_ref"
        else:
            refn_runs = analyse_runs
            ref_ws = basename + "_ref"
        self._filter_ws.set(valid_tubes=self._cfg._refn_tubes)
        ref_ws = self._pre_reduce(ref_ws, refn_runs, self._refn_load_opts, self._cfg._totbackground, direct=True, sum_spectra=True)

        # build the 2D detector grouping map and sum the spectra for the reference
        self._fractional_group._convert_spectrum(output_ws, output_ws)
        if not self._fractional_group.detector_map:
            if self._use_fractional_map:
                self._fractional_group.build_fractional_map(output_ws, self._spectrum_axis)
            else:
                self._fractional_group.build_detector_map(output_ws, self._spectrum_axis)
        self._fractional_group.apply_fractional_grouping(output_ws, output_ws)

        # normalize the analysed by the refn to account for flux,
        # the reference is applied across all spectrum uniformly
        # and add it to the list of intermediate workspaces
        Divide(LHSWorkspace=output_ws, RHSWorkspace=ref_ws, OutputWorkspace=output_ws)
        ReplaceSpecialValues(InputWorkspace=output_ws, OutputWorkspace=output_ws, NaNValue=0.0)

        # the analysed and refn dataset are total counts and need to be normalized for
        # total time to convert it to scattered neutrons per sec per flux
        factor = total_time(ref_ws) / total_time(output_ws)
        Scale(InputWorkspace=output_ws, Factor=factor, OutputWorkspace=output_ws)

        # append to the intermediate list
        for tag in [output_ws, ref_ws]:
            if tag not in self._intermediate_ws:
                self._intermediate_ws.append(tag)

        return output_ws


# Register algorithm with Mantid
AlgorithmFactory.subscribe(InelasticEMUauReduction)
