# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from SANSILLCommon import (
    AcqMode,
    add_correction_information,
    blank_monitor_ws_neg_index,
    check_axis_match,
    check_distances_match,
    check_processed_flag,
    check_wavelengths_match,
    EMPTY_TOKEN,
    get_vertical_grouping_pattern,
    main_detector_distance,
    monitor_id,
    real_monitor_ws_neg_index,
    return_numors_from_path,
)
from mantid.api import (
    mtd,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    FileAction,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    MultipleFileProperty,
    Progress,
    PropertyMode,
    WorkspaceGroup,
    WorkspaceProperty,
)
from mantid.dataobjects import SpecialWorkspace2D
from mantid.kernel import (
    config,
    Direction,
    EnabledWhenProperty,
    FloatBoundedValidator,
    LogicOperator,
    PropertyCriterion,
    StringListValidator,
)
from mantid.simpleapi import (
    AddSampleLog,
    AppendSpectra,
    ApplyTransmissionCorrection,
    CalculateEfficiency,
    CalculateFlux,
    CloneWorkspace,
    ConjoinXRuns,
    ConvertSpectrumAxis,
    ConvertToPointData,
    CreateWorkspace,
    DeadTimeCorrection,
    DeleteWorkspace,
    DeleteWorkspaces,
    Divide,
    ExtractSpectra,
    FindCenterOfMassPosition,
    Fit,
    GroupDetectors,
    GroupWorkspaces,
    LoadAndMerge,
    MaskAngle,
    MaskDetectors,
    Minus,
    MoveInstrumentComponent,
    ParallaxCorrection,
    RebinToWorkspace,
    RenameWorkspace,
    RotateInstrumentComponent,
    Scale,
    SolidAngle,
    SortXAxis,
    Transpose,
)
import os
import numpy as np


class SANSILLReduction(DataProcessorAlgorithm):
    """
    Performs unit data reduction of the given process type.
    Supports D11, D16, D22, and D33 instruments at the ILL.
    Supports stadard monochromatic, kinetic monochromatic, rebinned event (monochromatic)
    and TOF measurements (equidistant and non-equidistant, D33 only).
    """

    mode = None  # the acquisition mode of the reduction
    instrument = None  # the name of the instrument
    n_samples = None  # how many samples
    n_frames = None  # how many frames per sample in case of kinetic
    process = None  # the process type
    n_reports = None  # how many progress report checkpoints
    progress = None  # the global progress reporter
    processes = ["DarkCurrent", "EmptyBeam", "Transmission", "EmptyContainer", "Water", "Solvent", "Sample"]

    def category(self):
        return "ILL\\SANS"

    def summary(self):
        return "Performs SANS data reduction of a given process type."

    def seeAlso(self):
        return ["SANSILLMultiProcess", "SANSILLIntegration"]

    def name(self):
        return "SANSILLReduction"

    def version(self):
        return 2

    def validateInputs(self):
        issues = dict()

        runs = self.getPropertyValue("Runs").split(",")
        non_blank_runs = list(filter(lambda x: x != EMPTY_TOKEN, runs))
        if not non_blank_runs:
            issues["Runs"] = "Only blanks runs have been provided, there must be at least one non-blank run."

        process = self.getPropertyValue("ProcessAs")
        if process == "Transmission" and not self.getPropertyValue("FluxWorkspace"):
            issues["FluxWorkspace"] = "Empty beam flux input workspace is mandatory for transmission calculation."

        samples_thickness = len(self.getProperty("SampleThickness").value)
        if samples_thickness != 1 and samples_thickness != self.getPropertyValue("Runs").count(",") + 1:
            issues["SampleThickness"] = "Sample thickness must have either a single value or as many as there are samples."

        if not self.getPropertyValue("SampleWorkspace") and not self.getPropertyValue("Runs"):
            issues["SampleWorkspace"] = "Providing either a run file or an already loaded input workspace is mandatory."
            issues["Runs"] = "Providing either a run file or an already loaded input workspace is mandatory."

        return issues

    def PyInit(self):
        # ================================MAIN PARAMETERS================================#
        can_runs = EnabledWhenProperty("SampleWorkspace", PropertyCriterion.IsEqualTo, "")

        self.declareProperty(
            MultipleFileProperty(name="Runs", action=FileAction.OptionalLoad, extensions=["nxs"], allow_empty=True),
            doc="File path of run(s).",
        )
        self.setPropertySettings("Runs", can_runs)

        self.declareProperty(
            name="ProcessAs", defaultValue="Sample", validator=StringListValidator(self.processes), doc="Choose the process type."
        )

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="The output workspace based on the value of ProcessAs.",
        )

        not_dark = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsNotEqualTo, "DarkCurrent")
        beam = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "EmptyBeam")
        not_beam = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsNotEqualTo, "EmptyBeam")
        not_dark_nor_beam = EnabledWhenProperty(not_dark, not_beam, LogicOperator.And)
        transmission = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Transmission")
        beam_or_transmission = EnabledWhenProperty(beam, transmission, LogicOperator.Or)
        container = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "EmptyContainer")
        water = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Water")
        solvent = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Solvent")
        sample = EnabledWhenProperty("ProcessAs", PropertyCriterion.IsEqualTo, "Sample")
        solvent_sample = EnabledWhenProperty(solvent, sample, LogicOperator.Or)
        water_solvent_sample = EnabledWhenProperty(solvent_sample, water, LogicOperator.Or)
        can_water_solvent_sample = EnabledWhenProperty(water_solvent_sample, container, LogicOperator.Or)
        can_sample_workspace = EnabledWhenProperty("Runs", PropertyCriterion.IsEqualTo, "")

        # most of D33 data has 0 monitor counts, so normalise by time is a better universal default
        self.declareProperty(
            name="NormaliseBy",
            defaultValue="Time",
            validator=StringListValidator(["None", "Time", "Monitor"]),
            doc="Choose the normalisation type.",
        )

        self.declareProperty(
            name="BeamRadius",
            defaultValue=0.25,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Beam radius [m]; used for beam center search.",
        )

        self.setPropertySettings("BeamRadius", beam)

        self.declareProperty(
            name="TransmissionBeamRadius",
            defaultValue=0.1,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Beam radius [m]; used for flux and transmission calculations.",
        )

        self.setPropertySettings("TransmissionBeamRadius", beam_or_transmission)

        self.declareProperty(
            name="SampleThickness", defaultValue=[0.1], doc="Sample thickness [cm] (if -1, the value is taken from the nexus file)."
        )

        self.setPropertySettings("SampleThickness", water_solvent_sample)

        self.declareProperty(
            name="WaterCrossSection",
            defaultValue=1.0,
            doc="Provide water cross-section [cm-1]; used only if the absolute scale is performed by dividing to water.",
        )

        self.setPropertySettings("WaterCrossSection", solvent_sample)

        self.declareProperty(
            name="TransmissionThetaDependent", defaultValue=True, doc="Whether or not to use 2theta dependent transmission correction"
        )
        self.setPropertySettings("TransmissionThetaDependent", can_water_solvent_sample)

        # ================================INPUT WORKSPACES================================#

        self.declareProperty(
            MatrixWorkspaceProperty("SampleWorkspace", "", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Input workspace containing already loaded sample data, used for parameter scans.",
        )
        self.setPropertySettings("SampleWorkspace", can_sample_workspace)

        self.declareProperty(
            MatrixWorkspaceProperty(
                name="DarkCurrentWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="The name of the Cd/B4C input workspace.",
        )

        self.setPropertySettings("DarkCurrentWorkspace", not_dark)

        self.declareProperty(
            MatrixWorkspaceProperty(name="EmptyBeamWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the empty beam input workspace.",
        )

        self.setPropertySettings("EmptyBeamWorkspace", not_dark_nor_beam)

        self.declareProperty(
            MatrixWorkspaceProperty(name="FluxWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the input empty beam flux workspace.",
        )

        self.setPropertySettings("FluxWorkspace", EnabledWhenProperty(water_solvent_sample, transmission, LogicOperator.Or))

        self.declareProperty(
            MatrixWorkspaceProperty(
                name="TransmissionWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="The name of the transmission input workspace.",
        )

        self.setPropertySettings("TransmissionWorkspace", can_water_solvent_sample)

        self.declareProperty(
            MatrixWorkspaceProperty(
                name="EmptyContainerWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="The name of the container input workspace.",
        )

        self.setPropertySettings("EmptyContainerWorkspace", water_solvent_sample)

        self.declareProperty(
            MatrixWorkspaceProperty(name="FlatFieldWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the water input workspace.",
        )

        self.setPropertySettings("FlatFieldWorkspace", solvent_sample)

        self.declareProperty(
            MatrixWorkspaceProperty(name="SolventWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="The name of the solvent input workspace.",
        )

        self.setPropertySettings("SolventWorkspace", sample)

        self.declareProperty(
            MatrixWorkspaceProperty(
                name="SensitivityWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="The name of the input sensitivity map workspace.",
        )

        self.setPropertySettings("SensitivityWorkspace", solvent_sample)

        self.declareProperty(
            MatrixWorkspaceProperty(
                name="DefaultMaskWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional
            ),
            doc="Workspace to copy the mask from; for example, the bad detector edges.",
        )

        self.setPropertySettings("DefaultMaskWorkspace", water_solvent_sample)

        self.declareProperty(
            MatrixWorkspaceProperty(name="MaskWorkspace", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Workspace to copy the mask from; for example, the beam stop",
        )

        self.setPropertySettings("MaskWorkspace", water_solvent_sample)

        # ================================AUX OUTPUT WORKSPACES================================#

        self.declareProperty(
            MatrixWorkspaceProperty("OutputSensitivityWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the output sensitivity workspace.",
        )

        self.setPropertySettings("OutputSensitivityWorkspace", water)

        self.declareProperty(
            MatrixWorkspaceProperty("OutputFluxWorkspace", "", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="The name of the output empty beam flux workspace.",
        )

        self.setPropertySettings("OutputFluxWorkspace", beam)

        self.copyProperties("CalculateEfficiency", ["MinThreshold", "MaxThreshold"])
        # override default documentation of copied parameters to make them understandable by user
        threshold_property = self.getProperty("MinThreshold")
        threshold_property.setDocumentation("Minimum threshold for calculated efficiency.")
        threshold_property = self.getProperty("MaxThreshold")
        threshold_property.setDocumentation("Maximum threshold for calculated efficiency.")

    def _add_correction_information(self, ws):
        """Adds information regarding corrections and inputs to the provided workspace.

        Args:
            ws: (str) workspace name to which information is to be added
        """
        # first, let's create the dictionary containing all parameters that should be added to the metadata
        parameters = dict()
        parameters["numor_list"] = return_numors_from_path(self.getPropertyValue("Runs"))
        parameters["sample_transmission_ws"] = return_numors_from_path(self.getPropertyValue("TransmissionWorkspace"))
        parameters["container_ws"] = return_numors_from_path(self.getPropertyValue("EmptyContainerWorkspace"))
        parameters["absorber_ws"] = return_numors_from_path(self.getPropertyValue("DarkCurrentWorkspace"))
        parameters["beam_ws"] = return_numors_from_path(self.getPropertyValue("EmptyBeamWorkspace"))
        parameters["flux_ws"] = return_numors_from_path(self.getPropertyValue("FluxWorkspace"))
        parameters["sensitivity_ws"] = return_numors_from_path(self.getPropertyValue("SensitivityWorkspace"))
        parameters["mask_ws"] = return_numors_from_path(self.getPropertyValue("MaskWorkspace"))
        # when all is set, a common function can set them all
        add_correction_information(ws, parameters)

    def reset(self):
        """Resets the class member variables"""
        self.instrument = None
        self.mode = None
        self.n_frames = None
        self.n_reports = None
        self.n_samples = None
        self.process = None
        self.progress = None

    def setup(self, ws):
        """Performs a full setup, which can be done only after having loaded the sample data"""
        self.process = self.getPropertyValue("ProcessAs")
        self.instrument = ws.getInstrument().getName()
        self.log().notice(f"Set the instrument name to {self.instrument}")
        unit = ws.getAxis(0).getUnit().unitID()
        self.n_frames = ws.blocksize()
        if self.n_frames > 1:
            if unit == "Wavelength":
                self.mode = AcqMode.TOF
            elif unit == "TOF":
                self.mode = AcqMode.REVENT
            else:
                self.mode = AcqMode.KINETIC
        else:
            self.mode = AcqMode.MONO
        # store the n_frames and acq mode for future reference
        AddSampleLog(Workspace=ws, LogName="N_frames", LogType="Number", NumberType="Int", LogText=str(self.n_frames))
        AddSampleLog(Workspace=ws, LogName="AcqMode", LogType="Number", NumberType="Int", LogText=str(int(self.mode)))
        self.log().notice(f"Set the acquisition mode to {self.mode}")
        if self.mode == AcqMode.KINETIC:
            if self.process == "EmptyContainer":
                SortXAxis(InputWorkspace=ws, OutputWorkspace=ws)
            elif self.process != "Sample" and self.process != "Transmission":
                raise RuntimeError("Only the sample and transmission can be kinetic measurements, the calibration measurements cannot.")

    def check_zero_monitor(self, mon_ws):
        """
        Throws an error, if the sum of monitor spectra is not strictly positive.
        Logs an error, if there is a bin in monitor spectra that is not strictly positive.
        """
        if np.sum(mtd[mon_ws].readY(0)) <= 0:
            raise RuntimeError(
                "Normalise by monitor requested, but the monitor spectrum has no positive counts, please switch to time normalization."
            )
        if np.any(mtd[mon_ws].readY(0) <= 0):
            self.log().error(
                "Some bins in the monitor spectra have no positive counts, please check the monitor data, or switch to time normalization."
            )

    # ==============================METHODS TO APPLY CORRECTIONS==============================#

    def apply_normalisation(self, ws):
        """Normalizes the workspace by monitor (default) or acquisition time"""
        normalise_by = self.getPropertyValue("NormaliseBy")
        if self.mode != AcqMode.TOF:
            if normalise_by == "Monitor":
                mon = ws + "_mon"
                mon_ws_index = mtd[ws].getNumberHistograms() + real_monitor_ws_neg_index(self.instrument)
                ExtractSpectra(InputWorkspace=ws, WorkspaceIndexList=mon_ws_index, OutputWorkspace=mon)
                self.check_zero_monitor(mon)
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws, WarnOnZeroDivide=False)
                DeleteWorkspace(mon)
            elif normalise_by == "Time":
                # the durations are stored in the second monitor, after preprocessing step
                mon = ws + "_duration"
                mon_ws_index = mtd[ws].getNumberHistograms() + blank_monitor_ws_neg_index(self.instrument)
                ExtractSpectra(InputWorkspace=ws, WorkspaceIndexList=mon_ws_index, OutputWorkspace=mon)
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws, WarnOnZeroDivide=False)
                self.apply_dead_time(ws)
                DeleteWorkspace(mon)
        else:
            if normalise_by == "Monitor":
                mon = ws + "_mon"
                mon_ws_index = mtd[ws].getNumberHistograms() + real_monitor_ws_neg_index(self.instrument)
                ExtractSpectra(InputWorkspace=ws, WorkspaceIndexList=mon_ws_index, OutputWorkspace=mon)
                self.broadcast_tof(ws, mon)
                RebinToWorkspace(WorkspaceToMatch=ws, WorkspaceToRebin=mon, OutputWorkspace=mon)
                self.check_zero_monitor(mon)
                Divide(LHSWorkspace=ws, RHSWorkspace=mon, OutputWorkspace=ws, WarnOnZeroDivide=False)
                DeleteWorkspace(mon)
            elif normalise_by == "Time":
                Scale(InputWorkspace=ws, Factor=1.0 / mtd[ws].getRun()["duration"].value, OutputWorkspace=ws)
                self.apply_dead_time(ws)
        # regardless on normalisation mask out the monitors not to skew the scale in the instrument viewer
        # but do not extract them, since extracting by ID is slow, so just leave them masked
        MaskDetectors(Workspace=ws, DetectorList=monitor_id(self.instrument))

    def apply_dead_time(self, ws):
        """Performs the dead time correction"""
        instrument = mtd[ws].getInstrument()
        if instrument.hasParameter("tau"):
            tau = instrument.getNumberParameter("tau")[0]
            if self.instrument == "D33" or self.instrument == "D11B":
                grouping_filename = self.instrument + "_Grouping.xml"
                grouping_file = os.path.join(config["groupingFiles.directory"], grouping_filename)
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, MapFile=grouping_file, OutputWorkspace=ws)
            elif instrument.hasParameter("grouping"):
                pattern = instrument.getStringParameter("grouping")[0]
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, GroupingPattern=pattern, OutputWorkspace=ws)
            else:
                self.log().warning("No grouping available in IPF, dead time correction will be performed detector-wise.")
                DeadTimeCorrection(InputWorkspace=ws, Tau=tau, OutputWorkspace=ws)
        else:
            self.log().notice("No tau available in IPF, skipping dead time correction.")

    def apply_dark_current(self, ws):
        """Applies Cd/B4C subtraction"""
        cadmium_ws = self.getPropertyValue("DarkCurrentWorkspace")
        if cadmium_ws:
            check_processed_flag(mtd[cadmium_ws], "DarkCurrent")
            if self.mode == AcqMode.TOF:
                cadmium_ws_rebin = cadmium_ws + "_rebinned"
                RebinToWorkspace(WorkspaceToRebin=cadmium_ws, WorkspaceToMatch=ws, OutputWorkspace=cadmium_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=cadmium_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(cadmium_ws_rebin)
            else:
                Minus(LHSWorkspace=ws, RHSWorkspace=cadmium_ws, OutputWorkspace=ws)

    def apply_direct_beam(self, ws):
        """Applies the beam center correction"""
        beam_ws = self.getPropertyValue("EmptyBeamWorkspace")
        if beam_ws:
            check_processed_flag(mtd[beam_ws], "EmptyBeam")
            check_distances_match(mtd[ws], mtd[beam_ws])
            if self.mode != AcqMode.TOF:
                run = mtd[beam_ws].getRun()
                beam_x = run["BeamCenterX"].value
                beam_y = run["BeamCenterY"].value
                AddSampleLog(Workspace=ws, LogName="BeamCenterX", LogText=str(beam_x), LogType="Number")
                AddSampleLog(Workspace=ws, LogName="BeamCenterY", LogText=str(beam_y), LogType="Number")
                AddSampleLog(Workspace=ws, LogName="beam_ws", LogText=beam_ws, LogType="String")
                self.apply_multipanel_beam_center_corr(ws, beam_x, beam_y)
                if "BeamWidthX" in run:
                    AddSampleLog(Workspace=ws, LogName="BeamWidthX", LogText=str(run["BeamWidthX"].value), LogType="Number", LogUnit="rad")

    def apply_multipanel_beam_center_corr(self, ws, beam_x, beam_y):
        """Applies the beam center correction on multipanel detectors"""
        instrument = mtd[ws].getInstrument()
        l2_main = mtd[ws].getRun()["L2"].value
        if instrument.hasParameter("detector_panels"):
            panel_names = instrument.getStringParameter("detector_panels")[0].split(",")
            for panel in panel_names:
                l2_panel = instrument.getComponentByName(panel).getPos()[2]
                MoveInstrumentComponent(Workspace=ws, X=-beam_x * l2_panel / l2_main, Y=-beam_y * l2_panel / l2_main, ComponentName=panel)
        else:
            MoveInstrumentComponent(Workspace=ws, X=-beam_x, Y=-beam_y, ComponentName="detector")

    def apply_flux(self, ws):
        """Applies empty beam flux absolute scale normalisation"""
        flux_ws = self.getPropertyValue("FluxWorkspace")
        if flux_ws:
            if self.mode == AcqMode.TOF:
                tmp = flux_ws + "_rebinned"
                CloneWorkspace(InputWorkspace=flux_ws, OutputWorkspace=tmp)
                self.broadcast_tof(ws, tmp)
                RebinToWorkspace(WorkspaceToRebin=tmp, WorkspaceToMatch=ws, OutputWorkspace=tmp)
                Divide(LHSWorkspace=ws, RHSWorkspace=tmp, OutputWorkspace=ws, WarnOnZeroDivide=False)
                DeleteWorkspace(tmp)
            else:
                Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)
            AddSampleLog(Workspace=ws, LogText="True", LogType="String", LogName="NormalisedByFlux")
            AddSampleLog(Workspace=ws, LogName="flux_ws", LogText=flux_ws, LogType="String")

    def apply_transmission(self, ws):
        """Applies transmission correction"""
        tr_ws = self.getPropertyValue("TransmissionWorkspace")
        theta_dependent = self.getProperty("TransmissionThetaDependent").value
        if tr_ws:
            check_processed_flag(mtd[tr_ws], "Transmission")
            if self.mode == AcqMode.TOF:
                # wavelength dependent transmission, need to rebin
                tr_ws_rebin = tr_ws + "_tr_rebinned"
                RebinToWorkspace(WorkspaceToRebin=tr_ws, WorkspaceToMatch=ws, OutputWorkspace=tr_ws_rebin)
                ApplyTransmissionCorrection(
                    InputWorkspace=ws, TransmissionWorkspace=tr_ws_rebin, ThetaDependent=theta_dependent, OutputWorkspace=ws
                )
                mtd[ws].getRun().addProperty("sample.transmission", list(mtd[tr_ws_rebin].readY(0)), True)
                DeleteWorkspace(tr_ws_rebin)
            else:
                check_wavelengths_match(mtd[tr_ws], mtd[ws])
                tr_to_apply = tr_ws
                needs_broadcasting = self.mode == AcqMode.KINETIC and mtd[tr_ws].blocksize() < mtd[ws].blocksize()
                if needs_broadcasting:
                    # if the sample is kinetic, but the transmission is monochromatic, need to broadcast
                    # sometimes, the transmission itself can be kinetic, in which case there is nothing to do
                    # furthermore, in some configurations the same scattering run can be used for transmission calculation
                    tr_to_apply = self.broadcast_kinetic(tr_ws)
                ApplyTransmissionCorrection(
                    InputWorkspace=ws, TransmissionWorkspace=tr_to_apply, ThetaDependent=theta_dependent, OutputWorkspace=ws
                )
                mtd[ws].getRun().addProperty("sample.transmission", list(mtd[tr_to_apply].readY(0)), True)
                if needs_broadcasting:
                    DeleteWorkspace(tr_to_apply)
                if theta_dependent and self.instrument == "D16" and 75 < mtd[ws].getRun()["Gamma.value"].value < 105:
                    # D16 can do wide angles, which means it can cross 90 degrees, where theta dependent transmission is divergent
                    # gamma is the detector center's theta, if it is in a certain range, then some pixels are around 90 degrees
                    MaskAngle(Workspace=ws, MinAngle=89, MaxAngle=91, Angle="TwoTheta")

    def apply_container(self, ws):
        """Applies empty container subtraction"""
        can_ws = self.getPropertyValue("EmptyContainerWorkspace")
        if can_ws:
            check_processed_flag(mtd[can_ws], "EmptyContainer")
            check_distances_match(mtd[can_ws], mtd[ws])
            if self.mode == AcqMode.TOF:
                # wavelength dependent subtraction, need to rebin
                can_ws_rebin = can_ws + "_rebinned"
                RebinToWorkspace(WorkspaceToRebin=can_ws, WorkspaceToMatch=ws, OutputWorkspace=can_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=can_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(can_ws_rebin)
            else:
                check_wavelengths_match(mtd[can_ws], mtd[ws])
                if mtd[can_ws].blocksize() > 1:  # kinetic/scan mode
                    # if axes are widely inconsistent, then a warning is raised:
                    check_axis_match(mtd[can_ws], mtd[ws])
                    # whether they match or not, the X axis of the empty container is replaced by sample's one
                    CreateWorkspace(
                        DataX=mtd[ws].extractX(),
                        DataY=mtd[can_ws].extractY(),
                        DataE=mtd[can_ws].extractE(),
                        ParentWorkspace=can_ws,
                        OutputWorkspace=can_ws,
                        NSpec=mtd[can_ws].getNumberHistograms(),
                        UnitX=mtd[can_ws].getAxis(0).getUnit().unitID(),
                    )
                Minus(LHSWorkspace=ws, RHSWorkspace=can_ws, OutputWorkspace=ws)

    def apply_water(self, ws):
        """Applies flat-field (water) normalisation for detector efficiency and absolute scale"""
        flat_ws = self.getPropertyValue("FlatFieldWorkspace")
        if flat_ws:
            check_processed_flag(mtd[flat_ws], "Water")
            # do not check for distance, since flat field is typically not available at the longest distances
            if self.mode != AcqMode.TOF:
                check_wavelengths_match(mtd[flat_ws], mtd[ws])
            # flat field is time-independent, so even for tof and kinetic it must be just one frame
            Divide(LHSWorkspace=ws, RHSWorkspace=flat_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
            Scale(InputWorkspace=ws, Factor=self.getProperty("WaterCrossSection").value, OutputWorkspace=ws)
            # copy the mask of water to the ws as it might be larger than the beam stop used for ws
            MaskDetectors(Workspace=ws, MaskedWorkspace=flat_ws)
            # rescale the absolute scale if the ws and flat field are at different distances
            self.rescale_flux(ws, flat_ws)

    def apply_sensitivity(self, ws):
        """Applies inter-pixel detector efficiency map"""
        sens_ws = self.getPropertyValue("SensitivityWorkspace")
        if sens_ws:
            Divide(LHSWorkspace=ws, RHSWorkspace=sens_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
            # copy the mask of sensitivity to the ws as it might be larger than the beam stop used for ws
            MaskDetectors(Workspace=ws, MaskedWorkspace=sens_ws)

    def apply_solvent(self, ws):
        """Applies pixel-by-pixel solvent/buffer subtraction"""
        solvent_ws = self.getPropertyValue("SolventWorkspace")
        if solvent_ws:
            check_processed_flag(mtd[solvent_ws], "Solvent")
            check_distances_match(mtd[solvent_ws], mtd[ws])
            if self.mode == AcqMode.TOF:
                # wavelength dependent subtraction, need to rebin
                solvent_ws_rebin = solvent_ws + "_tr_rebinned"
                RebinToWorkspace(WorkspaceToRebin=solvent_ws, WorkspaceToMatch=ws, OutputWorkspace=solvent_ws_rebin)
                Minus(LHSWorkspace=ws, RHSWorkspace=solvent_ws_rebin, OutputWorkspace=ws)
                DeleteWorkspace(solvent_ws_rebin)
            else:
                check_wavelengths_match(mtd[solvent_ws], mtd[ws])
                Minus(LHSWorkspace=ws, RHSWorkspace=solvent_ws, OutputWorkspace=ws)

    def apply_solid_angle(self, ws: str):
        """
        Calculates the solid angle and divides by it.
        @param ws: the name of the workspace on which to apply the solid angle correction
        """
        sa_ws = ws + "_solidangle"
        method = "Rectangle "

        # D22B has the front panel tilted, hence the Rectangle approximation is wrong
        # D16 can be rotated around the sample, where again rectangle is wrong unless we rotate back
        if self.instrument == "D22B" or self.instrument == "D16":
            method = "GenericShape"
        # new D16 is a banana detector
        elif self.instrument == "D16B":
            method = "VerticalTube"

        SolidAngle(InputWorkspace=ws, OutputWorkspace=sa_ws, Method=method)
        Divide(LHSWorkspace=ws, RHSWorkspace=sa_ws, OutputWorkspace=ws, WarnOnZeroDivide=False)
        DeleteWorkspace(Workspace=sa_ws)

    def apply_masks(self, ws):
        """
        Applies default (edges and permanently bad pixels) and beam stop masks
        Masks can be created in many ways: manually, via MaskBTP, etc.
        When creating a mask with instrument viewer, there are 2 possibilities:
        o) hitting the button Apply and Save as > Detector Mask to Workspace
            This is the legacy way of propagating the masks.
            In this case the workspace is a SpecialWorkspace2D which just contains 1s as intensity for the detector that is masked
        o) hitting the button Apply to Data
            In this case the workspace has a proper mask.
        """
        edge_mask = self.getProperty("DefaultMaskWorkspace").value
        if edge_mask and (isinstance(edge_mask, SpecialWorkspace2D) or edge_mask.detectorInfo().hasMaskedDetectors()):
            MaskDetectors(Workspace=ws, MaskedWorkspace=edge_mask)
        beam_stop_mask = self.getProperty("MaskWorkspace").value
        if beam_stop_mask and (isinstance(beam_stop_mask, SpecialWorkspace2D) or beam_stop_mask.detectorInfo().hasMaskedDetectors()):
            MaskDetectors(Workspace=ws, MaskedWorkspace=beam_stop_mask)

    def apply_thickness(self, ws):
        """Normalises by sample thickness"""
        thicknesses = self.getProperty("SampleThickness").value
        length = len(thicknesses)
        if length == 1 and thicknesses[0] > 0:
            Scale(InputWorkspace=ws, Factor=1 / thicknesses[0], OutputWorkspace=ws)
        else:
            if thicknesses[0] < 0:
                if "sample.thickness" in mtd[ws].getRun():
                    thicknesses = mtd[ws].getRun()["sample.thickness"].value.split(",")
                else:
                    raise RuntimeError("Unable to find sample thickness from files, consider providing them manually.")
            thick_ws = ws + "_thickness"
            CreateWorkspace(NSpec=1, OutputWorkspace=thick_ws, DataY=np.array(thicknesses), DataE=np.zeros(length), DataX=np.arange(length))
            thick_to_apply = thick_ws
            if self.mode == AcqMode.KINETIC:
                thick_to_apply = self.broadcast_kinetic(thick_ws)
            Divide(LHSWorkspace=ws, RHSWorkspace=thick_to_apply, OutputWorkspace=ws)
            DeleteWorkspace(thick_to_apply)
            if mtd[thick_ws]:
                DeleteWorkspace(thick_ws)

    def apply_parallax(self, ws):
        """Applies the parallax correction"""
        components = ["detector"]
        offsets = [0.0]
        if self.instrument == "D22B":
            # The front detector of D22B is often tilted around its own axis
            # The tilt angle must be subtracted from 2thetas before putting them into the parallax correction formula
            # TODO: note that in cycle 211, the front detector was Detector 1, so we should rather get it from IPF
            offsets.append(mtd[ws].getRun()["Detector 2.dan2_actual"].value)
        if mtd[ws].getInstrument().hasParameter("detector_panels"):
            components = mtd[ws].getInstrument().getStringParameter("detector_panels")[0].split(",")
        if self.instrument in ["D11B", "D22", "D22lr", "D22B", "D33"]:
            ParallaxCorrection(InputWorkspace=ws, OutputWorkspace=ws, ComponentNames=components, AngleOffsets=offsets)

    # ===============================HELPER METHODS===============================#

    def rescale_flux(self, ws, flat_ws):
        """
        Rescales the absolute scale if ws and flat_ws are at different distances
        If both sample and flat runs are normalised by flux, there is nothing to do, they are both in abs scale
        If one is normalised, the other is not, we raise an error
        If neither is normalised by flux, only then we have to rescale by the factor
        """
        message = (
            "Sample and flat field runs are not consistent in terms of flux normalisation; "
            "unable to perform flat field normalisation. "
            "Make sure either they are both normalised or both not normalised by direct beam flux."
        )
        run = mtd[ws].getRun()
        run_ref = mtd[flat_ws].getRun()
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
                self.do_rescale_flux(ws, flat_ws)
        # Case where the "NormalisedByFlux" property has not been set to both flat field and data
        # Do the rescaling
        else:
            self.do_rescale_flux(ws, flat_ws)

    def do_rescale_flux(self, ws, flat_ws):
        """
        Scales ws by the flux factor wrt the flat field
        Formula 14, Grillo I. (2008) Small-Angle Neutron Scattering and Applications in Soft Condensed Matter
        """
        if self.mode != AcqMode.TOF:
            check_wavelengths_match(mtd[ws], mtd[flat_ws])
        sample_l2 = mtd[ws].getRun()["L2"].value
        ref_l2 = mtd[flat_ws].getRun()["L2"].value
        flux_factor = (sample_l2**2) / (ref_l2**2)
        self.log().notice(f"Flux factor is: {flux_factor}")
        Scale(InputWorkspace=ws, Factor=flux_factor, OutputWorkspace=ws)

    def broadcast_kinetic(self, ws):
        """
        Broadcasts the given workspace to the dimensions of the sample workspace
        Repeats the values by the number of frames in order to allow vectorized application of the correction
        Here we are matching in terms of the x-axis size
        """
        x = mtd[ws].readX(0)
        y = mtd[ws].readY(0)
        e = mtd[ws].readE(0)
        out = ws + "_broadcast"
        CreateWorkspace(
            ParentWorkspace=ws,
            OutputWorkspace=out,
            NSpec=1,
            DataX=np.repeat(x, self.n_frames),
            DataY=np.repeat(y, self.n_frames),
            DataE=np.repeat(e, self.n_frames),
        )
        return out

    def broadcast_tof(self, ws, flux):
        """
        Broadcasts the direct beam flux that can be easily rebinned at application time
        For TOF, the flux is wavelength dependent, and the sample workspace is ragged
        Hence the flux must be rebinned to the sample before it can be divided by flux
        That's why we broadcast the empty beam workspace to the same size as the sample
        This allows rebinning spectrum-wise when processing the sample w/o tiling the empty beam data for each sample
        Here we are matching in terms of vertical axis size (i.e. number of spectra)
        """
        nspec = mtd[ws].getNumberHistograms()
        x = mtd[flux].readX(0)
        y = mtd[flux].readY(0)
        e = mtd[flux].readE(0)
        CreateWorkspace(
            DataX=x,
            DataY=np.tile(y, nspec),
            DataE=np.tile(e, nspec),
            NSpec=nspec,
            ParentWorkspace=ws,
            OutputWorkspace=flux,
            UnitX=mtd[ws].getAxis(0).getUnit().unitID(),
        )

    def fit_beam_width(self, ws):
        """'Groups detectors vertically and fits the horizontal beam width with a Gaussian profile"""
        tmp_ws = ws + "_beam_width"
        CloneWorkspace(InputWorkspace=ws, OutputWorkspace=tmp_ws)
        grouping_pattern = get_vertical_grouping_pattern(tmp_ws)
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
        AddSampleLog(Workspace=ws, LogName="BeamWidthX", LogText=str(beam_width), LogType="Number", LogUnit="rad")
        DeleteWorkspaces(
            WorkspaceList=[
                tmp_ws,
                tmp_ws + "_fit_output_Parameters",
                tmp_ws + "_fit_output_Workspace",
                tmp_ws + "_fit_output_NormalisedCovarianceMatrix",
            ]
        )

    def inject_blank_samples(self, ws):
        """Creates blank workspaces with the right dimension to include in the input list of workspaces"""
        reference = mtd[ws][0]
        nbins = self.n_frames
        nspec = reference.getNumberHistograms()
        xaxis = reference.getAxis(0).extractValues()
        runs = self.getPropertyValue("Runs").split(",")
        result = [w.getName() for w in mtd[ws]]
        blank_indices = [i for i, r in enumerate(runs) if r == EMPTY_TOKEN]
        for index in blank_indices:
            blank = f"__blank_{index}"
            CreateWorkspace(
                ParentWorkspace=reference,
                NSpec=nspec,
                DataX=np.tile(xaxis, nspec),
                DataY=np.zeros(nspec * nbins),
                DataE=np.zeros(nspec * nbins),
                OutputWorkspace=blank,
                UnitX="Empty",
            )
            result.insert(index, blank)
        return result

    def inject_replica_transmissions(self, ws):
        """Repeats the transmission workspaces if they are reused for several samples"""
        result = []
        runs = self.getPropertyValue("Runs").split(",")
        runs_unique = self.remove_repeated(runs)
        if len(runs_unique) < len(runs):
            for index, run in enumerate(runs):
                to_clone_index = runs_unique.index(run)
                replica = f"__replica_{index}"
                CloneWorkspace(InputWorkspace=mtd[ws][to_clone_index], OutputWorkspace=replica)
                result.append(replica)
            DeleteWorkspace(ws)
        else:
            result = [w.getName() for w in mtd[ws]]
        return result

    def remove_repeated(self, list):
        """Removes the repetitions from the list"""
        result = []
        for it in list:
            if it not in result:
                result.append(it)
        return result

    def set_process_as(self, ws):
        """Sets the process as flag as sample log for future sanity checks"""
        AddSampleLog(Workspace=ws, LogName="ProcessedAs", LogText=self.process)

    def linearize_axis(self, ws):
        """Linearizes x-axis for a single rebinned event workspace to transform it to kinetic"""
        x = np.arange(mtd[ws].blocksize())
        for s in range(mtd[ws].getNumberHistograms()):
            mtd[ws].setX(s, x)

    # ===============================METHODS TO TREAT PROCESS TYPES===============================#

    def treat_empty_beam(self, ws):
        """Processes as empty beam, i.e. calculates beam center, beam width and incident flux"""
        centers = ws + "_centers"
        radius = self.getProperty("BeamRadius").value
        FindCenterOfMassPosition(InputWorkspace=ws, DirectBeam=True, BeamRadius=radius, Output=centers)
        beam_x = mtd[centers].cell(0, 1)
        beam_y = mtd[centers].cell(1, 1)
        AddSampleLog(Workspace=ws, LogName="BeamCenterX", LogText=str(beam_x), LogType="Number", LogUnit="meters")
        AddSampleLog(Workspace=ws, LogName="BeamCenterY", LogText=str(beam_y), LogType="Number", LogUnit="meters")
        DeleteWorkspace(centers)
        # correct for beam center before calculating the beam width and flux
        self.apply_multipanel_beam_center_corr(ws, beam_x, beam_y)
        if self.mode != AcqMode.TOF:
            self.fit_beam_width(ws)
        self.calculate_flux(ws)

    def calculate_flux(self, ws):
        """Calculates the incident flux"""
        if self.process == "EmptyBeam":
            flux = self.getPropertyValue("OutputFluxWorkspace")
            if not flux:
                return
        elif self.process == "Transmission":
            flux = ws
        run = mtd[ws].getRun()
        att_coeff = 1.0
        if run.hasProperty("attenuator.attenuation_coefficient"):
            att_coeff = run["attenuator.attenuation_coefficient"].value
        elif run.hasProperty("attenuator.attenuation_value"):
            att_value = run["attenuator.attenuation_value"].value
            if float(att_value) < 10.0 and self.instrument == "D33":
                # for D33, it's not always the attenuation value, it could be the index of the attenuator
                # if it is <10, we consider it's the index and take the corresponding value from the IPF
                instrument = mtd[ws].getInstrument()
                param = "att" + str(int(att_value))
                if instrument.hasParameter(param):
                    att_coeff = instrument.getNumberParameter(param)[0]
                else:
                    raise RuntimeError(f"Unable to find the attenuation coefficient for D33 attenuator #{att_value}")
            else:
                att_coeff = float(att_value)
        self.log().information("Attenuator 1 coefficient/value: {0}".format(att_coeff))
        if run.hasProperty("attenuator2.attenuation_value"):
            # D22 can have the second, chopper attenuator
            # In principle, either of the 2 attenuators can be in or out
            # In practice, only one (standard or chopper) is in at a time
            # If one is out, its attenuation_value is set to 1, so it's safe to take the product
            att2_value = run["attenuator2.attenuation_value"].value
            self.log().information(f"Attenuator 2 coefficient/value: {att2_value}")
            att_coeff *= float(att2_value)
        self.log().information(f"Attenuation coefficient used is: {att_coeff}")
        radius = self.getProperty("TransmissionBeamRadius").value
        CalculateFlux(InputWorkspace=ws, OutputWorkspace=flux, BeamRadius=radius)
        Scale(InputWorkspace=flux, Factor=att_coeff, OutputWorkspace=flux)
        if self.process == "EmptyBeam":
            self.setProperty("OutputFluxWorkspace", flux)

    def calculate_transmission(self, ws):
        """Calculates the transmission"""
        flux_ws = self.getPropertyValue("FluxWorkspace")
        check_distances_match(mtd[ws], mtd[flux_ws])
        self.calculate_flux(ws)
        if self.mode != AcqMode.TOF:
            check_wavelengths_match(mtd[ws], mtd[flux_ws])
        else:
            RebinToWorkspace(WorkspaceToRebin=ws, WorkspaceToMatch=flux_ws, OutputWorkspace=ws)
        Divide(LHSWorkspace=ws, RHSWorkspace=flux_ws, OutputWorkspace=ws)

    def generate_sensitivity(self, ws):
        """Creates relative inter-pixel efficiency map"""
        sens = self.getPropertyValue("OutputSensitivityWorkspace")
        if sens:
            CalculateEfficiency(
                InputWorkspace=ws,
                OutputWorkspace=sens,
                MinThreshold=self.getProperty("MinThreshold").value,
                MaxThreshold=self.getProperty("MaxThreshold").value,
            )
            self.setProperty("OutputSensitivityWorkspace", mtd[sens])

    # ===============================CORE LOGIC OF LOAD AND REDUCE===============================#

    def load(self):
        """
        Loads, merges and concatenates the input runs, as appropriate.
        TODO: Consider moving this out to a separate algorithm, once v2 of the loader is in
        There it could load the instrument only with the first run to save some time
        The main complexity here is the injection of blanks (for sample) and replicas (for transmission)
        """
        ws = self.getPropertyValue("OutputWorkspace")
        if not self.getProperty("Runs").isDefault:
            tmp = f"__{ws}"
            runs = self.getPropertyValue("Runs").split(",")
            non_blank_runs = list(filter(lambda x: x != EMPTY_TOKEN, runs))
            blank_runs = list(filter(lambda x: x == EMPTY_TOKEN, runs))

            nreports = len(non_blank_runs) + self.processes.index(self.getPropertyValue("ProcessAs")) + 2
            self.progress = Progress(self, start=0.0, end=1.0, nreports=nreports)

            if self.getPropertyValue("ProcessAs") == "Transmission":
                # sometimes the same transmission can be applied to many samples
                non_blank_runs = self.remove_repeated(non_blank_runs)

            LoadAndMerge(
                Filename=",".join(non_blank_runs), OutputWorkspace=tmp, startProgress=0.0, endProgress=len(non_blank_runs) / nreports
            )

            self.progress.report(len(non_blank_runs), "Loaded")
        else:
            # no runs have been provided, so the user has given an already loaded workspace
            tmp = self.getPropertyValue("SampleWorkspace")
            blank_runs = []
            self.progress = Progress(self, start=0.0, end=1.0, nreports=10)

        if isinstance(mtd[tmp], MatrixWorkspace) and blank_runs:
            # if we loaded a single workspace but there are blanks, need to make a group so the blanks can be inserted
            RenameWorkspace(InputWorkspace=tmp, OutputWorkspace=tmp + "_0")
            GroupWorkspaces(InputWorkspaces=[tmp + "_0"], OutputWorkspace=tmp)

        if isinstance(mtd[tmp], WorkspaceGroup):
            # the setup is performed based on the first workspace
            # this assumes that in one call of this algorithm, all the input runs are homogeneous; that is,
            # they correspond to the same instrument, same acquisition mode, and same number of frames for kinetic
            self.setup(mtd[tmp][0])
            self.preprocess_group(tmp)
            if self.mode != AcqMode.TOF:
                if self.process == "Sample" or self.process == "Transmission":
                    if self.process == "Sample":
                        ws_list = self.inject_blank_samples(tmp)
                    if self.process == "Transmission":
                        ws_list = self.inject_replica_transmissions(tmp)
                    ConjoinXRuns(InputWorkspaces=ws_list, OutputWorkspace=ws, LinearizeAxis=True)
                    DeleteWorkspaces(WorkspaceList=ws_list)
                else:
                    raise RuntimeError("Listing of runs in MONO mode is allowed only for sample and transmission measurements.")
            else:
                raise RuntimeError("Listing of runs is not allowed for TOF mode as concatenation of multiple runs is not possible.")
        else:
            self.setup(mtd[tmp])
            self.preprocess(tmp)
            if self.mode == AcqMode.REVENT or self.mode == AcqMode.MONO:
                self.linearize_axis(tmp)
            if tmp != ws:
                RenameWorkspace(InputWorkspace=tmp, OutputWorkspace=ws)

        self.set_process_as(ws)
        assert isinstance(mtd[ws], MatrixWorkspace)
        self.progress.report("Combined")

    def preprocess_group(self, wsg):
        """Preprocesses a loaded workspace group"""
        for ws in mtd[wsg]:
            self.preprocess(ws.name())

    def preprocess(self, ws):
        """
        Prepares the loaded workspace based on the acq mode
        """
        if self.mode != AcqMode.TOF:
            mtd[ws].getAxis(0).setUnit("Empty")
            run = mtd[ws].getRun()
            if "selector.wavelength" in run and "wavelength" not in run:
                AddSampleLog(
                    Workspace=ws, LogName="wavelength", LogText=str(run["selector.wavelength"].value), LogUnit="Angstrom", LogType="Number"
                )
        if self.mode == AcqMode.MONO:
            # TODO: all these must be done in v2 of the loader directly, so remove once it's in.
            ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
            blank_mon = mtd[ws].getNumberHistograms() + blank_monitor_ws_neg_index(self.instrument)
            run = mtd[ws].getRun()
            time = run["duration"].value
            if self.mode == AcqMode.MONO:
                mtd[ws].setY(blank_mon, np.array([time]))
                mtd[ws].setE(blank_mon, np.array([0.0]))
        if self.mode == AcqMode.REVENT:
            # if a workspace is a rebinned event, it is loaded by LoadNexusProcess and not by LoadILLSANS,
            # hence we have to prepare it similarly, including the placement of the detector
            durations = np.diff(mtd[ws].readX(0)) * 1e-6
            ConvertToPointData(InputWorkspace=ws, OutputWorkspace=ws)
            # note that, currently for event workspaces monitors are loaded to a separate workspace,
            # so monitor normalisation is not possible
            tmp_mon_spectra = "__tmp_mon_" + ws
            bsize = mtd[ws].blocksize()
            run = mtd[ws].getRun()
            CreateWorkspace(
                NSpec=2,
                DataX=np.tile(mtd[ws].readX(0), 2),
                DataY=np.ones(2 * bsize),
                DataE=np.zeros(2 * bsize),
                OutputWorkspace=tmp_mon_spectra,
            )
            AppendSpectra(InputWorkspace1=ws, InputWorkspace2=tmp_mon_spectra, OutputWorkspace=ws, ValidateInputs=False)
            blank_mon = mtd[ws].getNumberHistograms() + blank_monitor_ws_neg_index(self.instrument)
            mtd[ws].setY(blank_mon, durations)
            DeleteWorkspace(tmp_mon_spectra)
            l2 = main_detector_distance(run, self.instrument)
            AddSampleLog(Workspace=ws, LogName="L2", LogType="Number", LogText=str(l2), LogUnit="meters")
            self.place_detector(ws)

    def place_detector(self, ws):
        """
        Places the detector just as the LoadILLSANS would do.
        This is because event nexus are loaded by LoadEventNexus, and rebinned events are loaded by LoadNexusProcessed.
        None of those performs instrument specific placement, so we have to do it here.
        TODO: It is not good to repeat the logic of the LoadILLSANS, the only way to avoid this would be to create a small separate C++
        algorithm that does the placement, and it can be called both from within the loader, and here if needed; that is, for events.
        """
        instr = mtd[ws].getInstrument()
        run = mtd[ws].getRun()
        if instr.getName() == "D22B":
            back_pos = instr.getComponentByName("detector_back").getPos()
            distance = run["Detector 1.det1_calc"].value
            MoveInstrumentComponent(
                Workspace=ws,
                ComponentName="detector_back",
                RelativePosition=False,
                X=-run["Detector 1.dtr1_actual"].value / 1000.0,
                Y=back_pos[1],
                Z=distance,
            )
            front_pos = instr.getComponentByName("detector_front").getPos()
            MoveInstrumentComponent(
                Workspace=ws,
                ComponentName="detector_front",
                RelativePosition=False,
                X=-run["Detector 2.dtr2_actual"].value / 1000.0,
                Y=front_pos[1],
                Z=run["Detector 2.det2_calc"].value,
            )
            RotateInstrumentComponent(
                Workspace=ws,
                ComponentName="detector_front",
                RelativeRotation=False,
                Angle=-run["Detector 2.dan2_actual"].value,
                X=0,
                Y=1,
                Z=0,
            )
            AddSampleLog(Workspace=ws, LogName="L2", LogText=str(distance), LogType="Number")
        if instr.getName() == "D11B":
            back_pos = instr.getComponentByName("detector_center").getPos()
            det_pos = instr.getComponentByName("detector").getPos()
            distance = run["Detector 1.det_calc"].value - back_pos[2]
            MoveInstrumentComponent(Workspace=ws, ComponentName="detector", RelativePosition=False, X=det_pos[0], Y=det_pos[1], Z=distance)
            AddSampleLog(Workspace=ws, LogName="L2", LogText=str(distance), LogType="Number")

    def reduce(self):
        """
        Performs the corresponding reduction based on the process type
        This is the core logic of the reduction realised as a hard-wired dependency graph, for example:
        If we are processing the empty beam we apply the dark current correction and process as empty beam
        If we are processing transmission, we apply both dark current and empty beam corrections and process as transmission
        """
        ws = self.getPropertyValue("OutputWorkspace")

        self.apply_normalisation(ws)
        self.progress.report()
        if self.process != "DarkCurrent":
            self.apply_dark_current(ws)
            self.progress.report()
            if self.process == "EmptyBeam":
                self.treat_empty_beam(ws)
            else:
                self.apply_direct_beam(ws)
                self.progress.report()
                if self.process == "Transmission":
                    self.calculate_transmission(ws)
                else:
                    self.apply_transmission(ws)
                    self.apply_solid_angle(ws)
                    self.progress.report()
                    if self.process != "EmptyContainer":
                        self.apply_container(ws)
                        self.apply_masks(ws)
                        self.apply_parallax(ws)
                        self.apply_thickness(ws)
                        self.apply_flux(ws)
                        self.progress.report()
                        if self.process == "Water":
                            self.generate_sensitivity(ws)
                        else:
                            self.apply_water(ws)
                            self.apply_sensitivity(ws)
                            self.progress.report()
                            if self.process != "Solvent":
                                self.apply_solvent(ws)
                                self.progress.report()
        self._add_correction_information(ws)
        self.setProperty("OutputWorkspace", ws)

    def PyExec(self):
        """Entry point of the execution"""
        self.reset()
        self.load()
        self.reduce()


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSILLReduction)
