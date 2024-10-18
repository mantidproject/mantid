# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    MultipleFileProperty,
    PropertyMode,
    Progress,
    WorkspaceGroupProperty,
    FileAction,
    WorkspaceGroup,
)
from mantid.kernel import logger, Direction, FloatBoundedValidator, FloatArrayProperty, IntBoundedValidator
from mantid.simpleapi import (
    CalculateEfficiency,
    CloneWorkspace,
    ConvertToPointData,
    CropWorkspace,
    DeleteWorkspace,
    GroupWorkspaces,
    LoadNexusProcessed,
    UnGroupWorkspace,
    RenameWorkspace,
    ReplaceSpecialValues,
    SANSILLIntegration,
    SANSILLReduction,
    Stitch,
)
import SANSILLCommon as common
import numpy as np
from os import path

EMPTY_TOKEN = "000000"


def needs_processing(property_value, process_reduction_type):
    """
    Checks whether a given unary reduction needs processing or is already cached
    in ADS with expected name.
    @param property_value: the string value of the corresponding MultipleFile
                           input property
    @param process_reduction_type: the reduction_type of process
    """
    do_process = False
    ws_name = ""
    if property_value:
        run_number = common.get_run_number(property_value)
        ws_name = run_number + "_" + process_reduction_type
        if mtd.doesExist(ws_name):
            if isinstance(mtd[ws_name], WorkspaceGroup):
                run = mtd[ws_name][0].getRun()
            else:
                run = mtd[ws_name].getRun()
            if run.hasProperty("ProcessedAs"):
                process = run.getLogData("ProcessedAs").value
                if process == process_reduction_type:
                    logger.notice("Reusing {0} workspace: {1}".format(process_reduction_type, ws_name))
                else:
                    logger.warning("{0} workspace found, but processed " "differently: {1}".format(process_reduction_type, ws_name))
                    do_process = True
            else:
                logger.warning("{0} workspace found, but missing the " "ProcessedAs flag: {1}".format(process_reduction_type, ws_name))
                do_process = True
        else:
            do_process = True
    return [do_process, ws_name]


def needs_loading(property_value, loading_reduction_type):
    """
    Checks whether a given unary input needs loading or is already loaded in
    ADS.
    @param property_value: the string value of the corresponding FileProperty
    @param loading_reduction_type : the reduction_type of input to load
    """
    loading = False
    ws_name = ""
    if property_value:
        ws_name = path.splitext(path.basename(property_value))[0]
        if mtd.doesExist(ws_name):
            logger.notice("Reusing {0} workspace: {1}".format(loading_reduction_type, ws_name))
        else:
            loading = True
    return [loading, ws_name]


class SANSILLAutoProcess(DataProcessorAlgorithm):
    """
    Performs complete treatment of ILL SANS data; instruments D11, D11B, D16, D22, D22B, D33.
    """

    progress = None
    reduction_type = None
    sample = None
    absorber = None
    beam = None
    container = None
    stransmission = None
    ctransmission = None
    btransmission = None
    atransmission = None
    sensitivity = None
    mask = None
    flux = None
    default_mask = None
    output = None
    output_sens = None
    dimensionality = None
    reference = None
    normalise = None
    radius = None
    thickness = None
    theta_dependent = None
    solvent = None
    n_wedges = None

    def category(self):
        return "ILL\\SANS;ILL\\Auto"

    def summary(self):
        return "Performs complete SANS data reduction at the ILL."

    def seeAlso(self):
        return [
            "SANSILLReduction",
            "SANSILLIntegration",
        ]

    def name(self):
        return "SANSILLAutoProcess"

    def validateInputs(self):
        self.setUp()

        result = dict()
        message = "Wrong number of {0} runs: {1}. Provide one or as many as " "sample runs: {2}."
        message_value = "Wrong number of {0} values: {1}. Provide one or as " "many as sample runs: {2}."

        # array parameters checks
        sample_dim = len(self.sample)
        abs_dim = len(self.absorber)
        beam_dim = len(self.beam)
        flux_dim = len(self.flux)
        can_dim = len(self.container)
        mask_dim = len(self.mask)
        sens_dim = len(self.sensitivity)
        ref_dim = len(self.reference)
        solv_dim = len(self.solvent)
        maxqxy_dim = len(self.maxqxy)
        deltaq_dim = len(self.deltaq)
        radius_dim = len(self.radius)

        qvalid = self.validateQRanges()
        if qvalid:
            result["OutputBinning"] = qvalid

        if self.getPropertyValue("SampleRuns") == "":
            result["SampleRuns"] = "Please provide at least one sample run."
        if abs_dim != sample_dim and abs_dim > 1:
            result["AbsorberRuns"] = message.format("Absorber", abs_dim, sample_dim)
        if beam_dim != sample_dim and beam_dim > 1:
            result["BeamRuns"] = message.format("Beam", beam_dim, sample_dim)
        if can_dim != sample_dim and can_dim > 1:
            result["ContainerRuns"] = message.format("Container", can_dim, sample_dim)
        if mask_dim != sample_dim and mask_dim > 1:
            result["MaskFiles"] = message.format("Mask", mask_dim, sample_dim)
        if ref_dim != sample_dim and ref_dim > 1:
            result["ReferenceFiles"] = message.format("Reference", ref_dim, sample_dim)
        if sens_dim != sample_dim and sens_dim > 1:
            result["SensitivityMaps"] = message.format("Sensitivity", sens_dim, sample_dim)
        if flux_dim != sample_dim and flux_dim > 1:
            result["FluxRuns"] = message.format("Flux", flux_dim, sample_dim)
        if maxqxy_dim != sample_dim and maxqxy_dim > 1:
            result["MaxQxy"] = message_value.format("MaxQxy", maxqxy_dim, sample_dim)
        if deltaq_dim != sample_dim and deltaq_dim > 1:
            result["DeltaQ"] = message_value.format("DeltaQ", deltaq_dim, sample_dim)
        if radius_dim != sample_dim and radius_dim > 1:
            result["BeamRadius"] = message_value.format("BeamRadius", radius_dim, sample_dim)
        if solv_dim != sample_dim and solv_dim > 1:
            result["SolventFiles"] = message.format("Solvent", solv_dim, sample_dim)

        # transmission runs checks
        str_dim = len(self.stransmission.split(","))
        ctr_dim = len(self.ctransmission.split(","))
        btr_dim = len(self.btransmission.split(","))
        atr_dim = len(self.atransmission.split(","))
        if str_dim != sample_dim and str_dim != 1:
            result["SampleTransmissionRuns"] = message.format("SampleTransmission", str_dim, sample_dim)
        if ctr_dim != can_dim and ctr_dim != 1:
            result["ContainerTransmissionRuns"] = message.format("ContainerTransmission", ctr_dim, can_dim)
        if (btr_dim != sample_dim or btr_dim != can_dim) and btr_dim != 1:
            result["TransmissionBeamRuns"] = message.format("TransmissionBeam", btr_dim, sample_dim)
        if (atr_dim != sample_dim or atr_dim != can_dim) and atr_dim != 1:
            result["TransmissionAbsorberRuns"] = message.format("TransmissionAbsorber", atr_dim, sample_dim)

        # other checks
        if self.output_type == "I(Phi,Q)" and self.n_wedges == 0:
            result["NumberOfWedges"] = "For I(Phi,Q) processing, the number " "of wedges must be different from 0."

        if self.getPropertyValue("OutputWorkspace")[0].isdigit():
            result["OutputWorkspace"] = "Output workspace name must be alphanumeric, it should start with a letter."

        return result

    def validateQRanges(self):
        retval = ""
        qbinning = self.getPropertyValue("OutputBinning")
        if qbinning:
            n_items = qbinning.count(":") + 1
            n_samples = len(self.sample)
            if n_items != n_samples and n_items != 1:
                retval = f"Number of Q binning params must be equal to the number of distances; found {n_items} instead of {n_samples}."
            else:
                for qbin_params in qbinning.split(":"):
                    if qbin_params:
                        for qbin_param in qbin_params.split(","):
                            try:
                                float(qbin_param)
                            except ValueError:
                                retval = "Q binning params must be float."
                                break
        return retval

    def setUp(self):
        self.sample = self.getPropertyValue("SampleRuns").split(",")
        self.absorber = self.getPropertyValue("AbsorberRuns").split(",")
        self.beam = self.getPropertyValue("BeamRuns").split(",")
        self.flux = self.getPropertyValue("FluxRuns").split(",")
        self.container = self.getPropertyValue("ContainerRuns").split(",")
        self.stransmission = self.getPropertyValue("SampleTransmissionRuns")
        self.ctransmission = self.getPropertyValue("ContainerTransmissionRuns")
        self.btransmission = self.getPropertyValue("TransmissionBeamRuns")
        self.atransmission = self.getPropertyValue("TransmissionAbsorberRuns")
        self.sensitivity = self.getPropertyValue("SensitivityMaps").replace(" ", "").split(",")
        self.default_mask = self.getPropertyValue("DefaultMaskFile")
        self.mask = self.getPropertyValue("MaskFiles").replace(" ", "").split(",")
        self.reference = self.getPropertyValue("ReferenceFiles").replace(" ", "").split(",")
        self.solvent = self.getPropertyValue("SolventFiles").replace(" ", "").split(",")
        self.output = self.getPropertyValue("OutputWorkspace")
        self.output_panels = self.output + "_panels"
        self.output_sens = self.getPropertyValue("SensitivityOutputWorkspace")
        self.normalise = self.getPropertyValue("NormaliseBy")
        self.theta_dependent = self.getProperty("ThetaDependent").value
        self.tr_radius = self.getProperty("TransmissionBeamRadius").value
        self.radius = self.getPropertyValue("BeamRadius").split(",")
        self.dimensionality = len(self.sample)
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10 * self.dimensionality)
        self.cleanup = self.getProperty("ClearCorrected2DWorkspace").value
        self.n_wedges = self.getProperty("NumberOfWedges").value
        self.maxqxy = self.getPropertyValue("MaxQxy").split(",")
        self.deltaq = self.getPropertyValue("DeltaQ").split(",")
        self.output_type = self.getPropertyValue("OutputType")
        self.stitch_reference_index = self.getProperty("StitchReferenceIndex").value

    def PyInit(self):
        self.declareProperty(
            WorkspaceGroupProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The output workspace group containing reduced data.",
        )

        self.declareProperty(
            MultipleFileProperty("SampleRuns", action=FileAction.OptionalLoad, extensions=["nxs"], allow_empty=True), doc="Sample run(s)."
        )

        self.declareProperty(
            MultipleFileProperty("AbsorberRuns", action=FileAction.OptionalLoad, extensions=["nxs"]), doc="Absorber (Cd/B4C) run(s)."
        )

        self.declareProperty(MultipleFileProperty("BeamRuns", action=FileAction.OptionalLoad, extensions=["nxs"]), doc="Empty beam run(s).")

        self.declareProperty(
            MultipleFileProperty("FluxRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Empty beam run(s) for flux calculation only; " "if left blank flux will be calculated from BeamRuns.",
        )

        self.declareProperty(
            MultipleFileProperty("ContainerRuns", action=FileAction.OptionalLoad, extensions=["nxs"]), doc="Empty container run(s)."
        )

        self.setPropertyGroup("SampleRuns", "Numors")
        self.setPropertyGroup("AbsorberRuns", "Numors")
        self.setPropertyGroup("BeamRuns", "Numors")
        self.setPropertyGroup("FluxRuns", "Numors")
        self.setPropertyGroup("ContainerRuns", "Numors")

        self.declareProperty(
            MultipleFileProperty("SampleTransmissionRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Sample transmission run(s).",
        )

        self.declareProperty(
            MultipleFileProperty("ContainerTransmissionRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Container transmission run(s).",
        )

        self.declareProperty(
            MultipleFileProperty("TransmissionBeamRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Empty beam run(s) for transmission.",
        )

        self.declareProperty(
            MultipleFileProperty("TransmissionAbsorberRuns", action=FileAction.OptionalLoad, extensions=["nxs"]),
            doc="Absorber (Cd/B4C) run(s) for transmission.",
        )

        self.setPropertyGroup("SampleTransmissionRuns", "Transmissions")
        self.setPropertyGroup("ContainerTransmissionRuns", "Transmissions")
        self.setPropertyGroup("TransmissionBeamRuns", "Transmissions")
        self.setPropertyGroup("TransmissionAbsorberRuns", "Transmissions")
        self.copyProperties("SANSILLReduction", ["ThetaDependent"], version=1)
        self.setPropertyGroup("ThetaDependent", "Transmissions")

        self.declareProperty("SensitivityMaps", "", doc="File(s) or workspaces containing the maps of relative detector efficiencies.")

        self.declareProperty(
            "DefaultMaskFile",
            "",
            doc="File or workspace containing the default mask (typically the detector edges and dead pixels/tubes)"
            " to be applied to all the detector configurations.",
        )

        self.declareProperty("MaskFiles", "", doc="File(s) or workspaces containing the detector mask (typically beam stop).")

        self.declareProperty(
            "ReferenceFiles", "", doc="File(s) or workspaces containing the corrected water data (in 2D) for absolute normalisation."
        )

        self.declareProperty(
            "SolventFiles", "", doc="File(s) or workspaces containing the corrected solvent data (in 2D) for solvent subtraction."
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SensitivityOutputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The output sensitivity map workspace.",
        )

        self.copyProperties("SANSILLReduction", ["NormaliseBy"], version=1)

        self.declareProperty("SampleThickness", 0.1, validator=FloatBoundedValidator(lower=-1), doc="Sample thickness [cm]")

        self.declareProperty(
            "TransmissionBeamRadius",
            0.1,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Beam radius [m]; used for transmission " "calculations.",
        )

        self.declareProperty(
            FloatArrayProperty("BeamRadius", values=[0.1]), doc="Beam radius [m]; used for beam center " "finding and flux calculations."
        )

        self.declareProperty(
            "WaterCrossSection", 1.0, doc="Provide water cross-section; " "used only if the absolute scale is done by dividing to water."
        )

        self.setPropertyGroup("SensitivityMaps", "Options")
        self.setPropertyGroup("DefaultMaskFile", "Options")
        self.setPropertyGroup("MaskFiles", "Options")
        self.setPropertyGroup("ReferenceFiles", "Options")
        self.setPropertyGroup("SolventFiles", "Options")
        self.setPropertyGroup("SensitivityOutputWorkspace", "Options")
        self.setPropertyGroup("NormaliseBy", "Options")
        self.setPropertyGroup("SampleThickness", "Options")
        self.setPropertyGroup("BeamRadius", "Options")
        self.setPropertyGroup("TransmissionBeamRadius", "Options")
        self.setPropertyGroup("WaterCrossSection", "Options")

        self.declareProperty(FloatArrayProperty("MaxQxy", values=[-1]), doc="Maximum of absolute Qx and Qy.")
        self.declareProperty(FloatArrayProperty("DeltaQ", values=[-1]), doc="The dimension of a Qx-Qy cell.")

        self.declareProperty("OutputPanels", False, doc="Whether or not process the individual " "detector panels.")

        self.copyProperties(
            "SANSILLIntegration",
            [
                "OutputType",
                "CalculateResolution",
                "DefaultQBinning",
                "BinningFactor",
                "NPixelDivision",
                "NumberOfWedges",
                "WedgeAngle",
                "WedgeOffset",
                "AsymmetricWedges",
                "IQxQyLogBinning",
                "WavelengthRange",
            ],
        )

        self.declareProperty("OutputBinning", "", doc="Output binning for each distance. : separated list of binning params.")
        self.setPropertyGroup("OutputBinning", "Integration Options")
        self.setPropertyGroup("OutputType", "Integration Options")
        self.setPropertyGroup("CalculateResolution", "Integration Options")
        self.declareProperty("ClearCorrected2DWorkspace", True, "Whether to clear the fully corrected 2D workspace.")

        self.declareProperty(
            "SensitivityWithOffsets", False, "Whether the sensitivity data has been measured with different horizontal offsets."
        )

        self.declareProperty(
            "StitchReferenceIndex",
            defaultValue=1,
            validator=IntBoundedValidator(lower=0),
            doc="Index of reference workspace during stitching.",
        )

        self.copyProperties("SANSILLIntegration", ["ShapeTable"])

        self.copyProperties("SANSILLReduction", "Wavelength", 1)

        self.copyProperties("CalculateEfficiency", ["MinThreshold", "MaxThreshold"])
        # override default documentation of copied parameters to make them understandable by user
        threshold_property = self.getProperty("MinThreshold")
        threshold_property.setDocumentation("Minimum threshold for calculated efficiency.")
        threshold_property = self.getProperty("MaxThreshold")
        threshold_property.setDocumentation("Maximum threshold for calculated efficiency.")

        self.setPropertyGroup("MinThreshold", "Efficiency Options")
        self.setPropertyGroup("MaxThreshold", "Efficiency Options")

    def PyExec(self):
        self.setUp()
        outputSamples = []
        outputWedges = []
        outputPanels = []
        outputSens = []

        container_transmission, sample_transmission = self.processTransmissions()

        for d in range(self.dimensionality):
            if self.sample[d] != EMPTY_TOKEN:
                absorber = self.processAbsorber(d)
                flux = self.processFlux(d, absorber)
                if flux:
                    beam, _ = self.processBeam(d, absorber)
                else:
                    beam, flux = self.processBeam(d, absorber)
                container = self.processContainer(d, beam, absorber, container_transmission)
                sample, wedges, panels, sensitivity = self.processSample(d, flux, sample_transmission, beam, absorber, container)
                outputSamples.append(sample)

                if panels:
                    outputPanels.append(panels)
                if wedges:
                    outputWedges.append(wedges)
                if sensitivity:
                    outputSens.append(sensitivity)

            else:
                self.log().information("Skipping empty token run.")

        # rename to a user friendly naming scheme
        for i in range(len(outputSamples)):
            if isinstance(mtd[outputSamples[i]], WorkspaceGroup):  # kinetic
                for ws in mtd[outputSamples[i]]:
                    ws_name = ws.getName()
                    suffix = self.createCustomSuffix(ws_name)
                    RenameWorkspace(InputWorkspace=ws_name, OutputWorkspace=ws_name + suffix)
            else:
                suffix = self.createCustomSuffix(outputSamples[i])
                RenameWorkspace(InputWorkspace=outputSamples[i], OutputWorkspace=outputSamples[i] + suffix)
                outputSamples[i] += suffix

        # try to stitch automatically
        if len(outputSamples) > 1 and self.getPropertyValue("OutputType") == "I(Q)":
            try:
                stitched = f"{self.output}_stitched"
                stitch_params_ws = f"{self.output}_stitch_scale_factors"
                Stitch(
                    InputWorkspaces=outputSamples,
                    OutputWorkspace=stitched,
                    ReferenceWorkspace=outputSamples[self.stitch_reference_index],
                    OutputScaleFactorsWorkspace=stitch_params_ws,
                )
                mtd[stitched].getRun().addProperty("stitch_scale_factors", list(mtd[stitch_params_ws].readY(0)), True)
                DeleteWorkspace(stitch_params_ws)
                outputSamples.append(stitched)
            except RuntimeError as re:
                self.log().warning("Unable to stitch automatically, consider " "stitching manually: " + str(re))

        GroupWorkspaces(InputWorkspaces=outputSamples, OutputWorkspace=self.output)
        self.set_distribution(outputSamples)
        self.setProperty("OutputWorkspace", mtd[self.output])

        if outputWedges:
            self.outputWedges(outputWedges)

        if outputSens:
            self.outputSensitivity(outputSens)

        if outputPanels:
            self.set_distribution(outputPanels)
            self.outputPanels(outputPanels)

    def set_distribution(self, wslist):
        for ws in wslist:
            if isinstance(mtd[ws], WorkspaceGroup):
                for wsi in mtd[ws]:
                    wsi.setDistribution(True)
            else:
                mtd[ws].setDistribution(True)

    def outputWedges(self, outputWedges):
        # ungroup and regroup wedge outputs per wedge
        for wg in outputWedges:
            UnGroupWorkspace(wg)
        for w in range(self.n_wedges):
            group_name = self.output + "_wedge_" + str(w + 1)
            to_group_all = [self.output + "_wedge_#" + str(i + 1) + "_" + str(w + 1) for i in range(self.dimensionality)]
            to_group = []
            for ws_name in to_group_all:
                if mtd.doesExist(ws_name):
                    to_group.append(ws_name)
            if to_group:
                GroupWorkspaces(InputWorkspaces=to_group, OutputWorkspace=group_name)

        for i in range(self.dimensionality):
            for w in range(self.n_wedges):
                old_name = self.output + "_wedge_#" + str(i + 1) + "_" + str(w + 1)
                new_name = self.output + "_wedge_" + str(w + 1) + "_#" + str(i + 1)
                if mtd.doesExist(old_name):
                    RenameWorkspace(InputWorkspace=old_name, OutputWorkspace=new_name)
                    ConvertToPointData(InputWorkspace=new_name, OutputWorkspace=new_name)
                    ReplaceSpecialValues(InputWorkspace=new_name, OutputWorkspace=new_name, NaNValue=0)
                    y = mtd[new_name].readY(0)
                    x = mtd[new_name].readX(0)
                    nonzero = np.nonzero(y)
                    CropWorkspace(InputWorkspace=new_name, XMin=x[nonzero][0] - 1, XMax=x[nonzero][-1], OutputWorkspace=new_name)
                    suffix = self.createCustomSuffix(new_name)
                    RenameWorkspace(InputWorkspace=new_name, OutputWorkspace=new_name + suffix)

        for w in range(self.n_wedges):
            group_name = self.output + "_wedge_" + str(w + 1)
            if mtd.doesExist(group_name) and mtd[group_name].getNumberOfEntries() > 1:
                stitched = group_name + "_stitched"
                try:
                    Stitch(
                        InputWorkspaces=group_name,
                        OutputWorkspace=stitched,
                        ReferenceWorkspace=mtd[group_name][self.stitch_reference_index].getName(),
                    )
                    GroupWorkspaces(InputWorkspaces=[group_name, stitched], OutputWorkspace=group_name)
                except RuntimeError as re:
                    self.log().warning("Unable to stitch automatically, consider " "stitching manually: " + str(re))

        self.set_distribution([group_name])

    def outputSensitivity(self, sensitivity_outputs):
        if len(sensitivity_outputs) > 1:
            GroupWorkspaces(InputWorkspaces=sensitivity_outputs, OutputWorkspace=self.output_sens)
        if self.getProperty("SensitivityWithOffsets").value:
            tmp_group_name = self.output_sens + "_group"
            RenameWorkspace(InputWorkspace=self.output_sens, OutputWorkspace=tmp_group_name)
            CalculateEfficiency(
                InputWorkspace=tmp_group_name,
                MergeGroup=True,
                MinThreshold=self.getProperty("MinThreshold").value,
                MaxThreshold=self.getProperty("MaxThreshold").value,
                OutputWorkspace=self.output_sens,
            )
            DeleteWorkspace(Workspace=tmp_group_name)
        self.setProperty("SensitivityOutputWorkspace", mtd[self.output_sens])

    def outputPanels(self, panel_output_groups):
        panel_names = set()
        for groupName in panel_output_groups:
            if mtd.doesExist(groupName):
                old_names = mtd[groupName].getNames()
                UnGroupWorkspace(InputWorkspace=groupName)
                for old_name in old_names:
                    hash_suffix = old_name.split("#")[1]
                    panel_name_start = hash_suffix.find("_")
                    panel_name = hash_suffix[panel_name_start + 1 :]
                    panel_names.add(panel_name)
                    distance_mark = hash_suffix[:panel_name_start]
                    new_name = self.output + "_" + panel_name + "_#" + distance_mark
                    if mtd.doesExist(old_name):
                        RenameWorkspace(InputWorkspace=old_name, OutputWorkspace=new_name)
                        suffix = self.createCustomSuffix(new_name)
                        RenameWorkspace(InputWorkspace=new_name, OutputWorkspace=new_name + suffix)

        for panel_name in panel_names:
            GroupWorkspaces(GlobExpression=self.output + "_" + panel_name + "_*", OutputWorkspace=self.output + "_" + panel_name)

    def processTransmissions(self):
        absorber_transmission_names = []
        beam_transmission_names = []
        container_transmission_names = []
        sample_transmission_names = []
        for absorber in self.atransmission.split(","):
            [process_transmission_absorber, transmission_absorber_name] = needs_processing(absorber, "Absorber")
            absorber_transmission_names.append(transmission_absorber_name)
            self.progress.report("Processing transmission absorber")
            if process_transmission_absorber:
                SANSILLReduction(
                    Run=absorber,
                    ProcessAs="Absorber",
                    NormaliseBy=self.normalise,
                    OutputWorkspace=transmission_absorber_name,
                    Wavelength=self.getProperty("Wavelength").value,
                    Version=1,
                )
        for beam_no, beam in enumerate(self.btransmission.split(",")):
            [process_transmission_beam, transmission_beam_name] = needs_processing(beam, "Beam")
            beam_transmission_names.append(transmission_beam_name)
            flux_name = transmission_beam_name + "_Flux"
            if len(absorber_transmission_names) > 1:
                transmission_absorber_name = absorber_transmission_names[beam_no]
            else:
                transmission_absorber_name = absorber_transmission_names[0]
            self.progress.report("Processing transmission beam")
            if process_transmission_beam:
                SANSILLReduction(
                    Run=beam,
                    ProcessAs="Beam",
                    NormaliseBy=self.normalise,
                    OutputWorkspace=transmission_beam_name,
                    BeamRadius=self.tr_radius,
                    FluxOutputWorkspace=flux_name,
                    AbsorberInputWorkspace=transmission_absorber_name,
                    Wavelength=self.getProperty("Wavelength").value,
                    Version=1,
                )
        for transmission_no, transmission in enumerate(self.ctransmission.split(",")):
            [process_container_transmission, container_transmission_name] = needs_processing(transmission, "Transmission")
            self.progress.report("Processing container transmission")
            container_transmission_names.append(container_transmission_name)
            if len(absorber_transmission_names) > 1:
                transmission_absorber_name = absorber_transmission_names[transmission_no]
            else:
                transmission_absorber_name = absorber_transmission_names[0]
            if len(beam_transmission_names) > 1:
                transmission_beam_name = beam_transmission_names[transmission_no]
            else:
                transmission_beam_name = beam_transmission_names[0]
            if process_container_transmission:
                SANSILLReduction(
                    Run=transmission,
                    ProcessAs="Transmission",
                    OutputWorkspace=container_transmission_name,
                    AbsorberInputWorkspace=transmission_absorber_name,
                    BeamInputWorkspace=transmission_beam_name,
                    NormaliseBy=self.normalise,
                    BeamRadius=self.tr_radius,
                    Wavelength=self.getProperty("Wavelength").value,
                    Version=1,
                )
        for transmission_no, transmission in enumerate(self.stransmission.split(",")):
            [process_sample_transmission, sample_transmission_name] = needs_processing(transmission, "Transmission")
            self.progress.report("Processing sample transmission")
            sample_transmission_names.append(sample_transmission_name)
            if len(absorber_transmission_names) > 1:
                transmission_absorber_name = absorber_transmission_names[transmission_no]
            else:
                transmission_absorber_name = absorber_transmission_names[0]
            if len(beam_transmission_names) > 1:
                transmission_beam_name = beam_transmission_names[transmission_no]
            else:
                transmission_beam_name = beam_transmission_names[0]
            if process_sample_transmission:
                SANSILLReduction(
                    Run=transmission,
                    ProcessAs="Transmission",
                    OutputWorkspace=sample_transmission_name,
                    AbsorberInputWorkspace=transmission_absorber_name,
                    BeamInputWorkspace=transmission_beam_name,
                    NormaliseBy=self.normalise,
                    BeamRadius=self.tr_radius,
                    Wavelength=self.getProperty("Wavelength").value,
                    Version=1,
                )
        return container_transmission_names, sample_transmission_names

    def processAbsorber(self, i):
        absorber = self.absorber[i] if len(self.absorber) == self.dimensionality else self.absorber[0]
        [process_absorber, absorber_name] = needs_processing(absorber, "Absorber")
        self.progress.report("Processing absorber")
        if process_absorber:
            SANSILLReduction(
                Run=absorber,
                ProcessAs="Absorber",
                NormaliseBy=self.normalise,
                OutputWorkspace=absorber_name,
                Wavelength=self.getProperty("Wavelength").value,
                Version=1,
            )
        return absorber_name

    def processBeam(self, i, absorber_name):
        beam = self.beam[i] if len(self.beam) == self.dimensionality else self.beam[0]
        radius = self.radius[i] if len(self.radius) == self.dimensionality else self.radius[0]
        [process_beam, beam_name] = needs_processing(beam, "Beam")
        flux_name = beam_name + "_Flux" if not self.flux[0] else ""
        self.progress.report("Processing beam")
        if process_beam:
            SANSILLReduction(
                Run=beam,
                ProcessAs="Beam",
                OutputWorkspace=beam_name,
                NormaliseBy=self.normalise,
                BeamRadius=radius,
                AbsorberInputWorkspace=absorber_name,
                FluxOutputWorkspace=flux_name,
                Wavelength=self.getProperty("Wavelength").value,
                Version=1,
            )
        return beam_name, flux_name

    def processFlux(self, i, absorber_name):
        if self.flux[0]:
            flux = self.flux[i] if len(self.flux) == self.dimensionality else self.flux[0]
            radius = self.radius[i] if len(self.radius) == self.dimensionality else self.radius[0]
            [process_flux, flux_name] = needs_processing(flux, "Flux")
            self.progress.report("Processing flux")
            if process_flux:
                SANSILLReduction(
                    Run=flux,
                    ProcessAs="Beam",
                    OutputWorkspace=flux_name.replace("Flux", "Beam"),
                    NormaliseBy=self.normalise,
                    BeamRadius=radius,
                    AbsorberInputWorkspace=absorber_name,
                    FluxOutputWorkspace=flux_name,
                    Wavelength=self.getProperty("Wavelength").value,
                    Version=1,
                )
            return flux_name
        else:
            return None

    def processContainer(self, i, beam_name, absorber_name, container_transmission_names):
        container = self.container[i] if len(self.container) == self.dimensionality else self.container[0]
        [process_container, container_name] = needs_processing(container, "Container")
        if len(container_transmission_names) > 1:
            container_transmission_name = container_transmission_names[i]
        else:
            container_transmission_name = container_transmission_names[0]
        self.progress.report("Processing container")
        if process_container:
            SANSILLReduction(
                Run=container,
                ProcessAs="Container",
                OutputWorkspace=container_name,
                AbsorberInputWorkspace=absorber_name,
                BeamInputWorkspace=beam_name,
                CacheSolidAngle=True,
                TransmissionInputWorkspace=container_transmission_name,
                ThetaDependent=self.theta_dependent,
                NormaliseBy=self.normalise,
                Wavelength=self.getProperty("Wavelength").value,
                Version=1,
            )
        return container_name

    def getWavelength(self, logs):
        """Returns wavelength from the property Wavelength, if defined, otherwise attempts to obtain it from
        the logs."""
        WAVELENGTH_LOG1 = "wavelength"
        WAVELENGTH_LOG2 = "selector.wavelength"
        wavelength = None
        if not self.getProperty("Wavelength").isDefault:
            wavelength = self.getProperty("Wavelength").value
        else:
            try:
                wavelength = float(logs[WAVELENGTH_LOG1])
                if wavelength < 0.0:
                    wavelength = None
                    raise ValueError
            except:
                try:
                    wavelength = float(logs[WAVELENGTH_LOG2])
                    if wavelength < 0.0:
                        wavelength = None
                        raise ValueError
                except:
                    logger.notice("Unable to get a valid wavelength from the " "sample logs.")
        return wavelength

    def createCustomSuffix(self, ws):
        DISTANCE_LOG = "L2"
        COLLIMATION_LOG = "collimation.actual_position"

        logs = mtd[ws].run().getProperties()
        logs = {log.name: log.value for log in logs}

        distance = None
        try:
            instrument = mtd[ws].getInstrument()
            components = instrument.getStringParameter("detector_panels")
            if components:
                components = components[0].split(",")
                for c in components:
                    if c in ws:
                        distance = instrument.getComponentByName(c).getPos()[2]
                        break
            if not distance:
                distance = float(logs[DISTANCE_LOG])
            if distance < 0.0:
                distance = None
                raise ValueError
        except:
            logger.notice("Unable to get a valid detector distance value from " "the sample logs.")
        collimation = None
        try:
            collimation = float(logs[COLLIMATION_LOG])
            if collimation < 0.0:
                collimation = None
                raise ValueError
        except:
            logger.notice("Unable to get a valid collimation distance from " "the sample logs.")

        wavelength = self.getWavelength(logs)

        suffix = ""
        if distance:
            suffix += "_d{:.1f}m".format(distance)
        if collimation:
            suffix += "_c{:.1f}m".format(collimation)
        if wavelength:
            suffix += "_w{:.1f}A".format(wavelength)

        return suffix

    def processSample(self, i, flux_name, sample_transmission_names, beam_name, absorber_name, container_name):
        # this is the default mask, the same for all the distance configurations
        [load_default_mask, default_mask_name] = needs_loading(self.default_mask, "DefaultMask")
        self.progress.report("Loading default mask")
        if load_default_mask:
            LoadNexusProcessed(Filename=self.default_mask, OutputWorkspace=default_mask_name)

        # this is the beam stop mask, potentially different at each distance configuration
        mask = self.mask[i] if len(self.mask) == self.dimensionality else self.mask[0]
        [load_mask, mask_name] = needs_loading(mask, "Mask")
        self.progress.report("Loading mask")
        if load_mask:
            LoadNexusProcessed(Filename=mask, OutputWorkspace=mask_name)
        # sensitivity
        sens_input = ""
        if self.sensitivity:
            sens = self.sensitivity[i] if len(self.sensitivity) == self.dimensionality else self.sensitivity[0]
            [load_sensitivity, sensitivity_name] = needs_loading(sens, "Sensitivity")
            sens_input = sensitivity_name
            self.progress.report("Loading sensitivity")
            if load_sensitivity:
                LoadNexusProcessed(Filename=sens, OutputWorkspace=sensitivity_name)

        # reference
        ref_input = ""
        if self.reference:
            reference = self.reference[i] if len(self.reference) == self.dimensionality else self.reference[0]
            [load_reference, reference_name] = needs_loading(reference, "Reference")
            ref_input = reference_name
            self.progress.report("Loading reference")
            if load_reference:
                LoadNexusProcessed(Filename=reference, OutputWorkspace=reference_name)

        # solvent
        solv_input = ""
        if self.solvent:
            solvent = self.solvent[i] if len(self.solvent) == self.dimensionality else self.solvent[0]
            [load_solvent, solvent_name] = needs_loading(solvent, "Solvent")
            solv_input = solvent_name
            self.progress.report("Loading solvent")
            if load_solvent:
                LoadNexusProcessed(Filename=solvent, OutputWorkspace=solvent_name)

        # get correct transmission
        if len(sample_transmission_names) > 1:
            sample_transmission_name = sample_transmission_names[i]
        else:
            sample_transmission_name = sample_transmission_names[0]

        # sample
        [_, sample_name] = needs_processing(self.sample[i], "Sample")
        self.progress.report("Processing sample at detector configuration " + str(i + 1))

        output_sens = self.output_sens
        if self.getPropertyValue("SensitivityOutputWorkspace") and self.dimensionality > 1:
            output_sens += "_" + str(i + 1)

        SANSILLReduction(
            Run=self.sample[i],
            ProcessAs="Sample",
            OutputWorkspace=sample_name,
            ReferenceInputWorkspace=ref_input,
            AbsorberInputWorkspace=absorber_name,
            BeamInputWorkspace=beam_name,
            CacheSolidAngle=True,
            ContainerInputWorkspace=container_name,
            TransmissionInputWorkspace=sample_transmission_name,
            MaskedInputWorkspace=mask_name,
            DefaultMaskedInputWorkspace=default_mask_name,
            SensitivityInputWorkspace=sens_input,
            SensitivityOutputWorkspace=output_sens,
            FluxInputWorkspace=flux_name,
            SolventInputWorkspace=solv_input,
            NormaliseBy=self.normalise,
            ThetaDependent=self.theta_dependent,
            SampleThickness=self.getProperty("SampleThickness").value,
            WaterCrossSection=self.getProperty("WaterCrossSection").value,
            Wavelength=self.getProperty("Wavelength").value,
            MinThreshold=self.getProperty("MinThreshold").value,
            MaxThreshold=self.getProperty("MaxThreshold").value,
            Version=1,
        )

        common.add_correction_numors(
            ws=sample_name,
            stransmission=sample_transmission_name,
            container=container_name,
            absorber=absorber_name,
            beam=beam_name,
            flux=flux_name,
            solvent=solv_input,
            reference=ref_input,
            sensitivity=sens_input,
        )

        output_sample = self.output + "_#" + str(i + 1)

        output_panels = ""
        if self.getProperty("OutputPanels").value:
            output_panels = self.output_panels + "_#" + str(i + 1)

        output_wedges = ""
        if (self.n_wedges or self.getPropertyValue("ShapeTable")) and self.output_type == "I(Q)":
            output_wedges = self.output + "_wedge_#" + str(i + 1)

        if self.getProperty("SensitivityWithOffsets").value:
            CloneWorkspace(InputWorkspace=sample_name, OutputWorkspace=output_sens)

        qbinning = self.getPropertyValue("OutputBinning")
        if qbinning:
            qbinning = qbinning.split(":")
            if len(qbinning) > 1:
                qbinning = qbinning[i]
            else:
                qbinning = qbinning[0]
        SANSILLIntegration(
            InputWorkspace=sample_name,
            OutputWorkspace=output_sample,
            OutputType=self.output_type,
            CalculateResolution=self.getPropertyValue("CalculateResolution"),
            DefaultQBinning=self.getPropertyValue("DefaultQBinning"),
            BinningFactor=self.getProperty("BinningFactor").value,
            OutputBinning=qbinning,
            NPixelDivision=self.getProperty("NPixelDivision").value,
            NumberOfWedges=self.n_wedges,
            WedgeAngle=self.getProperty("WedgeAngle").value,
            WedgeOffset=self.getProperty("WedgeOffset").value,
            WedgeWorkspace=output_wedges,
            AsymmetricWedges=self.getProperty("AsymmetricWedges").value,
            PanelOutputWorkspaces=output_panels,
            MaxQxy=(self.maxqxy[i] if len(self.maxqxy) == self.dimensionality else self.maxqxy[0]),
            DeltaQ=(self.deltaq[i] if len(self.deltaq) == self.dimensionality else self.deltaq[0]),
            IQxQyLogBinning=self.getProperty("IQxQyLogBinning").value,
            WavelengthRange=self.getProperty("WavelengthRange").value,
            ShapeTable=self.getPropertyValue("ShapeTable"),
        )

        # if a shape table is used, we can not know upfront what is the number of wedges
        if self.getPropertyValue("ShapeTable") and self.output_type == "I(Q)":
            self.n_wedges = mtd[output_wedges].getNumberOfEntries()

        ConvertToPointData(InputWorkspace=output_sample, OutputWorkspace=output_sample)
        # Set to histogram to enable stitching
        mtd[output_sample].setDistribution(False)

        if self.cleanup:
            DeleteWorkspace(sample_name)

        return output_sample, output_wedges, output_panels, output_sens


AlgorithmFactory.subscribe(SANSILLAutoProcess)
