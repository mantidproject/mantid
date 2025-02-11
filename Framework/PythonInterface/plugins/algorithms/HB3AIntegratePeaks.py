# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    PythonAlgorithm,
    PropertyMode,
    ADSValidator,
    WorkspaceGroup,
    WorkspaceProperty,
    MultipleExperimentInfos,
    IPeaksWorkspace,
)
from mantid.kernel import Direction, FloatBoundedValidator, StringListValidator, StringArrayProperty
from mantid.simpleapi import (
    DeleteWorkspace,
    IntegratePeaksMD,
    SaveHKL,
    SaveReflections,
    CreatePeaksWorkspace,
    CopySample,
    AnalysisDataService,
    FilterPeaks,
    CombinePeaksWorkspaces,
    mtd,
)
import numpy as np


class HB3AIntegratePeaks(PythonAlgorithm):
    def category(self):
        return "Crystal\\Integration"

    def seeAlso(self):
        return ["HB3AFindPeaks", "IntegratePeaksMD", "HB3AAdjustSampleNorm", "HB3APredictPeaks"]

    def name(self):
        return "HB3AIntegratePeaks"

    def summary(self):
        return (
            "Integrates peaks from the input MDEvent workspace and can optionally apply a Lorentz correction to "
            "the output peaks workspace; output can be saved in different formats."
        )

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty("InputWorkspace", direction=Direction.Input, validator=ADSValidator()),
            doc="Input MDEvent workspace to use for integration",
        )

        self.declareProperty(
            StringArrayProperty("PeaksWorkspace", direction=Direction.Input, validator=ADSValidator()),
            doc="Peaks workspace containing peaks to integrate",
        )

        positive_val = FloatBoundedValidator(lower=0.0)
        self.declareProperty(
            "PeakRadius",
            defaultValue=1.0,
            validator=positive_val,
            doc="Fixed radius around each peak position in which to integrate (same units as input workspace) ",
        )

        self.declareProperty(
            "BackgroundInnerRadius", defaultValue=0.0, validator=positive_val, doc="Inner radius used to evaluate the peak background"
        )
        self.declareProperty(
            "BackgroundOuterRadius", defaultValue=0.0, validator=positive_val, doc="Outer radius used to evaluate the peak background"
        )

        self.declareProperty(
            "ApplyLorentz", defaultValue=True, doc="Whether the Lorentz correction should be applied to the integrated peaks"
        )

        self.declareProperty("RemoveZeroIntensity", defaultValue=True, doc="If to remove peaks with 0 or less intensity from the output")

        formats = StringListValidator()
        formats.addAllowedValue("SHELX")
        formats.addAllowedValue("Fullprof")
        self.declareProperty(
            "OutputFormat", defaultValue="SHELX", validator=formats, doc="Save direction cosines in HKL, or the fullprof format"
        )

        self.declareProperty(
            FileProperty(name="OutputFile", defaultValue="", direction=Direction.Input, action=FileAction.OptionalSave),
            doc="Filepath to save the integrated peaks workspace in HKL format",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output, optional=PropertyMode.Mandatory),
            doc="Output peaks workspace (copy of input with updated peak intensities)",
        )

    def validateInputs(self):
        issues = dict()

        # Make sure outer radius > inner radius
        inner_radius = self.getProperty("BackgroundInnerRadius").value
        outer_radius = self.getProperty("BackgroundOuterRadius").value

        if outer_radius < inner_radius:
            issues["BackgroundOuterRadius"] = "Outer radius should be >= to inner radius"

        input_workspaces, peak_workspaces = self._expand_groups()

        if len(input_workspaces) != len(peak_workspaces):
            issues["InputWorkspace"] = "The same number of PeaksWorkspace must be provided as InputWorkspace"

        for input_ws in input_workspaces:
            workspace = AnalysisDataService[input_ws]
            if not (isinstance(workspace, MultipleExperimentInfos)):
                issues["InputWorkspace"] = "Workspace need to be a MDEventWorkspace"
            elif workspace.getSpecialCoordinateSystem().name != "QSample":
                issues["InputWorkspace"] = "Input workspace expected to be in QSample, workspace is in '{}'".format(
                    workspace.getSpecialCoordinateSystem().name
                )
            elif workspace.getNumDims() != 3:
                issues["InputWorkspace"] = "Workspace has the wrong number of dimensions"

        for peak_ws in peak_workspaces:
            if not isinstance(AnalysisDataService[peak_ws], IPeaksWorkspace):
                issues["PeaksWorkspace"] = "Workspace need to be a PeaksWorkspace or LeanElasticPeaksWorkspace"

        return issues

    def PyExec(self):
        input_workspaces, peak_workspaces = self._expand_groups()
        output_workspace_name = self.getPropertyValue("OutputWorkspace")

        peak_radius = self.getProperty("PeakRadius").value
        inner_radius = self.getProperty("BackgroundInnerRadius").value
        outer_radius = self.getProperty("BackgroundOuterRadius").value

        remove_0_intensity = self.getProperty("RemoveZeroIntensity").value
        use_lorentz = self.getProperty("ApplyLorentz").value

        multi_ws = len(input_workspaces) > 1

        output_workspaces = []

        for input_ws, peak_ws in zip(input_workspaces, peak_workspaces):
            if multi_ws:
                peaks_ws_name = input_ws + "_" + output_workspace_name
                output_workspaces.append(peaks_ws_name)
            else:
                peaks_ws_name = output_workspace_name

            IntegratePeaksMD(
                InputWorkspace=input_ws,
                PeakRadius=peak_radius,
                BackgroundInnerRadius=inner_radius,
                BackgroundOuterRadius=outer_radius,
                PeaksWorkspace=peak_ws,
                OutputWorkspace=peaks_ws_name,
            )

        if multi_ws:
            peaks_ws_name = output_workspace_name
            CreatePeaksWorkspace(
                InstrumentWorkspace=input_workspaces[0],
                NumberOfPeaks=0,
                OutputWorkspace=peaks_ws_name,
                OutputType=mtd[peak_workspaces[0]].id().replace("sWorkspace", ""),
            )
            CopySample(
                InputWorkspace=output_workspaces[0],
                OutputWorkspace=peaks_ws_name,
                CopyName=False,
                CopyMaterial=False,
                CopyEnvironment=False,
                CopyShape=False,
                CopyLattice=True,
            )
            for peak_ws in output_workspaces:
                CombinePeaksWorkspaces(peaks_ws_name, peak_ws, OutputWorkspace=peaks_ws_name)
                DeleteWorkspace(peak_ws)

        if use_lorentz:
            # Apply Lorentz correction:
            peaks = AnalysisDataService[peaks_ws_name]
            for p in range(peaks.getNumberPeaks()):
                peak = peaks.getPeak(p)
                lorentz = abs(np.sin(peak.getScattering()) * np.cos(peak.getAzimuthal()))
                peak.setIntensity(peak.getIntensity() * lorentz)
                peak.setSigmaIntensity(peak.getSigmaIntensity() * lorentz)

        if remove_0_intensity:
            FilterPeaks(
                InputWorkspace=peaks_ws_name, OutputWorkspace=peaks_ws_name, FilterVariable="Intensity", FilterValue=0, Operator=">"
            )

        # Write output only if a file path was provided
        if not self.getProperty("OutputFile").isDefault:
            out_format = self.getProperty("OutputFormat").value
            filename = self.getProperty("OutputFile").value

            if out_format == "SHELX":
                SaveHKL(InputWorkspace=peaks_ws_name, Filename=filename, DirectionCosines=True, OutputWorkspace="__tmp")
                DeleteWorkspace("__tmp")
            elif out_format == "Fullprof":
                SaveReflections(InputWorkspace=peaks_ws_name, Filename=filename, Format="Fullprof")
            else:
                # This shouldn't happen
                RuntimeError("Invalid output format given")

        self.setProperty("OutputWorkspace", AnalysisDataService[peaks_ws_name])

    def _expand_groups(self):
        """expand workspace groups"""
        workspaces = self.getProperty("InputWorkspace").value
        input_workspaces = []
        for wsname in workspaces:
            wks = AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                input_workspaces.extend(wks.getNames())
            else:
                input_workspaces.append(wsname)

        workspaces = self.getProperty("PeaksWorkspace").value
        peaks_workspaces = []
        for wsname in workspaces:
            wks = AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                peaks_workspaces.extend(wks.getNames())
            else:
                peaks_workspaces.append(wsname)

        return input_workspaces, peaks_workspaces


AlgorithmFactory.subscribe(HB3AIntegratePeaks)
