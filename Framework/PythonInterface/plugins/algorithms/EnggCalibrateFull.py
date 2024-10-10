# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math

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
from mantid.kernel import Direction, FloatArrayProperty, StringListValidator, V3D
from mantid.simpleapi import SaveAscii, logger
import EnggUtils


class EnggCalibrateFull(PythonAlgorithm):
    INDICES_PROP_NAME = "SpectrumNumbers"

    def category(self):
        return "Diffraction\\Engineering"

    def seeAlso(self):
        return ["EnggCalibrate"]

    def name(self):
        return "EnggCalibrateFull"

    def summary(self):
        return (
            "This algorithm is deprecated as of May 2021, consider using PDCalibration instead."
            "Calibrates every detector/pixel position by performing single peak fitting."
        )

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty("Workspace", "", Direction.InOut),
            "Workspace with the calibration run to use. The calibration will be applied on it.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input, PropertyMode.Optional),
            "Workspace with the Vanadium (correction and calibration) run.",
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
            doc="A workspace2D with the fitting workspaces corresponding to "
            "the instrument banks. This workspace has three spectra per bank, as produced "
            "by the algorithm Fit. This is meant to be used as an alternative input "
            "VanadiumWorkspace for testing and performance reasons. If not given, no "
            "workspace is generated.",
        )

        vana_grp = "Vanadium (open beam) properties"
        self.setPropertyGroup("VanadiumWorkspace", vana_grp)
        self.setPropertyGroup("VanIntegrationWorkspace", vana_grp)
        self.setPropertyGroup("VanCurvesWorkspace", vana_grp)

        self.declareProperty(
            ITableWorkspaceProperty("OutDetPosTable", "", Direction.Output),
            doc="A table with the detector IDs and calibrated detector positions and "
            "additional calibration information. The table includes: the old positions "
            "in V3D format (3D vector with x, y, z values), the new positions in V3D, the "
            "new positions in spherical coordinates, the change in L2, and the DIFA, "
            "DIFC and TZERO calibration parameters.",
        )

        self.declareProperty(
            ITableWorkspaceProperty("FittedPeaks", "", Direction.Output),
            doc="Information on fitted peaks. The table has one row per calibrated "
            "detector contains. In each row, the parameters of all the peaks fitted "
            "are specified together with the expected peak value (in d-spacing). "
            "The expected values are given in the field labelled 'dSpacing'. When "
            "fitting back-to-back exponential functions, the 'X0' column has the fitted "
            "peak center.",
        )

        self.declareProperty(
            "Bank",
            "",
            StringListValidator(EnggUtils.ENGINX_BANKS),
            direction=Direction.Input,
            doc="Which bank to calibrate: It can be specified as 1 or 2, or "
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
            FileProperty("OutDetPosFilename", "", FileAction.OptionalSave, [".csv"]),
            doc="Name of the file to save the pre-/post-calibrated detector positions - this "
            "saves the same information that is provided in the output table workspace "
            "(OutDetPosTable).",
        )

        # The default value of '-0.0005' is borrowed from OG routines
        self.declareProperty(
            "RebinBinWidth",
            defaultValue="-0.0005",
            direction=Direction.Input,
            doc="Before calculating the calibrated positions (fitting peaks) this algorithms "
            "re-bins the input sample data using a single bin width parameter. This option is"
            "to change the default bin width which is set to the value traditionally used for "
            "the Engin-X instrument",
        )

        opt_outs_grp = "Optional outputs"
        self.setPropertyGroup("OutDetPosFilename", opt_outs_grp)

        self.declareProperty(
            FloatArrayProperty("ExpectedPeaks", values=EnggUtils.default_ceria_expected_peaks(), direction=Direction.Input),
            doc="A list of dSpacing values where peaks are expected.",
        )

        self.declareProperty(
            FileProperty(name="ExpectedPeaksFromFile", defaultValue="", action=FileAction.OptionalLoad, extensions=[".csv"]),
            doc="Load from file a list of dSpacing values to be translated into TOF to "
            "find expected peaks. This takes precedence over 'ExpectedPeaks' if both "
            "options are given.",
        )

        peaks_grp = "Peaks to fit"
        self.setPropertyGroup("ExpectedPeaks", peaks_grp)
        self.setPropertyGroup("ExpectedPeaksFromFile", peaks_grp)

    def PyExec(self):
        logger.warning("EnggCalibrateFull is deprecated as of May 2021. Please use PDCalibration instead.")
        # Get peaks in dSpacing from file, and check we have what we need, before doing anything
        expected_peaks_d = EnggUtils.read_in_expected_peaks(
            self.getPropertyValue("ExpectedPeaksFromFile"), self.getProperty("ExpectedPeaks").value
        )

        if len(expected_peaks_d) < 1:
            raise ValueError("Cannot run this algorithm without any input expected peaks")

        in_wks = self.getProperty("Workspace").value
        wks_indices = EnggUtils.get_ws_indices_from_input_properties(
            in_wks, self.getProperty("Bank").value, self.getProperty(self.INDICES_PROP_NAME).value
        )

        van_wks = self.getProperty("VanadiumWorkspace").value
        van_integ_wks = self.getProperty("VanIntegrationWorkspace").value
        van_curves_wks = self.getProperty("VanCurvesWorkspace").value
        # These corrections rely on ToF<->Dspacing conversions, so ideally they'd be done after the
        # calibration step, which creates a cycle / chicken-and-egg issue.
        EnggUtils.apply_vanadium_corrections(self, in_wks, wks_indices, van_wks, van_integ_wks, van_curves_wks)

        rebinned_ws = self._prepare_ws_for_fitting(in_wks, self.getProperty("RebinBinWidth").value)
        pos_tbl, peaks_tbl = self._calculate_calib_positions_tbl(rebinned_ws, wks_indices, expected_peaks_d)

        # Produce 2 results: 'output table' and 'apply calibration' + (optional) calibration file
        self.setProperty("OutDetPosTable", pos_tbl)
        self.setProperty("FittedPeaks", peaks_tbl)
        self._apply_calibration_table(in_wks, pos_tbl)
        self._output_det_pos_file(self.getPropertyValue("OutDetPosFilename"), pos_tbl)

    def _prepare_ws_for_fitting(self, ws, bin_width):
        """
        Rebins the workspace and converts it to distribution
        """
        rebin_alg = self.createChildAlgorithm("Rebin")
        rebin_alg.setProperty("InputWorkspace", ws)
        rebin_alg.setProperty("Params", bin_width)
        rebin_alg.execute()
        result = rebin_alg.getProperty("OutputWorkspace").value

        if not result.isDistribution():
            convert_alg = self.createChildAlgorithm("ConvertToDistribution")
            convert_alg.setProperty("Workspace", result)
            convert_alg.execute()

        return result

    def _calculate_calib_positions_tbl(self, ws, indices, expected_peaks_d):
        """
        Makes a table of calibrated positions (and additional parameters). It goes through
        the detectors of the workspace and calculates the DIFC and TZERO parameters by fitting
        peaks, then calculates the coordinates and adds them in the returned table.

        @param ws :: input workspace with an instrument definition
        @param indices :: workspace indices to consider for calibration (for example all the
        indices of a bank)
        @param expected_peaks_d :: expected peaks in d-spacing

        @return Two tables. A positions table, with the detector positions, one row per detector.
        A peaks details table with the fitted parameters for every of the peaks of every detector.

        """
        pos_tbl = self._create_positions_table()
        peaks_tbl = self._create_peaks_fitted_details_table()

        prog = Progress(self, start=0, end=1, nreports=len(indices))

        for i in indices:
            try:
                fitted_peaks_table = self._fit_peaks(ws, i, expected_peaks_d)
                difa, difc, tzero = self._fit_difc_linear(fitted_peaks_table)
            except RuntimeError as re:
                raise RuntimeError(
                    "Severe issue found when trying to fit peaks for the detector with ID %d. "
                    "This calibration algorithm cannot continue. Please check the expected "
                    "peaks provided. Details from "
                    "FindPeaks: %s" % (i, str(re))
                )

            det = ws.getDetector(i)
            new_pos, new_L2 = self._get_calibrated_det_pos(difc, det, ws)

            # get old (pre-calibration) detector coordinates
            old_L2 = det.getDistance(ws.getInstrument().getSample())
            det_2Theta = ws.detectorTwoTheta(det)
            det_phi = det.getPhi()
            old_pos = det.getPos()

            pos_tbl.addRow([det.getID(), old_pos, new_pos, new_L2, det_2Theta, det_phi, new_L2 - old_L2, difa, difc, tzero])

            # fitted parameter details as a string for every peak for one detector
            peaks_details = self._build_peaks_details_string(fitted_peaks_table)
            peaks_tbl.addRow([det.getID(), peaks_details])
            prog.report()

        return pos_tbl, peaks_tbl

    def _fit_peaks(self, ws, ws_index, expected_peaks_d):
        """
        Fits expected peaks to the spectrum, and returns calibrated zero and difc values.

        @param ws_index workspace index of the spectrum to fit
        @param expected_peaks_d :: expected peaks for the fitting, in d-spacing units

        @returns the peaks parameters in a table as produced by the algorithm
        EnggFitPeaks
        """
        alg = self.createChildAlgorithm("EnggFitPeaks")
        alg.setProperty("InputWorkspace", ws)
        alg.setProperty("WorkspaceIndex", ws_index)  # There should be only one index anyway
        alg.setProperty("ExpectedPeaks", expected_peaks_d)
        alg.execute()

        fitted_peaks_table = alg.getProperty("FittedPeaks").value

        return fitted_peaks_table

    def _fit_difc_linear(self, fitted_peaks_table):
        """
        Fit the DIFC-TZERO curve (line) to the peaks

        @param fitted_peaks_table a table with peaks parameters, expected
        in the format produced by EnggFitPeaks

        @returns the DIFA, DIFC, TZERO calibration parameters of GSAS
        """
        alg = self.createChildAlgorithm("EnggFitTOFFromPeaks")
        alg.setProperty("FittedPeaks", fitted_peaks_table)
        alg.execute()

        difa = alg.getProperty("DIFA").value
        difc = alg.getProperty("DIFC").value
        tzero = alg.getProperty("TZERO").value

        return difa, difc, tzero

    def _create_positions_table(self):
        """
        Helper method to create an empty table for storing detector positions

        @return table with the expected output columns
        """
        alg = self.createChildAlgorithm("CreateEmptyTableWorkspace")
        alg.execute()
        table = alg.getProperty("OutputWorkspace").value

        # Note: the colums 'Detector ID' and 'Detector Position' must have those
        # exact names (expected by child alg. ApplyCalibration in EnggFocus)
        table.addColumn("int", "Detector ID")
        table.addColumn("V3D", "Old Detector Position")
        table.addColumn("V3D", "Detector Position")
        table.addColumn("float", "L2")
        table.addColumn("float", "2 \\theta")
        table.addColumn("float", "\\phi")
        table.addColumn("float", "\\delta L2 (calibrated - old)")
        table.addColumn("float", "DIFA")
        table.addColumn("float", "DIFC")
        table.addColumn("float", "TZERO")

        return table

    def _create_peaks_fitted_details_table(self):
        """
        Prepare a table to output details about every peak fitted for every detector.

        Returns::
           Empty table that needs to be populated with peaks information (one row per detector)
        """
        alg = self.createChildAlgorithm("CreateEmptyTableWorkspace")
        alg.execute()
        table = alg.getProperty("OutputWorkspace").value

        table.addColumn("int", "Detector ID")
        table.addColumn("str", "Parameters")

        return table

    def _build_peaks_details_string(self, fitted_params_tbl):
        """
        Puts all the parameters from every peak (row) of the input table into a JSON string,
        with the keys set to the row/peak index, and values as the string of parameter names:fitted
        values

        @param fitted_params :: table with each row containing fitted parameters as a dictionary of
        (parameter name:fitted value), as returned by EnggFitPeaks

        Returns::
        Fitted parameters as a JSON string, for every peak (corresponding to a detector or spectrum).
        For example:
        '{{"1": {"A": 0.7349963692408943, "X0_Err": 58.963668189178975, "B": 0.16836661002332443,
        "A0": 0.05055372128720299, "I": 28.933988934616544, ... "B_Err": 1.827671204959159}}'
        ...
        '{"15": {"A": 0.7349963692408943, "X0_Err": 58.963668189178975, ... B_Err": 1.827671204959159}}}'

        """
        import json

        all_dict = {}
        for row_idx, row_txt in enumerate(fitted_params_tbl):
            all_dict[row_idx + 1] = row_txt

        return json.dumps(all_dict)

    @staticmethod
    def _output_det_pos_file(filename, tbl):
        """
        Writes a text (TSV) file with the detector positions information that also goes into the
        output DetectorPostions table workspace.

        @param filename :: name of the file to write. If it is empty nothing is saved/written.
        @param tbl :: detector positions table workspace
        """
        if not filename:
            return

        if filename.strip():
            SaveAscii(InputWorkspace=tbl, Filename=filename, WriteXError=True, WriteSpectrumID=False, Separator="Tab")

    def _get_calibrated_det_pos(self, new_difc, det, ws):
        """
        Returns a detector position which corresponds to the newDifc value.

        The two_theta and phi of the detector are preserved, L2 is changed.
        """
        # This is how det_L2 would be calculated
        # det_L2 = det.getDistance(ws.getInstrument().getSample())
        det_two_theta = ws.detectorTwoTheta(det)
        det_phi = det.getPhi()

        new_L2 = (new_difc / (252.816 * 2 * math.sin(det_two_theta / 2.0))) - 50

        new_pos = self._V3D_from_spherical(new_L2, det_two_theta, det_phi)

        return new_pos, new_L2

    def _apply_calibration_table(self, ws, detPos):
        """
        Corrects the detector positions using the result of calibration (if one is specified).
        The parameters of the instrument definition of the workspace are updated in place using the
        geometry parameters given in the input table.

        @param ws :: workspace to calibrate which should be consistent with the detector positions
        table passed

        @param detPos :: detector positions table to apply the calibration. It must have columns
        'Detector ID' and 'Detector Position'
        """
        self.log().notice("Applying calibration on the input workspace")
        alg = self.createChildAlgorithm("ApplyCalibration")
        alg.setProperty("Workspace", ws)
        alg.setProperty("CalibrationTable", detPos)
        alg.execute()

    def _V3D_from_spherical(self, R, polar, azimuth):
        """
        Returns a cartesian 3D vector for the given spherical coordinates.

        Borrowed from V3D::spherical (C++).
        """
        z = R * math.cos(polar)
        ct = R * math.sin(polar)
        x = ct * math.cos(azimuth)
        y = ct * math.sin(azimuth)

        return V3D(x, y, z)


AlgorithmFactory.subscribe(EnggCalibrateFull)
