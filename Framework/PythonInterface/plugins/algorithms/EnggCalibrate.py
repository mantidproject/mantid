# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    FileAction,
    FileProperty,
    ITableWorkspaceProperty,
    MatrixWorkspaceProperty,
    Progress,
    PropertyMode,
    PythonAlgorithm,
)
from mantid.kernel import Direction, FloatArrayProperty, StringListValidator
import mantid.simpleapi as mantid


class EnggCalibrate(PythonAlgorithm):
    INDICES_PROP_NAME = "SpectrumNumbers"

    def category(self):
        return "Diffraction\\Engineering"

    def seeAlso(self):
        return ["EnggCalibrateFull"]

    def name(self):
        return "EnggCalibrate"

    def summary(self):
        return (
            "This algorithm is deprecated as of May 2021, consider using PDCalibration instead."
            "Calibrates one or more detector banks (or group(s) of detectors) by performing single peak "
            "fitting."
        )

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="Workspace with the calibration run to use."
        )

        import EnggUtils

        self.declareProperty(
            FloatArrayProperty("ExpectedPeaks", values=EnggUtils.default_ceria_expected_peaks(), direction=Direction.Input),
            doc="A list of dSpacing values where peaks are expected.",
        )

        self.declareProperty(
            FileProperty(name="ExpectedPeaksFromFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".csv"]),
            doc="Load from file a list of dSpacing values to be translated into TOF to find expected "
            "peaks. This takes precedence over 'ExpectedPeaks' if both options are given.",
        )

        peaks_grp = "Peaks to fit"
        self.setPropertyGroup("ExpectedPeaks", peaks_grp)
        self.setPropertyGroup("ExpectedPeaksFromFile", peaks_grp)

        self.declareProperty(
            MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input, PropertyMode.Optional),
            doc="Workspace with the Vanadium (correction and calibration) run. Alternatively, "
            "when the Vanadium run has been already processed, the properties can be used",
        )

        self.declareProperty(
            ITableWorkspaceProperty("VanIntegrationWorkspace", "", Direction.Input, PropertyMode.Optional),
            doc="Results of integrating the spectra of a Vanadium run, with one column "
            "(integration result) and one row per spectrum. This can be used in "
            "combination with OutVanadiumCurveFits from a previous execution and "
            "VanadiumWorkspace to provide pre-calculated values for Vanadium correction.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("VanCurvesWorkspace", "", Direction.Input, PropertyMode.Optional),
            doc="A workspace2D with the fitting workspaces corresponding to the instrument banks. "
            "This workspace has three spectra per bank, as produced by the algorithm Fit. "
            "This is meant to be used as an alternative input VanadiumWorkspace for testing and "
            "performance reasons. If not given, no workspace is generated.",
        )

        vana_grp = "Vanadium (open beam) properties"
        self.setPropertyGroup("VanadiumWorkspace", vana_grp)
        self.setPropertyGroup("VanIntegrationWorkspace", vana_grp)
        self.setPropertyGroup("VanCurvesWorkspace", vana_grp)

        self.declareProperty(
            "Bank",
            "",
            StringListValidator(EnggUtils.ENGINX_BANKS),
            direction=Direction.Input,
            doc="Which bank to calibrate. It can be specified as 1 or 2, or "
            "equivalently, North or South. See also " + self.INDICES_PROP_NAME + " "
            "for a more flexible alternative to select specific detectors",
        )

        self.declareProperty(
            self.INDICES_PROP_NAME,
            "",
            direction=Direction.Input,
            doc="Sets the spectrum numbers for the detectors "
            "that should be considered in the calibration (all others will be "
            "ignored). This option cannot be used together with Bank, as they overlap. "
            'You can give multiple ranges, for example: "0-99", or "0-9, 50-59, 100-109".',
        )

        banks_grp = "Banks / spectra"
        self.setPropertyGroup("Bank", banks_grp)
        self.setPropertyGroup(self.INDICES_PROP_NAME, banks_grp)

        self.declareProperty(
            ITableWorkspaceProperty("DetectorPositions", "", Direction.Input, PropertyMode.Optional),
            "Calibrated detector positions. If not specified, default ones (from the " "current instrument definition) are used.",
        )

        self.declareProperty(
            "OutputParametersTableName",
            "",
            direction=Direction.Input,
            doc="Name for a table workspace with the calibration parameters calculated "
            "from this algorithm: difc and zero parameters for GSAS. these two parameters "
            "are added as two columns in a single row. If not given, no table is "
            "generated.",
        )

        self.declareProperty(
            "DIFA", 0.0, direction=Direction.Output, doc="Calibration parameter DIFA for the bank or range of pixels/detectors given"
        )

        self.declareProperty(
            "DIFC", 0.0, direction=Direction.Output, doc="Calibration parameter DIFC for the bank or range of pixels/detectors given"
        )

        self.declareProperty(
            "TZERO", 0.0, direction=Direction.Output, doc="Calibration parameter TZERO for the bank or range of pixels/detectors given"
        )

        self.declareProperty(
            ITableWorkspaceProperty("FittedPeaks", "", Direction.Output),
            doc="Information on fitted peaks as produced by the (child) algorithm EnggFitPeaks.",
        )

        out_grp = "Outputs"
        self.setPropertyGroup("DetectorPositions", out_grp)
        self.setPropertyGroup("OutputParametersTableName", out_grp)
        self.setPropertyGroup("DIFA", out_grp)
        self.setPropertyGroup("DIFC", out_grp)
        self.setPropertyGroup("TZERO", out_grp)
        self.setPropertyGroup("FittedPeaks", out_grp)

    def validateInputs(self):
        issues = dict()

        if not self.getPropertyValue("ExpectedPeaksFromFile") and not self.getPropertyValue("ExpectedPeaks"):
            issues["ExpectedPeaks"] = (
                "Cannot run this algorithm without any expected peak. Please provide "
                "either a list of peaks or a file with a list of peaks"
            )

        return issues

    def PyExec(self):
        mantid.logger.warning("EnggCalibrate is deprecated as of May 2021. Please use PDCalibration instead.")
        import EnggUtils

        max_reports = 20
        prog = Progress(self, start=0, end=1, nreports=max_reports)
        # Get peaks in dSpacing from file
        prog.report("Reading peaks")

        expected_peaks_dsp = EnggUtils.read_in_expected_peaks(
            filename=self.getPropertyValue("ExpectedPeaksFromFile"), expected_peaks=self.getProperty("ExpectedPeaks").value
        )
        if len(expected_peaks_dsp) < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        prog.report("Focusing the input workspace")
        focused_ws = self._focus_run(
            self.getProperty("InputWorkspace").value,
            self.getProperty("VanadiumWorkspace").value,
            self.getProperty("Bank").value,
            self.getProperty(self.INDICES_PROP_NAME).value,
            prog,
        )

        prog.report("Fitting parameters for the focused run")
        difa, difc, zero, fitted_peaks = self._fit_params(focused_ws, expected_peaks_dsp, prog)

        self.log().information(
            "Fitted {0} peaks. Resulting DIFA: {1}, DIFC: {2}, TZERO: {3}".format(fitted_peaks.rowCount(), difa, difc, zero)
        )
        self.log().information("Peaks fitted: {0}, centers in ToF: {1}".format(fitted_peaks.column("dSpacing"), fitted_peaks.column("X0")))

        prog.report("Producing outputs")
        self._produce_outputs(difa, difc, zero, fitted_peaks)

        prog.report(max_reports, "Calibration complete")

    def _fit_params(self, focused_ws, expected_peaks_d, prog):
        """
        Fit the GSAS parameters that this algorithm produces: DIFC and TZERO. Fits a
        number of peaks starting from the expected peak positions. Then it fits a line
        on the peak positions to produce the DIFC and TZERO as used in GSAS.

        @param focused_ws :: focused workspace to do the fitting on
        @param expected_peaks_d :: expected peaks, used as intial peak positions for the
        fitting, in d-spacing units
        @param prog :: progress reporter

        @returns a tuple with three GSAS calibration parameters (DIFA, DIFC, ZERO),
        and a list of peak centers as fitted
        """

        fit_alg = self.createChildAlgorithm("EnggFitPeaks")
        fit_alg.setProperty("InputWorkspace", focused_ws)
        fit_alg.setProperty("WorkspaceIndex", 0)  # There should be only one index anyway
        fit_alg.setProperty("ExpectedPeaks", expected_peaks_d)
        # we could also pass raw 'ExpectedPeaks' and 'ExpectedPeaksFromFile' to
        # EnggFitPaks, but better to check inputs early, before this
        fit_alg.execute()

        fitted_peaks = fit_alg.getProperty("FittedPeaks").value

        difc_alg = self.createChildAlgorithm("EnggFitTOFFromPeaks")
        difc_alg.setProperty("FittedPeaks", fitted_peaks)
        prog.report("Performing fit")
        difc_alg.execute()
        prog.report("Fit complete")

        difa = difc_alg.getProperty("DIFA").value
        difc = difc_alg.getProperty("DIFC").value
        zero = difc_alg.getProperty("TZERO").value

        return difa, difc, zero, fitted_peaks

    def _focus_run(self, ws, vanadium_ws, bank, indices, prog):
        """
        Focuses the input workspace by running EnggFocus as a child algorithm, which will produce a
        single spectrum workspace.

        @param ws :: workspace to focus
        @param vanadium_ws :: workspace with Vanadium run for corrections
        @param bank :: the focusing will be applied on the detectors of this bank
        @param indices :: list of indices to consider, as an alternative to bank (bank and indices are
        mutually exclusive)

        @return focused (summed) workspace
        """
        prog.report("Initialising EnggFocus")

        engg_focus_params = dict()

        detector_positions = self.getProperty("DetectorPositions").value
        if detector_positions:
            engg_focus_params["DetectorPositions"] = detector_positions

        if vanadium_ws:
            engg_focus_params["VanadiumWorkspace"] = vanadium_ws

        van_integration_ws = self.getProperty("VanIntegrationWorkspace").value
        if van_integration_ws:
            engg_focus_params["VanIntegrationWorkspace"] = van_integration_ws

        van_curves_ws = self.getProperty("VanCurvesWorkspace").value
        if van_curves_ws:
            engg_focus_params["VanCurvesWorkspace"] = van_curves_ws

        prog.report("Running EnggFocus")
        return mantid.EnggFocus(
            InputWorkspace=ws, Bank=bank, SpectrumNumbers=indices, StoreInADS=False, startProgress=0.3, endProgress=0.6, **engg_focus_params
        )

    def _produce_outputs(self, difa, difc, zero, fitted_peaks):
        """
        Just fills in the output properties as requested

        @param difa :: the DIFA GSAS parameter as fitted here
        @param difc :: the DIFC GSAS parameter as fitted here
        @param zero :: the TZERO GSAS parameter as fitted here
        @param fitted_peaks :: table workspace with peak parameters (one peak per row)
        """

        import EnggUtils

        self.setProperty("DIFA", difa)
        self.setProperty("DIFC", difc)
        self.setProperty("TZERO", zero)
        self.setProperty("FittedPeaks", fitted_peaks)

        # make output table if requested
        table_name = self.getPropertyValue("OutputParametersTableName")
        if "" != table_name:
            EnggUtils.generate_output_param_table(table_name, difa, difc, zero)
            self.log().information("Output parameters added into a table workspace: %s" % table_name)


AlgorithmFactory.subscribe(EnggCalibrate)
