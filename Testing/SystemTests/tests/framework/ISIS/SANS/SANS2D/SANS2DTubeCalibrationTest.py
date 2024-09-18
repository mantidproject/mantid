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
from mantid.simpleapi import SANSTubeCalibration

from collections import Counter


class SANS2DTubeCalibrationTest:
    _OUTPUT_WS_NAME = "result"
    _CVALUES_WS_PREFIX = "cvalues_"
    _DATA_FILES = ["SANS2D00069117.nxs", "SANS2D00069118.nxs", "SANS2D00069119.nxs", "SANS2D00069120.nxs", "SANS2D00069116.nxs"]
    _NUM_TUBES = 120

    def _run_calibration_algorithm(self, encoder_at_beam_centre, is_rear_det, threshold, skip_tubes_on_error):
        SANSTubeCalibration(
            StripPositions=[920, 755, 590, 425, 260],
            DataFiles=self._DATA_FILES,
            StripWidth=38.0,
            StripToTubeCentre=21.0,
            EncoderAtBeamCentre=encoder_at_beam_centre,
            EncoderAtBeamCentreForRear260Strip=470.0,
            RearDetector=is_rear_det,
            Threshold=threshold,
            SkipTubesOnError=skip_tubes_on_error,
            Margin=25,
            StartingPixel=20,
            EndingPixel=495,
            FitEdges=False,
            Timebins="5000,93000,98000",
            Background=10,
            VerticalOffset=-0.005,
            CValueThreshold=6.0,
        )

    @staticmethod
    def _merged_workspace_exists():
        """
        Check that the workspace created after merging the scaled workspaces exists in the ADS.
        Outputting this workspace can be useful for diagnostic purposes.
        """
        return "original" in AnalysisDataService.getObjectNames()

    @staticmethod
    def _tube_diagnostic_workspaces_exist(tube_id):
        """
        Check that the diagnostic workspaces for each tube have been created in the ADS.
        These give useful visibility of the calibration of each tube and can help with diagnosing any issues.
        """
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
    _CVALUES_REF_FILE = "SANS2DTubeCalibration_cvalues_rear.nxs"
    _DETECTOR_NAME = "rear"

    def requiredFile(self):
        return self._DATA_FILES + [self._REFERENCE_FILE, self._CVALUES_REF_FILE]

    def runTest(self):
        self._run_calibration_algorithm(encoder_at_beam_centre=270.0, is_rear_det=True, threshold=500, skip_tubes_on_error=False)
        self.assertTrue(self._merged_workspace_exists())
        for tube_id in range(self._NUM_TUBES):
            self.assertTrue(self._tube_diagnostic_workspaces_exist(tube_id))

    def validate(self):
        self.tolerance = 1e-7
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WS_NAME, self._REFERENCE_FILE, f"{self._CVALUES_WS_PREFIX}{self._DETECTOR_NAME}", self._CVALUES_REF_FILE


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTubeCalibrationFrontDetectorTest(systemtesting.MantidSystemTest, SANS2DTubeCalibrationTest):
    _REFERENCE_FILE = "SANS2DTubeMerge_front.nxs"
    _CVALUES_REF_FILE = "SANS2DTubeCalibration_cvalues_front.nxs"
    _DETECTOR_NAME = "front"

    def requiredFile(self):
        return self._DATA_FILES + [self._REFERENCE_FILE, self._CVALUES_REF_FILE]

    def runTest(self):
        self._run_calibration_algorithm(encoder_at_beam_centre=474.2, is_rear_det=False, threshold=1000, skip_tubes_on_error=True)
        self.assertTrue(self._merged_workspace_exists())
        for tube_id in range(self._NUM_TUBES):
            try:
                self.assertTrue(self._tube_diagnostic_workspaces_exist(tube_id))
            except KeyError as e:
                # We expect tube 103 in the test dataset to be skipped by the calibration and for there to be no diagnostic workspace
                self.assertEqual(str(e), "\"'Tube_103' does not exist.\"")

    def validate(self):
        self.tolerance = 1e-7
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WS_NAME, self._REFERENCE_FILE, f"{self._CVALUES_WS_PREFIX}{self._DETECTOR_NAME}", self._CVALUES_REF_FILE
