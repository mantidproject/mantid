# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import CreateEmptyTableWorkspace, ApplyCalibration, CloneWorkspace
from mantid.api import (AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm, MatrixWorkspaceProperty,
                        FileAction, FileProperty, PropertyMode)
from mantid.kernel import Direction
import csv
import math


class ReflectometryISISCalibration(DataProcessorAlgorithm):
    _WORKSPACE = 'InputWorkspace'
    _CALIBRATION_FILE = 'CalibrationFile'
    _OUTPUT_WORKSPACE = 'OutputWorkspace'

    def category(self):
        """Return the categories of the algorithm."""
        return 'Reflectometry\\ISIS;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryISISCalibration'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Apply calibration data to detector pixel Y locations"

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryISISLoadAndProcess']

    def PyInit(self):
        self.declareProperty(
            MatrixWorkspaceProperty(self._WORKSPACE, '', direction=Direction.Input, optional=PropertyMode.Mandatory),
            doc='An input workspace')
        self.declareProperty(FileProperty(self._CALIBRATION_FILE, "",
                                          action=FileAction.Load,
                                          direction=Direction.Input,
                                          extensions=["dat"]),
                             doc="Calibration data file containing Y locations for detector pixels.")
        self.declareProperty(
            MatrixWorkspaceProperty(self._OUTPUT_WORKSPACE, '', direction=Direction.Output),
            doc='The calibrated output workspace.')

    def PyExec(self):
        ws = self.getProperty(self._WORKSPACE).value
        spectra = range(7, 191)  # Spectra of detectors to be moved - these are the workspace indices

        compInfo = ws.componentInfo()
        detInfo = ws.detectorInfo()

        two_theta = ws.run().getProperty("Theta").timeAverageValue() * 2

        min_diff = None
        min_det_idx = None

        for i in range(0, ws.getNumberHistograms()):
            try:
                detector = ws.getDetector(i)
            except RuntimeError:
                # Exclude point detectors that don't have IDs
                continue
            detector_index = detInfo.indexOf(detector.getID())
            if detInfo.isMonitor(detector_index):
                continue
            # Get two theta for the detector in radians
            det_two_theta = detInfo.twoTheta(detector_index) * (180 / math.pi)
            diff = abs(two_theta - det_two_theta)
            if min_diff is None or diff < min_diff:
                min_diff = diff
                min_det_idx = i

        # Determine position of pixel 82 (our specular pixel) from the IDF:
        det = ws.getDetector(min_det_idx)
        y82 = det.getPos().Y()

        # create dict of spectrum and position from scan
        with open(self.getPropertyValue(self._CALIBRATION_FILE), 'r') as file:
            pixels = csv.reader(file)
            pixdict = {float(row[0].split()[0]): float(row[0].split()[1]) for row in pixels}

        # Get the specular pixel position from the calibration scan
        # This is assuming that there will be an entry in the calibration scan data for the specular pixel found in the workspace
        specpix_pos = pixdict[min_det_idx]
        detYCoord = []
        for i in spectra:
            try:
                # Find difference between pixel i and the specular pixel positions in the scan data
                # Set the pixel i Y co-ordinate by using the position difference from the scan data
                # added to the specular pixel position from the IDF
                # Not sure what the multiply 0.001 is for
                detYCoord.append(y82 + (specpix_pos - pixdict[i]) * 0.001)
            except KeyError:
                # Do nothing if there are any spectra/workspace indices in the range that are not in the calibration data
                pass

        detIDList = []
        detPosList = []
        detWidthList = []
        detHeightList = []

        # This seems to be assuming that the spectra range specified matches the indices available in the workspace
        for i in spectra:
            det = ws.getDetector(i)
            xyz = det.getPos()
            detIDList.append(det.getID())
            detPosList.append(xyz)
            index = detInfo.indexOf(det.getID())  # detectorInfo and componentInfo index
            box = compInfo.shape(index).getBoundingBox().width()
            scalings = compInfo.scaleFactor(index)
            width, height = box[0] * scalings[0], box[1] * scalings[1]
            detWidthList.append(width)
            detHeightList.append(height)

        # Create CalibrationTable - This would be done by the calibration functions
        calibTable = CreateEmptyTableWorkspace(OutputWorkspace="CalibTable")
        # Add required columns
        calibTable.addColumn(type="int", name="Detector ID")
        calibTable.addColumn(type="V3D", name="Detector Position")
        calibTable.addColumn(type="double", name="Detector Y Coordinate")
        calibTable.addColumn(type="double", name="Detector Height")
        calibTable.addColumn(type="double", name="Detector Width")

        # This seems to be assuming that detYCoord was created without any missing entries and
        # therefore the entries in the separate lists will match up
        for j in range(len(detYCoord)):
            nextRow = {'Detector ID': detIDList[j], 'Detector Position': detPosList[j],
                       'Detector Y Coordinate': detYCoord[j],
                       'Detector Width': detWidthList[j], 'Detector Height': detHeightList[j]}
            calibTable.addRow(nextRow)

        # Apply calibration to workspace
        output_ws = CloneWorkspace(InputWorkspace=ws)
        ApplyCalibration(Workspace=output_ws, CalibrationTable=calibTable)
        self.setProperty(self._OUTPUT_WORKSPACE, output_ws)
        AnalysisDataService.remove('output_ws')


AlgorithmFactory.subscribe(ReflectometryISISCalibration)
