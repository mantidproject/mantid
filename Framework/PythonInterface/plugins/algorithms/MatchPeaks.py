# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-branches
from mantid.api import (
    mtd,
    AlgorithmFactory,
    PythonAlgorithm,
    MatrixWorkspaceProperty,
    ITableWorkspace,
    ITableWorkspaceProperty,
    PropertyMode,
    MatrixWorkspace,
)
from mantid.kernel import logger, Direction
from mantid.simpleapi import CloneWorkspace, CreateEmptyTableWorkspace, DeleteWorkspace, FindEPP, MaskBins, ReplaceSpecialValues
import numpy as np


def mask_ws(ws_to_mask, xstart, xend):
    """
    Calls MaskBins twice, for masking the first and last bins of a workspace
    Args:
        red:      reduced workspace
        xstart:   MaskBins between x[0] and x[xstart]
        xend:     MaskBins between x[xend] and x[-1]

    """
    x_values = ws_to_mask.readX(0)

    if xstart > 0:
        logger.debug("Mask bins smaller than {0}".format(xstart))
        MaskBins(InputWorkspace=ws_to_mask, OutputWorkspace=ws_to_mask, XMin=x_values[0], XMax=x_values[int(xstart)])
    else:
        logger.debug("No masking due to x bin <= 0!: {0}".format(xstart))
    if xend < len(x_values) - 1:
        logger.debug("Mask bins larger than {0}".format(xend))
        MaskBins(InputWorkspace=ws_to_mask, OutputWorkspace=ws_to_mask, XMin=x_values[int(xend) + 1], XMax=x_values[-1])
    else:
        logger.debug("No masking due to x bin >= len(x_values) - 1!: {0}".format(xend))

    if xstart > 0 and xend < len(x_values) - 1:
        logger.information("Bins out of range {0} {1} [Unit of X-axis] are masked".format(x_values[int(xstart)], x_values[int(xend) + 1]))


class MatchPeaks(PythonAlgorithm):
    # Mandatory workspaces
    _input_ws = None
    _output_ws = None

    # Optional workspace
    _input_2_ws = None
    _input_3_ws = None
    _output_bin_range = None

    # Bool flags
    _masking = False
    _match_option = False

    def category(self):
        return "Transforms"

    def summary(self):
        return "Circular shifts (numpy.roll) the data of the input workspace to align peaks without modifying the x-axis."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", defaultValue="", direction=Direction.Input), doc="Input workspace")

        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace2", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Input workspace to align peaks with",
        )

        self.declareProperty(
            MatrixWorkspaceProperty("InputWorkspace3", defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional),
            doc="Input workspace to align peaks with",
        )

        self.declareProperty("MaskBins", defaultValue=False, doc="Whether to mask shifted bins")

        self.declareProperty("MatchInput2ToCenter", defaultValue=False, doc="Match peaks such that InputWorkspace2 would be centered")

        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", defaultValue="", direction=Direction.Output), doc="Shifted output workspace"
        )

        self.declareProperty(
            ITableWorkspaceProperty("BinRangeTable", defaultValue="", direction=Direction.Output, optional=PropertyMode.Optional),
            doc="Table workspace that contains two values defining a range. "
            "Bins outside this range are overflown out of range due to circular shift"
            "and can be masked.",
        )

    def setUp(self):
        self._input_ws = self.getPropertyValue("InputWorkspace")
        self._input_2_ws = self.getPropertyValue("InputWorkspace2")
        self._input_3_ws = self.getPropertyValue("InputWorkspace3")
        self._output_ws = self.getPropertyValue("OutputWorkspace")
        self._output_bin_range = self.getPropertyValue("BinRangeTable")

        self._masking = self.getProperty("MaskBins").value
        self._match_option = self.getProperty("MatchInput2ToCenter").value

        if self._input_ws:
            ReplaceSpecialValues(InputWorkspace=self._input_ws, OutputWorkspace=self._input_ws, NaNValue=0, InfinityValue=0)

        if self._input_2_ws:
            ReplaceSpecialValues(InputWorkspace=self._input_2_ws, OutputWorkspace=self._input_2_ws, NaNValue=0, InfinityValue=0)

        if self._input_3_ws:
            ReplaceSpecialValues(InputWorkspace=self._input_3_ws, OutputWorkspace=self._input_3_ws, NaNValue=0, InfinityValue=0)

    def validateInputs(self):
        issues = dict()
        input1 = self.getProperty("InputWorkspace").value
        input2 = self.getProperty("InputWorkspace2").value
        input3 = self.getProperty("InputWorkspace3").value
        input2_name = self.getPropertyValue("InputWorkspace2")
        input3_name = self.getPropertyValue("InputWorkspace3")

        if input1 is None:
            issues["InputWorkspace"] = "InputWorkspace must be a MatrixWorkspace"
        if input2_name and input2 is None:
            issues["InputWorkspace2"] = "InputWorkspace2 must be a MatrixWorkspace"
        if input3_name and input3 is None:
            issues["InputWorkspace3"] = "InputWorkspace3 must be a MatrixWorkspace"

        if issues:
            return issues

        if input3:
            if not input2:
                issues["InputWorkspace2"] = "When InputWorkspace3 is given, InputWorkspace2 is also required."
            else:
                if input3.isDistribution() and not input2.isDistribution():
                    issues["InputWorkspace3"] = "InputWorkspace2 and InputWorkspace3 must be either point data or " "histogram data"
                elif input3.blocksize() != input2.blocksize():
                    issues["InputWorkspace3"] = "Incompatible number of bins"
                elif input3.getNumberHistograms() != input2.getNumberHistograms():
                    issues["InputWorkspace3"] = "Incompatible number of spectra"
                elif np.any(input3.extractX() - input2.extractX()):
                    issues["InputWorkspace3"] = "Incompatible x-values"

        if input2:
            if input1.isDistribution() and not input2.isDistribution():
                issues["InputWorkspace2"] = "InputWorkspace2 and InputWorkspace3 must be either point data or " "histogram data"
            elif input1.blocksize() != input2.blocksize():
                issues["InputWorkspace2"] = "Incompatible number of bins"
            elif input1.getNumberHistograms() != input2.getNumberHistograms():
                issues["InputWorkspace2"] = "Incompatible number of spectra"
            elif np.any(input1.extractX() - input2.extractX()):
                issues["InputWorkspace2"] = "Incompatible x-values"

        return issues

    def PyExec(self):
        self.setUp()

        output_ws = CloneWorkspace(InputWorkspace=mtd[self._input_ws], OutputWorkspace=self._output_ws)

        size = mtd[self._input_ws].blocksize()
        mid_bin = int(size / 2)

        # Find peak positions in input workspace
        peak_bins1 = self._get_peak_position(output_ws)
        self.log().information("Peak bins {0}: {1}".format(self._input_ws, peak_bins1))

        # Find peak positions in second input workspace
        if self._input_2_ws:
            peak_bins2 = self._get_peak_position(mtd[self._input_2_ws])
            self.log().information("Peak bins {0}: {1}".format(self._input_2_ws, peak_bins2))

        # Find peak positions in third input workspace
        if self._input_3_ws:
            peak_bins3 = self._get_peak_position(mtd[self._input_3_ws])
            self.log().information("Peak bins {0}: {1}".format(self._input_3_ws, peak_bins3))

        # All bins must be positive and larger than zero
        if not self._input_2_ws:
            # Input workspace will match center
            to_shift = peak_bins1 - mid_bin * np.ones(mtd[self._input_ws].getNumberHistograms())
        else:
            if not self._input_3_ws:
                if self._match_option:
                    # Input workspace will be shifted according to centered peak of second input workspace
                    to_shift = peak_bins2 - mid_bin * np.ones(mtd[self._input_ws].getNumberHistograms())
                else:
                    # Input workspace will be shifted according to peak position of second input workspace
                    to_shift = peak_bins1 - peak_bins2
            else:
                # Input workspace will be shifted according to the difference of peak positions between second and third
                to_shift = peak_bins3 - peak_bins2

        # perform actual shift
        for i in range(output_ws.getNumberHistograms()):
            # Shift Y and E values of spectrum i
            output_ws.setY(i, np.roll(output_ws.readY(i), int(-to_shift[i])))
            output_ws.setE(i, np.roll(output_ws.readE(i), int(-to_shift[i])))

        self.log().debug("Shift array: {0}".format(-to_shift))

        # Final treatment of bins (masking, produce output)
        min_bin = 0
        max_bin = size

        # This is a left shift, bins at the end of the workspace may be masked
        if np.min(-to_shift) < 0:
            max_bin = size - 1 + np.min(-to_shift)

        # This is a right shift, bins at the beginning of the workspace may be masked
        if np.max(-to_shift) > 0:
            min_bin = np.max(-to_shift)

        if self._masking:
            mask_ws(output_ws, min_bin, max_bin)

        if self._output_bin_range:
            # Create table with its columns containing bin range
            bin_range = CreateEmptyTableWorkspace(OutputWorkspace=self._output_bin_range)
            bin_range.addColumn(type="double", name="MinBin")
            bin_range.addColumn(type="double", name="MaxBin")
            bin_range.addRow({"MinBin": min_bin, "MaxBin": max_bin})
            self.setProperty("BinRangeTable", bin_range)

        # Set output properties
        self.setProperty("OutputWorkspace", output_ws)

        return

    @staticmethod
    def _get_peak_position(input_ws):
        """
        Gives bin of the peak of each spectrum in the input_ws
        @param input_ws  :: input workspace
        @return          :: bin numbers of the peak positions
        """

        fit_table_name = input_ws.name() + "_epp"

        if isinstance(input_ws, MatrixWorkspace):
            fit_table = FindEPP(InputWorkspace=input_ws, OutputWorkspace=fit_table_name)
        elif isinstance(input_ws, ITableWorkspace):
            fit_table = input_ws
        else:
            logger.error("Workspace not defined")

        # Mid bin number
        mid_bin = int(input_ws.blocksize() / 2)
        # Initialisation
        peak_bin = np.ones(input_ws.getNumberHistograms()) * mid_bin
        # Bin range: difference between mid bin and peak bin should be in this range
        tolerance = int(mid_bin / 2)
        x_values = input_ws.readX(0)

        for i in range(input_ws.getNumberHistograms()):
            fit = fit_table.row(i)
            # Bin number, where Y has its maximum
            y_values = input_ws.readY(i)
            max_pos = np.argmax(y_values)
            peak_plus_error = abs(fit["PeakCentreError"]) + abs(fit["PeakCentre"])

            if peak_plus_error > x_values[0] and peak_plus_error < x_values[-1]:
                peak_plus_error_bin = input_ws.yIndexOfX(peak_plus_error)
                if abs(peak_plus_error_bin - mid_bin) < tolerance and fit["FitStatus"] == "success":
                    # fit succeeded, and fitted peak is within acceptance range, take it
                    peak_bin[i] = input_ws.yIndexOfX(fit["PeakCentre"])
                    logger.debug("Fit successfull, peak inside tolerance")
                elif abs(max_pos - mid_bin) < tolerance:
                    # fit not reliable, take the maximum if within acceptance
                    peak_bin[i] = max_pos
                    logger.debug("Fit outside the trusted range, take the maximum position")
                else:
                    # do not shift
                    logger.debug("Both the fit and the max are outside the trusted range. " "Do not shift the spectrum.")

            elif abs(max_pos - mid_bin) < tolerance:
                # fit not reliable, take the maximum if within acceptance
                peak_bin[i] = max_pos
                logger.debug("Fit outside the x-range, take the maximum position")

            else:
                # do not shift
                logger.debug("Both the fit and the max are outside the trusted range. " "Do not shift the spectrum.")

            logger.debug("Spectrum {0} will be shifted to bin {1}".format(i, peak_bin[i]))

        DeleteWorkspace(fit_table)

        return peak_bin


AlgorithmFactory.subscribe(MatchPeaks)
