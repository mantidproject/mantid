# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, FileAction, FileProperty, IMDEventWorkspaceProperty, IPeaksWorkspaceProperty, \
    PythonAlgorithm, PropertyMode
from mantid.kernel import Direction, FloatBoundedValidator, StringListValidator
from mantid.simpleapi import DeleteWorkspace, IntegratePeaksMD, SaveHKL, SaveReflections
import numpy as np


class HB3AIntegratePeaks(PythonAlgorithm):

    def category(self):
        return "Crystal\\Integration"

    def seeAlso(self):
        return ["HB3AFindPeaks", "IntegratePeaksMD"]

    def name(self):
        return "HB3AIntegratePeaks"

    def summary(self):
        return 'Integrates peaks from the input MDEvent workspace and can optionally apply a Lorentz correction to ' \
               'the output peaks workspace; output can be saved in different formats.'

    def PyInit(self):
        self.declareProperty(IMDEventWorkspaceProperty("InputWorkspace", defaultValue="",
                                                       optional=PropertyMode.Mandatory,
                                                       direction=Direction.Input),
                             doc="Input MDEvent workspace to use for integration")

        self.declareProperty(IPeaksWorkspaceProperty("PeaksWorkspace", defaultValue="", optional=PropertyMode.Mandatory,
                                                     direction=Direction.Input),
                             doc="Peaks workspace containing peaks to integrate")

        positive_val = FloatBoundedValidator(lower=0.0)
        self.declareProperty("PeakRadius", defaultValue=1.0, validator=positive_val,
                             doc="Fixed radius around each peak position in which to integrate"
                                 " (same units as input workspace) ")

        self.declareProperty("BackgroundInnerRadius", defaultValue=0.0, validator=positive_val,
                             doc="Inner radius used to evaluate the peak background")
        self.declareProperty("BackgroundOuterRadius", defaultValue=0.0, validator=positive_val,
                             doc="Outer radius used to evaluate the peak background")

        self.declareProperty("ApplyLorentz", defaultValue=True,
                             doc="Whether the Lorentz correction should be applied to the integrated peaks")

        formats = StringListValidator()
        formats.addAllowedValue("SHELX")
        formats.addAllowedValue("Fullprof")
        self.declareProperty("OutputFormat", defaultValue="SHELX", validator=formats,
                             doc="Save direction cosines in HKL, or the fullprof format")

        self.declareProperty(FileProperty(name="OutputFile", defaultValue="",
                                          direction=Direction.Input,
                                          action=FileAction.OptionalSave),
                             doc="Filepath to save the integrated peaks workspace in HKL format")

        self.declareProperty(IPeaksWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output,
                                                     optional=PropertyMode.Mandatory),
                             doc="Output peaks workspace (copy of input with updated peak intensities)")

    def validateInputs(self):
        issues = dict()

        # Make sure outer radius > inner radius
        inner_radius = self.getProperty("BackgroundInnerRadius").value
        outer_radius = self.getProperty("BackgroundOuterRadius").value

        if outer_radius < inner_radius:
            issues['BackgroundOuterRadius'] = "Outer radius should be >= to inner radius"

        return issues

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        peak_ws = self.getProperty("PeaksWorkspace").value

        peak_radius = self.getProperty("PeakRadius").value
        inner_radius = self.getProperty("BackgroundInnerRadius").value
        outer_radius = self.getProperty("BackgroundOuterRadius").value

        use_lorentz = self.getProperty("ApplyLorentz").value

        out_ws = IntegratePeaksMD(InputWorkspace=input_ws,
                                  PeakRadius=peak_radius,
                                  BackgroundInnerRadius=inner_radius,
                                  BackgroundOuterRadius=outer_radius,
                                  PeaksWorkspace=peak_ws)

        if use_lorentz:
            # Apply Lorentz correction:
            for p in range(out_ws.getNumberPeaks()):
                peak = out_ws.getPeak(p)
                lorentz = abs(np.sin(peak.getScattering() * np.cos(peak.getAzimuthal())))
                peak.setIntensity(peak.getIntensity() * lorentz)

        # Write output only if a file path was provided
        if not self.getProperty("OutputFile").isDefault:
            out_format = self.getProperty("OutputFormat").value
            filename = self.getProperty("OutputFile").value

            if out_format == "SHELX":
                SaveHKL(InputWorkspace=out_ws, Filename=filename, DirectionCosines=True, OutputWorkspace="__tmp")
                DeleteWorkspace("__tmp")
            elif out_format == "Fullprof":
                SaveReflections(InputWorkspace=out_ws, Filename=filename, Format="Fullprof")
            else:
                # This shouldn't happen
                RuntimeError("Invalid output format given")

        self.setProperty("OutputWorkspace", out_ws)

        DeleteWorkspace(out_ws)


AlgorithmFactory.subscribe(HB3AIntegratePeaks)
