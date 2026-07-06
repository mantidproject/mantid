# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import (
    DeleteWorkspace,
    LoadVesuvio,
    RebinToWorkspace,
    Divide,
    ConvertUnits,
    Rebin,
    ExtractSingleSpectrum,
    SmoothData,
    AppendSpectra,
    Logarithm,
    Scale,
    Integration,
)
from mantid.kernel import Direction, StringMandatoryValidator, StringListValidator
from mantid.api import AlgorithmFactory, PythonAlgorithm, MatrixWorkspaceProperty, mtd


class VesuvioTransmission(PythonAlgorithm):
    """
    Evaluate the transmission spectrum on the VESUVIO spectrometer for measured
    sample and empty run numbers.

    This version declares OutputWorkspace as a real Mantid output workspace
    property and explicitly sets it at the end of PyExec(). This allows Mantid
    to attach the PythonAlgorithm call to the output workspace history when the
    algorithm is run from within Mantid.
    """

    def name(self):
        return "VesuvioTransmission"

    def summary(self):
        return "This algorithm evaluates the transmission spectrum on the VESUVIO spectrometer for measured sample and empty run numbers."

    def category(self):
        return "Inelastic\\Indirect\\Vesuvio"

    def PyInit(self):
        # IMPORTANT FOR HISTORY:
        # This must be an OUTPUT workspace property, not just an input string.
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Output transmission workspace")

        self.declareProperty(
            "Runs",
            "",
            StringMandatoryValidator(),
            direction=Direction.Input,
            doc="Sample run number, comma/list expression, or range for TimeScan, e.g. '12345-12355'.",
        )
        self.declareProperty(
            "EmptyRuns", "", StringMandatoryValidator(), direction=Direction.Input, doc="Empty-can/empty-cell run number or run expression."
        )
        self.declareProperty(
            "Grouping",
            "SumOfAllRuns",
            StringListValidator(["TimeScan", "SumOfAllRuns"]),
            doc="Either sum all sample runs, or append one transmission spectrum per run in a run range.",
        )
        self.declareProperty(
            "Target", "Energy", StringListValidator(["Energy", "Wavelength"]), doc="Target unit for the final transmission spectrum."
        )
        self.declareProperty("Rebin", False, doc="If true, rebin the final summed transmission spectrum.")
        self.declareProperty(
            "RebinParameters",
            "0.6,-0.005,1.e7",
            StringMandatoryValidator(),
            direction=Direction.Input,
            doc="Mantid Rebin parameters, e.g. '0.6,-0.005,1.e7'. Used for Energy target only.",
        )
        self.declareProperty(
            "CalculateXS", False, doc="If true, also calculate a normalised effective cross-section workspace named '<OutputWorkspace>_XS'."
        )
        self.declareProperty("InvertMonitors", False, doc="If true, invert the monitor ratio used to form the transmission.")
        self.declareProperty(
            "SmoothIncidentSpectrum",
            False,
            doc="If true, apply SmoothData with NPoints=5 to the incident monitor spectrum before division.",
        )

    def validateInputs(self):
        issues = {}
        runs = self.getPropertyValue("Runs")
        grouping = self.getPropertyValue("Grouping")
        target = self.getPropertyValue("Target")
        rebin_enabled = self.getProperty("Rebin").value

        if grouping == "TimeScan" and "-" not in runs:
            issues["Runs"] = "For Grouping='TimeScan', Runs must be a range, e.g. '12345-12355'."

        if grouping == "TimeScan":
            try:
                lower, upper = runs.split("-")
                lower = int(lower.strip())
                upper = int(upper.strip())
                if upper < lower:
                    issues["Runs"] = "For Grouping='TimeScan', the upper run number must be >= lower run number."
            except Exception:
                issues["Runs"] = "For Grouping='TimeScan', Runs must be a simple integer range, e.g. '12345-12355'."

        if rebin_enabled and target != "Energy":
            # Original script only rebinned the Energy case. Keep behaviour, but make this explicit.
            self.log().warning("Rebin=True is currently only applied when Target='Energy'.")

        return issues

    def _delete_if_exists(self, ws_name):
        """Delete a workspace if it exists in the ADS."""
        if ws_name and mtd.doesExist(ws_name):
            DeleteWorkspace(Workspace=ws_name)

    def PyExec(self):
        invert = self.getProperty("InvertMonitors").value
        smooth = self.getProperty("SmoothIncidentSpectrum").value
        grouping = self.getPropertyValue("Grouping")
        target = self.getPropertyValue("Target")
        reb_bool = self.getProperty("Rebin").value
        reb_parameters = self.getPropertyValue("RebinParameters")
        runs = self.getPropertyValue("Runs")
        empty_runs = self.getPropertyValue("EmptyRuns")
        name = self.getPropertyValue("OutputWorkspace")
        calculate_cross_section = self.getProperty("CalculateXS").value

        # Use stable internal temporary names based on the requested output name.
        sample_ws = name + "__sample"
        empty_ws = name + "__empty"
        tmp_ws = name + "__tmp"
        tmp2_ws = name + "__tmp2"

        sample_mon = sample_ws + "_monitors"
        empty_mon = empty_ws + "_monitors"

        try:
            if grouping == "SumOfAllRuns":
                LoadVesuvio(
                    Filename=runs, OutputWorkspace=sample_ws, SpectrumList="3-134", Mode="FoilOut", SumSpectra=True, LoadMonitors=True
                )
                LoadVesuvio(
                    Filename=empty_runs, OutputWorkspace=empty_ws, SpectrumList="3-134", Mode="FoilOut", SumSpectra=True, LoadMonitors=True
                )

                RebinToWorkspace(WorkspaceToRebin=empty_mon, WorkspaceToMatch=sample_mon, PreserveEvents=True, OutputWorkspace=empty_mon)

                Divide(LHSWorkspace=sample_mon, RHSWorkspace=empty_mon, OutputWorkspace=name)

                ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target=target, EMode="Elastic")

                if reb_bool and target == "Energy":
                    Rebin(InputWorkspace=name, Params=reb_parameters, OutputWorkspace=name, FullBinsOnly=True, PreserveEvents=True)

                ExtractSingleSpectrum(InputWorkspace=name, OutputWorkspace=tmp_ws, WorkspaceIndex=0)
                ExtractSingleSpectrum(InputWorkspace=name, OutputWorkspace=name, WorkspaceIndex=1)

                RebinToWorkspace(WorkspaceToRebin=tmp_ws, WorkspaceToMatch=name, OutputWorkspace=tmp_ws)

                if not invert:
                    if smooth:
                        SmoothData(InputWorkspace=tmp_ws, NPoints=5, OutputWorkspace=tmp_ws)
                    Divide(LHSWorkspace=name, RHSWorkspace=tmp_ws, OutputWorkspace=name)
                else:
                    if smooth:
                        SmoothData(InputWorkspace=name, NPoints=5, OutputWorkspace=name)
                    Divide(LHSWorkspace=tmp_ws, RHSWorkspace=name, OutputWorkspace=name)

            elif grouping == "TimeScan":
                lower, upper = runs.split("-")
                lower = int(lower.strip())
                upper = int(upper.strip())

                LoadVesuvio(
                    Filename=str(lower), OutputWorkspace=sample_ws, SpectrumList="3-134", Mode="FoilOut", SumSpectra=True, LoadMonitors=True
                )
                LoadVesuvio(
                    Filename=empty_runs, OutputWorkspace=empty_ws, SpectrumList="3-134", Mode="FoilOut", SumSpectra=True, LoadMonitors=True
                )

                RebinToWorkspace(WorkspaceToRebin=empty_mon, WorkspaceToMatch=sample_mon, PreserveEvents=True, OutputWorkspace=empty_mon)

                Divide(LHSWorkspace=sample_mon, RHSWorkspace=empty_mon, OutputWorkspace=name)
                ConvertUnits(InputWorkspace=name, OutputWorkspace=name, Target="Energy", EMode="Elastic")
                ExtractSingleSpectrum(InputWorkspace=name, OutputWorkspace=tmp_ws, WorkspaceIndex=0)
                ExtractSingleSpectrum(InputWorkspace=name, OutputWorkspace=name, WorkspaceIndex=1)
                RebinToWorkspace(WorkspaceToRebin=tmp_ws, WorkspaceToMatch=name, OutputWorkspace=tmp_ws)
                Divide(LHSWorkspace=name, RHSWorkspace=tmp_ws, OutputWorkspace=name)

                for run in range(lower + 1, upper + 1):
                    self.log().information("Processing run {}".format(run))

                    LoadVesuvio(
                        Filename=str(run),
                        OutputWorkspace=sample_ws,
                        SpectrumList="3-134",
                        Mode="FoilOut",
                        SumSpectra=True,
                        LoadMonitors=True,
                    )

                    RebinToWorkspace(
                        WorkspaceToRebin=empty_mon, WorkspaceToMatch=sample_mon, PreserveEvents=True, OutputWorkspace=empty_mon
                    )

                    Divide(LHSWorkspace=sample_mon, RHSWorkspace=empty_mon, OutputWorkspace=tmp_ws)
                    ConvertUnits(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws, Target="Energy", EMode="Elastic")
                    ExtractSingleSpectrum(InputWorkspace=tmp_ws, OutputWorkspace=tmp2_ws, WorkspaceIndex=0)
                    ExtractSingleSpectrum(InputWorkspace=tmp_ws, OutputWorkspace=tmp_ws, WorkspaceIndex=1)
                    RebinToWorkspace(WorkspaceToRebin=tmp2_ws, WorkspaceToMatch=tmp_ws, OutputWorkspace=tmp2_ws)
                    Divide(LHSWorkspace=tmp_ws, RHSWorkspace=tmp2_ws, OutputWorkspace=tmp_ws)
                    AppendSpectra(InputWorkspace1=name, InputWorkspace2=tmp_ws, OutputWorkspace=name)

            if calculate_cross_section:
                xs_name = name + "_XS"
                Logarithm(InputWorkspace=name, OutputWorkspace=xs_name)
                Scale(InputWorkspace=xs_name, Factor=-1, Operation="Multiply", OutputWorkspace=xs_name)
                Integration(InputWorkspace=xs_name, RangeLower=1000, RangeUpper=11000, OutputWorkspace=tmp_ws)
                Divide(LHSWorkspace=xs_name, RHSWorkspace=tmp_ws, OutputWorkspace=xs_name)
                Scale(InputWorkspace=xs_name, Factor=10000, OutputWorkspace=xs_name)

            # IMPORTANT FOR HISTORY:
            # Attach the produced workspace to the declared output property.
            self.setProperty("OutputWorkspace", mtd[name])

        finally:
            # Clean up temporary workspaces without failing if one was not created.
            for ws in [empty_ws, empty_mon, sample_ws, sample_mon, tmp_ws, tmp2_ws]:
                self._delete_if_exists(ws)


AlgorithmFactory.subscribe(VesuvioTransmission)
