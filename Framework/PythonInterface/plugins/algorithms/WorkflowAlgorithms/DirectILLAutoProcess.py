# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import DirectILL_common as common
from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    MultipleFileProperty,
    Progress,
    PropertyMode,
    WorkspaceGroup,
    WorkspaceGroupProperty,
)
from mantid.kernel import (
    Direction,
    FloatArrayProperty,
    FloatArrayOrderedPairsValidator,
    FloatBoundedValidator,
    IntArrayProperty,
    PropertyManagerProperty,
    RebinParamsValidator,
    StringListValidator,
)
from mantid.simpleapi import (
    ApplyPaalmanPingsCorrection,
    ConvertUnits,
    CorrectKiKf,
    CreateSingleValuedWorkspace,
    DeleteWorkspace,
    DeleteWorkspaces,
    DetectorEfficiencyCorUser,
    DirectILLCollectData,
    DirectILLDiagnostics,
    DirectILLReduction,
    DirectILLIntegrateVanadium,
    Divide,
    GroupDetectors,
    GroupWorkspaces,
    Load,
    LoadEmptyInstrument,
    LoadMask,
    LoadNexus,
    MaskAngle,
    MaskBinsIf,
    MaskBTP,
    MaskDetectors,
    MergeRuns,
    Minus,
    PaalmanPingsAbsorptionCorrection,
    PaalmanPingsMonteCarloAbsorption,
    Rebin,
    RebinToWorkspace,
    RenameWorkspace,
    SaveMask,
    SaveNexus,
    SaveNXSPE,
    Scale,
    SetSample,
)

from os import path

import numpy as np


def get_run_number(value):
    """
    Extracts the run number from the first run out of the string value of a
    multiple file property of numors
    """
    return path.splitext(path.basename(value.split(",")[0].split("+")[0]))[0]


def get_vanadium_corrections(vanadium_ws):
    """
    Extracts vanadium integral and vanadium diagnostics workspaces from the provided list. If the provided group
    has only one workspace, then it is assumed it contains vanadium integral. Assumed is the following order
    of vanadium workspaces for each numor in the group: SofQ, SofTW, diagnostics, integral.
    :param vanadium_ws: workspace group with processed vanadium
    :return: vanadium integral and vanadium diagnostics (if exist)
    """
    diagnostics = []
    integrals = []
    nentries = mtd[vanadium_ws].getNumberOfEntries()
    if nentries == 1:
        integrals.append(mtd[vanadium_ws][0].name())
    else:
        for index in range(0, nentries, 4):
            diagnostics.append(mtd[vanadium_ws][index + 2].name())
            integrals.append(mtd[vanadium_ws][index + 3].name())
    return diagnostics, integrals


class DirectILLAutoProcess(DataProcessorAlgorithm):
    instrument = None
    sample = None
    process = None
    reduction_type = None
    incident_energy = None
    incident_energy_ws = None
    elastic_channel_ws = None
    masking = None
    mask_ws = None
    output = None
    vanadium = None
    vanadium_epp = None
    vanadium_diagnostics = None
    vanadium_integral = None
    empty = None
    cadmium = None
    flat_bkg_scaling = None
    flat_background = None
    to_clean = None
    absorption_corr = None
    save_output = None
    clear_cache = None
    temperatures = None

    def category(self):
        return "{};{}".format(common.CATEGORIES, "ILL\\Auto")

    def summary(self):
        return "Performs automatic data reduction for the direct geometry TOF spectrometers at ILL."

    def seeAlso(self):
        return ["DirectILLReduction"]

    def name(self):
        return "DirectILLAutoProcess"

    def validateInputs(self):
        issues = dict()

        run_no_err = "Wrong number of {0} runs: {1}. Provide one or as many as sample runs: {2}."
        runs_sample = len(self.getPropertyValue("Runs"))
        if not self.getProperty("EmptyContainerWorkspace").isDefault:
            runs_container = mtd[self.getPropertyValue("EmptyContainerWorkspace")].getNumberOfEntries()
            if runs_container != 1 and runs_container > runs_sample:
                issues["BeamRuns"] = run_no_err.format("EmptyContainerWorkspace", runs_container, runs_sample)

        grouping_err_msg = "Only one grouping method can be specified."
        if self.getProperty(common.PROP_DET_GROUPING).isDefault:
            if self.getProperty("ApplyGroupingBy").value and not self.getProperty(common.PROP_GROUPING_ANGLE_STEP).isDefault:
                issues["ApplyGroupingBy"] = grouping_err_msg
                issues[common.PROP_GROUPING_ANGLE_STEP] = grouping_err_msg
        else:
            if not self.getProperty(common.PROP_DET_GROUPING_BY).isDefault:
                issues[common.PROP_DET_GROUPING] = grouping_err_msg
                issues[common.PROP_DET_GROUPING_BY] = grouping_err_msg
            if not self.getProperty("ApplyGroupingBy").isDefault:
                issues[common.PROP_DET_GROUPING] = grouping_err_msg
                issues[common.PROP_DET_HOR_GROUPING] = grouping_err_msg
                issues[common.PROP_DET_VER_GROUPING] = grouping_err_msg
                issues["ApplyGroupingBy"] = grouping_err_msg
            if not self.getProperty(common.PROP_GROUPING_ANGLE_STEP).isDefault:
                issues[common.PROP_DET_GROUPING] = grouping_err_msg
                issues[common.PROP_GROUPING_ANGLE_STEP] = grouping_err_msg

        if not self.getProperty("VanadiumWorkspace").isDefault and self.getPropertyValue("VanadiumWorkspace") not in mtd:
            # attempts to load the file, raises a runtime error if the desired file does not exist
            vanadium_ws = self.getPropertyValue("VanadiumWorkspace")
            try:
                Load(Filename=vanadium_ws, OutputWorkspace=vanadium_ws)
                if not isinstance(mtd[vanadium_ws], WorkspaceGroup):
                    RenameWorkspace(InputWorkspace=vanadium_ws, OutputWorkspace="{}_1".format(vanadium_ws))
                    GroupWorkspaces(InputWorkspaces="{}_1".format(vanadium_ws), OutputWorkspace=vanadium_ws)
            except ValueError:
                issues["VanadiumWorkspace"] = "Desired vanadium workspace: {} cannot be found.".format(vanadium_ws)

        if not self.getProperty("FlatBackgroundSource").isDefault and self.getPropertyValue("FlatBackgroundSource") not in mtd:
            # attempts to load the file, raises a runtime error if the desired file does not exist
            flat_bkg_ws = self.getPropertyValue("FlatBackgroundSource")
            try:
                Load(Filename=flat_bkg_ws, OutputWorkspace=flat_bkg_ws)
            except ValueError:
                issues["FlatBackgroundSource"] = "Desired flat background workspace: {} cannot be found.".format(flat_bkg_ws)

        if self.getPropertyValue("AbsorptionCorrection") != "None":
            if self.getProperty("SampleMaterial").isDefault:
                issues["SampleMaterial"] = "Please define sample material."
            if self.getProperty("SampleGeometry").isDefault:
                issues["SampleGeometry"] = "Please define sample geometry."

        return issues

    def setUp(self):
        self.sample = self.getPropertyValue("Runs").split(",")
        self.output = self.getPropertyValue("OutputWorkspace")
        self.process = self.getPropertyValue("ProcessAs")
        self.reduction_type = self.getPropertyValue("ReductionType")
        self.to_clean = []
        if not self.getProperty("IncidentEnergy").isDefault:
            self.incident_energy = self.getProperty("IncidentEnergy").value
            self.incident_energy_ws = "incident_energy_ws"
            CreateSingleValuedWorkspace(DataValue=self.incident_energy, OutputWorkspace=self.incident_energy_ws)
            self.to_clean.append(self.incident_energy_ws)
        if not self.getProperty("ElasticChannelIndex").isDefault:
            self.elastic_channel_ws = "elastic_channel_index_ws"
            CreateSingleValuedWorkspace(DataValue=self.getProperty("ElasticChannelIndex").value, OutputWorkspace=self.elastic_channel_ws)
            self.to_clean.append(self.elastic_channel_ws)
        if (
            self.getProperty("MaskWorkspace").isDefault
            and self.getProperty("MaskedTubes").isDefault
            and self.getProperty("MaskThresholdMin").isDefault
            and self.getProperty("MaskThresholdMax").isDefault
            and self.getProperty("MaskedAngles").isDefault
        ):
            self.masking = False
        else:
            self.masking = True
        if not self.getProperty("CadmiumWorkspace").isDefault:
            self.cadmium = self.getPropertyValue("CadmiumWorkspace")
        self.flat_bkg_scaling = self.getProperty(common.PROP_FLAT_BKG_SCALING).value
        self.empty = self.getPropertyValue("EmptyContainerWorkspace")
        self.vanadium = self.getPropertyValue("VanadiumWorkspace")
        if self.vanadium != str():
            self.vanadium_diagnostics, self.vanadium_integral = get_vanadium_corrections(self.vanadium)
        self.flat_background = self.getPropertyValue("FlatBackgroundSource")
        self.save_output = self.getProperty("SaveOutput").value
        self.clear_cache = self.getProperty("ClearCache").value
        self.temperatures = set()

    def PyInit(self):
        positiveFloat = FloatBoundedValidator(0.0, exclusive=False)
        validRebinParams = RebinParamsValidator(AllowEmpty=True)
        orderedPairsValidator = FloatArrayOrderedPairsValidator()

        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The output workspace group containing reduced data.",
        )

        self.declareProperty(MultipleFileProperty("Runs", action=FileAction.Load, extensions=["nxs"]), doc="Run(s) to be processed.")

        processes = ["Cadmium", "Empty", "Vanadium", "Sample"]
        self.declareProperty(
            name="ProcessAs", defaultValue="Sample", validator=StringListValidator(processes), doc="Choose the process type."
        )

        reduction_options = ["Powder", "SingleCrystal"]
        self.declareProperty(
            name="ReductionType",
            defaultValue="Powder",
            validator=StringListValidator(reduction_options),
            doc="Choose the appropriate reduction type for the data to process.",
        )

        self.declareProperty("VanadiumWorkspace", "", doc="File(s) or workspaces containing vanadium data.")

        self.declareProperty("EmptyContainerWorkspace", "", doc="Empty container workspace.")

        self.declareProperty("EmptyContainerScaling", 1.0, doc="Scaling factor for the empty container.")

        self.declareProperty(
            WorkspaceGroupProperty("CadmiumWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Cadmium absorber workspace.",
        )

        self.copyProperties("DirectILLCollectData", [common.PROP_FLAT_BKG, common.PROP_FLAT_BKG_WINDOW])

        self.declareProperty("FlatBackgroundSource", "", doc="File(s) or workspaces containing the source to calculate flat background.")

        self.copyProperties("DirectILLCollectData", [common.PROP_FLAT_BKG_SCALING, common.PROP_OUTPUT_FLAT_BKG_WS])

        self.copyProperties("DirectILLReduction", common.PROP_ABSOLUTE_UNITS)

        additional_inputs_group = "Corrections"
        self.setPropertyGroup("VanadiumWorkspace", additional_inputs_group)
        self.setPropertyGroup("EmptyContainerWorkspace", additional_inputs_group)
        self.setPropertyGroup("EmptyContainerScaling", additional_inputs_group)
        self.setPropertyGroup("CadmiumWorkspace", additional_inputs_group)
        self.setPropertyGroup(common.PROP_FLAT_BKG, additional_inputs_group)
        self.setPropertyGroup(common.PROP_FLAT_BKG_WINDOW, additional_inputs_group)
        self.setPropertyGroup("FlatBackgroundSource", additional_inputs_group)
        self.setPropertyGroup(common.PROP_FLAT_BKG_SCALING, additional_inputs_group)
        self.setPropertyGroup(common.PROP_OUTPUT_FLAT_BKG_WS, additional_inputs_group)
        self.setPropertyGroup(common.PROP_ABSOLUTE_UNITS, additional_inputs_group)

        self.copyProperties("DirectILLCollectData", [common.PROP_NORMALISATION, common.PROP_MON_PEAK_SIGMA_MULTIPLIER])

        self.copyProperties("DirectILLCollectData", common.PROP_INCIDENT_ENERGY_CALIBRATION)

        self.declareProperty(
            name="IncidentEnergy", defaultValue=0.0, validator=positiveFloat, doc="Value for the calibrated incident energy (meV)."
        )

        self.copyProperties("DirectILLCollectData", [common.PROP_ELASTIC_CHANNEL_MODE, common.PROP_EPP_METHOD])

        self.declareProperty(
            name="ElasticChannelIndex",
            defaultValue=0.0,
            validator=positiveFloat,
            doc="Bin index value for the centre of the elastic peak. Can be a float.",
        )

        self.declareProperty("SampleAngleOffset", 0.0, doc="Value for the offset parameter in omega scan (degrees).")

        calibration_group = "Calibration"
        self.setPropertyGroup(common.PROP_INCIDENT_ENERGY_CALIBRATION, calibration_group)
        self.setPropertyGroup("IncidentEnergy", calibration_group)
        self.setPropertyGroup("ElasticChannelIndex", calibration_group)
        self.setPropertyGroup(common.PROP_ELASTIC_CHANNEL_MODE, calibration_group)
        self.setPropertyGroup(common.PROP_EPP_METHOD, calibration_group)
        self.setPropertyGroup("SampleAngleOffset", calibration_group)

        # The mask workspace replaces MaskWorkspace parameter from PantherSingle and DiagnosticsWorkspace from directred
        self.declareProperty("MaskWorkspace", "", doc="File(s) or workspaces containing the mask.")

        self.declareProperty(IntArrayProperty(name="MaskedTubes", values=[], direction=Direction.Input), doc="List of tubes to be masked.")

        self.declareProperty(
            "MaskThresholdMin", 0.0, doc="Threshold level below which bins will be masked to remove empty / background pixels."
        )

        self.declareProperty("MaskThresholdMax", 0.0, doc="Threshold level above which bins will be masked to remove noisy pixels.")

        self.declareProperty(
            FloatArrayProperty(name="MaskedAngles", values=[], validator=orderedPairsValidator),
            doc="Mask detectors in the given angular range.",
        )

        self.declareProperty("MaskWithVanadium", True, doc="Whether to mask using vanadium diagnostics workspace.")

        masking_group_name = "Masking"
        self.setPropertyGroup("MaskWorkspace", masking_group_name)
        self.setPropertyGroup("MaskedTubes", masking_group_name)
        self.setPropertyGroup("MaskThresholdMin", masking_group_name)
        self.setPropertyGroup("MaskThresholdMax", masking_group_name)
        self.setPropertyGroup("MaskedAngles", masking_group_name)
        self.setPropertyGroup("MaskWithVanadium", masking_group_name)

        self.copyProperties("DirectILLReduction", [common.PROP_REBINNING_W, common.PROP_REBINNING_PARAMS_W])

        self.declareProperty(
            FloatArrayProperty(name="MomentumTransferBinning", validator=validRebinParams), doc="Momentum transfer binning parameters."
        )

        rebinning_group = "Binning parameters"
        self.setPropertyGroup(common.PROP_REBINNING_W, rebinning_group)
        self.setPropertyGroup(common.PROP_REBINNING_PARAMS_W, rebinning_group)
        self.setPropertyGroup("MomentumTransferBinning", rebinning_group)

        self.declareProperty(
            name="AbsorptionCorrection",
            defaultValue="None",
            validator=StringListValidator(["None", "Fast", "Full"]),
            doc="Choice of approach to absorption correction.",
        )

        self.declareProperty(
            name="SelfAttenuationMethod",
            defaultValue="MonteCarlo",
            validator=StringListValidator(["Numerical", "MonteCarlo"]),
            doc="Choice of calculation method for the attenuation calculation.",
        )

        self.declareProperty(PropertyManagerProperty("SampleMaterial"), doc="Sample material definitions.")

        self.declareProperty(PropertyManagerProperty("SampleGeometry"), doc="Dictionary for the sample geometry.")

        self.declareProperty(PropertyManagerProperty("ContainerMaterial"), doc="Container material definitions.")

        self.declareProperty(PropertyManagerProperty("ContainerGeometry"), doc="Dictionary for the container geometry.")

        attenuation_group = "Sample attenuation"
        self.setPropertyGroup("AbsorptionCorrection", attenuation_group)
        self.setPropertyGroup("SelfAttenuationMethod", attenuation_group)
        self.setPropertyGroup("SampleMaterial", attenuation_group)
        self.setPropertyGroup("SampleGeometry", attenuation_group)
        self.setPropertyGroup("ContainerMaterial", attenuation_group)
        self.setPropertyGroup("ContainerGeometry", attenuation_group)

        self.declareProperty(
            name=common.PROP_DET_GROUPING, defaultValue="", doc="Grouping pattern to reduce the granularity of the output."
        )

        self.declareProperty(
            name=common.PROP_DET_GROUPING_BY,
            defaultValue=1,
            doc="Step to use when grouping detectors to reduce the granularity of the output.",
        )

        self.copyProperties("DirectILLCollectData", [common.PROP_DET_HOR_GROUPING, common.PROP_DET_VER_GROUPING])

        self.declareProperty(
            name="ApplyGroupingBy",
            defaultValue=False,
            doc="Whether to apply the pixel grouping horizontally or vertically to the data, and not"
            " only to increase the statistics of the flat background calculation.",
        )

        self.declareProperty(
            name=common.PROP_GROUPING_ANGLE_STEP,
            defaultValue=0.0,
            validator=positiveFloat,
            doc="A scattering angle step to which to group detectors, in degrees.",
        )

        self.declareProperty(
            name="GroupingBehaviour",
            defaultValue="Sum",
            validator=StringListValidator(["Sum", "Average"]),
            doc="Defines which behaviour should be used when grouping pixels.",
        )

        grouping_options_group = "Grouping options"
        self.setPropertyGroup(common.PROP_DET_GROUPING, grouping_options_group)
        self.setPropertyGroup(common.PROP_DET_GROUPING_BY, grouping_options_group)
        self.setPropertyGroup(common.PROP_DET_HOR_GROUPING, grouping_options_group)
        self.setPropertyGroup(common.PROP_DET_VER_GROUPING, grouping_options_group)
        self.setPropertyGroup("ApplyGroupingBy", grouping_options_group)
        self.setPropertyGroup(common.PROP_GROUPING_ANGLE_STEP, grouping_options_group)
        self.setPropertyGroup("GroupingBehaviour", grouping_options_group)

        self.declareProperty(name="SaveOutput", defaultValue=True, doc="Whether to save the output directly after processing.")

        self.declareProperty(name="ClearCache", defaultValue=False, doc="Whether to clear intermediate workspaces.")

    def PyExec(self):
        self.setUp()
        sample_runs = self.getPropertyValue("Runs").split(",")
        processes = ["Cadmium", "Empty", "Vanadium", "Sample"]
        nReports = np.array([3, 3, 3, 3])
        nReports *= len(sample_runs)
        nReports += 1  # for the wrapping up report
        if self.masking:
            nReports += 1
        progress = Progress(self, start=0.0, end=1.0, nreports=int(nReports[processes.index(self.process)]))
        output_samples = []
        for sample_no, sample in enumerate(sample_runs):
            progress.report("Collecting data")
            ws = self._collect_data(sample, vanadium=self.process == "Vanadium")
            if self.cadmium:
                self._subtract_cadmium(ws)
            if self.masking and sample_no == 0:  # prepares masks once, and when the instrument is known
                progress.report("Preparing masks")
                self.mask_ws = self._prepare_masks()
            if self.process == "Vanadium":
                progress.report("Processing vanadium")
                ws_sofq, ws_softw, ws_diag, ws_integral = self._process_vanadium(ws)
                current_output = [ws_sofq, ws_softw, ws_diag, ws_integral]
                progress.report("Renaming output")
                current_output = self._rename_workspaces(current_output)
                output_samples.extend(current_output)
            elif self.process == "Sample":
                progress.report("Processing sample")
                sample_sofq, sample_softw = self._process_sample(ws, sample_no)
                current_output = np.array([sample_sofq, sample_softw])
                current_output = current_output[[isinstance(elem, str) for elem in current_output]]
                progress.report("Renaming output")
                current_output = self._rename_workspaces(current_output)
                output_samples.extend(current_output)
            else:  # Empty or Cadmium
                progress.report("Renaming output")
                current_output = [ws]
                current_output = self._rename_workspaces(current_output)
                self._group_detectors(current_output)
                output_samples.extend(current_output)
            if self.save_output:
                self._save_output(current_output)

        progress.report("Grouping outputs")
        GroupWorkspaces(InputWorkspaces=output_samples, OutputWorkspace=self.output)
        if self.clear_cache:  # final clean up
            self._clean_up(self.to_clean)
        self._print_report()
        self.setProperty("OutputWorkspace", mtd[self.output])

    def _apply_mask(self, ws):
        """Applies selected masks."""
        if self.mask_ws is not None:
            MaskDetectors(Workspace=ws, MaskedWorkspace=self.mask_ws)
        # masks bins below the chosen threshold, this has to be applied for each ws and cannot be created ahead:
        min_threshold_defined = not self.getProperty("MaskThresholdMin").isDefault
        max_threshold_defined = not self.getProperty("MaskThresholdMax").isDefault
        if min_threshold_defined or max_threshold_defined:
            masking_criterion = []
            if min_threshold_defined:
                masking_criterion.append("y < {}".format(self.getPropertyValue("MaskThresholdMin")))
            if max_threshold_defined:
                masking_criterion.append("y > {}".format(self.getPropertyValue("MaskThresholdMax")))
            MaskBinsIf(InputWorkspace=ws, OutputWorkspace=ws, Criterion=" || ".join(masking_criterion))
        return ws

    def _collect_data(self, sample, vanadium=False):
        """Loads data if the corresponding workspace does not exist in the ADS."""
        ws = "{}_{}".format(get_run_number(sample), "raw")
        kwargs = dict()
        if not self.getProperty("FlatBackgroundSource").isDefault:
            kwargs["FlatBkgWorkspace"] = self.flat_background
        kwargs[common.PROP_FLAT_BKG_SCALING] = self.flat_bkg_scaling
        kwargs[common.PROP_OUTPUT_FLAT_BKG_WS] = self.getPropertyValue(common.PROP_OUTPUT_FLAT_BKG_WS)
        if not self.getProperty(common.PROP_FLAT_BKG).isDefault:
            kwargs[common.PROP_FLAT_BKG] = self.getPropertyValue(common.PROP_FLAT_BKG)
            kwargs[common.PROP_FLAT_BKG_WINDOW] = self.getPropertyValue(common.PROP_FLAT_BKG_WINDOW)
        if not self.getProperty(common.PROP_DET_HOR_GROUPING).isDefault:
            kwargs[common.PROP_DET_HOR_GROUPING] = self.getProperty(common.PROP_DET_HOR_GROUPING).value
        if not self.getProperty(common.PROP_DET_VER_GROUPING).isDefault:
            kwargs[common.PROP_DET_VER_GROUPING] = self.getProperty(common.PROP_DET_VER_GROUPING).value
        if self.elastic_channel_ws is not None:
            kwargs["ElasticChannelWorkspace"] = self.elastic_channel_ws
        if not self.getProperty(common.PROP_ELASTIC_CHANNEL_MODE).isDefault:
            kwargs[common.PROP_ELASTIC_CHANNEL_MODE] = self.getPropertyValue(common.PROP_ELASTIC_CHANNEL_MODE)
        if not self.getProperty(common.PROP_EPP_METHOD).isDefault:
            kwargs[common.PROP_EPP_METHOD] = self.getPropertyValue(common.PROP_EPP_METHOD)
        if not self.getProperty(common.PROP_INCIDENT_ENERGY_CALIBRATION).isDefault:
            kwargs[common.PROP_INCIDENT_ENERGY_CALIBRATION] = self.getPropertyValue(common.PROP_INCIDENT_ENERGY_CALIBRATION)
        if vanadium:
            self.vanadium_epp = "{}_epp".format(ws)
            kwargs["OutputEPPWorkspace"] = self.vanadium_epp
            self.to_clean.append(self.vanadium_epp)
            self.vanadium_raw = "{}_raw".format(ws)
            self.to_clean.append(self.vanadium_raw)
            kwargs["OutputRawWorkspace"] = self.vanadium_raw
        kwargs[common.PROP_NORMALISATION] = self.getPropertyValue(common.PROP_NORMALISATION)
        kwargs[common.PROP_MON_PEAK_SIGMA_MULTIPLIER] = self.getPropertyValue(common.PROP_MON_PEAK_SIGMA_MULTIPLIER)
        if not self.clear_cache:
            kwargs[common.PROP_CLEANUP_MODE] = "Cleanup OFF"

        DirectILLCollectData(Run=sample, OutputWorkspace=ws, IncidentEnergyWorkspace=self.incident_energy_ws, **kwargs)
        instrument = mtd[ws].getInstrument().getName()
        if self.instrument and instrument != self.instrument:
            self.log().error(
                "Sample data: {} comes from different instruments that the rest of the data: {} and {}".format(
                    sample, instrument, self.instrument
                )
            )
        else:
            self.instrument = instrument
        return ws

    def _correct_self_attenuation(self, ws, sample_no):
        """Creates, if necessary, a self-attenuation workspace and uses it to correct the provided sample workspace."""
        if sample_no == 0:
            self._prepare_self_attenuation_ws(ws)
        if self.absorption_corr:
            corr_list = []
            for ws_name in mtd[self.absorption_corr].getNames():
                tmp_corr = "tmp_{}".format(ws_name)
                corr_list.append(tmp_corr)
                RebinToWorkspace(WorkspaceToRebin=ws_name, WorkspaceToMatch=ws, OutputWorkspace=tmp_corr)
            tmp_corr = "tmp_{}".format(self.absorption_corr)
            GroupWorkspaces(InputWorkspaces=corr_list, OutputWorkspace=tmp_corr)
            ApplyPaalmanPingsCorrection(SampleWorkspace=ws, OutputWorkspace=ws, CorrectionsWorkspace=tmp_corr)
            to_remove = [tmp_corr]
            if "corrected" in mtd:
                to_remove.append("corrected")
            DeleteWorkspaces(WorkspaceList=to_remove)

    @staticmethod
    def _clean_up(to_clean):
        """Performs the clean up of intermediate workspaces that are created and used throughout the code."""
        if len(to_clean) > 0:
            DeleteWorkspaces(WorkspaceList=to_clean)

    def _group_detectors(self, ws_list):
        """Groups detectors for workspaces in the provided list according to the defined grouping pattern."""
        grouping_pattern = None
        if not self.getProperty(common.PROP_DET_GROUPING).isDefault:
            grouping_pattern = self.getProperty(common.PROP_DET_GROUPING).value
        elif not self.getProperty(common.PROP_DET_GROUPING_BY).isDefault:
            grouping_pattern = common.get_grouping_pattern(mtd[ws_list[0]], self.getProperty(common.PROP_DET_GROUPING_BY).value)
        elif self.getProperty("ApplyGroupingBy").value and not (
            self.getProperty(common.PROP_DET_HOR_GROUPING).isDefault and self.getProperty(common.PROP_DET_VER_GROUPING).isDefault
        ):
            grouping_pattern = common.get_grouping_pattern(
                mtd[ws_list[0]], self.getProperty(common.PROP_DET_VER_GROUPING).value, self.getProperty(common.PROP_DET_HOR_GROUPING).value
            )
        if grouping_pattern is not None:
            for ws in ws_list:
                GroupDetectors(
                    InputWorkspace=ws,
                    OutputWorkspace=ws,
                    GroupingPattern=grouping_pattern,
                    Behaviour=self.getPropertyValue("GroupingBehaviour"),
                )
        return grouping_pattern is not None

    def _normalise_sample(self, sample_ws, sample_no, numor):
        """Normalises sample using vanadium integral, if it has been provided. Returns either a normalised sample
         or the input, if vanadium is not provided.

        Keyword arguments:
        sample_ws -- sample being processed
        sample_no -- index of the sample workspace in the group to be normalised
        numor -- numor string of the input sample workspace
        """
        normalised_ws = "{}_norm".format(numor)
        if self.vanadium_integral is not None and self.vanadium_integral != list():
            nintegrals = len(self.vanadium_integral)
            vanadium_no = sample_no
            if nintegrals == 1:
                vanadium_no = 0
            elif sample_no > nintegrals:
                vanadium_no = sample_no % nintegrals
            Divide(LHSWorkspace=sample_ws, RHSWorkspace=self.vanadium_integral[vanadium_no], OutputWorkspace=normalised_ws)
        else:
            normalised_ws = sample_ws
            self.log().warning("Vanadium integral workspace not found.\nData will not be normalised to vanadium.")

        return normalised_ws

    def _prepare_masks(self):
        """Builds a masking workspace from the provided inputs. Masking using threshold cannot be prepared ahead."""
        existing_masks = []
        mask = self.getPropertyValue("MaskWorkspace")
        if mask != str():
            mask = self.getPropertyValue("MaskWorkspace")
            if mask not in mtd:
                if ".nxs" in mask:
                    LoadNexus(Filename=mask, OutputWorkspace=mask)
                else:
                    LoadMask(Instrument=self.instrument, InputFile=mask, OutputWorkspace=mask)
            existing_masks.append(mask)
        mask_tubes = self.getPropertyValue("MaskedTubes")
        if mask_tubes != str():
            MaskBTP(Instrument=self.instrument, Tube=self.getPropertyValue(mask_tubes))
            tube_mask_ws = "{}_masked_tubes".format(self.instrument)
            RenameWorkspace(InputWorkspace="{}MaskBTP".format(self.instrument), OutputWorkspace=tube_mask_ws)
            existing_masks.append(tube_mask_ws)

        mask_angles = self.getProperty("MaskedAngles").value
        if mask_angles.any():
            masked_angles_ws = "{}_masked_angles".format(self.instrument)
            LoadEmptyInstrument(InstrumentName=self.instrument, OutputWorkspace=masked_angles_ws)
            MaskAngle(Workspace=masked_angles_ws, MinAngle=mask_angles[0], MaxAngle=mask_angles[1])
            existing_masks.append(masked_angles_ws)

        mask_ws = "mask_ws"
        if len(existing_masks) == 0:
            mask_ws = None
        elif len(existing_masks) > 1:
            MergeRuns(InputWorkspaces=existing_masks, OutputWorkspace=mask_ws)
        else:  # exactly one
            RenameWorkspace(InputWorkspace=existing_masks[0], OutputWorkspace=mask_ws)
        if mask_ws is not None:
            self.to_clean.append(mask_ws)
        return mask_ws

    def _prepare_self_attenuation_ws(self, ws):
        """Creates a self-attenuation workspace using either a MonteCarlo approach or numerical integration."""
        sample_geometry = self.getProperty("SampleGeometry").value
        sample_material = self.getProperty("SampleMaterial").value
        container_geometry = self.getProperty("ContainerGeometry").value if not self.getProperty("ContainerGeometry").isDefault else ""
        container_material = self.getProperty("ContainerMaterial").value if not self.getProperty("ContainerMaterial").isDefault else ""

        self.absorption_corr = "{}_abs_corr".format(ws)
        SetSample(
            InputWorkspace=ws,
            Geometry=sample_geometry,
            Material=sample_material,
            ContainerGeometry=container_geometry,
            ContainerMaterial=container_material,
        )
        if self.getProperty("SelfAttenuationMethod").value == "MonteCarlo":
            sparse_parameters = dict()
            if self.getPropertyValue("AbsorptionCorrection") == "Fast":
                sparse_parameters["SparseInstrument"] = True
                n_detector_rows = 5
                n_detector_cols = 10
                sparse_parameters["NumberOfDetectorRows"] = n_detector_rows
                sparse_parameters["NumberOfDetectorColumns"] = n_detector_cols
            # ensure the beam dimensions are always larger than the sample:
            if "Height" in sample_geometry:
                sparse_parameters["BeamHeight"] = 2 * float(sample_geometry["Height"].value)
            if "Width" in sample_geometry:
                size_property = "Width"
            elif "Radius" in sample_geometry:
                size_property = "Radius"
            else:  # Annulus
                size_property = "OuterRadius"
            sparse_parameters["BeamWidth"] = 2 * float(sample_geometry[size_property].value)
            PaalmanPingsMonteCarloAbsorption(InputWorkspace=ws, CorrectionsWorkspace=self.absorption_corr, **sparse_parameters)
        else:
            PaalmanPingsAbsorptionCorrection(InputWorkspace=ws, OutputWorkspace=self.absorption_corr)

    def _print_report(self):
        """Prints a summary report containing the most relevant information about the performed data reduction."""
        # removes the full path of the data source and also the .nxs extension:
        first_sample_numor = self.sample[0][self.sample[0].rfind("/") + 1 : -4]
        used_samples = "Sample runs: {}".format(first_sample_numor)
        if len(self.sample) > 1:
            last_sample_numor = self.sample[-1][self.sample[-1].rfind("/") + 1 : -4]
            used_samples = "{} to {}".format(used_samples, last_sample_numor)

        used_inputs = ["Used inputs:"]
        if self.cadmium:
            used_inputs.append("Cadmium: {}".format(self.cadmium))
        if self.empty:
            used_inputs.append("Empty container: {}".format(self.empty))
            if not self.getProperty("EmptyContainerScaling").isDefault:
                used_inputs.append("Scaled by: {}".format(self.getProperty("EmptyContainerScaling").value))
        if self.vanadium:
            used_inputs.append("Vanadium: {}".format(self.vanadium))
        if self.flat_background:
            used_inputs.append("Flat background source: {}".format(self.flat_background))
            if not self.getProperty(common.PROP_FLAT_BKG_SCALING).isDefault:
                used_inputs.append("Scaled by: {}".format(self.getProperty(common.PROP_FLAT_BKG_SCALING).value))
        if not self.getProperty("MaskWorkspace").isDefault:
            used_inputs.append("Mask workspace: {}".format(self.getPropertyValue("MaskWorkspace")))
        used_inputs = "\n".join(used_inputs) if len(used_inputs) > 1 else None

        incident_energy = mtd[self.output][0].getRun().getLogData("Ei").value
        sample_temperatures = ", ".join(["{:.2f}".format(value) for value in sorted(self.temperatures)])
        experimental_conditions = [
            "Processed as: {}".format(self.process),
            "Reduction technique: {}".format(self.reduction_type),
            "Incident energy: {:.1f} meV.".format(incident_energy),
            "Sample temperature(s): {} K".format(sample_temperatures),
        ]
        experimental_conditions = "\n".join(experimental_conditions)
        self.log().notice("Summary report")
        self.log().notice(used_samples)
        if used_inputs is not None:
            self.log().notice(used_inputs)
        self.log().notice(experimental_conditions)

    def _process_sample(self, ws, sample_no):
        """Does the sample data reduction for single crystal."""
        to_remove = [ws]
        if self.empty:
            self._subtract_empty_container(ws, sample_no)
        if self.masking:
            ws = self._apply_mask(ws)
        numor = ws[: ws.rfind("_")]
        processed_sample_tw = None
        if self.reduction_type == "SingleCrystal":
            if self.getPropertyValue("AbsorptionCorrection") != "None":
                self._correct_self_attenuation(ws, sample_no)
            # normalises to vanadium integral
            normalised_ws = self._normalise_sample(ws, sample_no, numor)
            to_remove.append(normalised_ws)
            # converts to energy
            corrected_ws = "{}_ene".format(numor)
            ConvertUnits(
                InputWorkspace=normalised_ws, EFixed=self.incident_energy, Target="DeltaE", EMode="Direct", OutputWorkspace=corrected_ws
            )
            to_remove.append(corrected_ws)

            # transforms the distribution into dynamic structure factor
            CorrectKiKf(InputWorkspace=corrected_ws, EFixed=self.incident_energy, OutputWorkspace=corrected_ws)

            # corrects for detector efficiency
            DetectorEfficiencyCorUser(InputWorkspace=corrected_ws, IncidentEnergy=self.incident_energy, OutputWorkspace=corrected_ws)

            # rebin in energy or momentum transfer
            processed_sample = "{}_reb".format(numor)
            if not self.getProperty(common.PROP_REBINNING_PARAMS_W).isDefault:
                Rebin(
                    InputWorkspace=corrected_ws,
                    OutputWorkspace=processed_sample,
                    Params=self.getPropertyValue(common.PROP_REBINNING_PARAMS_W),
                )
            else:
                RenameWorkspace(InputWorkspace=corrected_ws, OutputWorkspace=processed_sample)
                to_remove.pop()
            self._group_detectors([processed_sample])
        else:
            processed_sample = "SofQW_{}".format(numor)  # name should contain only SofQW and numor
            processed_sample_tw = "SofTW_{}".format(numor)  # name should contain only SofTW and numor
            if self.getPropertyValue("AbsorptionCorrection") != "None":
                self._correct_self_attenuation(ws, sample_no)
            vanadium_integral = self.vanadium_integral[0] if self.vanadium_integral else ""
            vanadium_diagnostics = self.vanadium_diagnostics[0] if self.vanadium_diagnostics else ""
            optional_parameters = dict()
            if not self.getProperty(common.PROP_GROUPING_ANGLE_STEP).isDefault:
                optional_parameters["GroupingAngleStep"] = self.getProperty(common.PROP_GROUPING_ANGLE_STEP).value
            if not self.getProperty(common.PROP_REBINNING_W).isDefault:
                optional_parameters[common.PROP_REBINNING_W] = self.getProperty(common.PROP_REBINNING_W).value
            if not self.getProperty(common.PROP_REBINNING_PARAMS_W).isDefault:
                optional_parameters[common.PROP_REBINNING_PARAMS_W] = self.getProperty(common.PROP_REBINNING_PARAMS_W).value
            if not self.getProperty("MomentumTransferBinning").isDefault:
                optional_parameters["QBinningParams"] = self.getProperty("MomentumTransferBinning").value
            if not self.getProperty("VanadiumWorkspace").isDefault:
                optional_parameters["IntegratedVanadiumWorkspace"] = vanadium_integral
                if self.getProperty("MaskWithVanadium").value:
                    optional_parameters["DiagnosticsWorkspace"] = vanadium_diagnostics

            DirectILLReduction(
                InputWorkspace=ws,
                OutputWorkspace=processed_sample,
                OutputSofThetaEnergyWorkspace=processed_sample_tw,
                AbsoluteUnitsNormalisation=self.getProperty(common.PROP_ABSOLUTE_UNITS).value,
                **optional_parameters,
            )
        if len(to_remove) > 0 and self.clear_cache:
            self._clean_up(to_remove)

        return processed_sample, processed_sample_tw

    def _process_vanadium(self, ws):
        """Processes vanadium and creates workspaces with diagnostics, integrated vanadium, and reduced vanadium."""
        to_remove = [ws]
        numor = ws[: ws.rfind("_")]
        if self.masking:
            ws = self._apply_mask(ws)

        vanadium_diagnostics = "diag_{}".format(numor)
        kwargs = dict()
        if self.vanadium_epp:
            kwargs[common.PROP_EPP_WS] = self.vanadium_epp
        DirectILLDiagnostics(InputWorkspace=self.vanadium_raw, OutputWorkspace=vanadium_diagnostics, **kwargs)

        if self.empty:
            self._subtract_empty_container(ws)

        vanadium_integral = "integral_{}".format(numor)
        DirectILLIntegrateVanadium(InputWorkspace=ws, OutputWorkspace=vanadium_integral, EPPWorkspace=self.vanadium_epp)

        if self.getPropertyValue("AbsorptionCorrection") != "None":
            self._correct_self_attenuation(ws, 0)

        sofqw_output = "SofQW_{}".format(numor)
        softw_output = "SofTW_{}".format(numor)
        optional_parameters = dict()
        if not self.getProperty(common.PROP_GROUPING_ANGLE_STEP).isDefault:
            optional_parameters["GroupingAngleStep"] = self.getProperty(common.PROP_GROUPING_ANGLE_STEP).value
        if not self.getProperty(common.PROP_REBINNING_W).isDefault:
            optional_parameters[common.PROP_REBINNING_W] = self.getProperty(common.PROP_REBINNING_W).value
        if not self.getProperty(common.PROP_REBINNING_PARAMS_W).isDefault:
            optional_parameters[common.PROP_REBINNING_PARAMS_W] = self.getProperty(common.PROP_REBINNING_PARAMS_W).value
        if not self.getProperty("MomentumTransferBinning").isDefault:
            optional_parameters["QBinningParams"] = self.getProperty("MomentumTransferBinning").value
        DirectILLReduction(
            InputWorkspace=ws,
            OutputWorkspace=sofqw_output,
            OutputSofThetaEnergyWorkspace=softw_output,
            DiagnosticsWorkspace=vanadium_diagnostics,
            **optional_parameters,
        )

        if len(to_remove) > 0 and self.clear_cache:
            self._clean_up(to_remove)
        return sofqw_output, softw_output, vanadium_diagnostics, vanadium_integral

    def _rename_workspaces(self, ws_list):
        """Renames workspaces in the provided list by appending a custom suffix containing the user-defined output
        group name, incident energy, and sample temperature (for powder reduction only)."""
        output_group_name = self.output
        new_ws_list = []
        temp_log_name = "sample.setpoint_temperature"
        for name in ws_list:
            run = mtd[name].getRun()
            ei = run.getLogData("Ei").value
            ws_name = name
            if "raw" in name:  # remove 'raw' from the output name
                ws_name = "{}{}".format(name[: name.find("raw") - 1], name[name.find("raw") + 4 :])
            new_name = "{}_{}_Ei{:.0f}meV".format(output_group_name, ws_name, ei)
            temp = run.getLogData(temp_log_name).value
            if isinstance(temp, np.ndarray):
                for t in temp:
                    self.temperatures.add(t)
                temp = temp[0]
            else:
                self.temperatures.add(temp)
            if self.reduction_type == "Powder":
                new_name = "{}_T{:.1f}K".format(new_name, temp)
            RenameWorkspace(InputWorkspace=name, OutputWorkspace=new_name)
            new_ws_list.append(new_name)

        return new_ws_list

    def _save_output(self, ws_to_save):
        """Saves the output workspaces to an external file."""
        for ws_name in ws_to_save:
            if self.process == "Vanadium" and "diag" in ws_name:
                SaveMask(InputWorkspace=ws_name, OutputFile=ws_name)
                continue
            if self.reduction_type == "SingleCrystal":
                omega_log = "SRot.value"  # IN5, IN6
                if self.instrument == "PANTHER":
                    omega_log = "a3.value"
                elif self.instrument == "SHARP":
                    omega_log = "srotation.value"
                elif self.instrument == "IN4":
                    omega_log = "SampleRotation.value"
                psi = mtd[ws_name].run().getProperty(omega_log).value
                psi_offset = self.getProperty("SampleAngleOffset").value
                offset = psi - psi_offset
                SaveNXSPE(InputWorkspace=ws_name, Filename="{}.nxspe".format(ws_name), Psi=offset)
            else:  # powder reduction
                if "SofTW" in ws_name:
                    SaveNXSPE(InputWorkspace=ws_name, Filename="{}.nxspe".format(ws_name))
                else:
                    SaveNexus(InputWorkspace=ws_name, Filename="{}.nxs".format(ws_name))

    def _subtract_cadmium(self, ws):
        """Subtracts cadmium counts from the current input workspace."""
        Minus(LHSWorkspace=ws, RHSWorkspace=mtd[self.cadmium][0], OutputWorkspace=ws)

    def _subtract_empty_container(self, ws, sample_no=0):
        """Subtracts empty container counts from the input workspace."""
        empty_ws = self.getPropertyValue("EmptyContainerWorkspace")
        empty_correction_ws = "{}_correction".format(empty_ws)
        empty_scaling = self.getProperty("EmptyContainerScaling").value
        if empty_scaling != 1.0:
            Scale(InputWorkspace=empty_ws, OutputWorkspace=empty_correction_ws, Factor=empty_scaling)
        else:
            empty_correction_ws = empty_ws
        # match correct empty container workspace to the sample
        empty_no = sample_no
        empty_entries = len(mtd[empty_correction_ws])
        if empty_entries == 1:
            empty_no = 0
        elif sample_no > empty_entries:
            empty_no = sample_no % empty_entries
        try:
            Minus(
                LHSWorkspace=ws,
                RHSWorkspace=mtd[empty_correction_ws][empty_no],
                OutputWorkspace=ws,
                EnableLogging=False,  # suppressed to avoid logging error cases treated below
            )
        except ValueError:
            # case when workspaces X-axes don't match
            tmp_empty = "{}_tmp".format(mtd[empty_correction_ws][empty_no])
            RebinToWorkspace(WorkspaceToRebin=mtd[empty_correction_ws][empty_no], WorkspaceToMatch=ws, OutputWorkspace=tmp_empty)
            Minus(LHSWorkspace=ws, RHSWorkspace=tmp_empty, OutputWorkspace=ws)
            DeleteWorkspace(Workspace=tmp_empty)
        if self.clear_cache and empty_scaling != 1:
            DeleteWorkspace(Workspace=empty_correction_ws)


AlgorithmFactory.subscribe(DirectILLAutoProcess)
