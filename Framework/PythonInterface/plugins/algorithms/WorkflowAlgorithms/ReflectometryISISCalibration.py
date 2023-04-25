# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CloneWorkspace, SpecularReflectionPositionCorrect
from mantid.api import (
    AlgorithmFactory,
    AnalysisDataService,
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
        return "Corrects the positions of detector pixels by applying TwoTheta offsets provided in a calibration file"

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
            doc=f"Calibration data file containing a list of detector IDs and twoTheta offsets (in degrees)."
            f"These should be provided as two spaced-delimited columns, labelled {self._DET_ID_LABEL} and {self._THETA_LABEL}.",
        )
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WORKSPACE, "", direction=Direction.Output), doc="The calibrated output workspace."
        )

    def PyExec(self):
        try:
            calibration_data = self._parse_calibration_file(self.calibration_filepath)
        except FileNotFoundError:
            raise FileNotFoundError("Calibration file path cannot be found")

        ws = self.getProperty(self._WORKSPACE).value
        det_info = ws.detectorInfo()
        calibrated_ws = CloneWorkspace(InputWorkspace=ws)

        for det_id, theta_offset in calibration_data.items():
            new_two_theta = self._calculate_calibrated_two_theta(det_info, det_id, theta_offset)
            # Passing calibrated_ws as both input and output means all the detector moves are applied to this workspace
            SpecularReflectionPositionCorrect(
                InputWorkspace=calibrated_ws,
                TwoTheta=new_two_theta,
                DetectorID=det_id,
                MoveFixedDetectors=True,
                OutputWorkspace=calibrated_ws,
            )

        self.setProperty(self._OUTPUT_WORKSPACE, calibrated_ws)
        AnalysisDataService.remove(calibrated_ws.name())

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()

        self.calibration_filepath = self.getPropertyValue(self._CALIBRATION_FILE)
        if not self.calibration_filepath:
            issues[self._CALIBRATION_FILE] = "Calibration file path must be provided"
        return issues

    def _parse_calibration_file(self, filepath):
        # Create dictionary of detector ID and theta offset from scan
        scanned_theta_offsets = {}
        with open(filepath, "r") as file:
            labels_checked = False
            det_id_idx = 0
            theta_offset_idx = 1

            file_reader = csv.reader(file)
            for row in file_reader:
                entries = row[0].split()

                if entries[0][0] == "#":
                    # Ignore any lines that begin with a #
                    # This allows the user to add any metadata they would like
                    continue

                if len(entries) != 2:
                    raise RuntimeError(
                        f"Calibration file should contain two columns, labelled as {self._DET_ID_LABEL} and {self._THETA_LABEL}"
                    )

                if not labels_checked:
                    # The labels should be the first row in the file that doesn't begin with a #
                    valid_labels = collections.Counter([self._DET_ID_LABEL, self._THETA_LABEL])
                    first_label = entries[det_id_idx].lower()
                    second_label = entries[theta_offset_idx].lower()
                    if collections.Counter([first_label, second_label]) == valid_labels:
                        if first_label == self._THETA_LABEL:
                            # Allow the columns to be specified in any order
                            det_id_idx = 1
                            theta_offset_idx = 0
                        labels_checked = True
                        continue
                    else:
                        raise ValueError(
                            f"Incorrect column labels in calibration file - should be {self._DET_ID_LABEL} and {self._THETA_LABEL}"
                        )

                try:
                    scanned_theta_offsets[int(entries[det_id_idx])] = float(entries[theta_offset_idx])
                except ValueError:
                    raise ValueError(
                        "Invalid data in calibration file - detector ids should be integers and theta offsets should be floats"
                    )

            if len(scanned_theta_offsets) == 0:
                raise RuntimeError("Calibration file provided contains no data")
        return scanned_theta_offsets

    def _calculate_calibrated_two_theta(self, det_info, det_id, theta_offset):
        """Calculates the new twoTheta value for a detector from a given offset, in degrees"""
        try:
            det_idx = det_info.indexOf(det_id)
        except IndexError:
            raise RuntimeError(f"Detector id {det_id} from calibration file cannot be found in input workspace")

        current_two_theta = det_info.twoTheta(det_idx)
        return (current_two_theta * self._RAD_TO_DEG) + theta_offset


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
