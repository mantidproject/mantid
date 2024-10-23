# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import mtd, AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, WorkspaceProperty
from mantid.kernel import logger, Direction, FloatArrayProperty, StringListValidator
from mantid.simpleapi import (
    AppendSpectra,
    CropWorkspace,
    DeleteWorkspace,
    FindPeaks,
    GroupDetectors,
    RebinToWorkspace,
    ScaleX,
    SortTableWorkspace,
)


class TOSCABankCorrection(DataProcessorAlgorithm):
    _input_ws = None
    _output_ws = None
    _search_range = None
    _peak_tolerance = None
    _peak_position = None
    _peak_function = None

    def category(self):
        return "Inelastic\\Corrections;CorrectionFunctions\\SpecialCorrections"

    def summary(self):
        return "Corrects TOSCA reductions where the peaks across banks are not in alignment."

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input), doc="Input reduced workspace"
        )

        self.declareProperty(FloatArrayProperty(name="SearchRange", values=[200, 2000]), doc="Range over which to find peaks")

        self.declareProperty(name="PeakPosition", defaultValue="", doc="Specify a particular peak to use")

        self.declareProperty(name="ClosePeakTolerance", defaultValue=20.0, doc="Tolerance under which peaks are considered to be the same")

        self.declareProperty(
            name="PeakFunction",
            defaultValue="Lorentzian",
            validator=StringListValidator(["Lorentzian", "Gaussian"]),
            doc="Type of peak to search for",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output), doc="Output corrected workspace"
        )

        self.declareProperty(
            name="TargetPeakCentre",
            defaultValue=0.0,
            direction=Direction.Output,
            doc="X position between the centres of the two " "selected peaks",
        )

        self.declareProperty(
            name="ScaleFactor1", defaultValue=1.0, direction=Direction.Output, doc="Scale factor for the first bank (histogram 0)"
        )

        self.declareProperty(
            name="ScaleFactor2", defaultValue=1.0, direction=Direction.Output, doc="Scale factor for the second bank (histogram 1)"
        )

    def validateInputs(self):
        self._get_properties()
        issues = dict()

        # Validate search range
        if len(self._search_range) != 2:
            issues["SearchRange"] = "Search range must have two values"
        elif self._search_range[0] > self._search_range[1]:
            issues["SearchRange"] = 'Search range must be in format "low,high"'

        # Ensure manual peak position is inside search range
        if "SearchRange" not in issues and self._peak_position is not None:
            if self._peak_position < self._search_range[0] or self._peak_position > self._search_range[1]:
                issues["PeakPosition"] = "Peak position must be inside SearchRange"

        return issues

    def PyExec(self):
        self._get_properties()

        # Crop the sample workspace to the search range
        CropWorkspace(InputWorkspace=self._input_ws, OutputWorkspace="__search_ws", XMin=self._search_range[0], XMax=self._search_range[1])

        peak = self._get_peak("__search_ws")
        DeleteWorkspace("__search_ws")

        # Ensure there is at least one peak found
        if peak is None:
            raise RuntimeError("Could not find any peaks. Try increasing width of SearchRange and/or ClosePeakTolerance")

        target_centre = (peak[0] + peak[1]) / 2.0
        bank_1_scale_factor = target_centre / peak[0]
        bank_2_scale_factor = target_centre / peak[1]

        logger.information("Bank 1 scale factor: %f" % bank_1_scale_factor)
        logger.information("Bank 2 scale factor: %f" % bank_2_scale_factor)

        self._apply_correction(bank_1_scale_factor, bank_2_scale_factor)

        self.setPropertyValue("OutputWorkspace", self._output_ws)

        self.setProperty("TargetPeakCentre", target_centre)

        self.setProperty("ScaleFactor1", bank_1_scale_factor)
        self.setProperty("ScaleFactor2", bank_2_scale_factor)

    def _get_properties(self):
        self._input_ws = self.getPropertyValue("InputWorkspace")
        self._output_ws = self.getPropertyValue("OutputWorkspace")

        self._search_range = self.getProperty("SearchRange").value
        self._peak_tolerance = self.getProperty("ClosePeakTolerance").value

        self._peak_function = self.getPropertyValue("PeakFunction")

        try:
            self._peak_position = float(self.getPropertyValue("PeakPosition"))
        except ValueError:
            self._peak_position = None

    def _get_peak(self, search_ws):
        """
        Finds a matching peak over the two banks.

        @param search_ws Workspace to search
        @return Peak centres for matching peak over both banks
        """

        find_peak_args = dict()
        if self._peak_position is not None:
            find_peak_args["PeakPositions"] = [self._peak_position]

        # Find the peaks in each bank
        FindPeaks(
            InputWorkspace=search_ws, PeaksList="__bank_1_peaks", WorkspaceIndex=0, PeakFunction=self._peak_function, **find_peak_args
        )

        FindPeaks(
            InputWorkspace=search_ws, PeaksList="__bank_2_peaks", WorkspaceIndex=1, PeakFunction=self._peak_function, **find_peak_args
        )

        # Sort peaks by height, prefer to match tall peaks
        SortTableWorkspace(InputWorkspace="__bank_1_peaks", OutputWorkspace="__bank_1_peaks", Columns="centre", Ascending=False)

        bank_1_ws = mtd["__bank_1_peaks"]
        bank_2_ws = mtd["__bank_2_peaks"]

        matching_peaks = list()

        # Find the centres of two peaks that are close to each other on both banks
        for peak_idx in range(0, bank_1_ws.rowCount()):
            bank_1_centre = bank_1_ws.cell("centre", peak_idx)

            for other_peak_idx in range(0, bank_2_ws.rowCount()):
                bank_2_centre = bank_2_ws.cell("centre", other_peak_idx)

                if abs(bank_1_centre - bank_2_centre) < self._peak_tolerance:
                    matching_peaks.append((bank_1_centre, bank_2_centre))

        # Remove temporary workspaces
        DeleteWorkspace("__bank_1_peaks")
        DeleteWorkspace("__bank_2_peaks")

        if len(matching_peaks) > 0:
            selected_peak = matching_peaks[0]
            logger.debug("Found matching peak: %s" % (str(selected_peak)))
        else:
            selected_peak = None
            logger.warning("No peak found")

        return selected_peak

    def _apply_correction(self, bank_1_sf, bank_2_sf):
        """
        Applies correction to a copy of the input workspace.

        @param bank_1_sf Bank 1 scale factor
        @param bank_2_sf Bank 2 scale factor
        """

        # Create a copy of the workspace with just bank spectra
        CropWorkspace(InputWorkspace=self._input_ws, OutputWorkspace=self._output_ws, StartWorkspaceIndex=0, EndWorkspaceIndex=1)

        # Scale bank 1 X axis
        ScaleX(
            InputWorkspace=self._output_ws, OutputWorkspace=self._output_ws, Operation="Multiply", Factor=bank_1_sf, IndexMin=0, IndexMax=0
        )

        # Scale bank 2 X axis
        ScaleX(
            InputWorkspace=self._output_ws, OutputWorkspace=self._output_ws, Operation="Multiply", Factor=bank_2_sf, IndexMin=1, IndexMax=1
        )

        # Rebin the corrected workspace to match the input
        RebinToWorkspace(WorkspaceToRebin=self._output_ws, WorkspaceToMatch=self._input_ws, OutputWorkspace=self._output_ws)

        # Recalculate the sum spectra from corrected banks
        GroupDetectors(InputWorkspace=self._output_ws, OutputWorkspace="__sum", SpectraList=[0, 1], Behaviour="Average")

        # Add the new sum spectra to the output workspace
        AppendSpectra(InputWorkspace1=self._output_ws, InputWorkspace2="__sum", OutputWorkspace=self._output_ws)

        DeleteWorkspace("__sum")


AlgorithmFactory.subscribe(TOSCABankCorrection)
