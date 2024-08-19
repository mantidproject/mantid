# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import math
import numpy as np

import mantid.simpleapi as ms
from mantid import logger, mtd
from mantid.api import (
    PythonAlgorithm,
    AlgorithmFactory,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    ITableWorkspaceProperty,
    PropertyMode,
    WorkspaceFactory,
    Progress,
)
from mantid.kernel import Direction, IntArrayProperty


# pylint: disable=too-many-instance-attributes


class Symmetrise(PythonAlgorithm):
    _sample = None
    _x_min = None
    _x_max = None
    _spectra_range = None
    _output_workspace = None
    _props_output_workspace = None
    _positive_min_index = None
    _positive_max_index = None
    _negative_min_index = None

    def category(self):
        return "CorrectionFunctions\\SpecialCorrections"

    def summary(self):
        return "Make asymmetric workspace data symmetric."

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), doc="Sample to run with")

        self.declareProperty(
            IntArrayProperty(name="SpectraRange"), doc="Range of spectra to symmetrise (defaults to entire range if not set)"
        )

        self.declareProperty("XMin", 0.0, doc="X value marking lower limit of curve to copy")
        self.declareProperty("XMax", 0.0, doc="X value marking upper limit of curve to copy")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Name to call the output workspace.")

        self.declareProperty(
            ITableWorkspaceProperty("OutputPropertiesTable", "", Direction.Output, PropertyMode.Optional),
            doc="Name to call the properties output table workspace.",
        )

    # pylint: disable=too-many-locals
    def PyExec(self):
        workflow_prog = Progress(self, start=0.0, end=0.3, nreports=4)
        workflow_prog.report("Setting up algorithm")
        self._setup()

        input_ws = mtd[self._sample]

        min_spectrum_index = input_ws.getIndexFromSpectrumNumber(int(self._spectra_range[0]))
        max_spectrum_index = input_ws.getIndexFromSpectrumNumber(int(self._spectra_range[1]))

        # Crop to the required spectra range
        workflow_prog.report("Cropping Workspace")
        cropped_input = ms.CropWorkspace(
            InputWorkspace=input_ws, OutputWorkspace="__symm", StartWorkspaceIndex=min_spectrum_index, EndWorkspaceIndex=max_spectrum_index
        )
        # Find the smallest data array in the first spectra
        len_x = len(cropped_input.readX(0))
        len_y = len(cropped_input.readY(0))
        len_e = len(cropped_input.readE(0))
        sample_array_len = min(len_x, len_y, len_e)

        sample_x = cropped_input.readX(0)

        # Get slice bounds of array
        try:
            workflow_prog.report("Calculating array points")
            self._calculate_array_points(sample_x, sample_array_len)
        except Exception as exc:
            raise RuntimeError("Failed to calculate array slice boundaries: %s" % exc.message)

        max_sample_index = sample_array_len - 1
        centre_range_len = self._positive_min_index + self._negative_min_index
        positive_diff_range_len = max_sample_index - self._positive_max_index

        output_cut_index = max_sample_index - self._positive_min_index - positive_diff_range_len - 1
        new_array_len = 2 * max_sample_index - centre_range_len - 2 * positive_diff_range_len - 1

        logger.information("Sample array length = %d" % sample_array_len)

        logger.information("Positive X min at i=%d, x=%f" % (self._positive_min_index, sample_x[self._positive_min_index]))
        logger.information("Negative X min at i=%d, x=%f" % (self._negative_min_index, sample_x[self._negative_min_index]))

        logger.information("Positive X max at i=%d, x=%f" % (self._positive_max_index, sample_x[self._positive_max_index]))

        logger.information("New array length = %d" % new_array_len)
        logger.information("Output array LR split index = %d" % output_cut_index)

        # Create an empty workspace with enough storage for the new data
        workflow_prog.report("Creating OutputWorkspace")
        out_ws = WorkspaceFactory.Instance().create(
            cropped_input, cropped_input.getNumberHistograms(), int(new_array_len), int(new_array_len)
        )

        # For each spectrum copy positive values to the negative
        pop_prog = Progress(self, start=0.3, end=0.95, nreports=out_ws.getNumberHistograms())
        for idx in range(out_ws.getNumberHistograms()):
            pop_prog.report("Populating data in workspace %i" % idx)
            # Strip any additional array cells
            x_in = cropped_input.readX(idx)[:sample_array_len]
            y_in = cropped_input.readY(idx)[:sample_array_len]
            e_in = cropped_input.readE(idx)[:sample_array_len]

            # Get some zeroed data to overwrite with copies from sample
            x_out = np.zeros(new_array_len)
            y_out = np.zeros(new_array_len)
            e_out = np.zeros(new_array_len)

            # Left hand side (reflected)
            x_out[:output_cut_index] = -x_in[self._positive_max_index - 1 : self._positive_min_index : -1]
            y_out[:output_cut_index] = y_in[self._positive_max_index - 1 : self._positive_min_index : -1]
            e_out[:output_cut_index] = e_in[self._positive_max_index - 1 : self._positive_min_index : -1]

            # Right hand side (copied)
            x_out[output_cut_index:] = x_in[self._negative_min_index : self._positive_max_index]
            y_out[output_cut_index:] = y_in[self._negative_min_index : self._positive_max_index]
            e_out[output_cut_index:] = e_in[self._negative_min_index : self._positive_max_index]

            # Set output spectrum data
            out_ws.setX(idx, x_out)
            out_ws.setY(idx, y_out)
            out_ws.setE(idx, e_out)

            logger.information("Symmetrise spectrum %d" % idx)

        end_prog = Progress(self, start=0.95, end=1.0, nreports=3)
        end_prog.report("Deleting temp workspaces")
        ms.DeleteWorkspace(cropped_input)

        if self._props_output_workspace != "":
            end_prog.report("Generating property table")
            self._generate_props_table()

        self.setProperty("OutputWorkspace", out_ws)
        end_prog.report("Algorithm Complete")

    def validateInputs(self):
        """
        Checks for invalid input properties.
        """
        from IndirectCommon import check_hist_zero

        issues = dict()

        input_workspace = self.getProperty("InputWorkspace").value
        if not isinstance(input_workspace, MatrixWorkspace):
            issues["InputWorkspace"] = "The InputWorkspace must be a MatrixWorkspace."
            return issues

        # Validate spectra range
        spectra_range = self.getProperty("SpectraRange").value
        if len(spectra_range) != 0 and len(spectra_range) != 2:
            issues["SpectraRange"] = 'Must be in format "spec_min,spec_max"'

        if len(spectra_range) == 2:
            spec_min = spectra_range[0]
            spec_max = spectra_range[1]

            num_sample_spectra, _ = check_hist_zero(input_workspace.name())
            min_spectra_number = input_workspace.getSpectrum(0).getSpectrumNo()
            max_spectra_number = input_workspace.getSpectrum(num_sample_spectra - 1).getSpectrumNo()

            if spec_min < min_spectra_number:
                issues["SpectraRange"] = "Minimum spectra must be greater than or equal to %d" % min_spectra_number

            if spec_max > max_spectra_number:
                issues["SpectraRange"] = "Maximum spectra must be less than or equal to %d" % max_spectra_number

            if spec_max < spec_min:
                issues["SpectraRange"] = "Minimum spectra must be smaller than maximum spectra"

        # Validate X range
        x_min = self.getProperty("XMin").value
        if x_min < -1e-5:
            issues["XMin"] = "XMin must be greater than or equal to zero"

        x_max = self.getProperty("XMax").value
        if x_max < 1e-5:
            issues["XMax"] = "XMax must be greater than zero"

        if math.fabs(x_max - x_min) < 1e-5:
            issues["XMin"] = "X range is close to zero"
            issues["XMax"] = "X range is close to zero"

        if x_max < x_min:
            issues["XMin"] = "XMin must be less than XMax"
            issues["XMax"] = "XMax must be greater than XMin"

        # Valudate X range against workspace X range
        sample_x = input_workspace.readX(0)
        sample_x_min = sample_x.min()
        sample_x_max = sample_x.max()

        if x_max > sample_x_max:
            issues["XMax"] = "XMax value (%f) is greater than largest X value (%f)" % (x_max, sample_x_max)

        if -x_min < sample_x_min:
            issues["XMin"] = "Negative XMin value (%f) is less than smallest X value (%f)" % (-x_min, sample_x_min)

        return issues

    def _setup(self):
        """
        Get the algorithm properties and validate them.
        """
        from IndirectCommon import check_hist_zero

        self._sample = self.getPropertyValue("InputWorkspace")

        self._x_min = math.fabs(self.getProperty("XMin").value)
        self._x_max = math.fabs(self.getProperty("XMax").value)

        self._spectra_range = self.getProperty("SpectraRange").value
        # If the user did not enter a spectra range, use the spectra range of the workspace
        if len(self._spectra_range) == 0:
            num_sample_spectra, _ = check_hist_zero(self._sample)
            min_spectra_number = mtd[self._sample].getSpectrum(0).getSpectrumNo()
            max_spectra_number = mtd[self._sample].getSpectrum(num_sample_spectra - 1).getSpectrumNo()
            self._spectra_range = [min_spectra_number, max_spectra_number]

        self._output_workspace = self.getPropertyValue("OutputWorkspace")
        self._props_output_workspace = self.getPropertyValue("OutputPropertiesTable")

    def _calculate_array_points(self, sample_x, sample_array_len):
        """
        Finds the points in the array that match the cut points.

        @param sample_x - Sample X axis data
        @param sample_array_len - Length of data array for sample data
        """
        # Find array index of the first negative XMin
        negative_min_diff = sample_x + self._x_min
        self._negative_min_index = np.where(negative_min_diff > 0)[0][0]
        self._check_bounds(self._negative_min_index, sample_array_len, label="Negative")

        # Find array index of the first positive XMin, that is smaller than the required
        positive_min_diff = sample_x + sample_x[self._negative_min_index]
        self._positive_min_index = np.where(positive_min_diff > 0)[0][0]
        self._check_bounds(self._positive_min_index, sample_array_len, label="Positive")

        # Find array index of the first positive XMax, that is smaller than the required
        self._positive_max_index = np.where(sample_x < self._x_max)[0][-1]
        if self._positive_max_index == sample_array_len:
            self._positive_max_index -= 1
        self._check_bounds(self._positive_max_index, sample_array_len, label="Positive")

    def _check_bounds(self, index, num_pts, label=""):
        """
        Check if the index falls within the bounds of the x range.
        Throws a ValueError if the x point falls outside of the range.

        @param index  - value of the index within the x range.
        @param num_pts - total number of points in the range.
        @param label - label to call the point if an error is thrown.
        """
        if index < 0:
            raise ValueError("%s point %d < 0" % (label, index))
        elif index >= num_pts:
            raise ValueError("%s point %d > %d" % (label, index, num_pts))

    def _generate_props_table(self):
        """
        Creates a table workspace with values calculated in algorithm.
        """
        props_table = ms.CreateEmptyTableWorkspace(OutputWorkspace=self._props_output_workspace)

        props_table.addColumn("int", "NegativeXMinIndex")
        props_table.addColumn("int", "PositiveXMinIndex")
        props_table.addColumn("int", "PositiveXMaxIndex")

        props_table.addRow([int(self._negative_min_index), int(self._positive_min_index), int(self._positive_max_index)])

        self.setProperty("OutputPropertiesTable", self._props_output_workspace)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(Symmetrise)
