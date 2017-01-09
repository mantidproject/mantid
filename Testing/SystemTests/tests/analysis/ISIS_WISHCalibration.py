"""
System test for WISH calibration

This makes use of the tube calibration python package. This also includes 
additional processing to correct "bad" peaks fits in some tubes.

Note that this is a test for the newer configuration of WISH. Previously this
was fitted with 9 Gaussian peaks and this was done on a panel by panel basis.

Modern method (tested below) uses a single run for all panels and uses only
5 Gaussian peaks for calibration.

"""
from mantid.simpleapi import *
from tube_calib_fit_params import TubeCalibFitParams
from tube_spec import TubeSpec
from ideal_tube import IdealTube
import tube

import stresstesting
import numpy as np
import os


class WISHCalibrationTest(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["WISH30541_integrated.nxs"]

    def requiredMemoryMB(self):
        return 4000

    def cleanup(self):
        pass

    def runTest(self):
        ws = Load("WISH30541_integrated.nxs", OutputWorkspace="30541")

        # define the positions of each of calibration bands on WISH
        lower_tube = np.array([-274.81703361, -131.75052481, 0., 
                               131.75052481, 274.81703361])
        upper_tube = lower_tube # assume upper and lower tubes are the same
        funcForm = len(lower_tube)*[1] # 5 gaussian peaks
        margin = 20

        # start position of each of the peaks to fit
        fitPar = TubeCalibFitParams( [59, 161, 258, 353, 448])
        fitPar.setAutomatic(True)

        instrument = ws.getInstrument()
        spec = TubeSpec(ws)
        spec.setTubeSpecByString(instrument.getFullName())

        idealTube = IdealTube()
        idealTube.setArray(lower_tube)

        # First calibrate all of the detectors
        calibrationTable, peaks = tube.calibrate(ws, spec, lower_tube,
            funcForm, margin=15, outputPeak=True, fitPar=fitPar)

        ApplyCalibration(ws, calibrationTable)
        # should have # detectors to update
        self.assertEqual(calibrationTable.rowCount(), 778240)

        # now correct badly aligned tubes
        corrected_calibration_table = tube.correctMisalignedTubes(ws, 
                            calibrationTable, peaks, spec, idealTube, fitPar)
        ApplyCalibration(ws, corrected_calibration_table)
        # should have 12288 detectors to correct after refitting
        self.assertEqual(corrected_calibration_table.rowCount(), 12288)


