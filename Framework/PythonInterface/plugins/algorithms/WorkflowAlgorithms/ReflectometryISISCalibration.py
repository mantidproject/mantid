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
from mantid.kernel import (
    Direction,
    EnabledWhenProperty,
    FloatBoundedValidator,
    PropertyCriterion,
    StringListValidator,
)
import csv
import collections
import math


class ReflectometryISISCalibration(DataProcessorAlgorithm):
    _WORKSPACE = "InputWorkspace"
    _CALIBRATION_FILE = "CalibrationFile"
    _INSTRUMENT_WORKFLOW = "InstrumentWorkflow"
    _SPECULAR_PIXEL_INDEX = "SpecularPixelIndex"
    _EXPERIMENT_ANGLE = "ExperimentAngle"
    _DETECTOR_CORRECTION_TYPE = "DetectorCorrectionType"
    _OUTPUT_WORKSPACE = "OutputWorkspace"

    _COMMENT_PREFIX = "#"
    _NUM_COLUMNS_REQUIRED = 2
    _DET_ID_LABEL = "detectorid"
    _DET_INDEX_LABEL = "detectorindex"
    _THETA_LABEL = "theta_offset"
    _ANGLE_LABEL = "angle"
    _OFFSET = "Offset"
    _ABSOLUTE = "Absolute"
    _DEFAULT_WORKFLOW = "Default"
    _POLREF_WORKFLOW = "POLREF"
    _VERTICAL_SHIFT = "VerticalShift"
    _ROTATE_AROUND_SAMPLE = "RotateAroundSample"
    _RAD_TO_DEG = 180.0 / math.pi

    class CalibrationData:
        def __init__(self, data, require_contiguous_keys=False):
            self._data = dict(data)
            if require_contiguous_keys:
                self.validate_contiguous_keys()

        @property
        def data(self):
            return self._data

        def items(self):
            return self._data.items()

        def keys(self):
            return sorted(self._data)

        def first_key(self):
            return self.keys()[0]

        def last_key(self):
            return self.keys()[-1]

        def in_range(self, key):
            return self.first_key() <= key <= self.last_key()

        def validate_contiguous_keys(self):
            expected_keys = list(range(self.first_key(), self.last_key() + 1))
            if self.keys() != expected_keys:
                raise RuntimeError("Absolute calibration detector indexes must be contiguous")

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISCalibration"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Corrects detector pixel positions using a calibration file and a selected instrument workflow."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["ReflectometryISISLoadAndProcess", "SpecularReflectionPositionCorrect"]

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(self._WORKSPACE, "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="An input workspace or workspace group.",
        )
        self.declareProperty(
            FileProperty(
                self._CALIBRATION_FILE,
                "",
                action=FileAction.OptionalLoad,
                direction=Direction.Input,
                extensions=["dat"],
            ),
            doc="Calibration data file containing detector IDs and offsets for the default workflow, "
            "or detector indexes and absolute theta values for the POLREF workflow.",
        )
        self.declareProperty(
            self._INSTRUMENT_WORKFLOW,
            self._DEFAULT_WORKFLOW,
            StringListValidator([self._DEFAULT_WORKFLOW, self._POLREF_WORKFLOW]),
            "The instrument workflow that defines how the calibration file should be interpreted.",
        )
        non_negative_double = FloatBoundedValidator()
        non_negative_double.setLower(0.0)
        self.declareProperty(
            self._SPECULAR_PIXEL_INDEX,
            0.0,
            non_negative_double,
            "The detector index of the specular pixel in the subject run.",
        )
        self.declareProperty(
            self._EXPERIMENT_ANGLE,
            0.0,
            "The experiment theta angle in degrees. Required for the POLREF workflow.",
        )
        polref_workflow_enabled = EnabledWhenProperty(self._INSTRUMENT_WORKFLOW, PropertyCriterion.IsEqualTo, self._POLREF_WORKFLOW)
        self.setPropertySettings(self._SPECULAR_PIXEL_INDEX, polref_workflow_enabled)
        self.setPropertySettings(self._EXPERIMENT_ANGLE, polref_workflow_enabled)
        self.declareProperty(
            self._DETECTOR_CORRECTION_TYPE,
            self._VERTICAL_SHIFT,
            StringListValidator([self._VERTICAL_SHIFT, self._ROTATE_AROUND_SAMPLE]),
            "Whether detectors should be shifted vertically or rotated around the sample position.",
        )
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WORKSPACE, "", direction=Direction.Output),
            doc="The calibrated output workspace.",
        )

    def PyExec(self):
        # Set the expected order of the columns in the calibration file
        self._det_id_col_idx = 0
        self._angle_col_idx = 1
        self._calibration_angle_type = self._ABSOLUTE if self._is_polref_workflow() else self._OFFSET

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
        if self._is_polref_workflow() and self.getProperty(self._EXPERIMENT_ANGLE).isDefault:
            issues[self._EXPERIMENT_ANGLE] = "ExperimentAngle must be provided for the POLREF workflow"
        return issues

    def _parse_calibration_file(self, filepath):
        """Parse calibration data from the calibration file."""
        if self._calibration_angle_type == self._ABSOLUTE:
            return self.CalibrationData(self._parse_absolute_calibration_file(filepath), require_contiguous_keys=True)
        else:
            return self.CalibrationData(self._parse_offset_calibration_file(filepath))

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
        """Create a dictionary of detector indexes and absolute angles from the calibration file."""
        calibration_angles = {}
        with open(filepath, "r") as file:
            labels_checked = False

            for entries in self._file_entries(file):
                if len(entries) != self._NUM_COLUMNS_REQUIRED:
                    raise RuntimeError(
                        "Calibration file should contain two space de-limited columns, "
                        f"labelled as {self._DET_INDEX_LABEL} and {self._ANGLE_LABEL}"
                    )

                if not labels_checked:
                    self._check_absolute_file_column_labels(entries)
                    labels_checked = True
                    continue

                try:
                    detector_index = float(entries[self._det_id_col_idx])
                    if not detector_index.is_integer():
                        raise ValueError
                    calibration_angles[int(detector_index)] = float(entries[self._angle_col_idx])
                except ValueError:
                    error_message = (
                        f"Invalid data in calibration file entry {entries} - detector indexes should be integers "
                        "and angles should be numeric"
                    )
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
        valid_labels = collections.Counter([self._DET_INDEX_LABEL, self._ANGLE_LABEL])

        first_label = row_entries[self._det_id_col_idx].lower()
        second_label = row_entries[self._angle_col_idx].lower()

        if collections.Counter([first_label, second_label]) == valid_labels:
            if first_label == self._ANGLE_LABEL:
                # Allow the columns to be specified in any order
                self._angle_col_idx = 0
                self._det_id_col_idx = 1
        else:
            raise ValueError(f"Incorrect column labels in calibration file - should be {self._DET_INDEX_LABEL} and {self._ANGLE_LABEL}")

    def _clone_workspace(self, ws):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", InputWorkspace=ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _correct_detector_positions(self, ws, calibration_data):
        calibration_ws = self._clone_workspace(ws)
        det_info = calibration_ws.detectorInfo()

        if self._calibration_angle_type == self._ABSOLUTE:
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
        """Convert detector-indexed absolute angles to detector ID keyed twoTheta offsets."""
        detector_spectrum_indices = self._detector_spectrum_indices(ws)
        self._validate_absolute_calibration_inputs(detector_spectrum_indices, absolute_calibration_angles)

        calibration_specular_two_theta = 2.0 * self._interpolate_calibration_angle(
            absolute_calibration_angles.data, self._specular_pixel_index()
        )
        experiment_specular_two_theta = 2.0 * self._experiment_angle()

        offsets = {}
        for detector_index, spectrum_index in enumerate(detector_spectrum_indices):
            if not absolute_calibration_angles.in_range(detector_index):
                continue

            calibration_two_theta = 2.0 * self._interpolate_calibration_angle(absolute_calibration_angles.data, detector_index)
            two_theta_relative_to_calibration_specular = calibration_two_theta - calibration_specular_two_theta
            # POLREF calibration-map angle decreases with detector index, while workspace signed two theta increases.
            # This inverts the calibration-map relative offset before anchoring it at the experiment specular two theta.
            experiment_two_theta = experiment_specular_two_theta - two_theta_relative_to_calibration_specular
            workspace_two_theta = ws.spectrumInfo().signedTwoTheta(spectrum_index) * self._RAD_TO_DEG
            offsets[self._single_detector_id(ws, spectrum_index)] = experiment_two_theta - workspace_two_theta
        return self.CalibrationData(offsets)

    def _validate_absolute_calibration_inputs(self, detector_spectrum_indices, absolute_calibration_angles):
        if len(detector_spectrum_indices) == 0:
            raise RuntimeError("Absolute calibration requires at least one non-monitor detector in the input workspace")
        last_experiment_detector_index = len(detector_spectrum_indices) - 1
        if absolute_calibration_angles.first_key() > last_experiment_detector_index or absolute_calibration_angles.last_key() < 0:
            raise RuntimeError("Absolute calibration file detector indexes do not overlap the input workspace detector indexes")

        self._validate_interpolation_index(
            self._specular_pixel_index(),
            absolute_calibration_angles.first_key(),
            absolute_calibration_angles.last_key(),
            self._SPECULAR_PIXEL_INDEX,
        )
        if self._specular_pixel_index() > last_experiment_detector_index:
            raise RuntimeError(f"SpecularPixelIndex must be in the range 0 to {last_experiment_detector_index}")

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
    def _validate_interpolation_index(index, lower_bound, upper_bound, property_name):
        if index < lower_bound or index > upper_bound:
            raise RuntimeError(f"{property_name} must be in the range {lower_bound} to {upper_bound}")

    def _interpolate_calibration_angle(self, values_by_detector_index, detector_index):
        if detector_index in values_by_detector_index:
            return values_by_detector_index[detector_index]

        lower_detector_index = int(math.floor(detector_index))
        upper_detector_index = int(math.ceil(detector_index))
        return self._interpolate_between(
            detector_index,
            lower_detector_index,
            values_by_detector_index[lower_detector_index],
            upper_detector_index,
            values_by_detector_index[upper_detector_index],
        )

    @staticmethod
    def _interpolate_between(index, lower_index, lower_value, upper_index, upper_value):
        if lower_index == upper_index:
            return lower_value
        fraction = (index - lower_index) / (upper_index - lower_index)
        return lower_value + fraction * (upper_value - lower_value)

    def _is_polref_workflow(self):
        return self.getPropertyValue(self._INSTRUMENT_WORKFLOW) == self._POLREF_WORKFLOW

    def _detector_correction_type(self):
        if self._is_polref_workflow() and self.getProperty(self._DETECTOR_CORRECTION_TYPE).isDefault:
            return self._ROTATE_AROUND_SAMPLE
        return self.getPropertyValue(self._DETECTOR_CORRECTION_TYPE)

    def _specular_pixel_index(self):
        return self.getProperty(self._SPECULAR_PIXEL_INDEX).value

    def _experiment_angle(self):
        return self.getProperty(self._EXPERIMENT_ANGLE).value


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
