# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import random
import os
import string
import numpy as np
from collections import namedtuple
from contextlib import contextmanager

from enum import Enum
from mantid import config as mantid_config
from mantid.api import (
    AnalysisDataService,
    DataProcessorAlgorithm,
    AlgorithmFactory,
    FileProperty,
    FileAction,
    WorkspaceProperty,
    PropertyMode,
)
from mantid.simpleapi import (
    DeleteWorkspace,
    LoadMask,
    LoadEventNexus,
    CloneWorkspace,
    MaskDetectors,
    ModeratorTzeroLinear,
    ConvertUnits,
    CropWorkspace,
    RenameWorkspace,
    LoadNexusMonitors,
    OneMinusExponentialCor,
    Scale,
    Divide,
    Rebin,
    MedianDetectorTest,
    SumSpectra,
    Integration,
    CreateWorkspace,
    ScaleX,
    Plus,
)
from mantid.kernel import FloatArrayProperty, Direction, EnabledWhenProperty, PropertyCriterion, StringListValidator, logger

temp_prefix = "_tp_"  # marks a workspace as temporary


class VDAS(Enum):
    """Specifices the version of the Data Acquisition System (DAS)"""

    v1900_2018 = 0  # Up to Dec 31 2018
    v2019_2100 = 1  # From Jan 01 2018


def unique_workspace_name(n=5, prefix="", suffix=""):
    r"""
    Create a random sequence of `n` lowercase characters that is guaranteed
    not to collide with the name of any existing Mantid workspace registered
    in the analysis data service.

    uws stands for Unique Workspace Name

    Parameters
    ----------
    n: int
        Size of the sequence
    prefix: str
        String to prefix the randon sequence
    suffix: str
        String to suffix the randon sequence

    Returns
    -------
    str
    """

    ws_name = "".join(random.choice(string.ascii_lowercase) for _ in range(n))
    ws_name = "{}{}{}".format(str(prefix), ws_name, str(suffix))
    while ws_name in AnalysisDataService.getObjectNames():
        characters = [random.choice(string.ascii_lowercase) for _ in range(n)]
        ws_name = "".join(characters)
        ws_name = "{}{}{}".format(str(prefix), ws_name, str(suffix))
    return ws_name


def tws(marker=""):
    r"""
    String starting with temp_prefix and guaranteed not to collide with the name of
    any existing Mantid workspace in the analysis data service

    Parameters
    ----------
    marker: str
        String to identify the data contained in the workspace. Used as suffix

    Returns
    -------
    str
    """
    return unique_workspace_name(prefix=temp_prefix, suffix="_" + marker)


@contextmanager
def pyexec_setup(remove_temp, new_options):
    """
    Backup keys of mantid.config
    and clean up temporary files and workspaces
    upon algorithm completion or exception raised.
    Workspaces with names beginning
    with the temporary workspace marker
    are assumed temporary.

    Parameters
    ----------
    remove_temp: bool
        Determine wether to remove the temporary workspaces
    new_options: dict
        Dictionary of mantid configuration options to be modified.

    Yields
    ------
    namedtuple:
        tuple containing two lists.
        The first list to hold temporary workspaces of arbitrary names.
        The second list to hold temporary file names.
        Used to delete the workspaces and files upon algorithm completion.
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
        if remove_temp is False:
            return
        # delete temporary files
        for file_name in temps.files:
            os.remove(file_name)
        # remove any workspace added to temps.workspaces or whose name begins
        # with "_t_"
        to_be_removed = set()
        for name in AnalysisDataService.getObjectNames():
            if temp_prefix in name:
                to_be_removed.add(name)
        for workspace in temps.workspaces:
            if isinstance(workspace, str):
                to_be_removed.add(workspace)
            else:
                to_be_removed.add(workspace.name())
        for name in to_be_removed:
            DeleteWorkspace(name)


class BASISPowderDiffraction(DataProcessorAlgorithm):
    _mask_file = "/SNS/BSS/shared/autoreduce/new_masks_08_12_2015/" "BASIS_Mask_default_diff.xml"
    # Consider only events with these wavelengths
    _wavelength_bands = {"311": [3.07, 3.60], "111": [6.05, 6.60], "333": [2.02, 2.20]}
    _diff_bank_numbers = list(range(5, 14))
    _tzero = dict(gradient=11.967, intercept=-5.0)

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)
        self._wavelength_band = None
        self._wavelength_dl = 0.0025  # in Angstroms
        self._qbins = None
        self._short_inst = "BSS"
        self._temps = None
        self._bkg = None  # Events workspace for brackground runs
        self._bkg_scale = None
        self._van = None  # workspace for vanadium files
        self._v_mask = None  # mask pixels with low-counts in vanadium runs
        self._t_mask = None  # mask workspace
        self._das_version = None  # version of the Data Acquisition System
        self._flux_normalization_type = None  # default to no flux normalizat.

    @staticmethod
    def category():
        return "Diffraction\\Reduction"

    @staticmethod
    def version():
        return 1

    @staticmethod
    def summary():
        return "Diffraction pattern for powder samples"

    @staticmethod
    def seeAlso():
        return ["BASISReduction", "BASISCrystalDiffraction"]

    @staticmethod
    def _run_list(runs):
        """
        Obtain all run numbers from input string `runs`

        Parameters
        ----------
        runs: str
            Run numbers to be reduced
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
        local_name = tws("previous_pulse")
        _t_w = ScaleX(w, Factor=-pulse_width, Operation="Add", OutputWorkspace=local_name)
        _t_w = Plus(w, _t_w, OutputWorkspace=w.name())
        return _t_w

    def PyInit(self):
        #
        # Properties
        #
        self.declareProperty("RunNumbers", "", "Sample run numbers")

        #
        #  Normalization selector
        #
        title_flux_normalization = "Flux Normalization"
        self.declareProperty("DoFluxNormalization", True, direction=Direction.Input, doc="Do we normalize data by incoming flux?")
        self.setPropertyGroup("DoFluxNormalization", title_flux_normalization)
        if_flux_normalization = EnabledWhenProperty("DoFluxNormalization", PropertyCriterion.IsDefault)
        flux_normalization_types = ["Monitor", "Proton Charge", "Duration"]
        default_flux_normalization = flux_normalization_types[0]
        self.declareProperty(
            "FluxNormalizationType", default_flux_normalization, StringListValidator(flux_normalization_types), "Flux Normalization Type"
        )
        self.setPropertySettings("FluxNormalizationType", if_flux_normalization)
        self.setPropertyGroup("FluxNormalizationType", title_flux_normalization)

        self.declareProperty(
            FloatArrayProperty("MomentumTransferBins", [0.1, 0.0025, 2.5], direction=Direction.Input),  # invers A
            "Momentum transfer binning scheme",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", optional=PropertyMode.Mandatory, direction=Direction.Output),
            doc="Output reduced workspace",
        )

        self.declareProperty(
            FileProperty(name="MaskFile", defaultValue=self._mask_file, action=FileAction.OptionalLoad, extensions=[".xml"]),
            doc="See documentation for latest mask files.",
        )
        #
        # Background for the sample runs
        #
        background_title = "Background runs"
        self.declareProperty("BackgroundRuns", "", "Background run numbers")
        self.setPropertyGroup("BackgroundRuns", background_title)
        self.declareProperty("BackgroundScale", 1.0, doc="The background will be scaled by this " + "number before being subtracted.")
        self.setPropertyGroup("BackgroundScale", background_title)
        self.declareProperty(
            WorkspaceProperty("OutputBackground", "", optional=PropertyMode.Optional, direction=Direction.Output),
            doc="Reduced workspace for background runs",
        )
        self.setPropertyGroup("OutputBackground", background_title)
        #
        # Vanadium
        #
        vanadium_title = "Vanadium runs"
        self.declareProperty("VanadiumRuns", "", "Vanadium run numbers")
        self.setPropertyGroup("VanadiumRuns", vanadium_title)
        #
        # Aditional output properties
        #
        titleAddionalOutput = "Additional Output"
        self.declareProperty("RemoveTemporaryWorkspaces", True, direction=Direction.Input, doc="Remove temporary workspaces and files")
        self.setPropertyGroup("RemoveTemporaryWorkspaces", titleAddionalOutput)

    def PyExec(self):
        # Facility and database configuration
        config_new_options = {"default.facility": "SNS", "default.instrument": "BASIS", "datasearch.searcharchive": "On"}
        if self.getProperty("DoFluxNormalization").value is True:
            self._flux_normalization_type = self.getProperty("FluxNormalizationType").value
        #
        # Find desired Q-binning
        #
        self._qbins = np.array(self.getProperty("MomentumTransferBins").value)
        remove_temp = self.getProperty("RemoveTemporaryWorkspaces").value
        with pyexec_setup(remove_temp, config_new_options) as self._temps:
            #
            # Load the mask to a temporary workspace
            #
            self._t_mask = LoadMask(Instrument="BASIS", InputFile=self.getProperty("MaskFile").value, OutputWorkspace=tws("mask"))
            #
            # Find the version of the Data Acquisition System
            #
            self._find_das_version()
            #
            # Calculate the valid range of wavelengths for incoming neutrons
            #
            self._calculate_wavelength_band()
            #
            # Load and process vanadium runs, if applicable
            #
            if self.getProperty("VanadiumRuns").value != "":
                self._load_vanadium_runs()
            #
            # Process the sample
            #
            runs = self.getProperty("RunNumbers").value
            _t_sample = self._load_runs(runs, "_t_sample")
            _t_sample = self._apply_corrections_vanadium(_t_sample)
            if self.getProperty("BackgroundRuns").value != "":
                _t_sample, _t_bkg = self._subtract_background(_t_sample)
                if self.getPropertyValue("OutputBackground") != "":
                    _t_bkg_angle = self._convert_to_angle(_t_bkg, "_t_bkg_angle")
                    self._output_workspace(_t_bkg_angle, "OutputBackground", suffix="_angle")
                    _t_bkg = self._convert_to_q(_t_bkg)
                    self._output_workspace(_t_bkg, "OutputBackground")
            _t_sample_angle = self._convert_to_angle(_t_sample, "_t_sample_angle")
            self._output_workspace(_t_sample_angle, "OutputWorkspace", suffix="_angle")
            _t_sample = self._convert_to_q(_t_sample)
            self._output_workspace(_t_sample, "OutputWorkspace")

    def _load_runs(self, runs, w_name):
        """
        Load all run event Nexus files into a single `EventWorkspace`

        Parameters
        ----------
        runs: str
            Run numbers to be reduced. Symbol `;` separates the runs into
            substrings. Each substring represents a set of runs to be
            reduced together
        w_name: str
            Name of output workspace

        Returns
        -------
        Mantid.EventsWorkspace
        """
        rl = self._run_list(runs)
        #
        # Load files together
        #
        _t_all_w = None
        _t_all_w_name = tws("aggregate_load_run")
        _t_w_name = tws("load_run")
        for run in rl:
            _t_w = self._load_single_run(run, _t_w_name)
            if _t_all_w is None:
                _t_all_w = CloneWorkspace(_t_w, OutputWorkspace=_t_all_w_name)
            else:
                _t_all_w = Plus(_t_all_w, _t_w, OutputWorkspace=_t_all_w_name)
        RenameWorkspace(_t_all_w, OutputWorkspace=w_name)
        return _t_all_w

    def _apply_corrections_vanadium(self, w, target="sample"):
        """
        Apply a series of corrections and normalizations to the input
        workspace, plus normalization by vanadium.

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventsWorkspace
        """

        w_corr_van = self._apply_corrections(w, target=target)
        if self.getProperty("VanadiumRuns").value != "":
            w_corr_van = self._sensitivity_correction(w_corr_van)
        return w_corr_van

    def _apply_corrections(self, w, target="sample"):
        """
        Apply a series of corrections and normalizations to the input
        workspace

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventsWorkspace
        """
        MaskDetectors(w, MaskedWorkspace=self._t_mask)
        local_name = tws("corr")
        _t_corr = ModeratorTzeroLinear(w, Gradient=self._tzero["gradient"], Intercept=self._tzero["intercept"], OutputWorkspace=local_name)
        # Correct old DAS shift of fast neutrons. See GitHub issue 23855
        if self._das_version == VDAS.v1900_2018:
            _t_corr = self.add_previous_pulse(_t_corr)

        _t_corr = ConvertUnits(_t_corr, Target="Wavelength", Emode="Elastic", OutputWorkspace=local_name)
        l_s, l_e = self._wavelength_band[0], self._wavelength_band[1]
        _t_corr = CropWorkspace(_t_corr, XMin=l_s, XMax=l_e, OutputWorkspace=local_name)
        _t_corr = Rebin(_t_corr, Params=[l_s, self._wavelength_dl, l_e], PreserveEvents=False, OutputWorkspace=local_name)

        if self.getProperty("DoFluxNormalization").value is True:
            _t_corr = self._flux_normalization(_t_corr, target)
        RenameWorkspace(_t_corr, OutputWorkspace=w.name())
        return _t_corr

    def _flux_normalization(self, w, target):
        """
        Divide data by integrated flux intensity

        Parameters
        ----------
        w: Mantid.EventsWorkspace
            Input workspace
        target: str
            Specify the entity the workspace refers to. Valid options are
            'sample', 'background', and 'vanadium'

        Returns
        -------
        Mantid.EventWorkspace
        """
        valid_targets = ("sample", "background", "vanadium")
        if target not in valid_targets:
            raise KeyError("Target must be one of " + ", ".join(valid_targets))
        w_nor = None
        if self._flux_normalization_type == "Monitor":
            _t_flux = None
            _t_flux_name = tws("monitor_aggregate")
            target_to_runs = dict(sample="RunNumbers", background="BackgroundRuns", vanadium="VanadiumRuns")
            rl = self._run_list(self.getProperty(target_to_runs[target]).value)

            _t_w_name = tws("monitor")
            for run in rl:
                run_name = "{0}_{1}".format(self._short_inst, str(run))
                _t_w = LoadNexusMonitors(run_name, OutputWorkspace=_t_w_name)
                if _t_flux is None:
                    _t_flux = CloneWorkspace(_t_w, OutputWorkspace=_t_flux_name)
                else:
                    _t_flux = Plus(_t_flux, _t_w, OutputWorkspace=_t_flux_name)

            _t_flux = ConvertUnits(_t_flux, Target="Wavelength", Emode="Elastic", OutputWorkspace=_t_flux_name)
            _t_flux = CropWorkspace(_t_flux, XMin=self._wavelength_band[0], XMax=self._wavelength_band[1], OutputWorkspace=_t_flux_name)
            _t_flux = OneMinusExponentialCor(_t_flux, C="0.20749999999999999", C1="0.001276", OutputWorkspace=_t_flux_name)
            _t_flux = Scale(_t_flux, Factor="1e-06", Operation="Multiply", OutputWorkspace=_t_flux_name)
            _t_flux = Integration(
                _t_flux, RangeLower=self._wavelength_band[0], RangeUpper=self._wavelength_band[1], OutputWorkspace=_t_flux_name
            )
            w_nor = Divide(w, _t_flux, OutputWorkspace=w.name())
        else:
            aggregate_flux = None
            if self._flux_normalization_type == "Proton Charge":
                aggregate_flux = w.getRun().getProtonCharge()
            elif self._flux_normalization_type == "Duration":
                aggregate_flux = w.getRun().getProperty("duration").value
            w_nor = Scale(w, Operation="Multiply", Factor=1.0 / aggregate_flux, OutputWorkspace=w.name())
        return w_nor

    def _load_vanadium_runs(self):
        """
        Initialize the vanadium workspace and the related mask to avoid using
        pixels with low-counts.
        """
        runs = self.getProperty("VanadiumRuns").value
        _t_van_name = tws("vanadium")
        _t_van = self._load_runs(runs, _t_van_name)
        _t_van = self._apply_corrections(_t_van, target="vanadium")
        _t_van = Integration(_t_van, RangeLower=self._wavelength_band[0], RangeUpper=self._wavelength_band[1], OutputWorkspace=_t_van_name)
        _t_v_mask_name = tws("vanadium_mask")
        output = MedianDetectorTest(_t_van, OutputWorkspace=_t_v_mask_name)
        self._v_mask = output.OutputWorkspace
        MaskDetectors(_t_van, MaskedWorkspace=self._v_mask)
        self._van = _t_van

    def _sensitivity_correction(self, w):
        """
        Divide each pixel by the vanadium count

        Parameters
        ----------
        w: Events workspace in units of wavelength
        Returns
        -------
        Mantid.EventWorkspace
        """
        MaskDetectors(w, MaskedWorkspace=self._v_mask)
        _t_w = Divide(w, self._van, OutputWorkspace=w.name())
        return _t_w

    def _subtract_background(self, w):
        """
        Subtract background from sample

        Parameters
        ----------
        w: Mantid.EventWorkspace
            Sample workspace from which to subtract the background

        Returns
        -------
        Mantid.EventWorkspace
        """
        runs = self.getProperty("BackgroundRuns").value
        _t_bkg = self._load_runs(runs, "_t_bkg")
        _t_bkg = self._apply_corrections_vanadium(_t_bkg, target="background")
        x = self.getProperty("BackgroundScale").value
        _t_w = w - x * _t_bkg
        RenameWorkspace(_t_w, OutputWorkspace=w.name())
        return _t_w, _t_bkg

    def _convert_to_q(self, w):
        """
        Convert to momentum transfer with the desired binning

        Parameters
        ----------
        w: Mantid.MatrixWorkspace2D

        Returns
        -------
        Mantid.MatrixWorkspace2D
        """
        _t_w_name = tws("convert_to_q")
        _t_w = ConvertUnits(w, Target="MomentumTransfer", Emode="Elastic", OutputWorkspace=_t_w_name)
        _t_w = Rebin(_t_w, Params=self._qbins, PreserveEvents=False, OutputWorkspace=_t_w_name)
        _t_w = SumSpectra(_t_w, OutputWorkspace=w.name())
        return _t_w

    def _convert_to_angle(self, w, name):
        """
        Output the integrated intensity for each elastic detector versus
        detector angle with the neutron beam.

        Masked elastic detectors are assigned a zero intensity

        Parameters
        ----------
        w: Mantid.MatrixWorkspace2D
        name: str
            Name of output workspace
        Returns
        -------
        Mantid.MatrixWorkspace2D
        """
        id_s, id_e = 16386, 17534  # start and end for elastic detector ID's
        _t_w_name = tws("convert_to_angle")
        _t_w = Integration(w, OutputWorkspace=_t_w_name)
        sp = _t_w.spectrumInfo()
        x, y, e = [list(), list(), list()]
        for i in range(_t_w.getNumberHistograms()):
            id_i = _t_w.getDetector(i).getID()
            if id_s <= id_i <= id_e:
                x.append(np.degrees(sp.twoTheta(i)))
                if sp.isMasked(i) is True:
                    y.append(0.0)
                    e.append(1.0)
                else:
                    y.append(_t_w.readY(i)[0])
                    e.append(_t_w.readE(i)[0])
        x = np.asarray(x)
        y = np.asarray(y)
        e = np.asarray(e)
        od = np.argsort(x)  # order in ascending angles
        title = "Angle between detector and incoming neutron beam"
        _t_w = CreateWorkspace(
            DataX=x[od], DataY=y[od], DataE=e[od], NSpec=1, UnitX="Degrees", WorkspaceTitle=title, OutputWorkspace=_t_w_name
        )
        RenameWorkspace(_t_w, OutputWorkspace=name)
        return _t_w

    def _output_workspace(self, w, prop, suffix=""):
        """
        Rename workspace and set the related output property

        Parameters
        ----------
        w: Mantid.MatrixWorkspace
        prop: str
            Output property name
        """
        w_name = self.getProperty(prop).valueAsStr + suffix
        RenameWorkspace(w, OutputWorkspace=w_name)
        self.setProperty(prop, w)

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
        _t_w = self._load_single_run(run, tws("cal_wav_band"))
        wavelength = np.mean(_t_w.getRun().getProperty("LambdaRequest").value)
        for reflection, band in self._wavelength_bands.items():
            if band[0] <= wavelength <= band[1]:
                self._wavelength_band = band
                break

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


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISPowderDiffraction)
