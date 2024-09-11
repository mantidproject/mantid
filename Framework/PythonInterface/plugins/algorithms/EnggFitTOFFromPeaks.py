# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *


class EnggFitTOFFromPeaks(PythonAlgorithm):
    def category(self):
        return "Diffraction\\Engineering;Diffraction\\Fitting"

    def seeAlso(self):
        return ["EnggFitPeaks", "GSASIIRefineFitPeaks", "Fit"]

    def name(self):
        return "EnggFitPeaks"

    def summary(self):
        return "The algorithm fits an expected diffraction pattern to a workpace spectrum by " "performing single peak fits."

    def PyInit(self):
        self.declareProperty(
            ITableWorkspaceProperty("FittedPeaks", "", Direction.Input),
            doc="Information on fitted peaks, in the format produced by EnggFitPeaks. "
            "The table must contain, for every peak fitted the expected peak value "
            "(in d-spacing), and the parameters fitted. The expected values are given "
            "in the column labelled 'dSpacing'. When using the back-to-back exponential "
            "peak function, the 'X0' column must have the fitted peak center.",
        )

        self.declareProperty(
            "OutParametersTable",
            "",
            direction=Direction.Input,
            doc="Name for a table workspace with the fitted values calculated by "
            "this algorithm (DIFA, DIFC and TZERO calibration parameters) for GSAS. "
            "These three parameters are added as three columns in a single row. If not given, "
            "the table workspace is not created.",
        )

        self.declareProperty("DIFA", 0.0, direction=Direction.Output, doc="Fitted DIFA calibration parameter")

        self.declareProperty("DIFC", 0.0, direction=Direction.Output, doc="Fitted DIFC calibration parameter")

        self.declareProperty("TZERO", 0.0, direction=Direction.Output, doc="Fitted TZERO calibration parameter")

    def validateInputs(self):
        errors = dict()

        peaks = self.getProperty("FittedPeaks").value

        # Better than failing to fit the linear function
        if peaks.rowCount() < 3:
            errors["FittedPeaks"] = (
                "Less than three peaks were given in the input peaks table. This is not enough "
                "to fit the output parameters difa, difc and zero. Please check the list of "
                "expected peaks given and if it is appropriate for the workspace"
            )
        return errors

    def PyExec(self):
        peaks = self.getProperty("FittedPeaks").value

        difa, difc, tzero = self._fit_dSpacingTOF(peaks)

        out_tbl_name = self.getPropertyValue("OutParametersTable")
        self._produce_outputs(difa, difc, tzero, out_tbl_name)

        self.log().information("Fitted {0} peaks in total. DIFA: {1}, DIFC: {2}, TZERO: {3}".format(peaks.rowCount(), difa, difc, tzero))

    def _fit_dSpacingTOF(self, fitted_peaks_table):
        """
        Fits a linear function to the dSpacing-TOF relationship and
        returns the fitted DIFA, DIFC and TZERO values. If the
        table passed has less than 3 peaks this raises an exception,
        as it is not possible to fit the difa, difc, zero parameters.

        @param fitted_peaks_table :: table with one row per fitted peak, expecting column 'dSpacing'
        as x values and column 'X0' as y values.

        @returns DIFA, DIFC and TZERO parameters as defined in GSAS and GSAS-II. The difa, difc and zero
        parameters are obtained from fitting a quadratic background (in _fit_dSpacing_to_ToF) to the
        peaks fitted individually that have been passed in the input table

        """

        num_peaks = fitted_peaks_table.rowCount()
        if num_peaks < 3:
            raise ValueError(
                "Cannot fit a quadratic function with less than three peaks. Got a table of " + "peaks with " + str(num_peaks) + " peaks"
            )

        d_tof_conversion_ws = ConvertTableToMatrixWorkspace(
            InputWorkspace=fitted_peaks_table, ColumnX="dSpacing", ColumnY="X0", StoreInADS=False
        )

        # Fit the curve to get linear coefficients of TOF <-> dSpacing relationship for the detector
        fit_output = Fit(
            Function="name=Quadratic", InputWorkspace=d_tof_conversion_ws, WorkspaceIndex=0, CreateOutput=True, StoreInADS=False
        )

        tzero = fit_output.OutputParameters.cell("Value", 0)  # A0
        difc = fit_output.OutputParameters.cell("Value", 1)  # A1
        difa = fit_output.OutputParameters.cell("Value", 2)  # A2

        return difa, difc, tzero

    def _produce_outputs(self, difa, difc, tzero, tbl_name):
        """
        Fills in the output properties as requested via the input
        properties. Sets the output difa/difc/tzero values. It can
        also produces a table with these parameters if required in the
        inputs.

        @param difa :: the DIFA GSAS parameter as fitted here
        @param difc :: the DIFC GSAS parameter as fitted here
        @param tzero :: the TZERO GSAS parameter as fitted here
        @param tbl_name :: a table name, non-empty if a table workspace
        should be created with the calibration parameters
        """

        import EnggUtils

        # mandatory outputs
        self.setProperty("DIFA", difa)
        self.setProperty("DIFC", difc)
        self.setProperty("TZERO", tzero)

        # optional outputs
        if tbl_name:
            EnggUtils.generate_output_param_table(tbl_name, difa, difc, tzero)
            self.log().information("Output parameters added into a table workspace: %s" % tbl_name)


AlgorithmFactory.subscribe(EnggFitTOFFromPeaks)
