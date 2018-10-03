# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
#
# TUBE CALIBRATION DEMONSTRATION PROGRAM FOR MAPS - Execute this
#
# Here we run the calibration of a selected part of MAPS consisting of several components
# specifying them in an array of strings.

from __future__ import absolute_import, division, print_function

import tube
import mantid.simpleapi as mantid

# == Set parameters for calibration ==

filename = 'MAP14919.raw'  # Name of calibration run
rangeLower = 2000  # Integrate counts in each spectra from rangeLower to rangeUpper
rangeUpper = 10000  #

# Set what we want to calibrate (e.g whole instrument or one door )
# Set what we want to calibrate (e.g whole instrument or one door )
CalibratedComponents = ['C4_window', 'C3_window', 'C2_window']  # Calibrate three C windows

# Get calibration raw file and integrate it
rawCalibInstWS = mantid.Load(filename)  # 'raw' in 'rawCalibInstWS' means unintegrated.
print("Integrating Workspace")
CalibInstWS = mantid.Integration(rawCalibInstWS, RangeLower=rangeLower, RangeUpper=rangeUpper)
mantid.DeleteWorkspace(rawCalibInstWS)
print("Created workspace (CalibInstWS) with integrated data from run and instrument to calibrate")

# == Create Objects needed for calibration ==

# Specify the known positions
knownPos = [-0.50, -0.16, -0.00, 0.16, 0.50]
funcForm = [2, 1, 1, 1, 2]

print("Created objects needed for calibration.")

# == Get the calibration and put results into calibration table ==

calibrationTable, peakTable = tube.calibrate(CalibInstWS, CalibratedComponents, knownPos, funcForm,
                                             outputPeak=True)
print("Got calibration (new positions of detectors) ")

# == Apply the Calibation ==
mantid.ApplyCalibration(Workspace=CalibInstWS, PositionTable=calibrationTable)
print("Applied calibration")

# == Save workspace ==
# mantid.SaveNexusProcessed(CalibInstWS, path+'TubeCalibDemoMapsResult.nxs', "Result of Running TCDemoMaps.py")
# print("saved calibrated workspace (CalibInstWS) into Nexus file TubeCalibDemoMapsResult.nxs")
