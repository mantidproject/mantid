# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import filecmp
import numpy as np
import os
import systemtesting
import tempfile

import mantid.simpleapi as mantid
import mantid.kernel as kernel

from tube_calib_fit_params import TubeCalibFitParams
from tube_calib import getCalibratedPixelPositions, getPoints
from tube_spec import TubeSpec
from ideal_tube import IdealTube
import tube


class WishCalibration(systemtesting.MantidSystemTest):
    """
    Runs the WISH calibration script and checks the result produced is sane
    """

    def __init__(self):
        super(WishCalibration, self).__init__()
        self.calibration_table = None
        self.correction_table = None

        self.calibration_ref_name = "WishCalibrate_correction_table.csv"
        self.correction_ref_name = "WishCalibrate_calibration_table.csv"

        self.calibration_out_path = tempfile.NamedTemporaryFile().name
        self.correction_out_path = tempfile.NamedTemporaryFile().name

    def skipTests(self):
        return True

    def cleanup(self):
        mantid.mtd.clear()
        try:
            os.remove(self.calibration_out_path)
            os.remove(self.correction_out_path)
        except OSError:
            print("Failed to remove an temp output file in WishCalibration")

    def requiredFiles(self):
        return [self.calibration_ref_name, self.correction_ref_name]

    def validate(self):
        calibration_ref_path = mantid.FileFinder.getFullPath(self.calibration_ref_name)
        correction_ref_path = mantid.FileFinder.getFullPath(self.correction_ref_name)

        cal_result = filecmp.cmp(calibration_ref_path, self.calibration_out_path, False)
        cor_result = filecmp.cmp(correction_ref_path, self.correction_out_path, False)

        if not cal_result:
            print("Calibration did not match in WishCalibrate")

        if not cor_result:
            print("Correction did not match in WishCalibrate")

        return cal_result and cor_result

    def runTest(self):
        # This script calibrates WISH using known peak positions from
        # neutron absorbing bands. The workspace with suffix "_calib"
        # contains calibrated data. The workspace with suxxic "_corrected"
        # contains calibrated data with known problematic tubes also corrected

        ws = mantid.LoadNexusProcessed(Filename="WISH30541_integrated.nxs")

        # This array defines the positions of peaks on the detector in
        # meters from the center (0)

        # For wish this is calculated as follows:
        # Height of all 7 bands = 0.26m => each band is separated by 0.260 / 6 = 0.4333m

        # The bands are on a cylinder diameter 0.923m. So we can work out the angle as
        # (0.4333 * n) / (0.923 / 2) where n is the number of bands above (or below) the
        # center band.

        # Putting this together with the distance to the detector tubes (2.2m) we get
        # the following:  (0.4333n) / 0.4615 * 2200 = Expected peak positions
        # From this we can show there should be 5 peaks (peaks 6 + 7 are too high/low)
        # at: 0, 0.206, 0.413 respectively (this is symmetrical so +/-)

        peak_positions = np.array([-0.413, -0.206, 0, 0.206, 0.413])
        funcForm = 5 * [1]  # 5 gaussian peaks
        fitPar = TubeCalibFitParams([59, 161, 258, 353, 448])
        fitPar.setAutomatic(True)

        instrument = ws.getInstrument()
        spec = TubeSpec(ws)

        spec.setTubeSpecByString(instrument.getFullName())

        idealTube = IdealTube()
        idealTube.setArray(peak_positions)

        # First calibrate all of the detectors
        calibrationTable, peaks = tube.calibrate(ws, spec, peak_positions, funcForm, margin=15,
                                                 outputPeak=True, fitPar=fitPar)
        self.calibration_table = calibrationTable

        def findBadPeakFits(peaksTable, threshold=10):
            """ Find peaks whose fit values fall outside of a given tolerance
            of the mean peak centers across all tubes.

            Tubes are defined as have a bad fit if the absolute difference
            between the fitted peak centers for a specific tube and the
            mean of the fitted peak centers for all tubes differ more than
            the threshold parameter.

            @param peakTable: the table containing fitted peak centers
            @param threshold: the tolerance on the difference from the mean value
            @return A list of expected peak positions and a list of indices of tubes
            to correct
            """
            n = len(peaksTable)
            num_peaks = peaksTable.columnCount() - 1
            column_names = ['Peak%d' % i for i in range(1, num_peaks + 1)]
            data = np.zeros((n, num_peaks))
            for i, row in enumerate(peaksTable):
                data_row = [row[name] for name in column_names]
                data[i, :] = data_row

            # data now has all the peaks positions for each tube
            # the mean value is the expected value for the peak position for each tube
            expected_peak_pos = np.mean(data, axis=0)

            # calculate how far from the expected position each peak position is
            distance_from_expected = np.abs(data - expected_peak_pos)
            check = np.where(distance_from_expected > threshold)[0]
            problematic_tubes = list(set(check))
            print("Problematic tubes are: " + str(problematic_tubes))
            return expected_peak_pos, problematic_tubes

        def correctMisalignedTubes(ws, calibrationTable, peaksTable, spec, idealTube, fitPar, threshold=10):
            """ Correct misaligned tubes due to poor fitting results
            during the first round of calibration.

            Misaligned tubes are first identified according to a tolerance
            applied to the absolute difference between the fitted tube
            positions and the mean across all tubes.

            The FindPeaks algorithm is then used to find a better fit
            with the ideal tube positions as starting parameters
            for the peak centers.

            From the refitted peaks the positions of the detectors in the
            tube are recalculated.

            @param ws: the workspace to get the tube geometry from
            @param calibrationTable: the calibration table output from running calibration
            @param peaksTable: the table containing the fitted peak centers from calibration
            @param spec: the tube spec for the instrument
            @param idealTube: the ideal tube for the instrument
            @param fitPar: the fitting parameters for calibration
            @param threshold: tolerance defining is a peak is outside of the acceptable range
            @return table of corrected detector positions
            """
            table_name = calibrationTable.name() + 'Corrected'
            corrections_table = mantid.CreateEmptyTableWorkspace(OutputWorkspace=table_name)
            corrections_table.addColumn('int', "Detector ID")
            corrections_table.addColumn('V3D', "Detector Position")

            mean_peaks, bad_tubes = findBadPeakFits(peaksTable, threshold)

            for index in bad_tubes:
                print("Refitting tube %s" % spec.getTubeName(index))
                tube_dets, _ = spec.getTube(index)
                getPoints(ws, idealTube.getFunctionalForms(), fitPar, tube_dets)
                tube_ws = mantid.mtd['TubePlot']
                fit_ws = mantid.FindPeaks(InputWorkspace=tube_ws, WorkspaceIndex=0,
                                          PeakPositions=fitPar.getPeaks(), PeaksList='RefittedPeaks')
                centers = [row['centre'] for row in fit_ws]
                detIDList, detPosList = getCalibratedPixelPositions(ws, centers, idealTube.getArray(), tube_dets)

                for id, pos in zip(detIDList, detPosList):
                    corrections_table.addRow({'Detector ID': id, 'Detector Position': kernel.V3D(*pos)})

            return corrections_table

        corrected_calibration_table = correctMisalignedTubes(ws, calibrationTable, peaks, spec, idealTube, fitPar)
        self.correction_table = corrected_calibration_table
        tube.saveCalibration(self.correction_table.getName(), out_path=self.calibration_out_path)
        tube.saveCalibration(self.calibration_table.getName(), out_path=self.correction_out_path)
