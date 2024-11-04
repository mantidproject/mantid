# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from SANSILLCommon import (
    AcqMode,
    add_correction_numors,
    create_name,
    needs_loading,
    needs_processing,
)
from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    WorkspaceGroupProperty,
    MultipleFileProperty,
    FileAction,
    WorkspaceGroup,
    TextAxis,
    Progress,
    FileFinder,
)
from mantid.kernel import (
    Direction,
    FloatBoundedValidator,
    FloatArrayProperty,
    StringArrayProperty,
    IntArrayProperty,
    IntBoundedValidator,
    StringListValidator,
)
from mantid.simpleapi import (
    CalculateDynamicRange,
    CalculateEfficiency,
    ConvertToPointData,
    GroupWorkspaces,
    LoadNexusProcessed,
    MaskDetectorsIf,
    RenameWorkspace,
    SANSILLIntegration,
    SANSILLReduction,
    Stitch,
    Transpose,
    UnGroupWorkspace,
)


N_DISTANCES = 5  # maximum number of distinct distance configurations
N_LAMBDAS = 2  # maximum number of distinct wavelengths used in the experiment


class SANSILLMultiProcess(DataProcessorAlgorithm):
    """
    Performs an entire experiment processing from ILL SANS beamlines (D11, D22, D33 but NOT D16).
    Supports standard monochromatic, kinetic monochromatic, rebinned event (mono) and TOF SANS.
    Provides azimuthal integration - I(Q), optionally with sectors/wedges and panel separation.
    Reduces all the samples at all the distances together in the most optimal way.
    Allows for up to 2 wavelengths, and up to 5 detector distances.
    """

    instrument = None  # the name of the instrument [D11, D11B, D22, D22B, D33]
    rank = None  # the rank of the reduction, i.e. the number of (detector distance, wavelength) configurations
    lambda_rank = None  # how many wavelengths are we dealing with, i.e. how many transmissions need to be calculated
    n_samples = None  # how many samples are being reduced
    name_axis = None  # TextAxis holding the sample names
    progress = None  # the global progress object
    n_reports = None  # the number of progress reports

    def category(self):
        return "ILL\\SANS;ILL\\Auto"

    def summary(self):
        return "Performs SANS data reduction of the entire experiment."

    def seeAlso(self):
        return ["SANSILLReduction", "SANSILLIntegration"]

    def name(self):
        return "SANSILLMultiProcess"

    def PyInit(self):
        # ================================INPUT RUNS================================#

        for d in range(N_DISTANCES):
            p_name = f"SampleRunsD{d+1}"
            self.declareProperty(
                MultipleFileProperty(name=p_name, action=FileAction.OptionalLoad, extensions=["nxs"], allow_empty=True),
                doc=f"Sample run(s) at the distance #{d+1}.",
            )
            self.setPropertyGroup(p_name, "Numors")

        self.declareProperty(
            MultipleFileProperty(name="DarkCurrentRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Absorber (Cd/B4C) run(s).",
        )
        self.setPropertyGroup("DarkCurrentRuns", "Numors")

        self.declareProperty(
            MultipleFileProperty(name="EmptyBeamRuns", action=FileAction.OptionalLoad, extensions=["nxs"]), doc="Empty beam run(s)."
        )
        self.setPropertyGroup("EmptyBeamRuns", "Numors")

        self.declareProperty(
            MultipleFileProperty(name="FluxRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Empty beam run(s) for flux calculation only; " "if left blank the flux will be calculated from EmptyBeamRuns.",
        )
        self.setPropertyGroup("FluxRuns", "Numors")

        self.declareProperty(
            MultipleFileProperty(name="EmptyContainerRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Empty container run(s).",
        )
        self.setPropertyGroup("EmptyContainerRuns", "Numors")

        # ================================TR INPUT RUNS================================#

        for l in range(N_LAMBDAS):
            p_name = f"SampleTrRunsW{l+1}"
            self.declareProperty(
                MultipleFileProperty(name=p_name, action=FileAction.OptionalLoad, extensions=["nxs"], allow_empty=True),
                doc=f"Sample transmission run(s) at the wavelength #{l+1}.",
            )
            self.setPropertyGroup(p_name, "Transmission Numors")

        self.declareProperty(
            MultipleFileProperty(name="TrDarkCurrentRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Absorber (Cd/B4C) run(s) for transmission calculation.",
        )

        self.declareProperty(
            MultipleFileProperty(name="ContainerTrRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Container transmission run(s).",
        )
        self.setPropertyGroup("ContainerTrRuns", "Transmission Numors")

        self.declareProperty(
            MultipleFileProperty(name="TrEmptyBeamRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Empty beam run(s) for transmission calculation.",
        )

        self.setPropertyGroup("TrDarkCurrentRuns", "Transmission Numors")
        self.setPropertyGroup("ContainerTrRuns", "Transmission Numors")
        self.setPropertyGroup("TrEmptyBeamRuns", "Transmission Numors")

        # =============================INPUT FILES/WORKSPACES==============================#

        self.declareProperty(
            name="SensitivityMap", defaultValue="", doc="File or workspace containing the map of the relative detector efficiencies."
        )
        self.setPropertyGroup("SensitivityMap", "Input Files/Workspaces")

        self.declareProperty(
            name="DefaultMask",
            defaultValue="",
            doc="File or workspace containing the default mask (detector edges and dead pixels/tubes)"
            " to be applied to all the detector configurations.",
        )
        self.setPropertyGroup("DefaultMask", "Input Files/Workspaces")

        self.declareProperty(
            name="BeamStopMasks",
            defaultValue="",
            doc="File(s) or workspace(s) containing the detector mask per distance configuration (typically beam stop).",
        )
        self.setPropertyGroup("BeamStopMasks", "Input Files/Workspaces")

        self.declareProperty(
            name="FlatFields",
            defaultValue="",
            doc="File(s) or workspaces containing the reduced water data (in 2D) for absolute normalisation.",
        )
        self.setPropertyGroup("FlatFields", "Input Files/Workspaces")

        self.declareProperty(
            name="Solvents",
            defaultValue="",
            doc="File(s) or workspace(s) containing the reduced solvent/buffer data (in 2D) for solvent subtraction.",
        )
        self.setPropertyGroup("Solvents", "Input Files/Workspaces")

        # ==============================REDUCTION PARAMETERS===============================#

        self.declareProperty(
            name="TransmissionThetaDependent",
            defaultValue=True,
            doc="Whether or not to apply the transmission correction in 2theta-dependent way.",
        )
        self.setPropertyGroup("TransmissionThetaDependent", "Parameters")

        self.declareProperty(name="NormaliseBy", defaultValue="Monitor", validator=StringListValidator(["None", "Time", "Monitor"]))
        self.setPropertyGroup("NormaliseBy", "Parameters")

        self.declareProperty(
            FloatArrayProperty(name="TrBeamRadius", values=[0.1]), doc="Beam radius [m] used for transmission and flux calculations."
        )
        self.setPropertyGroup("TrBeamRadius", "Parameters")

        self.declareProperty(FloatArrayProperty(name="BeamRadius", values=[0.25]), doc="Beam radius [m] used for beam center search.")
        self.setPropertyGroup("BeamRadius", "Parameters")

        self.declareProperty(
            FloatArrayProperty(name="SampleThickness", values=[0.1]), doc="Sample thickness [cm] used in final normalisation."
        )
        self.setPropertyGroup("SampleThickness", "Parameters")

        self.declareProperty(
            name="SampleThicknessFrom",
            defaultValue="User",
            validator=StringListValidator(["User", "Nexus"]),
            doc="Define where to read the sample thicknesses from.",
        )
        self.setPropertyGroup("SampleThicknessFrom", "Parameters")

        self.declareProperty(
            StringArrayProperty(name="SampleNames", values=[]), doc="Sample names to put in the axis of the output workspaces."
        )
        self.setPropertyGroup("SampleNames", "Parameters")

        self.declareProperty(
            name="SampleNamesFrom",
            defaultValue="RunNumber",
            validator=StringListValidator(["User", "Nexus", "RunNumber"]),
            doc="Define where to read the sample names from.",
        )
        self.setPropertyGroup("SampleNamesFrom", "Parameters")

        self.declareProperty(
            name="WaterCrossSection",
            defaultValue=1.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Provide the water cross-section; used only if the absolute scale is done by dividing to water.",
        )
        self.setPropertyGroup("WaterCrossSection", "Parameters")

        self.declareProperty(
            name="ProduceSensitivity",
            defaultValue=False,
            doc="Whether or not to produce a sensitivity map; should be used for water reduction only.",
        )
        self.setPropertyGroup("ProduceSensitivity", "Parameters")

        self.declareProperty(
            name="SensitivityWithOffsets",
            defaultValue=False,
            doc="Whether the sensitivity data has been measured with different horizontal offsets (D22 only).",
        )
        self.setPropertyGroup("SensitivityWithOffsets", "Parameters")

        self.copyProperties("CalculateEfficiency", ["MinThreshold", "MaxThreshold"])
        # override default documentation of copied parameters to make them understandable by user
        threshold_property = self.getProperty("MinThreshold")
        threshold_property.setDocumentation("Minimum threshold for calculated sensitivity.")
        threshold_property = self.getProperty("MaxThreshold")
        threshold_property.setDocumentation("Maximum threshold for calculated sensitivity.")

        self.declareProperty(
            IntArrayProperty(name="DistancesAtWavelength2", values=[]),
            doc="Defines which distance indices (starting from 0) match to the 2nd wavelength",
        )
        self.setPropertyGroup("DistancesAtWavelength2", "Parameters")

        # ===================================I(Q) OPTIONS==================================#

        self.declareProperty(
            name="OutputBinning", defaultValue="", doc="Output binning for each distance( : separated list of binning params)."
        )
        self.setPropertyGroup("OutputBinning", "I(Q) Options")

        self.declareProperty(
            name="OutputType",
            defaultValue="I(Q)",
            validator=StringListValidator(["I(Q)"]),
            doc="Final integration type; at the moment only I(Q) is supported.",
        )
        self.setPropertyGroup("OutputType", "I(Q) Options")

        iq_options = [
            "CalculateResolution",
            "DefaultQBinning",
            "BinningFactor",
            "NumberOfWedges",
            "WedgeAngle",
            "WedgeOffset",
            "AsymmetricWedges",
            "WavelengthRange",
            "ShapeTable",
        ]
        self.copyProperties("SANSILLIntegration", iq_options)
        for opt in iq_options:
            self.setPropertyGroup(opt, "I(Q) Options")

        self.declareProperty(name="OutputPanels", defaultValue=False, doc="Output I(Q) per detector bank.")
        self.setPropertyGroup("OutputPanels", "I(Q) Options")

        # ==================================STITCH OPTIONS=================================#

        self.declareProperty(name="PerformStitching", defaultValue=True, doc="Wheter or not to perform stitching.")
        self.setPropertyGroup("PerformStitching", "Stitch Options")

        stitch_options = ["ManualScaleFactors", "TieScaleFactors", "ScaleFactorCalculation"]
        self.copyProperties("Stitch", stitch_options)
        for opt in stitch_options:
            self.setPropertyGroup(opt, "Stitch Options")

        self.declareProperty(
            name="StitchReferenceIndex",
            defaultValue=1,
            validator=IntBoundedValidator(lower=0),
            doc="The index of the reference workspace during stitching, "
            "by default the middle distance will be chosen as reference if there are 3.",
        )
        self.setPropertyGroup("StitchReferenceIndex", "Stitch Options")

        # ================================OUTPUT WORKSPACES================================#

        # This will be a group containing all the main and diagnostic outputs
        self.declareProperty(
            WorkspaceGroupProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="The output workspace group containing the reduced data.",
        )
        self.setPropertyGroup("OutputWorkspace", "Output Workspaces")

    # ================================CHECKS FOR INPUT VALIDATION================================#

    def _check_sample_runs_dimensions(self):
        """Makes sure all the sample inputs have matching extents"""
        issues = dict()
        for d in range(self.rank):
            prop = f"SampleRunsD{d+1}"
            n_items = self.getPropertyValue(prop).count(",") + 1
            if n_items != self.n_samples:
                issues[prop] = f"{prop} has {n_items} elements instead of {self.n_samples}"
        return issues

    def _check_tr_runs_dimensions(self):
        """Makes sure all the sample transmission inputs have matching extents"""
        issues = dict()
        for l in range(self.lambda_rank):
            prop = f"SampleTrRunsW{l+1}"
            n_items = self.getPropertyValue(prop).count(",") + 1
            if n_items > 1 and n_items != self.n_samples:
                issues[prop] = f"{prop} has {n_items} elements instead of {self.n_samples}"
        return issues

    def _check_aux_sample_input_dimensions(self):
        """Checks if the provided correction inputs have the right rank"""
        issues = dict()
        props_to_match_rank = [
            "DarkCurrentRuns",
            "EmptyBeamRuns",
            "FluxRuns",
            "EmptyContainerRuns",
            "BeamStopMasks",
            "FlatFields",
            "Solvents",
        ]
        for prop in props_to_match_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(",") + 1
                if prop_rank > 1 and prop_rank != self.rank:
                    issues[prop] = f"{prop} has {prop_rank} elements which does not match the number of distances {self.rank}"
        return issues

    def _check_aux_tr_input_dimensions(self):
        """Checks if the provided correction inputs have the right lambda rank"""
        issues = dict()
        props_to_match_lambda_rank = ["TrDarkCurrentRuns", "ContainerTrRuns", "TrEmptyBeamRuns"]
        for prop in props_to_match_lambda_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(",") + 1
                if prop_rank > 1 and prop_rank != self.lambda_rank:
                    issues[prop] = f"{prop} has {prop_rank} elements which does not match the number of wavelengths {self.lambda_rank}"
        return issues

    def _check_aux_sample_params_dimensions(self):
        """Checks if provided primitive parameters match the rank of the samples"""
        issues = dict()
        props_to_match_rank = ["BeamRadius"]
        for prop in props_to_match_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(",") + 1
                if prop_rank > 1 and prop_rank != self.rank:
                    issues[prop] = f"{prop} has {prop_rank} elements which does not match the number of distances {self.rank}"
        return issues

    def _check_sample_thickness_dimensions(self):
        """Checks if provided sample thickness length is acceptable"""
        issues = dict()
        read_from = self.getPropertyValue("SampleThicknessFrom")
        thicknesses = self.getProperty("SampleThickness").value
        if read_from == "User":
            n_thick = len(thicknesses)
            if n_thick > 1 and n_thick != self.n_samples:
                issues["SampleThickness"] = (
                    f"SampleThickness has {n_thick} elements which does not match the number of samples {self.n_samples}."
                )
        return issues

    def _check_sample_names_dimensions(self):
        """Checks if provided sample names length is acceptable"""
        issues = dict()
        read_from = self.getPropertyValue("SampleNamesFrom")
        user_names = self.getProperty("SampleNames").value
        if read_from == "User":
            n_names = len(user_names)
            if n_names != self.n_samples:
                issues["SampleNames"] = f"SampleNames has {n_names} elements which does not match the number of samples {self.n_samples}."
        return issues

    def _check_aux_tr_params_dimensions(self):
        """Checks if provided primitive parameters for transmissions match the rank of the wavelengths"""
        issues = dict()
        props_to_match_rank = ["TrBeamRadius"]
        for prop in props_to_match_rank:
            if self.getPropertyValue(prop):
                prop_rank = self.getPropertyValue(prop).count(",") + 1
                if prop_rank > 1 and prop_rank != self.lambda_rank:
                    issues[prop] = f"{prop} has {prop_rank} elements which does not match the number of distances {self.lambda_rank}"
        return issues

    def _check_q_ranges(self):
        """Makes sure the OutputBinning is composed of : delimited q rebin params"""
        issues = dict()
        qbinning = self.getPropertyValue("OutputBinning")
        if qbinning:
            if qbinning.count(":") + 1 != self.rank:
                issues["OutputBinning"] = "Number of Q binning parameter sets must be equal to the number of distances."
            else:
                for qbin_params in qbinning.split(":"):
                    if qbin_params:
                        for qbin_param in qbin_params.split(","):
                            try:
                                float(qbin_param)
                            except ValueError:
                                issues["OutputBinning"] = "Q binning params must be float numbers."
                                break
        return issues

    def _check_output_name(self):
        """Makes sure the name of the output workspace starts with a letter"""
        issues = dict()
        if self.getPropertyValue("OutputWorkspace")[0].isdigit():
            issues["OutputWorkspace"] = "Output workspace name must be alphanumeric, it should start with a letter."
        return issues

    def _check_wedges_panels_mutex(self):
        """Makes sure panels and wedges are not requested at the same time"""
        issues = dict()
        if (self.getProperty("NumberOfWedges").value > 0 or self.getPropertyValue("ShapeTable")) and self.getProperty("OutputPanels").value:
            issues["OutputPanels"] = "Panels cannot be calculated together with the wedges, please choose one or the other."
        return issues

    def _check_sample_runs_filling_order(self):
        """Makes sure that the sample runs are filled in from D1 onwards with no gaps"""
        issues = dict()
        is_empty = False
        for d in range(N_DISTANCES):
            prop = f"SampleRunsD{d+1}"
            if is_empty and self.getPropertyValue(prop):
                issues[prop] = f"Samples have to be filled from D1 onwards: found {d+1} non-empty, while another distance before was empty."
            if not self.getPropertyValue(prop):
                is_empty = True
        return issues

    def _check_tr_runs_filling_order(self):
        """Makes sure that the transmission runs are filled in from W1 onwards"""
        issues = dict()
        if not self.getPropertyValue("SampleTrRunsW1") and self.getPropertyValue("SampleTrRunsW2"):
            issues["SampleTrRunsW1"] = "If there is one wavelength, transmissions must be filled in W1."
        return issues

    def _check_optional_files_exist(self):
        """Checks if the requested calibration workspaces/files exist"""
        issues = dict()
        props = ["SensitivityMap", "DefaultMask", "BeamStopMasks", "FlatFields", "Solvents"]
        for prop in props:
            value = self.getPropertyValue(prop)
            if value:
                for item in value.split(","):
                    if not mtd.doesExist(item):
                        try:
                            FileFinder.Instance().findRuns(item)
                        except RuntimeError as re:
                            issues[prop] = str(re)
        return issues

    def validateInputs(self):
        """Validates all the inputs, one by one. Returns at first failure."""
        issues = dict()
        try:
            self._setup_light()
        except RuntimeError as re:
            issues["SampleRunsD1"] = "Unable to configure the algorithm: " + str(re)
        if not issues:
            issues = self._check_sample_runs_filling_order()
        if not issues:
            issues = self._check_tr_runs_filling_order()
        if not issues:
            issues = self._check_sample_runs_dimensions()
        if not issues:
            issues = self._check_tr_runs_dimensions()
        if not issues:
            issues = self._check_aux_sample_input_dimensions()
        if not issues:
            issues = self._check_sample_thickness_dimensions()
        if not issues:
            issues = self._check_sample_names_dimensions()
        if not issues:
            issues = self._check_aux_tr_input_dimensions()
        if not issues:
            issues = self._check_aux_sample_params_dimensions()
        if not issues:
            issues = self._check_aux_tr_params_dimensions()
        if not issues:
            issues = self._check_q_ranges()
        if not issues:
            issues = self._check_output_name()
        if not issues:
            issues = self._check_wedges_panels_mutex()
        if not issues:
            issues = self._check_optional_files_exist()
        return issues

    # ================================CONFIGURING LOGIC================================#

    def _reset(self):
        """Resets the class member variables"""
        self.rank = None
        self.instrument = None
        self.lambda_rank = None
        self.n_samples = None
        self.name_axis = None
        self.progress = None
        self.n_reports = None

    def _set_rank(self):
        """Sets the actual rank of the reduction"""
        self.rank = 0
        for d in range(N_DISTANCES):
            if self.getPropertyValue(f"SampleRunsD{d+1}"):
                self.rank += 1
        if self.rank == 0:
            raise RuntimeError("No sample runs are provided, at least one distance must not be empty.")
        else:
            self.log().notice(f"Set the rank of reduction to {self.rank}")

    def _set_lambda_rank(self):
        """Sets the actual lambda rank"""
        self.lambda_rank = 0
        for l in range(N_LAMBDAS):
            if self.getPropertyValue(f"SampleTrRunsW{l+1}"):
                self.lambda_rank += 1
        self.log().notice(f"Set the lambda rank of reduction to {self.lambda_rank}")

    def _set_n_samples(self):
        """Sets the number of samples based on the inputs of the sample runs in the first non-empty distance"""
        for d in range(N_DISTANCES):
            if self.getPropertyValue(f"SampleRunsD{d+1}"):
                self.n_samples = self.getPropertyValue(f"SampleRunsD{d+1}").count(",") + 1
                self.log().notice(f"Set number of samples to {self.n_samples}")
                break

    def _setup_light(self):
        """Performs a light setup, which can be done before loading any data"""
        self._reset()
        self._set_rank()
        self._set_lambda_rank()
        self._set_n_samples()  # must be after set_rank()
        self.n_reports = (self.rank + self.lambda_rank + 1) * self.n_samples
        self.progress = Progress(self, start=0.0, end=1.0, nreports=self.n_reports)

    def tr_index(self, d):
        """Returns the index of the transmission wavelength based on the index of distance"""
        return 1 if d + 1 in self.getProperty("DistancesAtWavelength2").value else 0

    def get_beam_radius(self, d):
        """Returns the beam search radius at the given distance"""
        radii = self.getProperty("BeamRadius").value
        if len(radii) == 1:
            return radii[0]
        else:
            return radii[d]

    def get_tr_beam_radius(self, l):
        """Returns transmission beam radius at the given wavelength"""
        radii = self.getProperty("TrBeamRadius").value
        if len(radii) == 1:
            return radii[0]
        else:
            return radii[l]

    # ================================PROCESSING ALL===============================#

    def process_all_transmissions(self):
        """Calculates all the transmissions"""
        all_outputs = []
        for l in range(self.lambda_rank):
            transmissions = self.process_all_transmissions_at_lambda(l)
            self.progress.report((l + 1) * self.n_samples, f"Calculated transmissions for wavelength index {l+1}")
            all_outputs.append(transmissions)
        return all_outputs

    def process_all_samples(self, transmissions):
        """Reduces all the samples"""
        all_outputs = []
        for d in range(self.rank):
            outputs = dict()
            sample_ws = self.process_all_samples_at_distance(d, transmissions)
            self.progress.report((d + 1) * self.n_samples, f"Reduced sample data at distance index {d+1}")
            outputs["RealSpace"] = sample_ws[0]
            if len(sample_ws) > 1:
                # if there is a 2nd output, it must be sensitivity
                outputs["Sensitivity"] = sample_ws[1]
            integrated_ws = self.integrate(d, sample_ws)
            # set distribution to false since stitch doesn't deal with frequencies
            self.set_distribution(integrated_ws, False)
            outputs["IQ"] = integrated_ws[0]
            if len(integrated_ws) > 1:
                # if there is a second output from integration, it must be either the panels or the wedges
                key = "IQP" if self.getProperty("OutputPanels").value else "IQW"
                outputs[key] = integrated_ws[1]
            all_outputs.append(outputs)
        return all_outputs

    def process_all_transmissions_at_lambda(self, l):
        """
        Calculates transmissions at the given lambda index
        """
        tr_dark_current_ws = self.process_tr_dark_current(l)
        [tr_empty_beam_ws, tr_empty_beam_flux] = self.process_tr_empty_beam(l, tr_dark_current_ws)
        tr_empty_can_ws = self.process_empty_can_tr(l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux)
        tr_sample_ws = self.process_sample_tr(l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux)
        results = dict()
        if tr_sample_ws:
            results["SampleTransmission"] = tr_sample_ws
        if tr_empty_can_ws:
            results["ContainerTransmission"] = tr_empty_can_ws
        return results

    def process_all_samples_at_distance(self, d, transmissions):
        """
        Reduces all the samples at a given distance
        """
        dark_current_ws = self.process_dark_current(d)
        [empty_beam1_ws, empty_beam1_flux] = self.process_empty_beam(d, dark_current_ws)
        [empty_beam2_ws, empty_beam2_flux] = self.process_flux(d, dark_current_ws)
        empty_can_ws = self.process_container(d, dark_current_ws, empty_beam1_ws, transmissions)
        actual_flux_ws = empty_beam2_flux if empty_beam2_flux else empty_beam1_flux
        return self.process_sample(d, dark_current_ws, empty_beam1_ws, empty_can_ws, actual_flux_ws, transmissions)

    # ================================LOAD PROCESSED CALIBRANTS================================#

    def load_sensitivity(self, d):
        """Loads the sensitivity map if it is not in ADS"""
        sens = self.getPropertyValue("SensitivityMap")
        if sens:
            [load_sens, sens_ws] = needs_loading(sens, "Sensitivity")
            if load_sens:
                LoadNexusProcessed(Filename=sens, OutputWorkspace=sens_ws)
            return sens_ws
        else:
            return ""

    def load_masks(self, d):
        """Loads the beam stop and edge masks if they are not in ADS"""
        edge_mask = self.getPropertyValue("DefaultMask")
        [load_edge_mask, edge_mask_ws] = needs_loading(edge_mask, "DefaultMask")
        if load_edge_mask:
            LoadNexusProcessed(Filename=edge_mask, OutputWorkspace=edge_mask_ws)
        beam_stop = self.getPropertyValue("BeamStopMasks")
        beam_stop_mask_ws = ""
        if beam_stop:
            beam_stop_mask = beam_stop.split(",")[d]
            [load_beam_stop_mask, beam_stop_mask_ws] = needs_loading(beam_stop_mask, "Mask")
            if load_beam_stop_mask:
                LoadNexusProcessed(Filename=beam_stop_mask, OutputWorkspace=beam_stop_mask_ws)
        return [edge_mask_ws, beam_stop_mask_ws]

    def load_flat_field(self, d):
        """Loads the flat field if it is not in ADS"""
        flat_fields = self.getPropertyValue("FlatFields")
        if flat_fields:
            flat_field = flat_fields.split(",")[d]
            [load_flat_field, flat_field_ws] = needs_loading(flat_field, "FlatField")
            if load_flat_field:
                LoadNexusProcessed(Filename=flat_field, OutputWorkspace=flat_field_ws)
            return flat_field_ws
        else:
            return ""

    def load_solvent(self, d):
        """Loads the solvent if it is not in ADS"""
        solvents = self.getPropertyValue("Solvents")
        if solvents:
            solvent = solvents.split(",")[d]
            [load_solvent, solvent_ws] = needs_loading(solvent, "Solvent")
            if load_solvent:
                LoadNexusProcessed(Filename=solvent, OutputWorkspace=solvent_ws)
            return solvent_ws
        else:
            return ""

    # ================================PROCESS REDUCTIONS================================#

    def process_tr_dark_current(self, l):
        """Processes the dark current at the tranmission configuration"""
        runs = self.getPropertyValue("TrDarkCurrentRuns")
        if runs:
            tr_dark_current = runs.split(",")[l]
            [process_tr_dark_current, tr_dark_current_ws] = needs_processing(tr_dark_current, "DarkCurrent")
            if process_tr_dark_current:
                SANSILLReduction(
                    Runs=tr_dark_current,
                    ProcessAs="DarkCurrent",
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    OutputWorkspace=tr_dark_current_ws,
                )
            return tr_dark_current_ws
        else:
            return ""

    def process_tr_empty_beam(self, l, tr_dark_current_ws):
        """Processes the empty beam at the tranmission configuration"""
        runs = self.getPropertyValue("TrEmptyBeamRuns")
        if runs:
            tr_empty_beam = runs.split(",")[l]
            [process_tr_empty_beam, tr_empty_beam_ws] = needs_processing(tr_empty_beam, "EmptyBeam")
            tr_empty_beam_flux = tr_empty_beam_ws + "Flux"
            if process_tr_empty_beam:
                SANSILLReduction(
                    Runs=tr_empty_beam,
                    ProcessAs="EmptyBeam",
                    DarkCurrentWorkspace=tr_dark_current_ws,
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    TransmissionBeamRadius=self.get_tr_beam_radius(l),
                    OutputWorkspace=tr_empty_beam_ws,
                    OutputFluxWorkspace=tr_empty_beam_flux,
                )
            return [tr_empty_beam_ws, tr_empty_beam_flux]
        else:
            return ["", ""]

    def process_empty_can_tr(self, l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux):
        """Processes the empty container transmission at the given wavelength"""
        runs = self.getPropertyValue("ContainerTrRuns")
        if runs:
            tr_empty_can = runs.split(",")[l]
            [process_tr_empty_can, tr_empty_can_ws] = needs_processing(tr_empty_can, "Transmission")
            if process_tr_empty_can:
                SANSILLReduction(
                    Runs=tr_empty_can,
                    ProcessAs="Transmission",
                    DarkCurrentWorkspace=tr_dark_current_ws,
                    EmptyBeamWorkspace=tr_empty_beam_ws,
                    FluxWorkspace=tr_empty_beam_flux,
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    TransmissionBeamRadius=self.get_tr_beam_radius(l),
                    OutputWorkspace=tr_empty_can_ws,
                )
            return tr_empty_can_ws
        else:
            return ""

    def process_sample_tr(self, l, tr_dark_current_ws, tr_empty_beam_ws, tr_empty_beam_flux):
        """Processes the sample transmissions at the given wavelength"""
        tr_sample = self.getPropertyValue(f"SampleTrRunsW{l+1}")
        if tr_sample:
            [_, tr_sample_ws] = needs_processing(tr_sample, "Transmission")
            SANSILLReduction(
                Runs=tr_sample,
                ProcessAs="Transmission",
                DarkCurrentWorkspace=tr_dark_current_ws,
                EmptyBeamWorkspace=tr_empty_beam_ws,
                FluxWorkspace=tr_empty_beam_flux,
                NormaliseBy=self.getProperty("NormaliseBy").value,
                TransmissionBeamRadius=self.get_tr_beam_radius(l),
                OutputWorkspace=tr_sample_ws,
                startProgress=l * self.n_samples / self.n_reports,
                endProgress=(l + 1) * self.n_samples / self.n_reports,
            )
            return tr_sample_ws
        else:
            return ""

    def process_dark_current(self, d):
        """Processes the dark current at the given distance"""
        runs = self.getPropertyValue("DarkCurrentRuns")
        if runs:
            dark_current = runs.split(",")[d]
            [process_dark_current, dark_current_ws] = needs_processing(dark_current, "DarkCurrent")
            if process_dark_current:
                SANSILLReduction(
                    Runs=dark_current,
                    ProcessAs="DarkCurrent",
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    OutputWorkspace=dark_current_ws,
                )
            return dark_current_ws
        else:
            return ""

    def process_empty_beam(self, d, dark_current_ws):
        """Processes the empty beam at the given distance"""
        runs = self.getPropertyValue("EmptyBeamRuns")
        if runs:
            empty_beam = runs.split(",")[d]
            [process_empty_beam, empty_beam_ws] = needs_processing(empty_beam, "EmptyBeam")
            empty_beam_flux = empty_beam_ws + "Flux"
            if process_empty_beam:
                SANSILLReduction(
                    Runs=empty_beam,
                    ProcessAs="EmptyBeam",
                    DarkCurrentWorkspace=dark_current_ws,
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    BeamRadius=self.get_beam_radius(d),
                    TransmissionBeamRadius=self.get_tr_beam_radius(self.tr_index(d)),
                    OutputWorkspace=empty_beam_ws,
                    OutputFluxWorkspace=empty_beam_flux,
                )
            return [empty_beam_ws, empty_beam_flux]
        else:
            return ["", ""]

    def process_flux(self, d, dark_current_ws):
        """Processes the empty beam for flux at the given distance"""
        runs = self.getPropertyValue("FluxRuns")
        if runs:
            empty_beam = runs.split(",")[d]
            [process_empty_beam, empty_beam_ws] = needs_processing(empty_beam, "EmptyBeam")
            empty_beam_flux = empty_beam_ws + "Flux"
            if process_empty_beam:
                SANSILLReduction(
                    Runs=empty_beam,
                    ProcessAs="EmptyBeam",
                    DarkCurrentWorkspace=dark_current_ws,
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    TransmissionBeamRadius=self.get_tr_beam_radius(self.tr_index(d)),
                    OutputWorkspace=empty_beam_ws,
                    OutputFluxWorkspace=empty_beam_flux,
                )
            return [empty_beam_ws, empty_beam_flux]
        else:
            return ["", ""]

    def process_container(self, d, dark_current_ws, empty_beam_ws, transmissions):
        """Processes the empty container at the given distance"""
        runs = self.getPropertyValue("EmptyContainerRuns")
        if runs:
            empty_can = runs.split(",")[d]
            [process_empty_can, empty_can_ws] = needs_processing(empty_can, "EmptyContainer")
            if process_empty_can:
                can_tr_ws = ""
                if transmissions and "Container" in transmissions[self.tr_index(d)]:
                    can_tr_ws = transmissions[self.tr_index(d)]["Container"]
                SANSILLReduction(
                    Runs=empty_can,
                    ProcessAs="EmptyContainer",
                    DarkCurrentWorkspace=dark_current_ws,
                    EmptyBeamWorkspace=empty_beam_ws,
                    TransmissionWorkspace=can_tr_ws,
                    TransmissionThetaDependent=self.getProperty("TransmissionThetaDependent").value,
                    NormaliseBy=self.getProperty("NormaliseBy").value,
                    OutputWorkspace=empty_can_ws,
                )
            return empty_can_ws
        else:
            return ""

    def process_sample(self, d, dark_current_ws, empty_beam_ws, empty_can_ws, flux_ws, transmissions):
        """Processes all the samples at the given distance"""
        runs = self.getPropertyValue(f"SampleRunsD{d+1}")
        if runs:
            [edge_mask_ws, beam_stop_mask_ws] = self.load_masks(d)
            flat_field_ws = self.load_flat_field(d)
            solvent_ws = self.load_solvent(d)
            sens_ws = self.load_sensitivity(d)
            sample_tr_ws = transmissions[self.tr_index(d)]["SampleTransmission"] if transmissions else ""
            process = "Sample"
            [_, sample_ws] = needs_processing(runs, "Sample")
            sens_out = ""
            if self.getProperty("ProduceSensitivity").value:
                process = "Water"
                sens_out = sample_ws + "_Sens"
            user_thickness = self.getProperty("SampleThickness").value
            thickness_from = self.getPropertyValue("SampleThicknessFrom")
            thickness_to_use = user_thickness if thickness_from == "User" else [-1]
            SANSILLReduction(
                Runs=runs,
                ProcessAs=process,
                DarkCurrentWorkspace=dark_current_ws,
                EmptyBeamWorkspace=empty_beam_ws,
                TransmissionWorkspace=sample_tr_ws,
                EmptyContainerWorkspace=empty_can_ws,
                DefaultMaskWorkspace=edge_mask_ws,
                MaskWorkspace=beam_stop_mask_ws,
                FlatFieldWorkspace=flat_field_ws,
                SensitivityWorkspace=sens_ws,
                SolventWorkspace=solvent_ws,
                FluxWorkspace=flux_ws,
                NormaliseBy=self.getProperty("NormaliseBy").value,
                TransmissionThetaDependent=self.getProperty("TransmissionThetaDependent").value,
                SampleThickness=thickness_to_use,
                WaterCrossSection=self.getProperty("WaterCrossSection").value,
                OutputWorkspace=sample_ws,
                OutputSensitivityWorkspace=sens_out,
                startProgress=(self.lambda_rank + d) * self.n_samples / self.n_reports,
                endProgress=(self.lambda_rank + d + 1) * self.n_samples / self.n_reports,
                MinThreshold=self.getProperty("MinThreshold").value,
                MaxThreshold=self.getProperty("MaxThreshold").value,
            )
            add_correction_numors(
                ws=sample_ws,
                stransmission=sample_tr_ws,
                container=empty_can_ws,
                absorber=dark_current_ws,
                beam=empty_beam_ws,
                flux=flux_ws,
                solvent=solvent_ws,
                reference="",
                sensitivity="",
            )
            return [sample_ws, sens_out]
        else:
            return []

    # ================================INTEGRATION AND COMBINATIONS================================#

    def integrate(self, d, sample_ws):
        """Performs azimuthal averaging, optionally with sectors"""
        results = []
        panel_names = ""
        instrument = mtd[sample_ws[0]].getInstrument()
        if instrument.hasParameter("detector_panels"):
            panel_names = instrument.getStringParameter("detector_panels")[0].split(",")
        CalculateDynamicRange(Workspace=sample_ws[0], ComponentNames=panel_names)
        # This will mask the pixel if it counts NaN in any of the frames
        MaskDetectorsIf(InputWorkspace=sample_ws[0], OutputWorkspace=sample_ws[0], Operator="NotFinite")
        kwargs = dict()
        kwargs["InputWorkspace"] = sample_ws[0]
        kwargs["OutputWorkspace"] = sample_ws[0] + "_iq"
        results.append(kwargs["OutputWorkspace"])
        # for the moment only I(Q) is supported, since with the workspace containing all the samples,
        # I(Phi,Q) and I(Qx,Qy) will need to be reworked to become MDHistoWorkspace, if they are required
        kwargs["OutputType"] = "I(Q)"
        props = [
            "CalculateResolution",
            "DefaultQBinning",
            "BinningFactor",
            "NumberOfWedges",
            "WedgeAngle",
            "WedgeOffset",
            "AsymmetricWedges",
            "WavelengthRange",
            "ShapeTable",
        ]
        for prop in props:
            kwargs[prop] = self.getProperty(prop).value
        if self.getProperty("NumberOfWedges").value > 0 or self.getPropertyValue("ShapeTable"):
            kwargs["WedgeWorkspace"] = sample_ws[0] + "_iq_wedges"
            results.append(kwargs["WedgeWorkspace"])
        qbinning = self.getPropertyValue("OutputBinning")
        if qbinning:
            kwargs["OutputBinning"] = qbinning.split(":")[d]
        if self.getProperty("OutputPanels").value:
            kwargs["PanelOutputWorkspaces"] = sample_ws[0] + "_iq_panels"
            results.append(kwargs["PanelOutputWorkspaces"])
        SANSILLIntegration(**kwargs)
        if not self.name_axis:
            self.name_axis = self.generate_name_axis(results[0])
        for output in results:
            self.replace_axis(output, self.name_axis, 1)
        return results

    def generate_name_axis(self, ws_name):
        """Generate a sample name axis to be put in the output workspaces"""
        names_from = self.getPropertyValue("SampleNamesFrom")
        user_names = self.getProperty("SampleNames").value
        ws = mtd[ws_name][0] if isinstance(mtd[ws_name], WorkspaceGroup) else mtd[ws_name]
        run = ws.getRun()
        actual_names = []
        if names_from == "User":
            actual_names = user_names
        elif names_from == "RunNumber":
            actual_names = str(run["run_number"].value).split(",")
        elif names_from == "Nexus":
            actual_names = run["sample_description"].value.split(",")
        mode = run["AcqMode"].value
        n_frames = run["N_frames"].value
        if mode == AcqMode.KINETIC or mode == AcqMode.REVENT:
            axis = TextAxis.create(len(actual_names) * n_frames)
            for index, label in enumerate(actual_names):
                for frame in range(n_frames):
                    axis.setLabel(index * n_frames + frame, f"{label}_frame_#{frame}")
        else:
            axis = TextAxis.create(len(actual_names))
            for index, label in enumerate(actual_names):
                axis.setLabel(index, label)
        return axis

    def replace_axis(self, ws_name, axis, index=1):
        """Replaces the requested axis of the workspace with the specified axis"""
        if isinstance(mtd[ws_name], WorkspaceGroup):
            # ws_name will be a group workspace for panels and wedges outputs
            for ws in mtd[ws_name]:
                ws.replaceAxis(index, axis)
        else:
            mtd[ws_name].replaceAxis(index, axis)

    def set_distribution(self, ws_list, flag):
        """Sets/unsets the distribution flag"""
        for ws in ws_list:
            if isinstance(mtd[ws], WorkspaceGroup):
                for wsi in mtd[ws]:
                    ConvertToPointData(InputWorkspace=wsi.name(), OutputWorkspace=wsi.name())
                    mtd[wsi.name()].setDistribution(flag)
            else:
                ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
                mtd[ws].setDistribution(flag)

    def generate_combined_sensitivity(self, samples):
        """
        Generates the combined sensitivity w/o beam stop shadow.
        This is possible only on D22 with and w/o offsets.
        """
        real_space_ws_list = []
        for ws_dict in samples:
            if "RealSpace" in ws_dict:
                real_space_ws_list.append(ws_dict["RealSpace"])
        if real_space_ws_list:
            out_ws = self.getPropertyValue("OutputWorkspace")
            tmp_group = "__combined_sens_grp_" + out_ws
            comb_sens = out_ws + "_sensitivity_combined"
            GroupWorkspaces(InputWorkspaces=real_space_ws_list, OutputWorkspace=tmp_group)
            CalculateEfficiency(
                InputWorkspace=tmp_group,
                MergeGroup=True,
                MinThreshold=self.getProperty("MinThreshold").value,
                MaxThreshold=self.getProperty("MaxThreshold").value,
                OutputWorkspace=comb_sens,
            )
            UnGroupWorkspace(tmp_group)
            return {"CombinedSens": comb_sens}
        else:
            return dict()

    def stitch(self, samples):
        """Stitches the full azimuthal integrals, and optionally the wedge integrals"""
        results = dict()
        to_stitch = []
        to_stitch_wedges = []
        for pack in samples:
            if "IQ" in pack:
                to_stitch.append(pack["IQ"])
            if "IQW" in pack:
                to_stitch_wedges.append(pack["IQW"])
        out = self.getPropertyValue("OutputWorkspace")
        stitched_ws = out + "_stitched"
        stitch_scale_factors = out + "_stitch_scale_factors"
        stitched_main = self.do_stitch(to_stitch, stitched_ws, stitch_scale_factors)
        if stitched_main:
            results["Stitched"] = stitched_main[0]
            results["StitchScaleFactors"] = stitched_main[1]
        if to_stitch_wedges:
            n_wedges = mtd[to_stitch_wedges[0]].getNumberOfEntries()
            for w in range(n_wedges):
                to_stitch_w = []
                for i in range(len(to_stitch_wedges)):
                    to_stitch_w.append(mtd[to_stitch_wedges[i]][w].name())
                stitched_wedge_ws = out + "_stitched_wedge_" + str(w + 1)
                stitch_scale_factors = out + "_stitch_scale_factors_wedge_" + str(w + 1)
                stitched_wedge = self.do_stitch(to_stitch_w, stitched_wedge_ws, stitch_scale_factors)
                if stitched_wedge:
                    results[f"Stitched_Wedge{w+1}"] = stitched_wedge[0]
                    results[f"StitchScaleFactors_Wedge{w+1}"] = stitched_wedge[1]
        return results

    def do_stitch(self, inputs, output, output_scale_factors):
        """Performs the stitching"""
        stitch_options = ["ManualScaleFactors", "TieScaleFactors", "ScaleFactorCalculation"]
        kwargs = dict()
        for opt in stitch_options:
            kwargs[opt] = self.getProperty(opt).value
        try:
            Stitch(
                InputWorkspaces=inputs,
                OutputWorkspace=output,
                OutputScaleFactorsWorkspace=output_scale_factors,
                ReferenceWorkspace=inputs[self.getProperty("StitchReferenceIndex").value],
                **kwargs,
            )
            mtd[output].getRun().addProperty("stitch_scale_factors", list(mtd[output_scale_factors].readY(0)), True)
            return [output, output_scale_factors]
        except RuntimeError as e:
            self.log().error("Unable to stitch, consider stitching manually: " + str(e))
        return []

    def combine(self, samples):
        """Combines stitched and combined sensitivity outputs to the package"""
        if len(samples) > 1:
            if self.getProperty("PerformStitching").value:
                stitched = self.stitch(samples)
                if stitched:
                    samples.append(stitched)
            if self.getProperty("SensitivityWithOffsets").value:
                comb_sens = self.generate_combined_sensitivity(samples)
                if comb_sens:
                    samples.append(comb_sens)
        return samples

    def rename(self, key, ws):
        """Renames the workspace ws to a user-friendly scheme based on the key"""
        name = ws
        out = self.getPropertyValue("OutputWorkspace")
        if isinstance(mtd[ws], WorkspaceGroup):
            # if it is a group, it can be either IQP or IQW
            # no need to rename the group, but need to rename the items in it
            for wsi in mtd[ws]:
                old_name = wsi.name()
                new_name = out + "_iq_" + create_name(old_name) + old_name[old_name.find("iq") + 2 :]
                RenameWorkspace(wsi, new_name)
        else:
            if not key.startswith("Stitch") and key != "CombinedSens":
                name = f"{out}_{key.lower()}_{create_name(ws)}"
            if name != ws:
                RenameWorkspace(ws, name)
        return name

    def rename_tr(self, tr):
        """Renames the transmission workspace"""
        out = self.getPropertyValue("OutputWorkspace")
        wavelength = 0.0
        if mtd[tr].getRun().hasProperty("wavelength"):
            wavelength = mtd[tr].getRun()["wavelength"].value
        name = f"{out}_transmissions_w{wavelength:.1f}A"
        RenameWorkspace(tr, name)
        tr_ws = mtd[name]
        for i in range(tr_ws.getNumberHistograms()):
            tr_ws.setX(i, [wavelength])
        tr_ws.getAxis(0).setUnit("Wavelength")
        return name

    def package(self, samples, transmissions):
        """Packages all the output workspaces into a workspace group"""
        out_ws = self.getPropertyValue("OutputWorkspace")
        outputs = []
        for pack in samples:
            for k, v in pack.items():
                if v:
                    if k.startswith("IQ") or k.startswith("Stitched"):
                        self.set_distribution([v], True)
                    outputs.append(self.rename(k, v))
        for tr in transmissions:
            Transpose(InputWorkspace=tr["SampleTransmission"], OutputWorkspace=tr["SampleTransmission"])
            self.replace_axis(tr["SampleTransmission"], self.generate_name_axis(tr["SampleTransmission"]), 1)
            outputs.append(self.rename_tr(tr["SampleTransmission"]))
        GroupWorkspaces(InputWorkspaces=outputs, OutputWorkspace=out_ws)
        self.setProperty("OutputWorkspace", out_ws)

    # ================================THE ENTRY POINT================================#
    def PyExec(self):
        """Executes the algorithm"""
        self._setup_light()
        transmissions = self.process_all_transmissions()
        samples = self.process_all_samples(transmissions)
        outputs = self.combine(samples)
        self.package(outputs, transmissions)
        self.progress.report(self.n_reports, "Combined and packaged reduced data")


AlgorithmFactory.subscribe(SANSILLMultiProcess)
