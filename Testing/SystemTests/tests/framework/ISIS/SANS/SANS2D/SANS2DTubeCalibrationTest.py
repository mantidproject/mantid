# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.common.enums import SANSInstrument

from mantid.api import AnalysisDataService
from mantid.simpleapi import *

from collections import Counter


class SANS2DTubeCalibrationTest:
    _REFERENCE_FILE = ""
    _OUTPUT_WORKSPACE_NAME = "result"
    _DATA_FILES = ["SANS2D00069117.nxs", "SANS2D00069118.nxs", "SANS2D00069119.nxs", "SANS2D00069120.nxs", "SANS2D00069116.nxs"]
    _NUM_TUBES = 120

    def requiredFile(self):
        return self._DATA_FILES + [self._REFERENCE_FILE]

    def _run_calibration_algorithm(self, side_offset, encoder_at_beam_centre, is_rear_det, threshold, skip_tubes_on_error):
        SANSTubeCalibration(
            StripPositions=[920, 755, 590, 425, 260],
            DataFiles=self._DATA_FILES,
            HalfDetectorWidth=520.7,
            StripWidth=38.0,
            StripToTubeCentre=21.0,
            SideOffset=side_offset,
            EncoderAtBeamCentre=encoder_at_beam_centre,
            EncoderAtBeamCentreForRear260Strip=470.0,
            RearDetector=is_rear_det,
            Threshold=threshold,
            SkipTubesOnEdgeFindingError=skip_tubes_on_error,
            Margin=25,
            StartingPixel=20,
            EndingPixel=495,
            FitEdges=False,
            Timebins="5000,93000,98000",
            Background=10,
            VerticalOffset=-0.005,
            CValueThreshold=6.0,
            SaveIntegratedWorkspaces=False,
        )

    @staticmethod
    def _required_workspaces_exist():
        required_workspaces = {"original", "result", "cvalues"}
        return required_workspaces.issubset(set(AnalysisDataService.getObjectNames()))

    @staticmethod
    def _tube_diagnostic_workspaces_exist(tube_id):
        diagnostic_ws_grp = AnalysisDataService.retrieve(f"Tube_{tube_id:03}")
        num_tubes_per_group = 24
        module = int(tube_id / num_tubes_per_group) + 1
        tube_num = tube_id % num_tubes_per_group
        ws_suffix = f"{tube_id}_{module}_{tube_num}"

        expected_workspaces = [f"Fit{ws_suffix}", f"Tube{ws_suffix}", f"Data{ws_suffix}", f"Shift{ws_suffix}"]
        return Counter(diagnostic_ws_grp.getNames()) == Counter(expected_workspaces)


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTubeCalibrationRearDetectorTest(systemtesting.MantidSystemTest, SANS2DTubeCalibrationTest):
    _REFERENCE_FILE = "SANS2DTubeMerge_rear.nxs"

    def runTest(self):
        self._run_calibration_algorithm(
            side_offset=0.0, encoder_at_beam_centre=270.0, is_rear_det=True, threshold=500, skip_tubes_on_error=False
        )
        self.assertTrue(self._required_workspaces_exist())
        for tube_id in range(self._NUM_TUBES):
            self.assertTrue(self._tube_diagnostic_workspaces_exist(tube_id))

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WORKSPACE_NAME, self._REFERENCE_FILE


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTubeCalibrationFrontDetectorTest(systemtesting.MantidSystemTest, SANS2DTubeCalibrationTest):
    _REFERENCE_FILE = "SANS2DTubeMerge_front.nxs"

    def runTest(self):
        self._run_calibration_algorithm(
            side_offset=1.1, encoder_at_beam_centre=474.2, is_rear_det=False, threshold=1000, skip_tubes_on_error=True
        )
        self.assertTrue(self._required_workspaces_exist())
        for tube_id in range(self._NUM_TUBES):
            try:
                self.assertTrue(self._tube_diagnostic_workspaces_exist(tube_id))
            except KeyError as e:
                # We expect tube 103 in the test dataset to be skipped by the calibration and for there to be no diagnostic workspace
                self.assertEqual(str(e), "\"'Tube_103' does not exist.\"")

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WORKSPACE_NAME, self._REFERENCE_FILE
