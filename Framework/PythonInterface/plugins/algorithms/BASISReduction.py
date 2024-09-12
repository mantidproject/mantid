# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Mantid Repository : https://github.com/mantidproject/mantid
# pylint: disable=no-init
import os
import re
import numpy as np
from collections import namedtuple
from contextlib import contextmanager
import json
import random
import string
import mantid.simpleapi as sapi
from mantid.api import mtd, PythonAlgorithm, AlgorithmFactory, FileProperty, FileAction, AnalysisDataService, ExperimentInfo
from mantid.kernel import IntArrayProperty, StringListValidator, FloatArrayProperty, EnabledWhenProperty, Direction, PropertyCriterion
from mantid import config as mantid_config
from os.path import join as pjoin

temp_prefix = "_ti_"  # marks a workspace as temporary


def unique_workspace_name(n=3, prefix="", suffix=""):
    r"""
    Create a random sequence of `n` lowercase characters that is guaranteed
    not to collide with the name of any existing Mantid workspace registered
    in the analysis data service.

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

    n_seq = "".join(random.choice(string.ascii_lowercase) for _ in range(n))
    ws_name = "{}{}{}".format(str(prefix), n_seq, str(suffix))
    while ws_name in AnalysisDataService.getObjectNames():
        characters = [random.choice(string.ascii_lowercase) for _ in range(n)]
        n_seq = "".join(characters)
        ws_name = "{}{}{}".format(str(prefix), n_seq, str(suffix))
    return ws_name


def tws(marker=""):
    r"""
    String starting with the temp_prefix
    and guaranteed not to collide
    with the name of any existing Mantid workspace
    in the analysis data service

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
        to_be_removed = set()
        for name in AnalysisDataService.getObjectNames():
            if temp_prefix in name:
                to_be_removed.add(name)
        for workspace in temps.workspaces:
            if isinstance(workspace, str) and AnalysisDataService.doesExist(workspace):
                to_be_removed.add(workspace)
            else:
                to_be_removed.add(workspace.name())
        for name in to_be_removed:
            sapi.DeleteWorkspace(name)


TEMPERATURE_SENSOR = "SensorA"
DEFAULT_MASK_GROUP_DIR = "/SNS/BSS/shared/autoreduce/new_masks_08_12_2015"
DEFAULT_CONFIG_DIR = mantid_config["instrumentDefinition.directory"]

# BASIS allows two possible reflections, with associated default properties
# pylint: disable=line-too-long
REFLECTIONS_DICT = {
    "silicon_111": {
        "name": "silicon_111",
        "energy_bins": [-150, 0.4, 500],  # micro-eV
        "q_bins": [0.3, 0.2, 1.9],  # inverse Angst
        "banks": "bank1,bank3,bank4",
        "mask_file": "BASIS_Mask_default_111.xml",
        "default_energy": 2.0826,  # meV
        "vanadium_bins": [-0.0034, 0.068, 0.0034],
        "vanadium_wav_range": [6.20, 6.33],
    },
    "silicon_333": {
        "name": "silicon_333",
        "energy_bins": [-1500, 3.2, 5000],
        "q_bins": [0.9, 0.2, 5.7],
        "banks": "bank1,bank3,bank4",
        "mask_file": "BASIS_Mask_default_333.xml",
        "default_energy": 18.7434,
        "vanadium_bins": [-0.0333, 0.0666, 0.0333],
        "vanadium_wav_range": [2.080, 2.108],
    },
    "silicon_311": {
        "name": "silicon_311",
        "energy_bins": [-740, 1.6, 740],
        "q_bins": [0.5, 0.2, 3.7],
        "banks": "bank2",
        "mask_file": "BASIS_Mask_default_311.xml",
        "default_energy": 7.6368,  # meV
        "vanadium_bins": [-0.015, 0.030, 0.015],
        "vanadium_wav_range": [3.263, 3.295],
    },
}

# pylint: disable=too-many-instance-attributes


class BASISReduction(PythonAlgorithm):
    _short_inst = None
    _long_inst = None
    _extension = None
    _doIndiv = None

    _flux_normalization_types = ["Monitor", "Proton Charge", "Duration"]
    _groupDetOpt = None
    _overrideMask = None
    _run_list = None  # a list of runs, or a list of sets of runs

    _samWsRun = None
    _samSqwWs = None

    def __init__(self):
        PythonAlgorithm.__init__(self)
        self._normalizeToFirst = False
        self._as_json = None
        self._temps = None  # hold names of temporary workspaces and files

        # properties related to the sample
        self._samWs = None  # name of the sample events file

        # properties related to flux normalization
        self._flux_normalization_type = None  # default to no flux normalizat.
        self._MonNorm = False  # flux normalization by monitor

        # properties related to the chosen reflection
        self._reflection = None  # entry in the reflections dictionary
        self._etBins = None
        self._qBins = None

        # properties related to the mask
        self._maskFile = None
        self._dMask = None  # list of masked detectors
        self._maskWs = None  # name of the mask workspace

        # properties related to division by Vanadium (normalization)
        self._doNorm = None  # stores the selected item from normalization_types
        self._normalizationType = None
        self._normRange = None
        self._norm_run_list = None
        self._normWs = None
        self._normMonWs = None
        self._normMask = None  # name of vanadium mask workspace

        # properties related to saving NSXPE file
        self._nsxpe_do = False
        self._nxspe_psi_angle_log = None
        self._nxspe_offset = 0.0

    def category(self):
        return "Inelastic\\Reduction"

    def name(self):
        return "BASISReduction"

    def version(self):
        return 1

    def summary(self):
        return "Multiple-file BASIS reduction for its two reflections."

    def PyInit(self):
        self._short_inst = "BSS"
        self._long_inst = "BASIS"
        self._extension = "_event.nxs"

        self.declareProperty("RunNumbers", "", "Sample run numbers")
        self.declareProperty("DoIndividual", False, "Do each run individually")
        self.declareProperty(
            "ExcludeTimeSegment",
            "",
            "Exclude a contigous time segment; "
            + 'Examples: "71546:0-60" filter run 71546 from '
            + 'start to 60 seconds, "71546:300-600", '
            + '"71546:120-end" from 120s to the end of the run',
        )
        help_doc = """Only retain events occurring within a time segment.
Examples: 71546:0-3600 only retains events from the first hour of run 71546.
71546:3600-7200 retain only the second hour. 71546:7200-end retain events after
the first two hours"""
        self.declareProperty("RetainTimeSegment", "", help_doc)
        grouping_type = ["None", "Low-Resolution", "By-Tube"]
        self.declareProperty("GroupDetectors", "None", StringListValidator(grouping_type), "Switch for grouping detectors")
        #
        #  Normalization selector
        #
        title_flux_normalization = "Flux Normalization"
        self.declareProperty("DoFluxNormalization", True, direction=Direction.Input, doc="Do we normalize data by incoming flux?")
        self.setPropertyGroup("DoFluxNormalization", title_flux_normalization)
        if_flux_normalization = EnabledWhenProperty("DoFluxNormalization", PropertyCriterion.IsDefault)
        default_flux_normalization = self._flux_normalization_types[0]
        self.declareProperty(
            "FluxNormalizationType",
            default_flux_normalization,
            StringListValidator(self._flux_normalization_types),
            "Flux Normalization Type",
        )
        self.setPropertySettings("FluxNormalizationType", if_flux_normalization)
        self.setPropertyGroup("FluxNormalizationType", title_flux_normalization)

        self.declareProperty("NormalizeToFirst", False, "Normalize spectra " + "to intensity of spectrum with lowest Q?")
        #
        # Properties affected by the reflection selected
        #
        titleReflection = "Reflection Selector"
        available_reflections = sorted(REFLECTIONS_DICT.keys())
        default_reflection = REFLECTIONS_DICT["silicon_111"]
        self.declareProperty(
            "ReflectionType",
            default_reflection["name"],
            StringListValidator(available_reflections),
            "Analyzer. Documentation lists typical \
                             associated property values.",
        )
        self.setPropertyGroup("ReflectionType", titleReflection)
        self.declareProperty(
            FloatArrayProperty("EnergyBins", default_reflection["energy_bins"], direction=Direction.Input),
            "Energy transfer binning scheme (in ueV)",
        )
        self.setPropertyGroup("EnergyBins", titleReflection)
        self.declareProperty(
            FloatArrayProperty("MomentumTransferBins", default_reflection["q_bins"], direction=Direction.Input),
            "Momentum transfer binning scheme",
        )
        self.setPropertyGroup("MomentumTransferBins", titleReflection)
        self.declareProperty(
            FileProperty(name="MaskFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".xml"]),
            "See documentation for latest mask files.",
        )
        self.setPropertyGroup("MaskFile", titleReflection)

        # Properties setting the division by vanadium
        titleDivideByVanadium = "Normalization by Vanadium"
        self.declareProperty("DivideByVanadium", False, direction=Direction.Input, doc="Do we normalize by the vanadium intensity?")
        self.setPropertyGroup("DivideByVanadium", titleDivideByVanadium)
        ifDivideByVanadium = EnabledWhenProperty("DivideByVanadium", PropertyCriterion.IsNotDefault)

        normalization_types = ["by Q slice", "by detector ID"]
        self.declareProperty("NormalizationType", "by Q slice", StringListValidator(normalization_types), "Select a Vanadium normalization")
        self.setPropertySettings("NormalizationType", ifDivideByVanadium)
        self.setPropertyGroup("NormalizationType", titleDivideByVanadium)

        self.declareProperty("NormRunNumbers", "", "Normalization run numbers")
        self.setPropertySettings("NormRunNumbers", ifDivideByVanadium)
        self.setPropertyGroup("NormRunNumbers", titleDivideByVanadium)

        # Properties setting the saving of NSXPE file
        title_nxspe = "Save to NXSPE"
        self.declareProperty("SaveNXSPE", False, direction=Direction.Input, doc="Do we save to NXSPE format?")
        nxspe_enabled = EnabledWhenProperty("SaveNXSPE", PropertyCriterion.IsNotDefault)
        self.setPropertyGroup("SaveNXSPE", title_nxspe)
        self.declareProperty(
            "PsiAngleLog",
            "SE50Rot",
            direction=Direction.Input,
            doc="name of entry in the logs storing the psi \
                             angle",
        )
        self.setPropertySettings("PsiAngleLog", nxspe_enabled)
        self.setPropertyGroup("PsiAngleLog", title_nxspe)
        self.declareProperty(
            "PsiOffset",
            0.0,
            direction=Direction.Input,
            doc="add this quantity to the psi angle stored \
                             in the log",
        )
        self.setPropertySettings("PsiOffset", nxspe_enabled)
        self.setPropertyGroup("PsiOffset", title_nxspe)

        # Aditional output properties
        titleAddionalOutput = "Additional Output"
        self.declareProperty("OutputSusceptibility", False, direction=Direction.Input, doc="Output dynamic susceptibility (Xqw)")
        self.setPropertyGroup("OutputSusceptibility", titleAddionalOutput)
        self.declareProperty("OutputPowderSpectrum", False, direction=Direction.Input, doc="Output S(Q) and S(theta) powder diffraction")
        self.setPropertyGroup("OutputPowderSpectrum", titleAddionalOutput)
        self.declareProperty("RemoveTemporaryWorkspaces", True, direction=Direction.Input, doc="Remove temporary workspaces and files")
        self.setPropertyGroup("RemoveTemporaryWorkspaces", titleAddionalOutput)

    # pylint: disable=too-many-branches
    def PyExec(self):
        # Facility and database configuration
        config_new_options = {"default.facility": "SNS", "default.instrument": "BASIS", "datasearch.searcharchive": "On"}
        #
        # implement with ContextDecorator after python2 is deprecated)
        #
        remove_temp = self.getProperty("RemoveTemporaryWorkspaces").value
        with pyexec_setup(remove_temp, config_new_options) as self._temps:
            self._PyExec()

    def _PyExec(self):
        # Collect Flux Normalization
        if self.getProperty("DoFluxNormalization").value is True:
            self._flux_normalization_type = self.getProperty("FluxNormalizationType").value
            if self._flux_normalization_type == "Monitor":
                self._MonNorm = True

        self._reflection = REFLECTIONS_DICT[self.getProperty("ReflectionType").value]
        self._doIndiv = self.getProperty("DoIndividual").value

        # micro-eV to mili-eV
        self._etBins = 1.0e-03 * self.getProperty("EnergyBins").value
        self._qBins = self.getProperty("MomentumTransferBins").value
        self._qBins[0] -= self._qBins[1] / 2.0  # leftmost bin boundary
        self._qBins[2] += self._qBins[1] / 2.0  # rightmost bin boundary

        self._maskFile = self.getProperty("MaskFile").value
        maskfile = self.getProperty("MaskFile").value
        self._maskFile = maskfile if maskfile else pjoin(DEFAULT_MASK_GROUP_DIR, self._reflection["mask_file"])

        self._groupDetOpt = self.getProperty("GroupDetectors").value
        self._normalizeToFirst = self.getProperty("NormalizeToFirst").value
        self._doNorm = self.getProperty("DivideByVanadium").value

        # retrieve properties pertaining to saving to NXSPE file
        self._nsxpe_do = self.getProperty("SaveNXSPE").value
        if self._nsxpe_do:
            self._nxspe_psi_angle_log = self.getProperty("PsiAngleLog").value
            self._nxspe_offset = self.getProperty("PsiOffset").value

        # Apply default mask if not supplied by user
        self._overrideMask = bool(self._maskFile)
        if not self._overrideMask:
            mantid_config.appendDataSearchDir(DEFAULT_MASK_GROUP_DIR)
            self._maskFile = self._reflection["mask_file"]

        self._maskWs = tws("BASIS_MASK")
        sapi.LoadMask(Instrument="BASIS", OutputWorkspace=self._maskWs, InputFile=self._maskFile)

        # Work around length issue
        _dMask = sapi.ExtractMask(InputWorkspace=self._maskWs, OutputWorkspace=tws("ExtractMask"))
        self._dMask = _dMask[1]

        #
        #  Process the Vanadium
        #
        norm_runs = self.getProperty("NormRunNumbers").value
        if self._doNorm and bool(norm_runs):
            self._normalizationType = self.getProperty("NormalizationType").value
            self.log().information("Divide by Vanadium with normalization" + self._normalizationType)

            # Following steps common to all types of Vanadium normalization

            # norm_runs encompasses a single set, thus _getRuns returns
            # a list of only one item
            norm_set = self._get_runs(norm_runs, doIndiv=False)[0]
            normWs = tws(self._make_run_name(norm_set[0]) + "_vanadium")
            self._sum_and_calibrate(norm_set, normWs)

            normRange = self._reflection["vanadium_wav_range"]
            bin_width = normRange[1] - normRange[0]
            # This rebin integrates counts onto a histogram of a single bin
            if self._normalizationType == "by detector ID":
                self._normRange = [normRange[0], bin_width, normRange[1]]
                sapi.Rebin(InputWorkspace=normWs, OutputWorkspace=normWs, Params=self._normRange)
                self._normWs = normWs
            # Detectors outside limits are substituted by MedianDetectorTest
            self._normMask = tws("BASIS_NORM_MASK")
            sapi.FindDetectorsOutsideLimits(
                InputWorkspace=normWs,
                LowThreshold=1.0 * bin_width,
                # no count events outside ranges
                RangeLower=normRange[0],
                RangeUpper=normRange[1],
                OutputWorkspace=self._normMask,
            )
            # additional reduction steps when normalizing by Q slice
            if self._normalizationType == "by Q slice":
                self._normWs = self._group_and_SofQW(normWs, normWs, self._etBins, isSample=False)
        #
        #  Process the sample
        #
        self._run_list = self._get_runs(self.getProperty("RunNumbers").value, doIndiv=self._doIndiv)
        for run_set in self._run_list:
            self._samWs = tws(self._make_run_name(run_set[0]))
            self._sum_and_calibrate(run_set, self._samWs)
            self._samWsRun = str(run_set[0])
            # Divide by Vanadium detector ID, if pertinent
            if self._normalizationType == "by detector ID":
                # Mask detectors with low Vanadium signal before dividing
                sapi.MaskDetectors(Workspace=self._samWs, MaskedWorkspace=self._normMask)
                sapi.Divide(LHSWorkspace=self._samWs, RHSWorkspace=self._normWs, OutputWorkspace=self._samWs)
            # additional reduction steps
            prefix = self._make_run_name(run_set[0])
            self._samSqwWs = self._group_and_SofQW(self._samWs, prefix, self._etBins, isSample=True)
            # Divide by Vanadium Q slice, if pertinent
            if self._normalizationType == "by Q slice":
                sapi.Divide(LHSWorkspace=self._samSqwWs, RHSWorkspace=self._normWs, OutputWorkspace=self._samSqwWs)
            # Clear mask from reduced file. Needed for binary operations
            # involving this S(Q,w)
            sapi.ClearMaskFlag(Workspace=self._samSqwWs)
            # Scale so that elastic line has Y-values ~ 1
            if self._normalizeToFirst:
                self._ScaleY(self._samSqwWs)

            # Transform the vertical axis (Q) to point data
            # Q-values are in X-axis now
            sapi.Transpose(InputWorkspace=self._samSqwWs, OutputWorkspace=self._samSqwWs)
            # from histo to point
            sapi.ConvertToPointData(InputWorkspace=self._samSqwWs, OutputWorkspace=self._samSqwWs)
            # Q-values back to vertical axis
            sapi.Transpose(InputWorkspace=self._samSqwWs, OutputWorkspace=self._samSqwWs)
            self.serialize_in_log(self._samSqwWs)  # store the call
            # Output Dave and Nexus files
            extension = "_divided.dat" if self._doNorm else ".dat"
            dave_grp_filename = self._make_run_name(self._samWsRun, False) + extension
            sapi.SaveDaveGrp(Filename=dave_grp_filename, InputWorkspace=self._samSqwWs, ToMicroEV=True)
            extension = "_divided_sqw.nxs" if self._doNorm else "_sqw.nxs"
            processed_filename = self._make_run_name(self._samWsRun, False) + extension
            sapi.SaveNexus(Filename=processed_filename, InputWorkspace=self._samSqwWs)

            # additional output
            if self.getProperty("OutputSusceptibility").value is True:
                temperature = mtd[self._samSqwWs].getRun().getProperty(TEMPERATURE_SENSOR).getStatistics().mean
                samXqsWs = self._samSqwWs.replace("sqw", "Xqw")
                sapi.ApplyDetailedBalance(InputWorkspace=self._samSqwWs, OutputWorkspace=samXqsWs, Temperature=str(temperature))
                sapi.ConvertUnits(
                    InputWorkspace=samXqsWs,
                    OutputWorkspace=samXqsWs,
                    Target="DeltaE_inFrequency",
                    Emode="Indirect",
                    Efixed=self._reflection["default_energy"],
                )
                self.serialize_in_log(samXqsWs)
                susceptibility_filename = processed_filename.replace("sqw", "Xqw")
                sapi.SaveNexus(Filename=susceptibility_filename, InputWorkspace=samXqsWs)
            if self.getProperty("OutputPowderSpectrum").value:
                self.generatePowderSpectrum()

    def _get_runs(self, rlist, doIndiv=True):
        r"""
        Create sets of run numbers for analysis. A semicolon indicates a
        separate group of runs to be processed together.

        Parameters
        ----------
        rlist: str
            All the run numbers to be reduced.
        doIndiv: bool
            Return each run on its own list

        Returns
        -------
        list
            Items of this list are lists. If `doIndiv` is True, each item
            list is made up of a single run number. If `doIndiv` is False,
            each item list is made up of several run numbers which are to
            be reduced together.
        """
        run_list = []
        # ';' separates the runs into substrings. Each substring
        #  represents a set of runs
        rlvals = rlist.split(";")
        for rlval in rlvals:
            iap = IntArrayProperty("_get_runs_iap", rlval)  # substring split
            if doIndiv:
                run_list.extend([[str(x)] for x in iap.value])
            else:
                run_list.append([str(x) for x in iap.value])
        return run_list

    def _make_run_name(self, run, useShort=True):
        r"""
        Make name like BSS_24234, for instance

        Parameters
        ----------
        run: str
            Run number
        useShort: bool
            Whether to use 'BSS' or 'BASIS'

        Returns
        -------
        str
            Identifier of instrument and run numbers
        """
        if useShort:
            return self._short_inst + "_" + str(run)
        else:
            return self._long_inst + "_" + str(run)

    def _make_run_file(self, run):
        r"""
        Make name like BSS_24234_event.nxs

        Parameters
        ----------
        run: str
            Run number

        Returns
        -------
        str
            events file name
        """
        return "{0}_{1}_event.nxs".format(self._short_inst, str(run))

    def _sum_runs(self, run_set, sam_ws):
        r"""
        Aggregate the set of runs

        Parameters
        ----------
        run_set: list
            Run numbers
        sam_ws:  str
            Name of aggregate workspace for the sample
        extra_ext: str
            Suffix to be added to the temporary workspaces
        """
        self.load_single_run(run_set[0], sam_ws)
        for run in run_set[1:]:
            ws_name = tws("sum_runs_" + run)
            self.load_single_run(run, ws_name)
            sapi.Plus(LHSWorkspace=sam_ws, RHSWorkspace=ws_name, OutputWorkspace=sam_ws)

    def load_single_run(self, run, name):
        r"""
        Find and load events.

        Applies event filtering if necessary.

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

        kwargs = dict(Filename=self._make_run_file(run), BankName=self._reflection["banks"], OutputWorkspace=name)
        if str(run) + ":" in self.getProperty("RetainTimeSegment").value:
            kwargs.update(self._retainEvents(run))
        sapi.LoadEventNexus(**kwargs)
        if str(run) + ":" in self.getProperty("ExcludeTimeSegment").value:
            self._filterEvents(run, name)

    def _sum_monitors(self, run_set, mon_ws):
        r"""
        Generate aggregate monitor workspace from a list of run numbers

        Parameters
        ----------
        run_set: list
            List of run numbers
        mon_ws: str
            Name of output workspace
        """
        sapi.LoadNexusMonitors(Filename=self._make_run_file(run_set[0]), OutputWorkspace=mon_ws)
        for run in run_set[1:]:
            ws_name = tws("sum_monitors_" + run)
            sapi.LoadNexusMonitors(Filename=self._make_run_file(run), OutputWorkspace=ws_name)
            sapi.Plus(LHSWorkspace=mon_ws, RHSWorkspace=ws_name, OutputWorkspace=mon_ws)

    def _generate_flux_spectrum(self, run_set, sam_ws):
        r"""
        Retrieve the aggregate flux and create an spectrum of intensities
        versus wavelength such that intensities will be similar for any
        of the possible flux normalization types.

        Parameters
        ----------
        sam_ws: str
            Name of aggregated sample workspace

        Returns
        -------
        str
            Name of aggregated flux workspace (output workspace)
        """

        flux_binning = [1.5, 0.0005, 7.5]  # wavelength binning
        suffix = re.sub("[^0-9a-zA-Z]+", "_", self._flux_normalization_type)
        flux_ws = tws(self._make_run_name(run_set[0]) + "_" + suffix)
        if self._MonNorm:
            self._sum_monitors(run_set, flux_ws)
            rpf = self._elucidate_reflection_parameter_file(sam_ws)
            sapi.LoadParameterFile(Workspace=flux_ws, Filename=rpf)
            sapi.ModeratorTzeroLinear(InputWorkspace=flux_ws, OutputWorkspace=flux_ws)
            sapi.Rebin(InputWorkspace=flux_ws, OutputWorkspace=flux_ws, Params="10", PreserveEvents=False)  # 10 microseconds TOF bin width
            sapi.ConvertUnits(InputWorkspace=flux_ws, OutputWorkspace=flux_ws, Target="Wavelength")
            sapi.OneMinusExponentialCor(InputWorkspace=flux_ws, OutputWorkspace=flux_ws, C="0.20749999999999999", C1="0.001276")
            sapi.Scale(InputWorkspace=flux_ws, OutputWorkspace=flux_ws, Factor="1e-06")
            sapi.Rebin(InputWorkspace=flux_ws, OutputWorkspace=flux_ws, Params=flux_binning)
        else:
            ws = mtd[sam_ws].getRun()
            if self._flux_normalization_type == "Proton Charge":
                aggregate_flux = ws.getProtonCharge()
            elif self._flux_normalization_type == "Duration":
                aggregate_flux = ws.getProperty("duration").value
            # These factors ensure intensities typical of flux workspaces
            # derived from monitor data
            f = {"Proton Charge": 0.00874, "Duration": 0.003333}
            x = np.arange(flux_binning[0], flux_binning[2], flux_binning[1])
            y = f[self._flux_normalization_type] * aggregate_flux * np.ones(len(x) - 1)
            _flux_ws = sapi.CreateWorkspace(OutputWorkspace=flux_ws, DataX=x, DataY=y, UnitX="Wavelength")
            _flux_ws.setYUnit(mtd[sam_ws].YUnit())
        return flux_ws

    def _calibrate_data(self, run_set, sam_ws):
        sapi.MaskDetectors(Workspace=sam_ws, DetectorList=self._dMask)
        rpf = self._elucidate_reflection_parameter_file(sam_ws)
        sapi.LoadParameterFile(Workspace=sam_ws, Filename=rpf)
        sapi.ModeratorTzeroLinear(InputWorkspace=sam_ws, OutputWorkspace=sam_ws)
        sapi.ConvertUnits(
            InputWorkspace=sam_ws, OutputWorkspace=sam_ws, Target="Wavelength", EMode="Indirect", EFixed=self._reflection["default_energy"]
        )
        if self._flux_normalization_type is not None:
            flux_ws = self._generate_flux_spectrum(run_set, sam_ws)
            sapi.RebinToWorkspace(WorkspaceToRebin=sam_ws, WorkspaceToMatch=flux_ws, OutputWorkspace=sam_ws)
            sapi.Divide(LHSWorkspace=sam_ws, RHSWorkspace=flux_ws, OutputWorkspace=sam_ws)

    def _sum_and_calibrate(self, run_set, wsName):
        """
        Aggregate the set of runs and calibrate

        Parameters
        ----------
        run_set: list
            Run numbers
        extra_extension: str
            Suffix to be added to the workspace names

        Returns
        -------
        str
            workspace name of the aggregated and calibrated data
        """
        self._sum_runs(run_set, wsName)
        self._calibrate_data(run_set, wsName)

    def _group_and_SofQW(self, wsName, prefix, etRebins, isSample=True):
        r"""
        Transforms from wavelength and detector ID to S(Q,E)

        Parameters
        ----------
        wsName: str
            Name of a workspace as a function of wavelength and detector id
        prefix: str
            Name prefix for output workspaces and files
        etRebins: list
            Final energy domain and bin width
        isSample: bool
            Discriminates between sample and vanadium

        Returns
        -------
        str
            Name of S(Q,E) workspace
        """
        sapi.ConvertUnits(
            InputWorkspace=wsName, OutputWorkspace=wsName, Target="DeltaE", EMode="Indirect", EFixed=self._reflection["default_energy"]
        )
        sapi.CorrectKiKf(InputWorkspace=wsName, OutputWorkspace=wsName, EMode="Indirect", EFixed=self._reflection["default_energy"])
        sapi.Rebin(InputWorkspace=wsName, OutputWorkspace=wsName, Params=etRebins)
        if self._groupDetOpt != "None":
            if self._groupDetOpt == "Low-Resolution":
                grp_file = "BASIS_Grouping_LR.xml"
            else:
                grp_file = "BASIS_Grouping.xml"
            # If mask override used, we need to add default grouping file
            # location to search paths
            if self._overrideMask:
                mantid_config.appendDataSearchDir(DEFAULT_MASK_GROUP_DIR)
                sapi.GroupDetectors(InputWorkspace=wsName, OutputWorkspace=wsName, MapFile=grp_file, Behaviour="Sum")

        # Output NXSPE file (must be done before transforming the
        # vertical axis to point data)
        if isSample and self._nsxpe_do:
            extension = ".nxspe"
            run = mtd[wsName].getRun()
            if run.hasProperty(self._nxspe_psi_angle_log):
                psi_angle_logproperty = run.getProperty(self._nxspe_psi_angle_log)
                psi_angle = np.average(psi_angle_logproperty.value)
                psi_angle += self._nxspe_offset
                nxspe_filename = prefix + extension
                sapi.SaveNXSPE(
                    InputWorkspace=wsName,
                    Filename=nxspe_filename,
                    Efixed=self._reflection["default_energy"],
                    Psi=psi_angle,
                    KiOverKfScaling=1,
                )
            else:
                error_message = "Runs have no log entry named {}".format(self._nxspe_psi_angle_log)
                self.log().error(error_message)

        wsSqwName = prefix if isSample is True else wsName
        wsSqwName += "_divided_sqw" if self._doNorm is True else "_sqw"
        sapi.SofQW3(
            InputWorkspace=wsName,
            QAxisBinning=self._qBins,
            EMode="Indirect",
            EFixed=self._reflection["default_energy"],
            OutputWorkspace=wsSqwName,
        )
        # Rebin the vanadium within the elastic line
        if not isSample:
            sapi.Rebin(InputWorkspace=wsSqwName, OutputWorkspace=wsSqwName, Params=self._reflection["vanadium_bins"])
        return wsSqwName

    def _ScaleY(self, wsName):
        r"""
        Scale all spectra by a number so that the maximum of the first spectra
        is rescaled to 1

        Parameters
        ----------
        wsName: str
            Name of the workspace to rescale
        """
        workspace = sapi.mtd[wsName]
        maximumYvalue = workspace.dataY(0).max()
        sapi.Scale(InputWorkspace=wsName, OutputWorkspace=wsName, Factor=1.0 / maximumYvalue, Operation="Multiply")

    def generateSplitterWorkspace(self, fragment):
        r"""
        Create a table workspace with time intervals to keep

        Parameters
        ----------
        fragment: str
            a-b  start and end of time fragment to filter out
        """
        inf = 172800  # a run two full days long
        a, b = fragment.split("-")
        b = inf if "end" in b else float(b)
        a = float(a)
        splitter = sapi.CreateEmptyTableWorkspace(OutputWorkspace=tws("splitter"))
        splitter.addColumn("double", "start")
        splitter.addColumn("double", "stop")
        splitter.addColumn("str", "target")
        if a == 0.0:
            splitter.addRow([b, inf, "0"])
        elif b == inf:
            splitter.addRow([0, a, "0"])
        else:
            splitter.addRow([0, a, "0"])
            splitter.addRow([b, inf, "0"])
        self._temps.extend("splitted_unfiltered", "TOFCorrectWS")

    def _filterEvents(self, run, ws_name):
        r"""
        Filter out ExcludeTimeSegment if applicable

        Parameters
        ----------
        run: str
            run number
        ws_name : str
            name of the workspace to filter
        """
        for run_fragment in self.getProperty("ExcludeTimeSegment").value.split(","):
            if run + ":" in run_fragment:
                self.generateSplitterWorkspace(run_fragment.split(":")[1])
                sapi.FilterEvents(
                    InputWorkspace=ws_name,
                    SplitterWorkspace="splitter",
                    OutputWorkspaceBaseName="splitted",
                    GroupWorkspaces=True,
                    OutputWorkspaceIndexedFrom1=True,
                    RelativeTime=True,
                )
                sapi.UnGroupWorkspace("splitted")
                sapi.RenameWorkspace(InputWorkspace="splitted_0", OutputWorkspace=ws_name)
                break

    def _retainEvents(self, run):
        r"""
        Retain only events in a time segment

        Parameters
        ----------
        run: str
            run number
        ws_name : str
            name of the workspace to filter
        """
        inf = 172800  # a run two full days long
        for run_fragment in self.getProperty("RetainTimeSegment").value.split(","):
            if str(run) + ":" in run_fragment:
                a, b = run_fragment.split(":")[1].split("-")
                b = inf if "end" in b else float(b)
                return dict(FilterByTimeStart=float(a), FilterByTimeStop=float(b))
        raise RuntimeError("Run {} not in RetainTimeSegment ".format(run))

    def serialize_in_log(self, ws_name):
        r"""Save the serialization of the algorithm in the logs.

        Parameters
        ----------
        ws_name: str
            Name of the workspace from which to retrieve and modify the logs
        """

        def jsonify(value):
            r"""Cast non-standard objects to their closest standard
            representation to enable JSON serialiation"""
            if isinstance(value, np.ndarray):
                return value.tolist()
            return value

        if self._as_json is None:
            self._as_json = json.loads(str(self))
            # Force serialization of the following properties even if having
            # their default values
            forced = {
                name: jsonify(self.getProperty(name).value)
                for name in (
                    "DoIndividual",
                    "DoFluxNormalization",
                    "FluxNormalizationType",
                    "NormalizeToFirst",
                    "ReflectionType",
                    "EnergyBins",
                    "MomentumTransferBins",
                    "MaskFile",
                    "DivideByVanadium",
                )
            }
            self._as_json["properties"].update(forced)
        r = mtd[ws_name].mutableRun()
        r.addProperty("asString", json.dumps(self._as_json), True)

    def _elucidate_reflection_parameter_file(self, ws_name):
        r"""
        Search a parameter file for the selected reflection and with
        valid-from, valid-to dates framing the starting date for the run(s)

        Parameters
        ----------
        ws_name: str
            Name of the workspace from which to retrieve the starting date

        Returns
        -------
        str
            Full path to the parameter file
        """
        prefix = "BASIS_{}_Parameters".format(self._reflection["name"])
        instr_directories = [DEFAULT_CONFIG_DIR]
        start_date = str(mtd[ws_name].getRun().startTime()).strip()
        parm_files = ExperimentInfo.getResourceFilenames(prefix, ["xml"], instr_directories, start_date)
        parm_file = parm_files[0]  # first in the list is the most appropriate
        # store selected parameter file in the logs
        mtd[ws_name].getRun().addProperty("reflParmFile", os.path.basename(parm_file), True)
        return parm_file

    def generatePowderSpectrum(self):
        r"""Call BASISPowderReduction to generate a powder spectrum"""
        powder_reducer = self.createChildAlgorithm("BASISPowderDiffraction", enableLogging=False)
        for prop in ("RunNumbers", "DoFluxNormalization", "FluxNormalizationType"):
            powder_reducer.setProperty(prop, self.getProperty(prop).value)
        # This may go later into a input property
        powder_reducer.setProperty("MomentumTransferBins", [0.1, 0.02, 4.0])

        vanadium_runs = self.getProperty("NormRunNumbers").value
        if bool(vanadium_runs):
            powder_reducer.setProperty("VanadiumRuns", vanadium_runs)
            suffix = "_divided_sq"
        else:
            suffix = "_sq"

        first_run = re.compile("^([0-9]+)").search(self.getProperty("RunNumbers").value).group(0)
        powder_reducer.setProperty("OutputWorkspace", self._make_run_name(first_run) + suffix)
        powder_reducer.execute()


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISReduction)
