# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import AlgorithmFactory, ITableWorkspaceProperty, MatrixWorkspaceProperty, Progress, PropertyMode, PythonAlgorithm
from mantid.kernel import logger, Direction, FloatArrayProperty, StringListValidator

import EnggUtils


class EnggFocus(PythonAlgorithm):
    INDICES_PROP_NAME = "SpectrumNumbers"

    def category(self):
        return "Diffraction\\Engineering"

    def seeAlso(self):
        return ["AlignDetectors", "DiffractionFocussing"]

    def name(self):
        return "EnggFocus"

    def summary(self):
        return (
            "This algorithm is deprecated as of May 2021, consider using DiffractionFocussing instead."
            "Focuses a run by summing up all the spectra into a single one."
        )

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), "Workspace with the run to focus.")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "A workspace with focussed data")

        self.declareProperty(
            ITableWorkspaceProperty("DetectorPositions", "", Direction.Input, PropertyMode.Optional),
            "Calibrated detector positions. If not specified, default ones are used.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("VanadiumWorkspace", "", Direction.Input, PropertyMode.Optional),
            doc="Workspace with the Vanadium (correction and calibration) run. "
            "Alternatively, when the Vanadium run has been already processed, "
            "the properties can be used",
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
            "Bank",
            "",
            StringListValidator(EnggUtils.ENGINX_BANKS),
            direction=Direction.Input,
            doc="Which bank to focus: It can be specified as 1 or 2, or "
            "equivalently, North or South. See also " + self.INDICES_PROP_NAME + " "
            "for a more flexible alternative to select specific detectors",
        )

        self.declareProperty(
            self.INDICES_PROP_NAME,
            "",
            direction=Direction.Input,
            doc="Sets the spectrum numbers for the detectors "
            "that should be considered in the focusing operation (all others will be "
            "ignored). This option cannot be used together with Bank, as they overlap. "
            'You can give multiple ranges, for example: "0-99", or "0-9, 50-59, 100-109".',
        )

        banks_grp = "Banks / spectra"
        self.setPropertyGroup("Bank", banks_grp)
        self.setPropertyGroup(self.INDICES_PROP_NAME, banks_grp)

        self.declareProperty(
            "NormaliseByCurrent",
            True,
            direction=Direction.Input,
            doc="Normalize the input data by applying the NormaliseByCurrent algorithm "
            "which use the log entry gd_proton_charge. If there is no proton charge "
            "the data are not normalised.",
        )

        self.declareProperty(
            FloatArrayProperty("MaskBinsXMins", EnggUtils.ENGINX_MASK_BIN_MINS, direction=Direction.Input),
            doc="List of minimum bin values to mask, separated by commas.",
        )

        self.declareProperty(
            FloatArrayProperty("MaskBinsXMaxs", EnggUtils.ENGINX_MASK_BIN_MAXS, direction=Direction.Input),
            doc="List of maximum bin values to mask, separated by commas.",
        )

        prep_grp = "Data preparation/pre-processing"
        self.setPropertyGroup("NormaliseByCurrent", prep_grp)
        self.setPropertyGroup("MaskBinsXMins", prep_grp)
        self.setPropertyGroup("MaskBinsXMaxs", prep_grp)

    def validateInputs(self):
        issues = dict()

        if (
            not self.getPropertyValue("MaskBinsXMins")
            and self.getPropertyValue("MaskBinsXMaxs")
            or self.getPropertyValue("MaskBinsXMins")
            and not self.getPropertyValue("MaskBinsXMaxs")
        ):
            issues["MaskBinsXMins"] = "Both minimum and maximum values need to be given, or none"

        min_list = self.getProperty("MaskBinsXMins").value
        max_list = self.getProperty("MaskBinsXMaxs").value
        if len(min_list) > 0 and len(max_list) > 0:
            len_min = len(min_list)
            len_max = len(max_list)
            if len_min != len_max:
                issues["MaskBinsXMins"] = (
                    "The number of minimum and maximum values must match. Got "
                    "{0} and {1} for the minimum and maximum, respectively".format(len_min, len_max)
                )

        return issues

    def PyExec(self):
        logger.warning("EnggFocus is deprecated as of May 2021. Please use DiffractionFocussing instead.")
        # Get the run workspace
        input_ws = self.getProperty("InputWorkspace").value

        # Get spectra indices either from bank or direct list of indices, checking for errors
        bank = self.getProperty("Bank").value
        spectra = self.getProperty(self.INDICES_PROP_NAME).value
        indices = EnggUtils.get_ws_indices_from_input_properties(input_ws, bank, spectra)

        detector_positions = self.getProperty("DetectorPositions").value
        n_reports = 8
        prog = Progress(self, start=0, end=1, nreports=n_reports)

        # Leave only the data for the bank/spectra list requested
        prog.report("Selecting spectra from input workspace")
        input_ws = EnggUtils.crop_data(self, input_ws, indices)

        prog.report("Masking some bins if requested")
        self._mask_bins(input_ws, self.getProperty("MaskBinsXMins").value, self.getProperty("MaskBinsXMaxs").value)

        prog.report("Applying vanadium corrections")
        # Leave data for the same bank in the vanadium workspace too
        vanadium_ws = self.getProperty("VanadiumWorkspace").value
        van_integration_ws = self.getProperty("VanIntegrationWorkspace").value
        van_curves_ws = self.getProperty("VanCurvesWorkspace").value
        EnggUtils.apply_vanadium_corrections(
            parent=self,
            ws=input_ws,
            indices=indices,
            vanadium_ws=vanadium_ws,
            van_integration_ws=van_integration_ws,
            van_curves_ws=van_curves_ws,
            progress_range=(0.65, 0.8),
        )

        prog.report("Applying calibration if requested")
        # Apply calibration
        if detector_positions:
            self._apply_calibration(input_ws, detector_positions)

        # Convert to dSpacing
        prog.report("Converting to d")
        input_ws = EnggUtils.convert_to_d_spacing(self, input_ws)

        prog.report("Summing spectra")
        # Sum the values across spectra
        input_ws = EnggUtils.sum_spectra(self, input_ws)

        prog.report("Preparing output workspace")
        # Convert back to time of flight
        input_ws = EnggUtils.convert_to_TOF(self, input_ws)

        prog.report("Normalizing input workspace if needed")
        if self.getProperty("NormaliseByCurrent").value:
            self._normalize_by_current(input_ws)

        # OpenGenie displays distributions instead of pure counts (this is done implicitly when
        # converting units), so I guess that's what users will expect
        self._convert_to_distribution(input_ws)

        self._add_bank_number(input_ws, bank)

        self.setProperty("OutputWorkspace", input_ws)

    def _bank_to_int(self, bank):
        if bank == "North":
            return "1"
        if bank == "South":
            return "2"
        if bank in ("1", "2"):
            return bank
        # The convention is to set bank ID to 0 for cropped / texture runs
        return "0"

    def _add_bank_number(self, ws, bank):
        alg = self.createChildAlgorithm("AddSampleLog")
        alg.setProperty("Workspace", ws)
        alg.setProperty("LogName", "bankid")
        alg.setProperty("LogText", self._bank_to_int(bank))
        alg.setProperty("LogType", "Number")
        alg.execute()

    def _mask_bins(self, wks, min_bins, max_bins):
        """
        Mask multiple ranges of bins, given multiple pairs min-max

        @param wks :: workspace that will be masked (in/out, masked in place)
        @param min_bins :: list of minimum values for every range to mask
        @param max_bins :: list of maxima
        """
        for min_x, max_x in zip(min_bins, max_bins):
            alg = self.createChildAlgorithm("MaskBins")
            alg.setProperty("InputWorkspace", wks)
            alg.setProperty("OutputWorkspace", wks)
            alg.setProperty("XMin", min_x)
            alg.setProperty("XMax", max_x)
            alg.execute()

    def _normalize_by_current(self, wks):
        """
        Apply the normalize by current algorithm on a workspace

        @param wks :: workspace (in/out, modified in place)
        """
        if wks.getRun().getProtonCharge() > 0:
            alg = self.createChildAlgorithm("NormaliseByCurrent")
            alg.setProperty("InputWorkspace", wks)
            alg.setProperty("OutputWorkspace", wks)
            alg.execute()
        else:
            self.log().warning(f"Cannot normalize by current because workspace {wks.name()} has invalid proton charge")

    def _apply_calibration(self, wks, detector_positions):
        """
        Refines the detector positions using the result of calibration (if one is specified).

        @param wks :: workspace to apply the calibration (on its instrument)
        @param detector_positions :: detector positions (as a table of positions, one row per detector)
        """
        alg = self.createChildAlgorithm("ApplyCalibration")
        alg.setProperty("Workspace", wks)
        alg.setProperty("CalibrationTable", detector_positions)
        alg.execute()

    def _convert_to_distribution(self, wks):
        """
        Convert workspace to distribution

        @param wks :: workspace, which is modified/converted in place
        """
        alg = self.createChildAlgorithm("ConvertToDistribution")
        alg.setProperty("Workspace", wks)
        alg.execute()


AlgorithmFactory.subscribe(EnggFocus)
