# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from sans.common.enums import SANSInstrument

from mantid.simpleapi import *


class SANS2DTubeCalibrationTest:
    _OUTPUT_WORKSPACE_NAME = "result"
    _DATA_FILES = ["SANS2D00069117.nxs", "SANS2D00069118.nxs", "SANS2D00069119.nxs", "SANS2D00069120.nxs", "SANS2D00069116.nxs"]

    def run_calibration_algorithm(self, side_offset, encoder_at_beam_centre, is_rear_det, threshold, skip_tubes_on_error):
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


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTubeCalibrationRearDetectorTest(systemtesting.MantidSystemTest, SANS2DTubeCalibrationTest):
    _REFERENCE_FILE = "SANS2DTubeMerge_rear.nxs"

    def requiredFile(self):
        return self._DATA_FILES + [self._REFERENCE_FILE]

    def runTest(self):
        self.run_calibration_algorithm(
            side_offset=0.0, encoder_at_beam_centre=270.0, is_rear_det=True, threshold=500, skip_tubes_on_error=False
        )

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WORKSPACE_NAME, self._REFERENCE_FILE


@ISISSansSystemTest(SANSInstrument.SANS2D)
class SANS2DTubeCalibrationFrontDetectorTest(systemtesting.MantidSystemTest, SANS2DTubeCalibrationTest):
    _REFERENCE_FILE = "SANS2DTubeMerge_front.nxs"

    def requiredFile(self):
        return self._DATA_FILES + [self._REFERENCE_FILE]

    def runTest(self):
        self.run_calibration_algorithm(
            side_offset=1.1, encoder_at_beam_centre=474.2, is_rear_det=False, threshold=1000, skip_tubes_on_error=True
        )

    def validate(self):
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Axes")

        return self._OUTPUT_WORKSPACE_NAME, self._REFERENCE_FILE
