# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-branches

import os
import tempfile
from collections import namedtuple
from contextlib import contextmanager
import numpy as np

from enum import Enum
from mantid import config as mantid_config
from mantid.api import (
    DataProcessorAlgorithm,
    AlgorithmFactory,
    FileProperty,
    WorkspaceProperty,
    FileAction,
    PropertyMode,
    mtd,
    AnalysisDataService,
    Progress,
)
from mantid.simpleapi import (
    DeleteWorkspace,
    LoadEventNexus,
    SetGoniometer,
    SetUB,
    ModeratorTzeroLinear,
    SaveNexus,
    ConvertToMD,
    LoadMask,
    MaskDetectors,
    LoadNexus,
    MDNormSCDPreprocessIncoherent,
    MDNormSCD,
    MultiplyMD,
    CreateSingleValuedWorkspace,
    ConvertUnits,
    CropWorkspace,
    DivideMD,
    MinusMD,
    RenameWorkspace,
    ConvertToMDMinMaxGlobal,
    ClearMaskFlag,
    ScaleX,
    Plus,
)
from mantid.kernel import Direction, FloatArrayProperty, FloatArrayLengthValidator, logger


class VDAS(Enum):
    """Specifices the version of the Data Acquisition System (DAS)"""

    v1900_2018 = 0  # Up to Dec 31 2018
    v2019_2100 = 1  # From Jan 01 2018


@contextmanager
def pyexec_setup(new_options):
    """
    Backup keys of mantid.config and clean up temporary files and workspaces
    upon algorithm completion or exception raised.
    Workspaces with names beginning with '_t_' are assumed temporary.

    Parameters
    ----------
    new_options: dict
        Dictionary of mantid configuration options to be modified.
    """
    # Hold in this tuple all temporary objects to be removed after completion
    temp_objects = namedtuple("temp_objects", "files workspaces")
    temps = temp_objects(list(), list())

    previous_config = dict()
    for key, value in new_options.items():
        previous_config[key] = mantid_config[key]
        mantid_config[key] = value
    try:
        yield temps
    finally:
        # reinstate the mantid options
        for key, value in previous_config.items():
            mantid_config[key] = value
        # delete temporary files
        for file_name in temps.files:
            os.remove(file_name)
        # remove any workspace added to temps.workspaces or whose name begins
        # with "_t_"
        to_be_removed = set()
        for name in AnalysisDataService.getObjectNames():
            if "_t_" == name[0:3]:
                to_be_removed.add(name)
        for workspace in temps.workspaces:
            if isinstance(workspace, str):
                to_be_removed.add(workspace)
            else:
                to_be_removed.add(workspace.name())
        for name in to_be_removed:
            DeleteWorkspace(name)


class BASISCrystalDiffraction(DataProcessorAlgorithm):
    _mask_file = "/SNS/BSS/shared/autoreduce/new_masks_08_12_2015/" "BASIS_Mask_default_diff.xml"
    _solid_angle_ws_ = "/SNS/BSS/shared/autoreduce/solid_angle_diff.nxs"
    _flux_ws_ = "/SNS/BSS/shared/autoreduce/int_flux.nxs"
    _wavelength_bands = {"311": [3.07, 3.60], "111": [6.05, 6.60]}
    _diff_bank_numbers = list(range(5, 14))
    _tzero = dict(gradient=11.967, intercept=-5.0)

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)
        self._wavelength_band = None  # units of inverse Angstroms
        self._short_inst = "BSS"
        self._long_inst = "BASIS"
        self._temps = None
        self._bkg = None  # Events workspace for brackground runs
        self._bkg_scale = None
        self._vanadium_files = None
        self._momentum_range = None
        self._t_mask = None
        self._n_bins = None

    @staticmethod
    def category():
        return "Diffraction\\Reduction"

    @staticmethod
    def version():
        return 1

    @staticmethod
    def summary():
        return "Multiple-file BASIS crystal reduction for diffraction " "detectors."

    @staticmethod
    def seeAlso():
        return ["AlignDetectors", "DiffractionFocussing", "SNSPowderReduction"]

    @staticmethod
    def _run_list(runs):
        """
        Obtain all run numbers from input string `runs`

        Parameters
        ----------
        runs: str
            Run numbers to be reduced.
        Returns
        -------
            list
        """
        rl = list()
        rn = runs.replace(" ", "")  # remove spaces
        for x in rn.split(","):
            if "-" in x:
                b, e = [int(y) for y in x.split("-")]
                rl.extend([str(z) for z in range(b, e + 1)])
            else:
                rl.append(x)
        return rl

    @staticmethod
    def add_previous_pulse(w):
        """
        Duplicate the events but shift them by one pulse, then add to
        input workspace

        Parameters
        ----------
        w: Mantid.EventsWorkspace

        Returns
        -------
        Mantid.EventsWorkspace
        """
        pulse_width = 1.0e6 / 60  # in micro-seconds
        _t_w = ScaleX(w, Factor=-pulse_width, Operation="Add")
        _t_w = Plus(w, _t_w, OutputWorkspace=w.name())
        return _t_w

    def PyInit(self):
        # Input validators
        array_length_three = FloatArrayLengthValidator(3)
        # Properties
        self.declareProperty("RunNumbers", "", "Sample run numbers")

        self.declareProperty(
            FileProperty(name="MaskFile", defaultValue=self._mask_file, action=FileAction.OptionalLoad, extensions=[".xml"]),
            doc="See documentation for latest mask files.",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output Workspace. If background is " + "subtracted, _data and _background " + "workspaces will also be generated",
        )

        #
        # Background for the sample runs
        #
        background_title = "Background runs"
        self.declareProperty("BackgroundRuns", "", "Background run numbers")
        self.setPropertyGroup("BackgroundRuns", background_title)
        self.declareProperty("BackgroundScale", 1.0, doc="The background will be scaled by this " + "number before being subtracted.")
        self.setPropertyGroup("BackgroundScale", background_title)
        #
        # Vanadium
        #
        vanadium_title = "Vanadium runs"
        self.declareProperty("VanadiumRuns", "", "Vanadium run numbers")
        self.setPropertyGroup("VanadiumRuns", vanadium_title)

        #
        # Single Crystal Diffraction
        #
        crystal_diffraction_title = "Single Crystal Diffraction"
        self.declareProperty(
            "PsiAngleLog", "SE50Rot", direction=Direction.Input, doc="log entry storing rotation of the sample" "around the vertical axis"
        )
        self.declareProperty("PsiOffset", 0.0, direction=Direction.Input, doc="Add this quantity to PsiAngleLog")
        self.declareProperty(
            FloatArrayProperty("LatticeSizes", [0, 0, 0], array_length_three, direction=Direction.Input),
            doc='three item comma-separated list "a, b, c"',
        )
        self.declareProperty(
            FloatArrayProperty("LatticeAngles", [90.0, 90.0, 90.0], array_length_three, direction=Direction.Input),
            doc="three item comma-separated " + 'list "alpha, beta, gamma"',
        )
        #    Reciprocal vector to be aligned with incoming beam
        self.declareProperty(
            FloatArrayProperty("VectorU", [1, 0, 0], array_length_three, direction=Direction.Input),
            doc="three item, comma-separated, HKL indexes" "of the diffracting plane",
        )
        #    Reciprocal vector orthogonal to VectorU and in-plane with
        #    incoming beam
        self.declareProperty(
            FloatArrayProperty("VectorV", [0, 1, 0], array_length_three, direction=Direction.Input),
            doc="three item, comma-separated, HKL indexes" "of the direction perpendicular to VectorV" "and the vertical axis",
        )
        #    Abscissa view
        self.declareProperty(
            FloatArrayProperty("Uproj", [1, 0, 0], array_length_three, direction=Direction.Input),
            doc="three item comma-separated Abscissa view" "of the diffraction pattern",
        )
        #    Ordinate view
        self.declareProperty(
            FloatArrayProperty("Vproj", [0, 1, 0], array_length_three, direction=Direction.Input),
            doc="three item comma-separated Ordinate view" "of the diffraction pattern",
        )
        #    Hidden axis
        self.declareProperty(FloatArrayProperty("Wproj", [0, 0, 1], array_length_three, direction=Direction.Input), doc="Hidden axis view")
        #    Binnin in reciprocal slice
        self.declareProperty("NBins", 400, direction=Direction.Input, doc="number of bins in the HKL slice")

        for a_property in (
            "PsiAngleLog",
            "PsiOffset",
            "LatticeSizes",
            "LatticeAngles",
            "VectorU",
            "VectorV",
            "Uproj",
            "Vproj",
            "Wproj",
            "NBins",
        ):
            self.setPropertyGroup(a_property, crystal_diffraction_title)

    def PyExec(self):
        # Facility and database configuration
        config_new_options = {"default.facility": "SNS", "default.instrument": "BASIS", "datasearch.searcharchive": "On"}

        with pyexec_setup(config_new_options) as self._temps:
            # Load the mask to a temporary workspace
            self._t_mask = LoadMask(Instrument="BASIS", InputFile=self.getProperty("MaskFile").value, OutputWorkspace="_t_mask")
            #
            # Find the version of the Data Acquisition System
            #
            self._find_das_version()
            #
            # Find valid incoming momentum range
            #
            self._calculate_wavelength_band()
            self._momentum_range = np.sort(2 * np.pi / self._wavelength_band)
            #
            # Pre-process the background runs
            #
            if self.getProperty("BackgroundRuns").value:
                bkg_run_numbers = self._run_list(self.getProperty("BackgroundRuns").value)
                background_reporter = Progress(self, start=0.0, end=1.0, nreports=len(bkg_run_numbers))
                for i, run in enumerate(bkg_run_numbers):
                    if self._bkg is None:
                        self._bkg = self._mask_t0_crop(run, "_bkg")
                        self._temps.workspaces.append("_bkg")
                    else:
                        _ws = self._mask_t0_crop(run, "_ws")
                        self._bkg += _ws
                        if "_ws" not in self._temps.workspaces:
                            self._temps.workspaces.append("_ws")
                    message = "Pre-processing background: {} of {}".format(i + 1, len(bkg_run_numbers))
                    background_reporter.report(message)
                SetGoniometer(self._bkg, Axis0="0,0,1,0,1")
                self._bkg_scale = self.getProperty("BackgroundScale").value
                background_reporter.report(len(bkg_run_numbers), "Done")

            # Pre-process the vanadium run(s) by removing the delayed
            # emission time from the moderator and then saving to file(s)
            if self.getProperty("VanadiumRuns").value:
                run_numbers = self._run_list(self.getProperty("VanadiumRuns").value)
                vanadium_reporter = Progress(self, start=0.0, end=1.0, nreports=len(run_numbers))
                self._vanadium_files = list()
                for i, run in enumerate(run_numbers):
                    self._vanadium_files.append(self._save_t0(run))
                    message = "Pre-processing vanadium: {} of {}".format(i + 1, len(run_numbers))
                    vanadium_reporter.report(message)
                vanadium_reporter.report(len(run_numbers), "Done")

            # Determination of single crystal diffraction
            self._determine_single_crystal_diffraction()

    def _determine_single_crystal_diffraction(self):
        """
        All work related to the determination of the diffraction pattern
        """

        a, b, c = self.getProperty("LatticeSizes").value
        alpha, beta, gamma = self.getProperty("LatticeAngles").value

        u = self.getProperty("VectorU").value
        v = self.getProperty("VectorV").value

        uproj = self.getProperty("Uproj").value
        vproj = self.getProperty("Vproj").value
        wproj = self.getProperty("Wproj").value

        n_bins = self.getProperty("NBins").value
        self._n_bins = (n_bins, n_bins, 1)

        axis0 = "{},0,1,0,1".format(self.getProperty("PsiAngleLog").value)
        axis1 = "{},0,1,0,1".format(self.getProperty("PsiOffset").value)

        # Options for SetUB independent of run
        ub_args = dict(a=a, b=b, c=c, alpha=alpha, beta=beta, gamma=gamma, u=u, v=v)
        min_values = None
        # Options for algorithm ConvertToMD independent of run
        convert_to_md_kwargs = dict(
            QDimensions="Q3D", dEAnalysisMode="Elastic", Q3DFrames="HKL", QConversionScales="HKL", Uproj=uproj, Vproj=vproj, Wproj=wproj
        )
        md_norm_scd_kwargs = None  # Options for algorithm MDNormSCD

        # Find solid angle and flux
        if self._vanadium_files:
            kwargs = dict(
                Filename="+".join(self._vanadium_files),
                MaskFile=self.getProperty("MaskFile").value,
                MomentumMin=self._momentum_range[0],
                MomentumMax=self._momentum_range[1],
            )
            _t_solid_angle, _t_int_flux = MDNormSCDPreprocessIncoherent(**kwargs)
        else:
            _t_solid_angle = self.nominal_solid_angle("_t_solid_angle")
            _t_int_flux = self.nominal_integrated_flux("_t_int_flux")

        # Process a sample at a time
        run_numbers = self._run_list(self.getProperty("RunNumbers").value)
        diffraction_reporter = Progress(self, start=0.0, end=1.0, nreports=len(run_numbers))
        for i_run, run in enumerate(run_numbers):
            _t_sample = self._mask_t0_crop(run, "_t_sample")

            # Set Goniometer and UB matrix
            SetGoniometer(_t_sample, Axis0=axis0, Axis1=axis1)
            SetUB(_t_sample, **ub_args)
            if self._bkg:
                self._bkg.run().getGoniometer().setR(_t_sample.run().getGoniometer().getR())
                SetUB(self._bkg, **ub_args)
            # Determine limits for momentum transfer in HKL space. Needs to be
            # done only once. We use the first run.
            if min_values is None:
                kwargs = dict(QDimensions="Q3D", dEAnalysisMode="Elastic", Q3DFrames="HKL")
                min_values, max_values = ConvertToMDMinMaxGlobal(_t_sample, **kwargs)
                convert_to_md_kwargs.update({"MinValues": min_values, "MaxValues": max_values})

            # Convert to MD
            _t_md = ConvertToMD(_t_sample, OutputWorkspace="_t_md", **convert_to_md_kwargs)
            if self._bkg:
                _t_bkg_md = ConvertToMD(self._bkg, OutputWorkspace="_t_bkg_md", **convert_to_md_kwargs)

            # Determine aligned dimensions. Need to be done only once
            if md_norm_scd_kwargs is None:
                aligned = list()
                for i_dim in range(3):
                    kwargs = {
                        "name": _t_md.getDimension(i_dim).name,
                        "min": min_values[i_dim],
                        "max": max_values[i_dim],
                        "n_bins": self._n_bins[i_dim],
                    }
                    aligned.append("{name},{min},{max},{n_bins}".format(**kwargs))
                md_norm_scd_kwargs = dict(
                    AlignedDim0=aligned[0],
                    AlignedDim1=aligned[1],
                    AlignedDim2=aligned[2],
                    FluxWorkspace=_t_int_flux,
                    SolidAngleWorkspace=_t_solid_angle,
                    SkipSafetyCheck=True,
                )

            # Normalize sample by solid angle and integrated flux;
            # Accumulate runs into the temporary workspaces
            MDNormSCD(
                _t_md,
                OutputWorkspace="_t_data",
                OutputNormalizationWorkspace="_t_norm",
                TemporaryDataWorkspace="_t_data" if mtd.doesExist("_t_data") else None,
                TemporaryNormalizationWorkspace="_t_norm" if mtd.doesExist("_t_norm") else None,
                **md_norm_scd_kwargs,
            )
            if self._bkg:
                MDNormSCD(
                    _t_bkg_md,
                    OutputWorkspace="_t_bkg_data",
                    OutputNormalizationWorkspace="_t_bkg_norm",
                    TemporaryDataWorkspace="_t_bkg_data" if mtd.doesExist("_t_bkg_data") else None,
                    TemporaryNormalizationWorkspace="_t_bkg_norm" if mtd.doesExist("_t_bkg_norm") else None,
                    **md_norm_scd_kwargs,
                )
            message = "Processing sample {} of {}".format(i_run + 1, len(run_numbers))
            diffraction_reporter.report(message)
        self._temps.workspaces.append("PreprocessedDetectorsWS")  # to remove
        # Iteration over the sample runs is done.

        # Division by vanadium, subtract background, and rename workspaces
        name = self.getPropertyValue("OutputWorkspace")
        _t_data = DivideMD(LHSWorkspace="_t_data", RHSWorkspace="_t_norm")
        if self._bkg:
            _t_bkg_data = DivideMD(LHSWorkspace="_t_bkg_data", RHSWorkspace="_t_bkg_norm")
            _t_scale = CreateSingleValuedWorkspace(DataValue=self._bkg_scale)
            _t_bkg_data = MultiplyMD(_t_bkg_data, _t_scale)
            ws = MinusMD(_t_data, _t_bkg_data)
            RenameWorkspace(_t_data, OutputWorkspace=name + "_dat")
            RenameWorkspace(_t_bkg_data, OutputWorkspace=name + "_bkg")
        else:
            ws = _t_data
        RenameWorkspace(ws, OutputWorkspace=name)
        self.setProperty("OutputWorkspace", ws)
        diffraction_reporter.report(len(run_numbers), "Done")

    def _save_t0(self, run_number, name="_t_ws"):
        """
        Create temporary events file with delayed emission time from
        moderator removed
        :param run: run number
        :param name: name for the output workspace
        :return: file name of event file with events treated with algorithm
        ModeratorTzeroLinear.
        """
        ws = self._load_single_run(run_number, name)
        ws = ModeratorTzeroLinear(
            InputWorkspace=ws.name(), Gradient=self._tzero["gradient"], Intercept=self._tzero["intercept"], OutputWorkspace=ws.name()
        )
        # Correct old DAS shift of fast neutrons. See GitHub issue 23855
        if self._das_version == VDAS.v1900_2018:
            ws = self.add_previous_pulse(ws)
        file_name = self._spawn_tempnexus()
        SaveNexus(ws, file_name)
        return file_name

    def _mask_t0_crop(self, run_number, name):
        """
        Load a run into a workspace with:
         1. Masked detectors
         2. Delayed emission time from  moderator removed
         3. Conversion of units to momentum
         4. Remove events outside the valid momentum range
        :param run_number: BASIS run number
        :param name: name for the output workspace
        :return: workspace object
        """
        ws = self._load_single_run(run_number, name)
        MaskDetectors(ws, MaskedWorkspace=self._t_mask)
        ws = ModeratorTzeroLinear(
            InputWorkspace=ws.name(), Gradient=self._tzero["gradient"], Intercept=self._tzero["intercept"], OutputWorkspace=ws.name()
        )
        # Correct old DAS shift of fast neutrons. See GitHub issue 23855
        if self._das_version == VDAS.v1900_2018:
            ws = self.add_previous_pulse(ws)
        ws = ConvertUnits(ws, Target="Momentum", OutputWorkspace=ws.name())
        ws = CropWorkspace(ws, OutputWorkspace=ws.name(), XMin=self._momentum_range[0], XMax=self._momentum_range[1])
        return ws

    def _load_single_run(self, run, name):
        """
        Find and load events from the diffraction tubes.

        Run number 90000 discriminates between the old and new DAS

        Parameters
        ----------
        run: str
            Run number
        name: str
            Name of the output EventsWorkspace

        Returns
        -------
        EventsWorkspace
        """
        banks = ",".join(["bank{}".format(i) for i in self._diff_bank_numbers])
        particular = {VDAS.v1900_2018: dict(NXentryName="entry-diff"), VDAS.v2019_2100: dict(BankName=banks)}
        identifier = "{0}_{1}".format(self._short_inst, str(run))
        kwargs = dict(Filename=identifier, SingleBankPixelsOnly=False, OutputWorkspace=name)
        kwargs.update(particular[self._das_version])
        return LoadEventNexus(**kwargs)

    def _spawn_tempnexus(self):
        """
        Create a temporary file and flag for removal upon algorithm completion.
        :return: (str) absolute path to the temporary file.
        """
        f = tempfile.NamedTemporaryFile(
            prefix="BASISCrystalDiffraction_", suffix=".nxs", dir=mantid_config["defaultsave.directory"], delete=False
        )
        file_name = f.name
        f.close()
        self._temps.files.append(file_name)  # flag for removal
        return file_name

    def nominal_solid_angle(self, name):
        """
        Generate an isotropic solid angle
        :param name: Name of the output workspace
        :return: reference to solid angle workspace
        """
        ws = LoadNexus(Filename=self._solid_angle_ws_, OutputWorkspace=name)
        ClearMaskFlag(ws)
        MaskDetectors(ws, MaskedWorkspace=self._t_mask)
        for i in range(ws.getNumberHistograms()):
            ws.dataY(i)[0] = 0.0 if ws.getDetector(i).isMasked() else 1.0
            ws.setX(i, self._momentum_range)
        return ws

    def nominal_integrated_flux(self, name):
        """
        Generate a flux independent of momentum
        :param name: Name of the output workspace
        :return: reference to flux workspace
        """
        ws = LoadNexus(Filename=self._flux_ws_, OutputWorkspace=name)
        ClearMaskFlag(ws)
        MaskDetectors(ws, MaskedWorkspace=self._t_mask)
        return ws

    def _find_das_version(self):
        boundary_run = 90000  # from VDAS.v1900_2018 to VDAS.v2019_2100
        runs = self.getProperty("RunNumbers").value
        first_run = int(self._run_list(runs)[0])
        if first_run < boundary_run:
            self._das_version = VDAS.v1900_2018
        else:
            self._das_version = VDAS.v2019_2100
        logger.information("DAS version is " + str(self._das_version))

    def _calculate_wavelength_band(self):
        """
        Select the wavelength band examining the logs of the first sample
        """
        runs = self.getProperty("RunNumbers").value
        run = self._run_list(runs)[0]
        _t_w = self._load_single_run(run, "_t_w")
        wavelength = np.mean(_t_w.getRun().getProperty("LambdaRequest").value)
        logger.error("DEBUG wavelength = " + str(wavelength))
        for reflection, band in self._wavelength_bands.items():
            if band[0] <= wavelength <= band[1]:
                self._wavelength_band = np.array(band)
                break


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISCrystalDiffraction)
