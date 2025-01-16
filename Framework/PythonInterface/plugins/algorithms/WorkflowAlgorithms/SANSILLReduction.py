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
    WorkspaceGroup,
    WorkspaceProperty,
    FileAction,
)
from mantid.kernel import (
    config,
    logger,
    Direction,
    EnabledWhenProperty,
    FloatBoundedValidator,
    LogicOperator,
    PropertyCriterion,
    StringListValidator,
)
from mantid.simpleapi import (
    AddSampleLog,
    ApplyTransmissionCorrection,
    CalculateDynamicRange,
    CalculateEfficiency,
    CalculateFlux,
    CalculateTransmission,
    CloneWorkspace,
    ConvertSpectrumAxis,
    ConvertToHistogram,
    CreateWorkspace,
    CropToComponent,
    CropWorkspace,
    DeadTimeCorrection,
    DeleteWorkspace,
    DeleteWorkspaces,
    Divide,
    ExtractSpectra,
    FindDetectorsInShape,
    FindCenterOfMassPosition,
    Fit,
    GroupDetectors,
    GroupWorkspaces,
    LoadAndMerge,
    MaskAngle,
    MaskDetectors,
    MaskDetectorsIf,
    MergeRuns,
    Minus,
    MoveInstrumentComponent,
    NormaliseByThickness,
    ParallaxCorrection,
    Rebin,
    RebinToWorkspace,
    RenameWorkspace,
    ReplaceSpecialValues,
    RotateInstrumentComponent,
    Scale,
    SolidAngle,
    Transpose,
)
import SANSILLCommon as common
from math import fabs
import numpy as np
import os


class SANSILLReduction(DataProcessorAlgorithm):
    _mode = "Monochromatic"
    _instrument = None

    def category(self):
        return "ILL\\SANS"

    def summary(self):
        return "Performs SANS data reduction at the ILL."

    def seeAlso(self):
        return ["SANSILLIntegration"]

    def name(self):
        return "SANSILLReduction"

    def validateInputs(self):
        issues = dict()
        process = self.getPropertyValue("ProcessAs")
        if process == "Transmission" and self.getProperty("BeamInputWorkspace").isDefault:
            issues["BeamInputWorkspace"] = "Beam input workspace is mandatory for transmission calculation."
        if bool(self.getPropertyValue("InputWorkspace")) == bool(self.getPropertyValue("Run")):
            issues["Run"] = "Please provide either Run (for standard SANS) or InputWorkspace (for parameter scans)."
            issues["InputWorkspace"] = "Please provide either Run (for standard SANS) or InputWorkspace (for parameter scans)."
        return issues

    @staticmethod
    def _make_solid_angle_name(ws):
        return mtd[ws].getInstrument().getName() + "_" + str(round(mtd[ws].getRun().getLogData("L2").value)) + "m_SolidAngle"

    @staticmethod
    def _check_distances_match(ws1, ws2):
        """
        Checks if the detector distance between two workspaces are close enough
        @param ws1 : workspace 1
        @param ws2 : workspace 2
        """
        tolerance = 0.01  # m
        l2_1 = ws1.getRun().getLogData("L2").value
        l2_2 = ws2.getRun().getLogData("L2").value
        r1 = ws1.getRunNumber()
        r2 = ws2.getRunNumber()
        if fabs(l2_1 - l2_2) > tolerance:
            logger.warning("Different distances detected! {0}: {1}, {2}: {3}".format(r1, l2_1, r2, l2_2))

    @staticmethod
    def _check_wavelengths_match(ws1, ws2):
        """
        Checks if the wavelength difference between the data is close enough
        @param ws1 : workspace 1
        @param ws2 : workspace 2
        """
        tolerance = 0.01  # A
        wavelength_1 = ws1.getRun().getLogData("wavelength").value
        wavelength_2 = ws2.getRun().getLogData("wavelength").value
        r1 = ws1.getRunNumber()
        r2 = ws2.getRunNumber()
        if fabs(wavelength_1 - wavelength_2) > tolerance:
            logger.warning("Different wavelengths detected! {0}: {1}, {2}: {3}".format(r1, wavelength_1, r2, wavelength_2))

    @staticmethod
    def _check_processed_flag(ws, value):
        return ws.getRun().getLogData("ProcessedAs").value == value

    @staticmethod
    def _cylinder(radius):
        """
        Returns XML for an infinite cylinder with axis of z (beam) and given radius [m]
        @param radius : the radius of the cylinder [m]
        @return : XML string for the geometry shape
        """
        return (
            '<infinite-cylinder id="flux"><centre x="0.0" y="0.0" z="0.0"/><axis x="0.0" y="0.0" z="1.0"/>'
            '<radius val="{0}"/></infinite-cylinder>'.format(radius)
        )

    @staticmethod
    def _mask(ws, masked_ws, check_if_masked_detectors=True):
        if not check_if_masked_detectors or masked_ws.detectorInfo().hasMaskedDetectors():
            MaskDetectors(Workspace=ws, MaskedWorkspace=masked_ws)

    def PyInit(self):
        self.declareProperty(
            MultipleFileProperty("Run", action=FileAction.OptionalLoad, extensions=["nxs"], allow_empty=True), doc="File path of run(s)."
        )

        options = ["Absorber", "Beam", "Transmission", "Container", "Sample"]

        self.declareProperty(
            name="ProcessAs", defaultValue="Sample", validator=StringListValidator(options), doc="Choose the process type."
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="The output workspace based on the value of ProcessAs.",
        )

        not_absorber = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsNotEqualTo, "Absorber")

        sample = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Sample")

        beam = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Beam")

        transmission = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Transmission")

        not_beam = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsNotEqualTo, "Beam")

        container = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Container")

        self.declareProperty(
            name="NormaliseBy",
            defaultValue="Timer",
            validator=StringListValidator(["None", "Timer", "Monitor"]),
            doc="Choose the normalisation type.",
        )

        self.declareProperty(
            "BeamRadius",
            0.05,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Beam radius [m]; used for beam center finding, transmission and flux calculations.",
        )

        self.setPropertySettings("BeamRadius", EnabledWhenProperty(beam, transmission, LogicOperator.Or))

        self.declareProperty(
            "BeamFinderMethod",
            "DirectBeam",
            StringListValidator(["DirectBeam", "ScatteredBeam"]),
            doc="Choose between direct beam or scattered beam method for beam center finding.",
        )

        self.setPropertySettings("BeamFinderMethod", beam)

        self.declareProperty(
            "SampleThickness",
            0.1,
            validator=FloatBoundedValidator(lower=-1),
            doc="Sample thickness [cm] (if -1, the value is taken from the nexus file).",
        )

        self.setPropertySettings("SampleThickness", sample)

        self.declareProperty(
            MatrixWorkspaceProperty("AbsorberInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the absorber workspace.",
        )

        self.setPropertySettings("AbsorberInputWorkspace", not_absorber)

        self.declareProperty(
            MatrixWorkspaceProperty("BeamInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the empty beam input workspace.",
        )

        self.setPropertySettings("BeamInputWorkspace", EnabledWhenProperty(not_absorber, not_beam, LogicOperator.And))

        self.declareProperty(
            MatrixWorkspaceProperty("TransmissionInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the transmission input workspace.",
        )

        self.setPropertySettings("TransmissionInputWorkspace", EnabledWhenProperty(container, sample, LogicOperator.Or))

        self.declareProperty(
            MatrixWorkspaceProperty("ContainerInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the container workspace.",
        )

        self.setPropertySettings("ContainerInputWorkspace", sample)

        self.declareProperty(
            MatrixWorkspaceProperty("ReferenceInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the reference workspace.",
        )

        self.setPropertySettings("ReferenceInputWorkspace", sample)

        self.declareProperty(
            MatrixWorkspaceProperty("SensitivityInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the input sensitivity workspace.",
        )

        self.setPropertySettings(
            "SensitivityInputWorkspace",
            EnabledWhenProperty(sample, EnabledWhenProperty("ReferenceInputWorkspace", PropertyCriterion.IsEqualTo, ""), LogicOperator.And),
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SensitivityOutputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the output sensitivity workspace.",
        )

        self.setPropertySettings("SensitivityOutputWorkspace", sample)

        self.declareProperty(
            MatrixWorkspaceProperty("MaskedInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace to copy the mask from; for example, the beam stop",
        )

        self.setPropertySettings("MaskedInputWorkspace", sample)

        self.declareProperty(
            MatrixWorkspaceProperty("FluxInputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the input direct beam flux workspace.",
        )

        self.setPropertySettings("FluxInputWorkspace", sample)

        self.declareProperty(
            MatrixWorkspaceProperty("FluxOutputWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the output direct beam flux workspace.",
        )

        self.setPropertySettings("FluxOutputWorkspace", beam)

        self.declareProperty("CacheSolidAngle", False, doc="Whether or not to cache the solid angle workspace.")

        self.declareProperty(
            "WaterCrossSection", 1.0, doc="Provide water cross-section; used only if the absolute scale is done by dividing to water."
        )

        self.declareProperty(
            MatrixWorkspaceProperty("DefaultMaskedInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace to copy the mask from; for example, the bad detector edges.",
        )

        self.setPropertySettings("DefaultMaskedInputWorkspace", sample)

        self.declareProperty("ThetaDependent", True, doc="Whether or not to use 2theta dependent transmission correction")

        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Input workspace containing already loaded raw data, used for parameter scans.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("SolventInputWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the solvent workspace.",
        )

        self.setPropertySettings("SolventInputWorkspace", sample)

        self.declareProperty(
            "Wavelength",
            0.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Wavelength set for the data, will override nexus, intended for D16 reduction.",
        )

        self.copyProperties("CalculateEfficiency", ["MinThreshold", "MaxThreshold"])
        # override default documentation of copied parameters to make them understandable by user
        threshold_property = self.getProperty("MinThreshold")
        threshold_property.setDocumentation("Minimum threshold for calculated efficiency.")
        threshold_property = self.getProperty("MaxThreshold")
        threshold_property.setDocumentation("Maximum threshold for calculated efficiency.")

    def _normalise(self, ws, monID=None):
        """
        Normalizes the workspace by time (SampleLog Timer) or Monitor
        @param ws : the input workspace
        """
        normalise_by = self.getPropertyValue("NormaliseBy")
        if monID is None:
            if self._instrument == "D33":
                monID = 500000
            elif self._instrument == "D16":
                monID = 500001
            else:
                monID = 100000
        if normalise_by == "Monitor":
            mon = ws + "_mon"
            ExtractSpectra(InputWorkspace=ws, DetectorList=monID, OutputWorkspace=mon)
            if mtd[mon].readY(0)[0] == 0:
                raise RuntimeError("Normalise to monitor requested, but monitor has 0 counts.")
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws)
                DeleteWorkspace(mon)
        elif normalise_by == "Timer":
            if mtd[ws].getRun().hasProperty("duration"):
                duration = mtd[ws].getRun().getLogData("duration").value
                if duration != 0.0:
                    Scale(InputWorkspace=ws, Factor=1.0 / duration, OutputWorkspace=ws)
                    self._apply_dead_time(ws)
                else:
                    raise RuntimeError("Unable to normalise to time; duration found is 0 seconds.")
            else:
                raise RuntimeError("Normalise to timer requested, but timer information is not available.")
        # regardless on normalisation, mask out the monitors, but do not extract them, since extracting is slow
        # masking however is needed to get more reasonable scales in the instrument view
        MaskDetectors(Workspace=ws, DetectorList=[monID, monID + 1])

    def _get_vertical_grouping_pattern(self, ws):
        """
        Provides vertical grouping pattern and crops to the main detector panel where counts from the beam are measured.
        :param ws: Empty beam workspace.
        """
        inst_name = mtd[ws].getInstrument().getName()
        min_id = 0
        if "D11" in inst_name:
            if "lr" in inst_name:
                step = 128
                max_id = 16384
            elif "B" in inst_name:
                CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames="detector_center")
                max_id = 49152
                step = 192
            else:
                step = 256
                max_id = 65536
        elif "D22" in inst_name:
            max_id = 32768
            step = 256
            if "lr" in inst_name:
                step = 128
                max_id = 16384
            elif "B" in inst_name:
                CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames="detector_back")
        elif "D33" in inst_name:
            CropToComponent(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames="back_detector")
            max_id = 32768
            step = 128
        else:
            self.log().warning("Instruments other than D11, D22, and D33 are not yet supported.")
            return
        return ",".join(["{}-{}".format(start, start + step - 1) for start in range(min_id, max_id, step)])

    def _fit_beam_width(self, input_ws):
        """
        Groups detectors vertically and fits the horizontal beam width with a Gaussian distribution to obtain
        horizontal beam resolution wich is added to sample logs under 'BeamWidthVertical' entry.
        :param input_ws: Empty beam workspace
        """
        tmp_ws = input_ws + "_beam_width"
        CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=tmp_ws)
        grouping_pattern = self._get_vertical_grouping_pattern(tmp_ws)
        if not grouping_pattern:  # unsupported instrument
            return
        GroupDetectors(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws, GroupingPattern=grouping_pattern)
        ConvertSpectrumAxis(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws, Target="SignedInPlaneTwoTheta")
        Transpose(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws)
        background = "name=FlatBackground, A0=1e-4"
        distribution_width = np.max(mtd[tmp_ws].getAxis(0).extractValues())
        function = "name=Gaussian, PeakCentre={0}, Height={1}, Sigma={2}".format(
            0, np.max(mtd[tmp_ws].getAxis(1).extractValues()), 0.1 * distribution_width
        )
        constraints = "{0} < f1.PeakCentre < {1}".format(-0.1 * distribution_width, 0.1 * distribution_width)
        fit_function = [background, function]
        fit_output = Fit(
            Function=";".join(fit_function),
            InputWorkspace=tmp_ws,
            Constraints=constraints,
            CreateOutput=False,
            IgnoreInvalidData=True,
            Output=tmp_ws + "_fit_output",
        )
        param_table = fit_output.OutputParameters
        beam_width = param_table.column(1)[3] * np.pi / 180.0
        AddSampleLog(Workspace=input_ws, LogName="BeamWidthX", LogText=str(beam_width), LogType="Number", LogUnit="rad")
        DeleteWorkspaces(
            WorkspaceList=[
                tmp_ws,
                tmp_ws + "_fit_output_Parameters",
                tmp_ws + "_fit_output_Workspace",
                tmp_ws + "_fit_output_NormalisedCovarianceMatrix",
            ]
        )

    def _process_beam(self, ws):
        """
        Calculates the beam center's x,y coordinates, and the beam flux
        @param ws : the input [empty beam] workspace
        """
        centers = ws + "_centers"
        method = self.getPropertyValue("BeamFinderMethod")
        radius = self.getProperty("BeamRadius").value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=(method == "DirectBeam"), BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0, 1)
        beam_y = mtd[centers].cell(1, 1)
        AddSampleLog(Workspace=ws, LogName="BeamCenterX", LogText=str(beam_x), LogType="Number")
        AddSampleLog(Workspace=ws, LogName="BeamCenterY", LogText=str(beam_y), LogType="Number")
        DeleteWorkspace(centers)
        if self._mode != "TOF":
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName="detector")
            self._fit_beam_width(input_ws=ws)
        run = mtd[ws].getRun()
        att_coeff = 1.0
        if run.hasProperty("attenuator.attenuation_coefficient"):
            att_coeff = run.getLogData("attenuator.attenuation_coefficient").value
            self.log().information("Found attenuator coefficient/value: {0}".format(att_coeff))
        elif run.hasProperty("attenuator.attenuation_value"):
            att_value = run.getLogData("attenuator.attenuation_value").value
            if float(att_value) < 10.0 and self._instrument == "D33":
                instrument = mtd[ws].getInstrument()
                param = "att" + str(int(att_value))
                if instrument.hasParameter(param):
                    att_coeff = instrument.getNumberParameter(param)[0]
                else:
                    raise RuntimeError("Unable to find the attenuation coefficient for D33 attenuator #" + str(int(att_value)))
            else:
                att_coeff = float(att_value)
            self.log().information("Found attenuator coefficient/value: {0}".format(att_coeff))
        if run.hasProperty("attenuator2.attenuation_value"):
            # D22 can have the second, chopper attenuator
            # In principle, either of the 2 attenuators can be in or out
            # In practice, only one (standard or chopper) is in at a time
            # If one is out, its attenuation_value is set to 1, so it's safe to take the product
            att2_value = run.getLogData("attenuator2.attenuation_value").value
            self.log().information("Found attenuator 2 value: {0}".format(att2_value))
            att_coeff *= float(att2_value)
        self.log().information("Attenuation coefficient used is: {0}".format(att_coeff))
        flux_out = self.getPropertyValue("FluxOutputWorkspace")
        if flux_out:
            flux = ws + "_flux"
            CalculateFlux(InputWorkspace=ws, OutputWorkspace=flux, BeamRadius=radius)
            Scale(InputWorkspace=flux, Factor=att_coeff, OutputWorkspace=flux)
            nspec = mtd[ws].getNumberHistograms()
            x = mtd[flux].readX(0)
            y = mtd[flux].readY(0)
            e = mtd[flux].readE(0)
            CreateWorkspace(
                DataX=x,
                DataY=np.tile(y, nspec),
                DataE=np.tile(e, nspec),
                NSpec=nspec,
                ParentWorkspace=flux,
                UnitX="Wavelength",
                OutputWorkspace=flux,
            )
            mtd[flux].getRun().addProperty("ProcessedAs", "Beam", True)
            RenameWorkspace(InputWorkspace=flux, OutputWorkspace=flux_out)
            self.setProperty("FluxOutputWorkspace", mtd[flux_out])

    def _process_transmission(self, ws, beam_ws):
        """
        Calculates the transmission
        @param ws: input workspace name
        @param beam_ws: empty beam workspace
        """
        self._check_distances_match(mtd[ws], beam_ws)
        if self._mode != "TOF":
            self._check_wavelengths_match(mtd[ws], beam_ws)
        RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=beam_ws, OutputWorkspace=ws)
        radius = self.getProperty("BeamRadius").value
        shapeXML = self._cylinder(radius)
        det_list = FindDetectorsInShape(Workspace=ws, ShapeXML=shapeXML)
        lambdas = mtd[ws].extractX()
        min_lambda = np.min(lambdas)
        max_lambda = np.max(lambdas)
        width_lambda = lambdas[0][1] - lambdas[0][0]
        lambda_binning = [min_lambda, width_lambda, max_lambda]
        self.log().information("Rebinning for transmission calculation to: " + str(lambda_binning))
        Rebin(InputWorkspace=ws, Params=lambda_binning, OutputWorkspace=ws)
        beam_rebinned = Rebin(InputWorkspace=beam_ws, Params=lambda_binning, StoreInADS=False)
        CalculateTransmission(
            SampleRunWorkspace=ws,
            DirectRunWorkspace=beam_rebinned,
            TransmissionROI=det_list,
            OutputWorkspace=ws,
            RebinParams=lambda_binning,
        )

    def _process_sensitivity(self, ws, sensitivity_out):
        """
        Generates the detector sensitivity map
        @param ws: input workspace
        @param sensitivity_out: sensitivity output map
        """
        CalculateEfficiency(
            InputWorkspace=ws,
            OutputWorkspace=sensitivity_out,
            MinThreshold=self.getProperty("MinThreshold").value,
            MaxThreshold=self.getProperty("MaxThreshold").value,
        )
        mtd[sensitivity_out].getRun().addProperty("ProcessedAs", "Sensitivity", True)
        self.setProperty("SensitivityOutputWorkspace", mtd[sensitivity_out])

    def _process_sample(self, ws):
        """
        Processes the sample
        @param ws: input workspace
        """
        sensitivity_in = self.getProperty("SensitivityInputWorkspace").value
        if sensitivity_in:
            if not self._check_processed_flag(sensitivity_in, "Sensitivity"):
                self.log().warning("Sensitivity input workspace is not processed as sensitivity.")
            Divide(LHSWorkspace=ws, RHSWorkspace=sensitivity_in, OutputWorkspace=ws, WarnOnZeroDivide=False)
            self._mask(ws, sensitivity_in)
        flux_in = self.getProperty("FluxInputWorkspace").value
        if flux_in:
            flux_ws = ws + "_flux"
            if self._mode == "TOF":
                RebinToWorkspace(WorkspaceToRebin=flux_in, WorkspaceToMatch=ws, OutputWorkspace=flux_ws)
                Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
                DeleteWorkspace(flux_ws)
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=flux_in, OutputWorkspace=ws, WarnOnZeroDivide=False)
            AddSampleLog(Workspace=ws, LogText="True", LogType="String", LogName="NormalisedByFlux")
        reference_ws = self.getProperty("ReferenceInputWorkspace").value
        if reference_ws:
            if not self._check_processed_flag(reference_ws, "Sample"):
                self.log().warning("Reference input workspace is not processed as sample.")
            Divide(LHSWorkspace=ws, RHSWorkspace=reference_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
            Scale(InputWorkspace=ws, Factor=self.getProperty("WaterCrossSection").value, OutputWorkspace=ws)
            self._mask(ws, reference_ws)
            self._rescale_flux(ws, reference_ws)
        solvent_ws = self.getProperty("SolventInputWorkspace").value
        if solvent_ws:
            self._apply_solvent(ws, solvent_ws)
        MaskDetectorsIf(InputWorkspace=ws, OutputWorkspace=ws, Operator="NotFinite")

    def _rescale_flux(self, ws, ref_ws):
        """
        This adjusts the absolute scale after normalising by water
        If both sample and water runs are normalised by flux, there is nothing to do
        If one is normalised, the other is not, we log a warning
        If neither is normalised by flux, we have to rescale by the factor
        @param ws : the workspace to scale (sample)
        @param ref_ws : the reference workspace (water)
        """
        message = (
            "Sample and water runs are not consistent in terms of flux normalisation; "
            "the absolute scale will not be correct. "
            "Make sure they are either both normalised or both not normalised by flux."
            "Consider specifying the sample flux also to water reduction."
            "Even if it would be at different distance, it will be rescaled correctly."
        )
        run = mtd[ws].getRun()
        run_ref = ref_ws.getRun()
        has_log = run.hasProperty("NormalisedByFlux")
        has_log_ref = run_ref.hasProperty("NormalisedByFlux")

        # Case where the "NormalisedByFlux" has been set to only in one of the flat field or the data
        # Raise an error
        if has_log ^ has_log_ref:
            raise RuntimeError(message)
        # Case where the "NormalisedByFlux" property has been set to both flat field and data
        # Check now the actual value of the property to decide whether or not to perform the rescaling
        elif has_log and has_log_ref:
            # Fetch the values of "NormalisedByFlux" property for both the data and flat field
            log_val = run["NormalisedByFlux"].value == "True"
            log_val_ref = run_ref["NormalisedByFlux"].value == "True"
            # If those values are different raise an error
            if log_val ^ log_val_ref:
                raise RuntimeError(message)
            # If both values are True, both flat field and data have been normalised then do nothing
            elif log_val and log_val_ref:
                return
            # Otherwise do the rescaling
            else:
                self._do_rescale_flux(ws, ref_ws)
        # Case where the "NormalisedByFlux" property has not been set to both flat field and data
        # Do the rescaling
        else:
            self._do_rescale_flux(ws, ref_ws)

    def _do_rescale_flux(self, ws, ref_ws):
        """
        Scales ws by the flux factor wrt the reference
        @ws : input workspace to scale (sample)
        @ref_ws : reference workspace (water)
        """
        self._check_distances_match(mtd[ws], ref_ws)
        if self._mode != "TOF":
            self._check_wavelengths_match(mtd[ws], ref_ws)
        sample_l2 = mtd[ws].getRun().getLogData("L2").value
        ref_l2 = ref_ws.getRun().getLogData("L2").value
        flux_factor = (sample_l2**2) / (ref_l2**2)
        self.log().notice("Flux factor is: " + str(flux_factor))
        Scale(InputWorkspace=ws, Factor=flux_factor, OutputWorkspace=ws)

    def _apply_absorber(self, ws, absorber_ws):
        """
        Subtracts the dark current
        @param ws: input workspace
        @param absorber_ws: dark current workspace
        """
        if not self._check_processed_flag(absorber_ws, "Absorber"):
            self.log().warning("Absorber input workspace is not processed as absorber.")
        Minus(LHSWorkspace=ws, RHSWorkspace=absorber_ws, OutputWorkspace=ws)

    def _apply_beam(self, ws, beam_ws):
        """
        Applies the beam center correction
        @param ws: input workspace
        @parma beam_ws: empty beam workspace
        """
        if not self._check_processed_flag(beam_ws, "Beam"):
            self.log().warning("Beam input workspace is not processed as beam.")
        if self._mode != "TOF":
            beam_x = beam_ws.getRun().getLogData("BeamCenterX").value
            beam_y = beam_ws.getRun().getLogData("BeamCenterY").value
            AddSampleLog(Workspace=ws, LogName="BeamCenterX", LogText=str(beam_x), LogType="Number")
            AddSampleLog(Workspace=ws, LogName="BeamCenterY", LogText=str(beam_y), LogType="Number")
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName="detector")
            if "BeamWidthX" in beam_ws.getRun():
                beam_width_x = beam_ws.getRun().getLogData("BeamWidthX").value
                AddSampleLog(Workspace=ws, LogName="BeamWidthX", LogText=str(beam_width_x), LogType="Number", LogUnit="rad")
            else:
                self.log().warning("Beam width resolution not available.")
        self._check_distances_match(mtd[ws], beam_ws)
        if self._mode != "TOF":
            self._check_wavelengths_match(mtd[ws], beam_ws)

    def _apply_transmission(self, ws, transmission_ws):
        """
        Applies transmission correction
        @param ws: input workspace
        @param transmission_ws: transmission workspace
        """
        theta_dependent = self.getProperty("ThetaDependent").value
        run = mtd[ws].getRun()

        if theta_dependent and run.hasProperty("Gamma.value") and 75 < run.getLogData("Gamma.value").value < 105:
            # range in which it possible some pixels are nearing 90 degrees
            # normally, we are talking about D16 here

            epsilon = 1
            MaskAngle(Workspace=ws, MinAngle=90 - epsilon, MaxAngle=90 + epsilon, Angle="TwoTheta")

        if not self._check_processed_flag(transmission_ws, "Transmission"):
            self.log().warning("Transmission input workspace is not processed as transmission.")
        if transmission_ws.blocksize() == 1:
            # monochromatic mode, scalar transmission
            transmission = transmission_ws.readY(0)[0]
            transmission_err = transmission_ws.readE(0)[0]
            ApplyTransmissionCorrection(
                InputWorkspace=ws,
                TransmissionValue=transmission,
                TransmissionError=transmission_err,
                ThetaDependent=theta_dependent,
                OutputWorkspace=ws,
            )
            mtd[ws].getRun().addProperty("sample.transmission", float(transmission), True)
        else:
            # wavelength dependent transmission, need to rebin
            transmission_rebinned = ws + "_tr_rebinned"
            RebinToWorkspace(WorkspaceToRebin=transmission_ws, WorkspaceToMatch=ws, OutputWorkspace=transmission_rebinned)
            ApplyTransmissionCorrection(
                InputWorkspace=ws, TransmissionWorkspace=transmission_rebinned, ThetaDependent=theta_dependent, OutputWorkspace=ws
            )
            mtd[ws].getRun().addProperty("sample.transmission", list(mtd[transmission_rebinned].readY(0)), True)
            DeleteWorkspace(transmission_rebinned)

    def _apply_container(self, ws, container_ws):
        """
        Applies empty container subtraction
        @param ws: input workspace
        @param container_ws: empty container workspace
        """
        if not self._check_processed_flag(container_ws, "Container"):
            self.log().warning("Container input workspace is not processed as container.")
        self._check_distances_match(mtd[ws], container_ws)
        if self._mode != "TOF":
            self._check_wavelengths_match(mtd[ws], container_ws)
        Minus(LHSWorkspace=ws, RHSWorkspace=container_ws, OutputWorkspace=ws)

    def _apply_parallax(self, ws):
        """
        Applies the parallax correction
        @param ws : the input workspace
        """
        self.log().information("Performing parallax correction")
        if self._instrument in ["D33", "D11B", "D22B"]:
            components = mtd[ws].getInstrument().getStringParameter("detector_panels")[0].split(",")
        else:
            components = ["detector"]
        ParallaxCorrection(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames=components)

    def _apply_dead_time(self, ws):
        """
        Performs the dead time correction
        @param ws : the input workspace
        """

        instrument = mtd[ws].getInstrument()
        if instrument.hasParameter("tau"):
            tau = instrument.getNumberParameter("tau")[0]
            if self._instrument == "D33" or self._instrument == "D11B":
                grouping_filename = self._instrument + "_Grouping.xml"
                grouping_file = os.path.join(config["groupingFiles.directory"], grouping_filename)
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, MapFile=grouping_file, OutputWorkspace=ws)
            elif instrument.hasParameter("grouping"):
                pattern = instrument.getStringParameter("grouping")[0]
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, GroupingPattern=pattern, OutputWorkspace=ws)
            else:
                self.log().warning("No grouping available in IPF, dead time correction will be performed detector-wise.")
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, OutputWorkspace=ws)
        else:
            self.log().information("No tau available in IPF, skipping dead time correction.")

    def _apply_solvent(self, ws, solvent_ws):
        """
        Applies solvent subtraction
        @param ws: input workspace
        @param solvent_ws: empty container workspace
        """
        solvent_ws.setDistribution(False)
        if not self._check_processed_flag(solvent_ws, "Sample"):
            self.log().warning("Solvent input workspace is not processed as sample.")
        self._check_distances_match(mtd[ws], solvent_ws)
        solvent_ws_name = solvent_ws.getName()
        if self._mode != "TOF":
            self._check_wavelengths_match(mtd[ws], solvent_ws)
        else:
            solvent_ws_name += "_tmp"
            RebinToWorkspace(WorkspaceToRebin=solvent_ws, WorkspaceToMatch=ws, OutputWorkspace=solvent_ws_name)
        Minus(LHSWorkspace=ws, RHSWorkspace=solvent_ws_name, OutputWorkspace=ws)

    def _finalize(self, ws, process):
        if process != "Transmission":
            if self._instrument in ["D33", "D11B", "D22B"]:
                components = mtd[ws].getInstrument().getStringParameter("detector_panels")[0]
                CalculateDynamicRange(Workspace=ws, ComponentNames=components.split(","))
            elif self._instrument == "D16" and mtd[ws].getAxis(0).getUnit().caption() != "Wavelength":
                # D16 omega scan case : we have an histogram indexed by omega, not wavelength
                pass
            else:
                CalculateDynamicRange(Workspace=ws)
        ReplaceSpecialValues(InputWorkspace=ws, OutputWorkspace=ws, NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0)
        mtd[ws].getRun().addProperty("ProcessedAs", process, True)
        self._add_correction_information(ws)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        self.setProperty("OutputWorkspace", mtd[ws[2:]])

    def _add_correction_information(self, ws):
        """Adds information regarding corrections and inputs to the provided workspace.

        Args:
            ws: (str) workspace name to which information is to be added
        """
        # first, let's create the dictionary containing all parameters that should be added to the metadata
        parameters = dict()
        parameters["numor_list"] = common.return_numors_from_path(self.getPropertyValue("Run"))
        parameters["sample_transmission_ws"] = common.return_numors_from_path(self.getPropertyValue("TransmissionInputWorkspace"))
        parameters["container_ws"] = common.return_numors_from_path(self.getPropertyValue("ContainerInputWorkspace"))
        parameters["absorber_ws"] = common.return_numors_from_path(self.getPropertyValue("AbsorberInputWorkspace"))
        parameters["beam_ws"] = common.return_numors_from_path(self.getPropertyValue("BeamInputWorkspace"))
        parameters["flux_ws"] = common.return_numors_from_path(self.getPropertyValue("FluxInputWorkspace"))
        parameters["reference_ws"] = common.return_numors_from_path(self.getPropertyValue("ReferenceInputWorkspace"))
        parameters["sensitivity_ws"] = common.return_numors_from_path(self.getPropertyValue("SensitivityInputWorkspace"))
        parameters["mask_ws"] = common.return_numors_from_path(self.getPropertyValue("MaskedInputWorkspace"))
        # when all is set, a common function can set them all
        common.add_correction_information(ws, parameters)

    def _apply_masks(self, ws):
        # apply the default mask, e.g. the bad detector edges
        default_mask_ws = self.getProperty("DefaultMaskedInputWorkspace").value
        if default_mask_ws:
            self._mask(ws, default_mask_ws, False)
        # apply the beam stop mask
        mask_ws = self.getProperty("MaskedInputWorkspace").value
        if mask_ws:
            self._mask(ws, mask_ws, False)

    def _apply_thickness(self, ws):
        """
        Perform the normalization by sample thickness. In case the provided
        thickness is -1, this method will try to get it from the sample
        logs.
        @param ws : input workspace on wich the normalization is applied.
        """
        thickness = self.getProperty("SampleThickness").value
        if thickness == -1:
            try:
                run = mtd[ws].getRun()
                thickness = run.getLogData("sample.thickness").value
                self.log().information("Sample thickness read from the sample logs: {0} cm.".format(thickness))
            except:
                thickness = self.getProperty("SampleThickness").getDefault
                thickness = float(thickness)
                self.log().warning("Sample thickness not found in the sample logs. Using the default value: {:.2f}".format(thickness))
            finally:
                self.setProperty("SampleThickness", thickness)
        NormaliseByThickness(InputWorkspace=ws, OutputWorkspace=ws, SampleThickness=thickness)

    def _set_sample_title(self, ws):
        """
        Set the workspace title using Nexus file fields.
        @param ws : input workspace
        """
        run = mtd[ws].getRun()
        title = ""
        if run.hasProperty("sample_description"):
            title = run.getLogData("sample_description").value
            if title:
                mtd[ws].setTitle(title)
                return

        if run.hasProperty("sample.sampleId"):
            title = run.getLogData("sample.sampleId").value
            if title:
                title = "Sample ID = " + title
                mtd[ws].setTitle(title)

    def _set_mode(self, ws):
        if mtd[ws].blocksize() > 1:
            if mtd[ws].getAxis(0).getUnit().unitID() == "Wavelength":
                self._mode = "TOF"
            elif mtd[ws].getInstrument().getName() != "D16":
                self._mode = "Kinetic"

    def PyExec(self):
        process = self.getPropertyValue("ProcessAs")
        processes = ["Absorber", "Beam", "Transmission", "Container", "Sample"]
        progress = Progress(self, start=0.0, end=1.0, nreports=processes.index(process) + 1)
        ws = "__" + self.getPropertyValue("OutputWorkspace")
        if self.getPropertyValue("Run"):
            loader_options = {}
            if not self.getProperty("Wavelength").isDefault:
                loader_options["Wavelength"] = self.getProperty("Wavelength").value
            LoadAndMerge(
                Filename=self.getPropertyValue("Run").replace("+", ","),
                LoaderName="LoadILLSANS",
                OutputWorkspace=ws,
                LoaderOptions=loader_options,
            )
            if isinstance(mtd[ws], WorkspaceGroup):
                # we do not want the summing done by LoadAndMerge since it will be pair-wise and slow
                # instead we load and list, and merge once with merge runs
                tmp = "__tmp" + ws
                MergeRuns(InputWorkspaces=ws, OutputWorkspace=tmp)
                DeleteWorkspaces(ws)
                RenameWorkspace(InputWorkspace=tmp, OutputWorkspace=ws)
        else:
            in_ws = self.getPropertyValue("InputWorkspace")
            CloneWorkspace(InputWorkspace=in_ws, OutputWorkspace=ws)
        progress.report()
        self._set_mode(ws)

        if self._mode == "Kinetic":
            if process == "Sample":
                self.log().notice("Doing kinetic mode (monochromatic)")
                self._process_kinetic_sample(ws)
                return
            else:
                raise RuntimeError("Only the sample can be in kinetic mode, the calibration measurements cannot be.")

        self._instrument = mtd[ws].getInstrument().getName()
        self._normalise(ws)
        if process in ["Beam", "Transmission", "Container", "Sample"]:
            absorber_ws = self.getProperty("AbsorberInputWorkspace").value
            if absorber_ws:
                self._apply_absorber(ws, absorber_ws)
            if process == "Beam":
                self._process_beam(ws)
                progress.report()
            else:
                beam_ws = self.getProperty("BeamInputWorkspace").value
                if beam_ws:
                    self._apply_beam(ws, beam_ws)
                if process == "Transmission":
                    self._process_transmission(ws, beam_ws)
                    progress.report()
                else:
                    transmission_ws = self.getProperty("TransmissionInputWorkspace").value
                    if transmission_ws:
                        self._apply_transmission(ws, transmission_ws)
                    solid_angle = self._make_solid_angle_name(ws)
                    cache = self.getProperty("CacheSolidAngle").value
                    if (cache and not mtd.doesExist(solid_angle)) or not cache:
                        if self._instrument == "D16":
                            run = mtd[ws].getRun()
                            distance = run.getLogData("L2").value
                            CloneWorkspace(InputWorkspace=ws, OutputWorkspace=solid_angle)
                            MoveInstrumentComponent(
                                Workspace=solid_angle, X=0, Y=0, Z=distance, RelativePosition=False, ComponentName="detector"
                            )
                            RotateInstrumentComponent(
                                Workspace=solid_angle, X=0, Y=1, Z=0, angle=0, RelativeRotation=False, ComponentName="detector"
                            )
                            input_solid = solid_angle
                        else:
                            input_solid = ws
                        SolidAngle(InputWorkspace=input_solid, OutputWorkspace=solid_angle, Method="Rectangle")
                    Divide(LHSWorkspace=ws, RHSWorkspace=solid_angle, OutputWorkspace=ws, WarnOnZeroDivide=False)
                    if not cache:
                        DeleteWorkspace(solid_angle)
                    progress.report()
                    if process == "Sample":
                        container_ws = self.getProperty("ContainerInputWorkspace").value
                        if container_ws:
                            self._apply_container(ws, container_ws)
                        self._apply_masks(ws)
                        self._apply_thickness(ws)
                        # parallax (gondola) effect
                        if self._instrument in ["D22", "D22lr", "D33", "D11B", "D22B"]:
                            self._apply_parallax(ws)
                        progress.report()
                        sensitivity_out = self.getPropertyValue("SensitivityOutputWorkspace")
                        if sensitivity_out:
                            self._process_sensitivity(ws, sensitivity_out)
                        self._process_sample(ws)
                        self._set_sample_title(ws)
                        progress.report()
        self._finalize(ws, process)

    def _split_kinetic_frames(self, ws):
        """Explodes the frames of a kinetic workspace into separate workspaces"""
        n_frames = mtd[ws].blocksize()
        n_hist = mtd[ws].getNumberHistograms()
        wavelength = round(mtd[ws].getRun().getLogData("wavelength").value * 100) / 100
        wave_bins = [wavelength * 0.9, wavelength * 1.1]
        frames = []
        for frame_index in range(n_frames):
            frame_name = ws + "_t" + str(frame_index)
            frames.append(frame_name)
            CropWorkspace(InputWorkspace=ws, OutputWorkspace=frame_name, XMin=frame_index, XMax=frame_index)
            ConvertToHistogram(InputWorkspace=frame_name, OutputWorkspace=frame_name)
            mtd[frame_name].getAxis(0).setUnit("Wavelength")
            for s in range(n_hist):
                mtd[frame_name].setX(s, wave_bins)
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        return frames

    def _process_kinetic_sample(self, ws):
        """
        Processes a kinetic monochromatic workspace by exploding the frames into separate workspaces and reducing them individually
        Note that this is extremely slow and needs to be deprecated in favour of the v2 of this algorithm."""

        if self.getPropertyValue("NormaliseBy") == "Timer":
            # trick the normalise to use the second monitor which contains the durations per frame
            self.setPropertyValue("NormaliseBy", "Monitor")
            self._normalise(ws, 100001)
        else:
            self._normalise(ws)
        group_name = ws
        frames = self._split_kinetic_frames(ws)
        frames_to_group = []

        for ws in frames:
            frames_to_group.append(ws[2:])

            absorber_ws = self.getProperty("AbsorberInputWorkspace").value
            if absorber_ws:
                self._apply_absorber(ws, absorber_ws)

            beam_ws = self.getProperty("BeamInputWorkspace").value
            if beam_ws:
                self._apply_beam(ws, beam_ws)

            transmission_ws = self.getProperty("TransmissionInputWorkspace").value
            if transmission_ws:
                self._apply_transmission(ws, transmission_ws)
            solid_angle = self._make_solid_angle_name(ws)
            cache = self.getProperty("CacheSolidAngle").value
            if (cache and not mtd.doesExist(solid_angle)) or not cache:
                SolidAngle(InputWorkspace=ws, OutputWorkspace=solid_angle, Method="Rectangle")
            Divide(LHSWorkspace=ws, RHSWorkspace=solid_angle, OutputWorkspace=ws, WarnOnZeroDivide=False)
            if not cache:
                DeleteWorkspace(solid_angle)

            container_ws = self.getProperty("ContainerInputWorkspace").value
            if container_ws:
                self._apply_container(ws, container_ws)

            self._apply_masks(ws)
            self._apply_thickness(ws)
            # parallax (gondola) effect
            if self._instrument in ["D22", "D22lr", "D33", "D11B", "D22B"]:
                self._apply_parallax(ws)

            self._process_sample(ws)
            self._set_sample_title(ws)

            components = mtd[ws].getInstrument().getStringParameter("detector_panels")[0]
            CalculateDynamicRange(Workspace=ws, ComponentNames=components.split(","))
            MaskDetectorsIf(InputWorkspace=ws, OutputWorkspace=ws, Operator="NotFinite")

            mtd[ws].getRun().addProperty("ProcessedAs", "Sample", True)
            self._add_correction_information(ws)

            RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])

        GroupWorkspaces(InputWorkspaces=frames_to_group, OutputWorkspace=group_name[2:])
        self.setProperty("OutputWorkspace", mtd[group_name[2:]])


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLReduction)
