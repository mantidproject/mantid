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
    Property,
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
    _SPECULAR_PIXEL_SPECTRUM_NO = "SpecularPixelSpectrumNo"
    _EXPERIMENT_ANGLE = "ExperimentAngle"
    _OUTPUT_WORKSPACE = "OutputWorkspace"

    _POSITION_CORRECTION_TYPE = "DetectorCorrectionType"
    _COMMENT_PREFIX = "#"
    _NUM_COLUMNS_REQUIRED = 2
    _DET_ID_LABEL = "detectorid"
    _SPECTRUM_NUMBER_LABEL = "spectrumnumber"
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
        def __init__(self, data):
            self._data = dict(data)

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

    class WorkflowOptions:
        def __init__(
            self,
            calibration_angle_type,
            detector_correction_type,
            enabled_properties=None,
        ):
            self.calibration_angle_type = calibration_angle_type
            self.detector_correction_type = detector_correction_type
            self.enabled_properties = set(enabled_properties or [])

        def enable_property(self, property_name):
            return property_name in self.enabled_properties

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
            "or spectrum numbers and absolute theta values for the POLREF workflow.",
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
            self._SPECULAR_PIXEL_SPECTRUM_NO,
            Property.EMPTY_DBL,
            non_negative_double,
            "The spectrum number of the specular pixel in the subject run. Required for the POLREF workflow.",
        )
        self.declareProperty(
            self._EXPERIMENT_ANGLE,
            Property.EMPTY_DBL,
            "The experiment theta angle in degrees. Required for the POLREF workflow.",
        )
        self._enable_property_when_workflow_option_enables(self._SPECULAR_PIXEL_SPECTRUM_NO)
        self._enable_property_when_workflow_option_enables(self._EXPERIMENT_ANGLE)
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WORKSPACE, "", direction=Direction.Output),
            doc="The calibrated output workspace.",
        )

    def PyExec(self):
        self._workflow_options = self._selected_workflow_options()

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

        workflow_options = self._selected_workflow_options()
        self._calibration_filepath = self.getPropertyValue(self._CALIBRATION_FILE)
        if not self._calibration_filepath:
            issues[self._CALIBRATION_FILE] = "Calibration file path must be provided"
        else:
            calibration_file_issue = self._validate_calibration_file_header(self._calibration_filepath, workflow_options)
            if calibration_file_issue:
                issues[self._CALIBRATION_FILE] = calibration_file_issue
        for property_name in [self._SPECULAR_PIXEL_SPECTRUM_NO, self._EXPERIMENT_ANGLE]:
            if workflow_options.enable_property(property_name) and self.getProperty(property_name).isDefault:
                issues[property_name] = f"{property_name} must be provided for the POLREF workflow"
        return issues

    def _validate_calibration_file_header(self, filepath, workflow_options):
        """Return an issue found in the first non-comment row of a calibration file."""
        try:
            with open(filepath, "r") as file:
                header_entries = next(self._file_entries(file), None)
        except FileNotFoundError:
            return "Calibration file path cannot be found"

        if header_entries is None:
            return "Calibration file provided contains no data"

        identifier_label, angle_label = self._calibration_file_column_labels(workflow_options)
        try:
            self._det_id_col_idx, self._angle_col_idx = self._validate_calibration_file_header_and_get_column_indices(
                header_entries, identifier_label, angle_label
            )
        except (RuntimeError, ValueError) as error:
            return str(error)
        return None

    def _parse_calibration_file(self, filepath):
        """Parse calibration data from the calibration file."""
        if self._workflow_options.calibration_angle_type == self._ABSOLUTE:
            return self.CalibrationData(self._parse_absolute_calibration_file(filepath))
        else:
            return self.CalibrationData(self._parse_offset_calibration_file(filepath))

    def _parse_offset_calibration_file(self, filepath):
        """Create a dictionary of detector IDs and theta offsets from the calibration file."""
        scanned_theta_offsets = {}
        with open(filepath, "r") as file:
            file_entries = self._file_entries(file)
            next(file_entries, None)
            for entries in file_entries:
                self._check_calibration_file_column_count(entries, self._DET_ID_LABEL, self._THETA_LABEL)

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
        """Create a dictionary of spectrum numbers and absolute angles from the calibration file."""
        calibration_angles = {}
        with open(filepath, "r") as file:
            file_entries = self._file_entries(file)
            next(file_entries, None)
            for entries in file_entries:
                self._check_calibration_file_column_count(entries, self._SPECTRUM_NUMBER_LABEL, self._ANGLE_LABEL)

                try:
                    spectrum_number = float(entries[self._det_id_col_idx])
                    if not spectrum_number.is_integer():
                        raise ValueError
                    calibration_angles[int(spectrum_number)] = float(entries[self._angle_col_idx])
                except ValueError:
                    error_message = (
                        f"Invalid data in calibration file entry {entries} - spectrum numbers should be integers "
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
            if not entries:
                # Ignore whitespace-only lines
                continue

            if entries[0][0] == self._COMMENT_PREFIX:
                # Ignore any lines that begin with a #
                # This allows the user to add any metadata they would like
                continue

            yield entries

    def _calibration_file_column_labels(self, workflow_options):
        if workflow_options.calibration_angle_type == self._ABSOLUTE:
            return self._SPECTRUM_NUMBER_LABEL, self._ANGLE_LABEL
        return self._DET_ID_LABEL, self._THETA_LABEL

    def _check_calibration_file_column_count(self, row_entries, identifier_label, angle_label):
        if len(row_entries) != self._NUM_COLUMNS_REQUIRED:
            raise RuntimeError(
                f"Calibration file should contain two space de-limited columns, labelled as {identifier_label} and {angle_label}"
            )

    def _validate_calibration_file_header_and_get_column_indices(self, row_entries, identifier_label, angle_label):
        """Validate calibration file labels and return their column indices."""
        self._check_calibration_file_column_count(row_entries, identifier_label, angle_label)
        labels = [entry.lower() for entry in row_entries]
        valid_labels = collections.Counter([identifier_label, angle_label])
        if collections.Counter(labels) != valid_labels:
            raise ValueError(f"Incorrect column labels in calibration file - should be {identifier_label} and {angle_label}")
        return labels.index(identifier_label), labels.index(angle_label)

    def _clone_workspace(self, ws):
        clone_alg = self.createChildAlgorithm("CloneWorkspace", InputWorkspace=ws)
        clone_alg.execute()
        return clone_alg.getProperty("OutputWorkspace").value

    def _correct_detector_positions(self, ws, calibration_data):
        calibration_ws = self._clone_workspace(ws)
        det_info = calibration_ws.detectorInfo()

        if self._workflow_options.calibration_angle_type == self._ABSOLUTE:
            calibration_data = self._convert_absolute_angles_to_offsets(calibration_ws, calibration_data)

        correction_alg = self.createChildAlgorithm("SpecularReflectionPositionCorrect")
        # Turn off history recording to prevent the history becoming very large, as this causes reloading of a
        # calibrated workspace to be very slow
        correction_alg.enableHistoryRecordingForChild(False)
        # Passing the same workspace as both input and output means all the detector moves are applied to it
        correction_alg.setProperty("InputWorkspace", calibration_ws)
        correction_alg.setProperty("MoveFixedDetectors", True)
        correction_alg.setProperty("OutputWorkspace", calibration_ws)
        correction_alg.setProperty(self._POSITION_CORRECTION_TYPE, self._workflow_options.detector_correction_type)

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
        """Convert spectrum-number-indexed absolute angles to detector ID keyed twoTheta offsets."""
        detector_spectrum_indices = self._detector_spectrum_indices(ws)
        self._validate_absolute_calibration_inputs(ws, detector_spectrum_indices, absolute_calibration_angles)

        calibration_specular_two_theta = 2.0 * self._interpolate_calibration_angle(
            absolute_calibration_angles.data, self._specular_pixel_spectrum_number()
        )
        experiment_specular_two_theta = 2.0 * self._experiment_angle()

        offsets = {}
        for spectrum_index in detector_spectrum_indices:
            spectrum_number = ws.getSpectrum(spectrum_index).getSpectrumNo()
            if not absolute_calibration_angles.in_range(spectrum_number):
                continue

            calibration_two_theta = 2.0 * self._interpolate_calibration_angle(absolute_calibration_angles.data, spectrum_number)
            two_theta_relative_to_calibration_specular = calibration_two_theta - calibration_specular_two_theta
            # POLREF calibration-map angle decreases with spectrum number, while workspace signed two theta increases.
            # This inverts the calibration-map relative offset before anchoring it at the experiment specular two theta.
            experiment_two_theta = experiment_specular_two_theta - two_theta_relative_to_calibration_specular
            workspace_two_theta = ws.spectrumInfo().signedTwoTheta(spectrum_index) * self._RAD_TO_DEG
            offsets[self._single_detector_id(ws, spectrum_index)] = experiment_two_theta - workspace_two_theta
        return self.CalibrationData(offsets)

    def _validate_absolute_calibration_inputs(self, ws, detector_spectrum_indices, absolute_calibration_angles):
        if len(detector_spectrum_indices) == 0:
            raise RuntimeError("Absolute calibration requires at least one non-monitor detector in the input workspace")
        spectrum_numbers = self._spectrum_numbers(ws, detector_spectrum_indices)
        first_spectrum_number = min(spectrum_numbers)
        last_spectrum_number = max(spectrum_numbers)
        if absolute_calibration_angles.first_key() > last_spectrum_number or absolute_calibration_angles.last_key() < first_spectrum_number:
            raise RuntimeError("Absolute calibration file spectrum numbers do not overlap the input workspace spectrum numbers")

        self._validate_interpolation_index(
            self._specular_pixel_spectrum_number(),
            absolute_calibration_angles.first_key(),
            absolute_calibration_angles.last_key(),
            self._SPECULAR_PIXEL_SPECTRUM_NO,
        )
        self._validate_interpolation_index(
            self._specular_pixel_spectrum_number(),
            first_spectrum_number,
            last_spectrum_number,
            self._SPECULAR_PIXEL_SPECTRUM_NO,
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
    def _spectrum_numbers(ws, spectrum_indices):
        return [ws.getSpectrum(spectrum_index).getSpectrumNo() for spectrum_index in spectrum_indices]

    @staticmethod
    def _validate_interpolation_index(index, lower_bound, upper_bound, property_name):
        if index < lower_bound or index > upper_bound:
            raise RuntimeError(f"{property_name} must be in the range {lower_bound} to {upper_bound}")

    def _interpolate_calibration_angle(self, values_by_spectrum_number, spectrum_number):
        if spectrum_number in values_by_spectrum_number:
            return values_by_spectrum_number[spectrum_number]

        spectrum_numbers = sorted(values_by_spectrum_number)
        lower_spectrum_number = max(index for index in spectrum_numbers if index < spectrum_number)
        upper_spectrum_number = min(index for index in spectrum_numbers if index > spectrum_number)
        return self._interpolate_between(
            spectrum_number,
            lower_spectrum_number,
            values_by_spectrum_number[lower_spectrum_number],
            upper_spectrum_number,
            values_by_spectrum_number[upper_spectrum_number],
        )

    @staticmethod
    def _interpolate_between(index, lower_index, lower_value, upper_index, upper_value):
        if lower_index == upper_index:
            return lower_value
        fraction = (index - lower_index) / (upper_index - lower_index)
        return lower_value + fraction * (upper_value - lower_value)

    def _selected_workflow_options(self):
        return self._workflow_options_by_name()[self.getPropertyValue(self._INSTRUMENT_WORKFLOW)]

    def _workflow_options_by_name(self):
        return {
            self._DEFAULT_WORKFLOW: self.WorkflowOptions(
                calibration_angle_type=self._OFFSET,
                detector_correction_type=self._VERTICAL_SHIFT,
            ),
            self._POLREF_WORKFLOW: self.WorkflowOptions(
                calibration_angle_type=self._ABSOLUTE,
                detector_correction_type=self._ROTATE_AROUND_SAMPLE,
                enabled_properties={self._SPECULAR_PIXEL_SPECTRUM_NO, self._EXPERIMENT_ANGLE},
            ),
        }

    def _enable_property_when_workflow_option_enables(self, property_name):
        workflow_names = [
            workflow_name
            for workflow_name, workflow_options in self._workflow_options_by_name().items()
            if workflow_options.enable_property(property_name)
        ]
        if len(workflow_names) != 1:
            raise RuntimeError(f"Expected one workflow to enable {property_name}; found {workflow_names}")
        self.setPropertySettings(
            property_name,
            EnabledWhenProperty(self._INSTRUMENT_WORKFLOW, PropertyCriterion.IsEqualTo, workflow_names[0]),
        )

    def _specular_pixel_spectrum_number(self):
        return self.getProperty(self._SPECULAR_PIXEL_SPECTRUM_NO).value

    def _experiment_angle(self):
        return self.getProperty(self._EXPERIMENT_ANGLE).value


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
