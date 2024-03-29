# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    WorkspaceProperty,
    FileAction,
    FileProperty,
    PropertyMode,
)
from mantid.kernel import Direction
import csv
import collections
import math


class ReflectometryISISCalibration(DataProcessorAlgorithm):
    _WORKSPACE = "InputWorkspace"
    _CALIBRATION_FILE = "CalibrationFile"
    _OUTPUT_WORKSPACE = "OutputWorkspace"

    _COMMENT_PREFIX = "#"
    _NUM_COLUMNS_REQUIRED = 2
    _DET_ID_LABEL = "detectorid"
    _THETA_LABEL = "theta_offset"
    _RAD_TO_DEG = 180.0 / math.pi

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISCalibration"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Corrects the positions of detector pixels by applying TwoTheta offsets provided in a calibration file."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["ReflectometryISISLoadAndProcess", "SpecularReflectionPositionCorrect"]

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(self._WORKSPACE, "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="An input workspace or workspace group.",
        )
        self.declareProperty(
            FileProperty(self._CALIBRATION_FILE, "", action=FileAction.OptionalLoad, direction=Direction.Input, extensions=["dat"]),
            doc="Calibration data file containing a list of detector IDs and twoTheta offsets (in degrees)."
            f"These should be provided as two spaced-delimited columns, labelled {self._DET_ID_LABEL} and {self._THETA_LABEL}.",
        )
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WORKSPACE, "", direction=Direction.Output), doc="The calibrated output workspace."
        )

    def PyExec(self):
        # Set the expected order of the columns in the calibration file
        self._det_id_col_idx = 0
        self._theta_offset_col_idx = 1

        try:
            calibration_data = self._parse_calibration_file(self._calibration_filepath)
        except FileNotFoundError:
            raise FileNotFoundError("Calibration file path cannot be found")

        ws = self.getProperty(self._WORKSPACE).value
        calibrated_ws = self._correct_detector_positions(ws, calibration_data)

        self.setProperty(self._OUTPUT_WORKSPACE, calibrated_ws)

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()

        self._calibration_filepath = self.getPropertyValue(self._CALIBRATION_FILE)
        if not self._calibration_filepath:
            issues[self._CALIBRATION_FILE] = "Calibration file path must be provided"
        return issues

    def _parse_calibration_file(self, filepath):
        """Create a dictionary of detector IDs and theta offsets from the calibration file"""
        scanned_theta_offsets = {}
        with open(filepath, "r") as file:
            labels_checked = False

            file_reader = csv.reader(file)
            for row in file_reader:
                if len(row) == 0:
                    # Ignore any blank lines
                    continue

                entries = row[0].split()

                if entries[0][0] == self._COMMENT_PREFIX:
                    # Ignore any lines that begin with a #
                    # This allows the user to add any metadata they would like
                    continue

                if len(entries) != self._NUM_COLUMNS_REQUIRED:
                    raise RuntimeError(
                        "Calibration file should contain two space de-limited columns, "
                        f"labelled as {self._DET_ID_LABEL} and {self._THETA_LABEL}"
                    )

                if not labels_checked:
                    # The labels should be the first row in the file that doesn't begin with a #
                    self._check_file_column_labels(entries)
                    labels_checked = True
                    continue

                try:
                    scanned_theta_offsets[int(entries[self._det_id_col_idx])] = float(entries[self._theta_offset_col_idx])
                except ValueError:
                    raise ValueError(
                        f"Invalid data in calibration file entry {entries} "
                        "- detector ids should be integers and theta offsets should be floats"
                    )

            if len(scanned_theta_offsets) == 0:
                raise RuntimeError("Calibration file provided contains no data")
        return scanned_theta_offsets

    def _check_file_column_labels(self, row_entries):
        """Check that the file contains the required column labels"""
        valid_labels = collections.Counter([self._DET_ID_LABEL, self._THETA_LABEL])

        first_label = row_entries[self._det_id_col_idx].lower()
        second_label = row_entries[self._theta_offset_col_idx].lower()

        if collections.Counter([first_label, second_label]) == valid_labels:
            if first_label == self._THETA_LABEL:
                # Allow the columns to be specified in any order
                self._theta_offset_col_idx = 0
                self._det_id_col_idx = 1
        else:
            raise ValueError(f"Incorrect column labels in calibration file - should be {self._DET_ID_LABEL} and {self._THETA_LABEL}")

    def _clone_workspace(self, ws):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", InputWorkspace=ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _correct_detector_positions(self, ws, calibration_data):
        calibration_ws = self._clone_workspace(ws)
        det_info = calibration_ws.detectorInfo()

        correction_alg = self.createChildAlgorithm("SpecularReflectionPositionCorrect")
        # Turn off history recording to prevent the history becoming very large, as this causes reloading of a
        # calibrated workspace to be very slow
        correction_alg.enableHistoryRecordingForChild(False)
        # Passing the same workspace as both input and output means all the detector moves are applied to it
        correction_alg.setProperty("InputWorkspace", calibration_ws)
        correction_alg.setProperty("MoveFixedDetectors", True)
        correction_alg.setProperty("OutputWorkspace", calibration_ws)

        for det_id, theta_offset in calibration_data.items():
            if theta_offset == 0:
                # Detector position does not need to be changed
                continue

            new_two_theta = self._calculate_calibrated_two_theta(det_info, det_id, theta_offset)
            correction_alg.setProperty("TwoTheta", new_two_theta)
            correction_alg.setProperty("DetectorID", det_id)
            correction_alg.execute()

        return calibration_ws

    def _calculate_calibrated_two_theta(self, det_info, det_id, theta_offset):
        """Calculates the new twoTheta value for a detector from a given offset, in degrees"""
        try:
            det_idx = det_info.indexOf(det_id)
        except IndexError:
            raise RuntimeError(f"Detector id {det_id} from calibration file cannot be found in input workspace")

        current_two_theta = det_info.signedTwoTheta(det_idx)
        return (current_two_theta * self._RAD_TO_DEG) + theta_offset


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
