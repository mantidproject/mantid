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
from mantid.kernel import Direction, EnabledWhenProperty, FloatBoundedValidator, PropertyCriterion, StringListValidator
import csv
import collections
import math


class ReflectometryISISCalibration(DataProcessorAlgorithm):
    _WORKSPACE = "InputWorkspace"
    _CALIBRATION_FILE = "CalibrationFile"
    _CALIBRATION_ANGLE_TYPE = "CalibrationAngleType"
    _ABSOLUTE_ANGLE_TYPE = "AbsoluteAngleType"
    _CALIBRATION_SPECULAR_PIXEL_INDEX = "CalibrationSpecularPixelIndex"
    _EXPERIMENT_SPECULAR_PIXEL_INDEX = "ExperimentSpecularPixelIndex"
    _DETECTOR_CORRECTION_TYPE = "DetectorCorrectionType"
    _OUTPUT_WORKSPACE = "OutputWorkspace"

    _COMMENT_PREFIX = "#"
    _NUM_COLUMNS_REQUIRED = 2
    _DET_ID_LABEL = "detectorid"
    _THETA_LABEL = "theta_offset"
    _ANGLE_LABEL = "angle"
    _OFFSET = "Offset"
    _ABSOLUTE = "Absolute"
    _THETA = "Theta"
    _TWO_THETA = "TwoTheta"
    _VERTICAL_SHIFT = "VerticalShift"
    _ROTATE_AROUND_SAMPLE = "RotateAroundSample"
    _RAD_TO_DEG = 180.0 / math.pi

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISCalibration"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Corrects the positions of detector pixels using offsets or absolute angles provided in a calibration file."

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
            doc="Calibration data file containing a list of detector IDs and offsets, or an ordered list of absolute angles.",
        )
        self.declareProperty(
            self._CALIBRATION_ANGLE_TYPE,
            self._OFFSET,
            StringListValidator([self._OFFSET, self._ABSOLUTE]),
            "Whether the calibration file contains twoTheta offsets or absolute detector angles.",
        )
        self.declareProperty(
            self._ABSOLUTE_ANGLE_TYPE,
            self._THETA,
            StringListValidator([self._THETA, self._TWO_THETA]),
            "Whether absolute detector angle values are theta or twoTheta values.",
        )
        non_negative_double = FloatBoundedValidator()
        non_negative_double.setLower(0.0)
        self.declareProperty(
            self._CALIBRATION_SPECULAR_PIXEL_INDEX,
            0.0,
            non_negative_double,
            "The detector index of the specular pixel in the absolute calibration file.",
        )
        self.declareProperty(
            self._EXPERIMENT_SPECULAR_PIXEL_INDEX,
            0.0,
            non_negative_double,
            "The detector index of the specular pixel in the input workspace.",
        )
        absolute_angle_enabled = EnabledWhenProperty(self._CALIBRATION_ANGLE_TYPE, PropertyCriterion.IsEqualTo, self._ABSOLUTE)
        self.setPropertySettings(self._ABSOLUTE_ANGLE_TYPE, absolute_angle_enabled)
        self.setPropertySettings(self._CALIBRATION_SPECULAR_PIXEL_INDEX, absolute_angle_enabled)
        self.setPropertySettings(self._EXPERIMENT_SPECULAR_PIXEL_INDEX, absolute_angle_enabled)
        self.declareProperty(
            self._DETECTOR_CORRECTION_TYPE,
            self._VERTICAL_SHIFT,
            StringListValidator([self._VERTICAL_SHIFT, self._ROTATE_AROUND_SAMPLE]),
            "Whether detectors should be shifted vertically or rotated around the sample position.",
        )
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WORKSPACE, "", direction=Direction.Output), doc="The calibrated output workspace."
        )

    def PyExec(self):
        # Set the expected order of the columns in the calibration file
        self._det_id_col_idx = 0
        self._angle_col_idx = 1

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
        """Parse calibration data from the calibration file."""
        if self._calibration_angle_type() == self._ABSOLUTE:
            return self._parse_absolute_calibration_file(filepath)
        else:
            return self._parse_offset_calibration_file(filepath)

    def _parse_offset_calibration_file(self, filepath):
        """Create a dictionary of detector IDs and theta offsets from the calibration file."""
        scanned_theta_offsets = {}
        with open(filepath, "r") as file:
            labels_checked = False

            for entries in self._file_entries(file):
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
                    scanned_theta_offsets[int(entries[self._det_id_col_idx])] = float(entries[self._angle_col_idx])
                except ValueError:
                    raise ValueError(
                        f"Invalid data in calibration file entry {entries} "
                        "- detector ids should be integers and theta offsets should be floats"
                    )

            if len(scanned_theta_offsets) == 0:
                raise RuntimeError("Calibration file provided contains no data")
        return scanned_theta_offsets

    def _parse_absolute_calibration_file(self, filepath):
        """Create an ordered list of absolute angles from the calibration file."""
        calibration_angles = []
        with open(filepath, "r") as file:
            labels_checked = False

            for entries in self._file_entries(file):
                if len(entries) != self._NUM_COLUMNS_REQUIRED:
                    raise RuntimeError(
                        "Calibration file should contain two space de-limited columns, "
                        f"labelled as {self._DET_ID_LABEL} and {self._ANGLE_LABEL}"
                    )

                if not labels_checked:
                    self._check_absolute_file_column_labels(entries)
                    labels_checked = True
                    continue

                try:
                    float(entries[self._det_id_col_idx])
                    calibration_angles.append(float(entries[self._angle_col_idx]))
                except ValueError:
                    error_message = f"Invalid data in calibration file entry {entries} - detector indexes and angles should be numeric"
                    raise ValueError(error_message)

            if len(calibration_angles) == 0:
                raise RuntimeError("Calibration file provided contains no data")
        return calibration_angles

    def _file_entries(self, file):
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

            yield entries

    def _check_file_column_labels(self, row_entries):
        """Check that the file contains the required column labels"""
        valid_labels = collections.Counter([self._DET_ID_LABEL, self._THETA_LABEL])

        first_label = row_entries[self._det_id_col_idx].lower()
        second_label = row_entries[self._angle_col_idx].lower()

        if collections.Counter([first_label, second_label]) == valid_labels:
            if first_label == self._THETA_LABEL:
                # Allow the columns to be specified in any order
                self._angle_col_idx = 0
                self._det_id_col_idx = 1
        else:
            raise ValueError(f"Incorrect column labels in calibration file - should be {self._DET_ID_LABEL} and {self._THETA_LABEL}")

    def _check_absolute_file_column_labels(self, row_entries):
        """Check that an absolute-angle file contains the required column labels."""
        valid_labels = collections.Counter([self._DET_ID_LABEL, self._ANGLE_LABEL])

        first_label = row_entries[self._det_id_col_idx].lower()
        second_label = row_entries[self._angle_col_idx].lower()

        if collections.Counter([first_label, second_label]) == valid_labels:
            if first_label == self._ANGLE_LABEL:
                # Allow the columns to be specified in any order
                self._angle_col_idx = 0
                self._det_id_col_idx = 1
        else:
            raise ValueError(f"Incorrect column labels in calibration file - should be {self._DET_ID_LABEL} and {self._ANGLE_LABEL}")

    def _clone_workspace(self, ws):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", InputWorkspace=ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _correct_detector_positions(self, ws, calibration_data):
        calibration_ws = self._clone_workspace(ws)
        det_info = calibration_ws.detectorInfo()

        if self._calibration_angle_type() == self._ABSOLUTE:
            calibration_data = self._convert_absolute_angles_to_offsets(calibration_ws, calibration_data)

        correction_alg = self.createChildAlgorithm("SpecularReflectionPositionCorrect")
        # Turn off history recording to prevent the history becoming very large, as this causes reloading of a
        # calibrated workspace to be very slow
        correction_alg.enableHistoryRecordingForChild(False)
        # Passing the same workspace as both input and output means all the detector moves are applied to it
        correction_alg.setProperty("InputWorkspace", calibration_ws)
        correction_alg.setProperty("MoveFixedDetectors", True)
        correction_alg.setProperty("OutputWorkspace", calibration_ws)
        correction_alg.setProperty(self._DETECTOR_CORRECTION_TYPE, self._detector_correction_type())

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

    def _convert_absolute_angles_to_offsets(self, ws, absolute_calibration_angles):
        """Convert ordered absolute angles to detector ID keyed twoTheta offsets."""
        detector_spectrum_indices = self._detector_spectrum_indices(ws)
        self._validate_absolute_calibration_inputs(detector_spectrum_indices, absolute_calibration_angles)

        experiment_specular_index = self._experiment_specular_pixel_index()
        spectrum_info = ws.spectrumInfo()
        experiment_specular_two_theta = self._interpolate_experiment_two_theta(
            spectrum_info, detector_spectrum_indices, experiment_specular_index
        )
        calibration_specular_angle = self._interpolate(absolute_calibration_angles, self._calibration_specular_pixel_index())
        two_theta_conversion_factor = 2.0 if self._absolute_angle_type() == self._THETA else 1.0

        offsets = {}
        for pixel_index_to_calibrate, spectrum_index in enumerate(detector_spectrum_indices):
            angle_relative_to_calibration_specular = (
                self._interpolate(absolute_calibration_angles, pixel_index_to_calibrate) - calibration_specular_angle
            )
            # Use the angle between this pixel and the calibration specular pixel to place it relative to the
            # experiment specular pixel.
            relative_two_theta = two_theta_conversion_factor * angle_relative_to_calibration_specular
            calibrated_two_theta = experiment_specular_two_theta - relative_two_theta
            experiment_two_theta = spectrum_info.signedTwoTheta(spectrum_index) * self._RAD_TO_DEG
            offsets[self._single_detector_id(ws, spectrum_index)] = calibrated_two_theta - experiment_two_theta
        return offsets

    def _validate_absolute_calibration_inputs(self, detector_spectrum_indices, absolute_calibration_angles):
        if len(detector_spectrum_indices) > len(absolute_calibration_angles):
            raise RuntimeError(
                "Absolute calibration file does not contain enough detector angle values "
                f"for the input workspace: {len(absolute_calibration_angles)} values for {len(detector_spectrum_indices)} detectors"
            )
        self._validate_interpolation_index(
            self._calibration_specular_pixel_index(), len(absolute_calibration_angles), "CalibrationSpecularPixelIndex"
        )
        self._validate_interpolation_index(
            self._experiment_specular_pixel_index(), len(detector_spectrum_indices), "ExperimentSpecularPixelIndex"
        )

    @staticmethod
    def _detector_spectrum_indices(ws):
        spectrum_info = ws.spectrumInfo()
        return [
            index
            for index in range(ws.getNumberHistograms())
            if spectrum_info.hasDetectors(index) and spectrum_info.hasUniqueDetector(index) and not spectrum_info.isMonitor(index)
        ]

    @staticmethod
    def _single_detector_id(ws, spectrum_index):
        detector_ids = ws.getSpectrum(spectrum_index).getDetectorIDs()
        if len(detector_ids) != 1:
            raise RuntimeError(f"Absolute calibration requires one detector ID for spectrum index {spectrum_index}")
        return int(next(iter(detector_ids)))

    @staticmethod
    def _validate_interpolation_index(index, size, property_name):
        if index < 0 or index > size - 1:
            raise RuntimeError(f"{property_name} must be in the range 0 to {size - 1}")

    def _interpolate_experiment_two_theta(self, spectrum_info, detector_spectrum_indices, detector_index):
        lower_pixel_index = int(math.floor(detector_index))
        upper_pixel_index = int(math.ceil(detector_index))
        lower_two_theta = spectrum_info.signedTwoTheta(detector_spectrum_indices[lower_pixel_index]) * self._RAD_TO_DEG
        upper_two_theta = spectrum_info.signedTwoTheta(detector_spectrum_indices[upper_pixel_index]) * self._RAD_TO_DEG
        return self._interpolate_between(detector_index, lower_pixel_index, lower_two_theta, upper_pixel_index, upper_two_theta)

    def _interpolate(self, values, index):
        lower_index = int(math.floor(index))
        upper_index = int(math.ceil(index))
        return self._interpolate_between(index, lower_index, values[lower_index], upper_index, values[upper_index])

    @staticmethod
    def _interpolate_between(index, lower_index, lower_value, upper_index, upper_value):
        if lower_index == upper_index:
            return lower_value
        fraction = (index - lower_index) / (upper_index - lower_index)
        return lower_value + fraction * (upper_value - lower_value)

    def _calibration_angle_type(self):
        return self.getPropertyValue(self._CALIBRATION_ANGLE_TYPE)

    def _absolute_angle_type(self):
        return self.getPropertyValue(self._ABSOLUTE_ANGLE_TYPE)

    def _detector_correction_type(self):
        return self.getPropertyValue(self._DETECTOR_CORRECTION_TYPE)

    def _calibration_specular_pixel_index(self):
        return self.getProperty(self._CALIBRATION_SPECULAR_PIXEL_INDEX).value

    def _experiment_specular_pixel_index(self):
        return self.getProperty(self._EXPERIMENT_SPECULAR_PIXEL_INDEX).value


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
