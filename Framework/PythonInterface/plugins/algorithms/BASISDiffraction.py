# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-branches

import os
import tempfile
import itertools
from collections import namedtuple
from contextlib import contextmanager
import numpy as np

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
)
from mantid.kernel import Direction, EnabledWhenProperty, PropertyCriterion, IntArrayProperty, FloatArrayProperty, FloatArrayLengthValidator

DEPRECATION_NOTICE = """BASISDiffraction is deprecated (on 2018-08-27).
Instead, use BASISCrystalDiffraction or BASISPowderReduction."""

_SOLID_ANGLE_WS_ = "/tmp/solid_angle_diff.nxs"
_FLUX_WS_ = "/tmp/int_flux.nxs"


@contextmanager
def pyexec_setup(new_options):
    """
    Backup keys of mantid.config and clean up temporary files and workspaces
    upon algorithm completion or exception raised.
    :param new_options: dictionary of mantid configuration options
     to be modified.
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


class BASISDiffraction(DataProcessorAlgorithm):
    _mask_file = "/SNS/BSS/shared/autoreduce/new_masks_08_12_2015/BASIS_Mask_default_diff.xml"
    _solid_angle_ws_ = "/SNS/BSS/shared/autoreduce/solid_angle_diff.nxs"
    _flux_ws_ = "/SNS/BSS/shared/autoreduce/int_flux.nxs"

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)
        self._lambda_range = [5.86, 6.75]  # units of inverse Angstroms
        self._short_inst = "BSS"
        self._long_inst = "BASIS"
        self._run_list = None
        self._temps = None
        self._bkg = None
        self._bkg_scale = None
        self._vanadium_files = None
        self._momentum_range = None
        self._t_mask = None
        self._n_bins = None

    @classmethod
    def category(self):
        return "Diffraction\\Reduction"

    @classmethod
    def version(self):
        return 1

    @classmethod
    def summary(self):
        return DEPRECATION_NOTICE

    def seeAlso(self):
        return ["AlignDetectors", "DiffractionFocussing", "SNSPowderReduction"]

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
            FloatArrayProperty("LambdaRange", self._lambda_range, direction=Direction.Input), doc="Incoming neutron wavelength range"
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
        self.declareProperty("SingleCrystalDiffraction", False, direction=Direction.Input, doc="Calculate diffraction pattern?")
        crystal_diffraction_enabled = EnabledWhenProperty("SingleCrystalDiffraction", PropertyCriterion.IsNotDefault)
        self.declareProperty(
            "PsiAngleLog", "SE50Rot", direction=Direction.Input, doc="log entry storing rotation of the sample around the vertical axis"
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
            doc="three item, comma-separated, HKL indices of the diffracting plane",
        )
        #    Reciprocal vector orthogonal to VectorU and in-plane with
        #    incoming beam
        self.declareProperty(
            FloatArrayProperty("VectorV", [0, 1, 0], array_length_three, direction=Direction.Input),
            doc="three item, comma-separated, HKL indices of the direction perpendicular to VectorVand the vertical axis",
        )
        #    Abscissa view
        self.declareProperty(
            FloatArrayProperty("Uproj", [1, 0, 0], array_length_three, direction=Direction.Input),
            doc="three item comma-separated Abscissa view of the diffraction pattern",
        )
        #    Ordinate view
        self.declareProperty(
            FloatArrayProperty("Vproj", [0, 1, 0], array_length_three, direction=Direction.Input),
            doc="three item comma-separated Ordinate view of the diffraction pattern",
        )
        #    Hidden axis
        self.declareProperty(FloatArrayProperty("Wproj", [0, 0, 1], array_length_three, direction=Direction.Input), doc="Hidden axis view")
        #    Binnin in reciprocal slice
        self.declareProperty("NBins", 400, direction=Direction.Input, doc="number of bins in the HKL slice")

        self.setPropertyGroup("SingleCrystalDiffraction", crystal_diffraction_title)
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
            self.setPropertySettings(a_property, crystal_diffraction_enabled)

    def PyExec(self):
        # Exit with deprecation notice
        self.log().error(DEPRECATION_NOTICE)

        # Facility and database configuration
        config_new_options = {"default.facility": "SNS", "default.instrument": "BASIS", "datasearch.searcharchive": "On"}

        # Find valid incoming momentum range
        self._lambda_range = np.array(self.getProperty("LambdaRange").value)
        self._momentum_range = np.sort(2 * np.pi / self._lambda_range)

        with pyexec_setup(config_new_options) as self._temps:
            # Load the mask to a workspace
            self._t_mask = LoadMask(Instrument="BASIS", InputFile=self.getProperty("MaskFile").value, OutputWorkspace="_t_mask")

            # Pre-process the background runs
            if self.getProperty("BackgroundRuns").value:
                bkg_run_numbers = self._getRuns(self.getProperty("BackgroundRuns").value, doIndiv=True)
                bkg_run_numbers = list(itertools.chain.from_iterable(bkg_run_numbers))
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

            # Pre-process the vanadium run(s)
            if self.getProperty("VanadiumRuns").value:
                run_numbers = self._getRuns(self.getProperty("VanadiumRuns").value, doIndiv=True)
                run_numbers = list(itertools.chain.from_iterable(run_numbers))
                vanadium_reporter = Progress(self, start=0.0, end=1.0, nreports=len(run_numbers))
                self._vanadium_files = list()
                for i, run in enumerate(run_numbers):
                    self._vanadium_files.append(self._save_t0(run))
                    message = "Pre-processing vanadium: {} of {}".format(i + 1, len(run_numbers))
                    vanadium_reporter.report(message)
                vanadium_reporter.report(len(run_numbers), "Done")

            # Determination of single crystal diffraction
            if self.getProperty("SingleCrystalDiffraction").value:
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
        cmd_args = dict(
            QDimensions="Q3D", dEAnalysisMode="Elastic", Q3DFrames="HKL", QConversionScales="HKL", Uproj=uproj, Vproj=vproj, Wproj=wproj
        )
        mdn_args = None  # Options for algorithm MDNormSCD

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
        run_numbers = self._getRuns(self.getProperty("RunNumbers").value, doIndiv=True)
        run_numbers = list(itertools.chain.from_iterable(run_numbers))
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
                cmd_args.update({"MinValues": min_values, "MaxValues": max_values})

            # Convert to MD
            _t_md = ConvertToMD(_t_sample, OutputWorkspace="_t_md", **cmd_args)
            if self._bkg:
                _t_bkg_md = ConvertToMD(self._bkg, OutputWorkspace="_t_bkg_md", **cmd_args)

            # Determine aligned dimensions. Need to be done only once
            if mdn_args is None:
                aligned = list()
                for i_dim in range(3):
                    kwargs = {
                        "name": _t_md.getDimension(i_dim).name,
                        "min": min_values[i_dim],
                        "max": max_values[i_dim],
                        "n_bins": self._n_bins[i_dim],
                    }
                    aligned.append("{name},{min},{max},{n_bins}".format(**kwargs))
                mdn_args = dict(
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
                **mdn_args,
            )
            if self._bkg:
                MDNormSCD(
                    _t_bkg_md,
                    OutputWorkspace="_t_bkg_data",
                    OutputNormalizationWorkspace="_t_bkg_norm",
                    TemporaryDataWorkspace="_t_bkg_data" if mtd.doesExist("_t_bkg_data") else None,
                    TemporaryNormalizationWorkspace="_t_bkg_norm" if mtd.doesExist("_t_bkg_norm") else None,
                    **mdn_args,
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
        ws = LoadEventNexus(Filename=self._makeRunFile(run_number), NXentryName="entry-diff", OutputWorkspace=name)
        ws = ModeratorTzeroLinear(InputWorkspace=ws.name(), OutputWorkspace=ws.name())
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
        ws = LoadEventNexus(
            Filename=self._makeRunFile(run_number), NXentryName="entry-diff", SingleBankPixelsOnly=False, OutputWorkspace=name
        )
        MaskDetectors(ws, MaskedWorkspace=self._t_mask)
        ws = ModeratorTzeroLinear(InputWorkspace=ws.name(), OutputWorkspace=ws.name())
        ws = ConvertUnits(ws, Target="Momentum", OutputWorkspace=ws.name())
        ws = CropWorkspace(ws, OutputWorkspace=ws.name(), XMin=self._momentum_range[0], XMax=self._momentum_range[1])
        return ws

    def _getRuns(self, rlist, doIndiv=True):
        """
        Create sets of run numbers for analysis. A semicolon indicates a
        separate group of runs to be processed together.
        :param rlist: string containing all the run numbers to be reduced.
        :return: if doIndiv is False, return a list of IntArrayProperty objects.
         Each item is a pseudolist containing a set of runs to be reduced together.
         if doIndiv is True, return a list of strings, each string is a run number.
        """
        run_list = []
        # ";" separates the runs into substrings. Each substring represents a set of runs
        rlvals = rlist.split(";")
        for rlval in rlvals:
            iap = IntArrayProperty("", rlval)  # split the substring
            if doIndiv:
                run_list.extend([[x] for x in iap.value])
            else:
                run_list.append(iap.value)
        return run_list

    def _makeRunFile(self, run):
        """
        Make name like BSS_24234_event.nxs
        """
        return "{0}_{1}_event.nxs".format(self._short_inst, str(run))

    def _spawn_tempnexus(self):
        """
        Create a temporary file and flag for removal upon algorithm completion.
        :return: (str) absolute path to the temporary file.
        """
        f = tempfile.NamedTemporaryFile(prefix="BASISDiffraction_", suffix=".nxs", dir=mantid_config["defaultsave.directory"], delete=False)
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


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(BASISDiffraction)
