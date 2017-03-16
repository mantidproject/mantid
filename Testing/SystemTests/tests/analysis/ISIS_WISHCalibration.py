"""
System test for WISH calibration

This makes use of the tube calibration python package. This also includes
additional processing to correct "bad" peaks fits in some tubes.

Note that this is a test for the newer configuration of WISH. Previously this
was fitted with 9 Gaussian peaks and this was done on a panel by panel basis.

Modern method (tested below) uses a single run for all panels and uses only
5 Gaussian peaks for calibration.

"""
from mantid.simpleapi import (Load, ApplyCalibration)
from tube_calib_fit_params import TubeCalibFitParams
from tube_spec import TubeSpec
from ideal_tube import IdealTube
import tube

import stresstesting
import numpy as np


class WISHCalibrationTest(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["WISH30541_integrated.nxs"]

    def requiredMemoryMB(self):
        return 4000

    def cleanup(self):
        pass

    def runTest(self):
        workspace = Load("WISH30541_integrated.nxs", OutputWorkspace="30541")

        # define the positions of each of calibration bands on WISH
        tube_positions = np.array([-274.81703361, -131.75052481, 0.,
                                   131.75052481, 274.81703361])
        func_form = len(tube_positions)*[1] # 5 gaussian peaks
        margin = 15

        # start position of each of the peaks to fit
        fit_params = TubeCalibFitParams([59, 161, 258, 353, 448])
        fit_params.setAutomatic(True)

        instrument = workspace.getInstrument()
        spec = TubeSpec(workspace)
        spec.setTubeSpecByString(instrument.getFullName())

        ideal_tube = IdealTube()
        ideal_tube.setArray(tube_positions)

        # First calibrate all of the detectors
        calibration_table, peaks = \
            tube.calibrate(workspace, spec, tube_positions, func_form,
                           margin=margin, outputPeak=True,
                           fitPar=fit_params)

        ApplyCalibration(workspace, calibration_table)
        # should have # detectors to update
        self.assertEqual(calibration_table.rowCount(), 778240)

        # now correct badly aligned tubes
        corrected_calibration_table = \
            tube.correctMisalignedTubes(workspace, calibration_table, peaks, spec,
                                        ideal_tube, fit_params)

        ApplyCalibration(workspace, corrected_calibration_table)
        # should have 12288 detectors to correct after refitting
        self.assertEqual(corrected_calibration_table.rowCount(), 12288)
