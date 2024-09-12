# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
"""
Verifies that a calibration file can be loaded once and reused to apply, using CopyInstrumentParameters, the same calibration
in successive reductions.
"""

import systemtesting


class ReuseExistingCalibration(systemtesting.MantidSystemTest):
    det_pos_first_run = None
    det_pos_second_run = None

    def requiredFiles(self):
        return ["HRP39180.RAW", "HRP38094Calib.nxs"]

    def runTest(self):
        import mantid.simpleapi as ms

        def do_reduction(calibration):
            # load data
            data = ms.Load("HRP39180.RAW")
            # copy parameters from calibration to data
            ms.CopyInstrumentParameters(calibration, data)
            # Now move component on data workspace using a relative move, where that component was a detector in the calibrated workspace
            ms.MoveInstrumentComponent(data, DetectorID=1100, X=0.0, Y=0.0, Z=5.0, RelativePosition=True)
            return data.getDetector(0).getPos()

        ####

        # load calibration
        calibration = ms.Load("HRP38094Calib")
        self.det_pos_first_run = do_reduction(calibration)
        # again not reloading of calibration
        self.det_pos_second_run = do_reduction(calibration)

    def validate(self):
        if self.det_pos_second_run == self.det_pos_first_run:
            return True
        else:
            print("Error: Detector position is not the same after the second reduction!")
            return False
