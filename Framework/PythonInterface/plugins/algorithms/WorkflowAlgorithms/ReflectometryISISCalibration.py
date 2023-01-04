# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateEmptyTableWorkspace, ApplyCalibration, CloneWorkspace
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
import math


class ReflectometryISISCalibration(DataProcessorAlgorithm):
    _WORKSPACE = "InputWorkspace"
    _CALIBRATION_FILE = "CalibrationFile"
    _OUTPUT_WORKSPACE = "OutputWorkspace"

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISCalibration"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Apply calibration data to detector pixel Y locations"

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["ReflectometryISISLoadAndProcess"]

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(self._WORKSPACE, "", direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc="An input workspace or workspace group",
        )
        self.declareProperty(
            FileProperty(self._CALIBRATION_FILE, "", action=FileAction.OptionalLoad, direction=Direction.Input, extensions=["dat"]),
            doc="Calibration data file containing Y locations for detector pixels in mm.",
        )
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WORKSPACE, "", direction=Direction.Output), doc="The calibrated output workspace."
        )

    def PyExec(self):
        ws = self.getProperty(self._WORKSPACE).value
        det_info = ws.detectorInfo()
        comp_info = ws.componentInfo()

        specular_pixel_idx = self._find_specular_pixel_index(ws, det_info)
        calib_table = self._create_calibration_table_from_scan(ws, det_info, comp_info, specular_pixel_idx)

        # Apply calibration to workspace
        output_ws = CloneWorkspace(InputWorkspace=ws)
        ApplyCalibration(Workspace=output_ws, CalibrationTable=calib_table)
        self.setProperty(self._OUTPUT_WORKSPACE, output_ws)
        AnalysisDataService.remove("output_ws")

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()

        calibration_file = self.getPropertyValue(self._CALIBRATION_FILE)
        if not calibration_file:
            issues[self._CALIBRATION_FILE] = "Calibration file path must be provided"

        return issues

    def _find_specular_pixel_index(self, ws, det_info):
        # Determine position of the specular pixel from the workspace:
        ws_two_theta = ws.run().getProperty("Theta").timeAverageValue() * 2

        min_theta_diff = None
        specular_pixel_idx = None

        for i in range(0, ws.getNumberHistograms()):
            try:
                detector = ws.getDetector(i)
            except RuntimeError:
                # Exclude point detectors that don't have IDs
                continue

            detector_index = det_info.indexOf(detector.getID())

            if det_info.isMonitor(detector_index):
                continue

            # Get two theta for the detector in radians
            det_two_theta = det_info.twoTheta(detector_index) * (180 / math.pi)

            # The specular pixel is the one that has a two theta value closest to the workspace two theta value
            theta_diff = abs(ws_two_theta - det_two_theta)
            if min_theta_diff is None or theta_diff < min_theta_diff:
                min_theta_diff = theta_diff
                specular_pixel_idx = i

        if not specular_pixel_idx:
            raise RuntimeError("Could not find specular pixel index from the workspace")

        return specular_pixel_idx

    def _get_pixel_positions_from_file(self):
        # Create dictionary of pixel spectrum number and position from scan
        scanned_pixel_positions = {}
        with open(self.getPropertyValue(self._CALIBRATION_FILE), "r") as file:
            pixels = csv.reader(file)
            for pixel in pixels:
                pixel_info = pixel[0].split()
                scanned_pixel_positions[int(pixel_info[0])] = float(pixel_info[1])
        return scanned_pixel_positions

    def _create_calibration_table_from_scan(self, ws, det_info, comp_info, specular_pixel_idx):
        scanned_pixel_positions = self._get_pixel_positions_from_file()

        # Get the current specular pixel Y position and the position of the specular pixel from the calibration scan
        specpixel_y = ws.getDetector(specular_pixel_idx).getPos().Y()

        scanned_specpixel_y = scanned_pixel_positions.get(specular_pixel_idx)
        if not scanned_specpixel_y:
            raise RuntimeError(f"Missing calibration data for specular pixel with workspace index {specular_pixel_idx}")

        table = CreateEmptyTableWorkspace(OutputWorkspace=f"Calib_Table_{str(ws.getRunNumber())}")
        table.addColumn(type="int", name="Detector ID")
        table.addColumn(type="V3D", name="Detector Position")
        table.addColumn(type="double", name="Detector Y Coordinate")
        table.addColumn(type="double", name="Detector Height")
        table.addColumn(type="double", name="Detector Width")

        # Populate the table with the calibrated data
        for ws_id, scanned_pixel_pos in scanned_pixel_positions.items():
            try:
                det = ws.getDetector(ws_id)
            except RuntimeError:
                raise RuntimeError(f"Could not find detector to calibrate at workspace id {ws_id}")

            det_idx = det_info.indexOf(det.getID())  # detectorInfo and componentInfo index
            box = comp_info.shape(det_idx).getBoundingBox().width()
            scalings = comp_info.scaleFactor(det_idx)

            next_row = {
                "Detector ID": det.getID(),
                "Detector Position": det.getPos(),
                "Detector Y Coordinate": self._calculate_calibrated_y_pos(specpixel_y, scanned_specpixel_y, scanned_pixel_pos),
                "Detector Width": box[0] * scalings[0],
                "Detector Height": box[1] * scalings[1],
            }
            table.addRow(next_row)
        return table

    def _calculate_calibrated_y_pos(self, specpixel_y, scanned_specpixel_y, scanned_pixel_pos):
        # First find the difference between the specular pixel position and the pixel position in the scan data,
        # and convert to metres.
        # The calibrated Y co-ordinate is then found by adding this difference to the specular pixel position from
        # the IDF.
        return specpixel_y + (scanned_specpixel_y - scanned_pixel_pos) * 0.001


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
