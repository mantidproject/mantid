# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import tempfile
import os
import numpy as np
from mantid.kernel import V3D
from mantid.simpleapi import (SaveHKLCW, CreateSampleWorkspace, CreatePeaksWorkspace, SetUB, DeleteWorkspace,
                              SetGoniometer, LoadEmptyInstrument, AddSampleLog, LoadInstrument)


class SaveHKLCWTest(unittest.TestCase):
    def setUp(self):
        self._tmp_directory = tempfile.gettempdir()
        self._ws_name = 'SaveHKLCWTest'
        CreateSampleWorkspace(OutputWorkspace=self._ws_name)
        peaks = CreatePeaksWorkspace(self._ws_name, NumberOfPeaks=0, OutputWorkspace=self._ws_name+"_peaks")
        SetUB(peaks)
        peaks.addPeak(peaks.createPeakHKL([1, 1, 1]))
        peaks.addPeak(peaks.createPeakHKL([1, -1, 1]))
        leanpeaks = CreatePeaksWorkspace(OutputType="LeanElasticPeak", NumberOfPeaks=0, OutputWorkspace=self._ws_name+"_leanelasticpeaks")
        SetUB(leanpeaks)
        leanpeaks.addPeak(leanpeaks.createPeakHKL([1, 1, 1]))
        leanpeaks.addPeak(leanpeaks.createPeakHKL([1, -1, 1]))

    def tearDown(self):
        DeleteWorkspace(self._ws_name, self._ws_name+"_peaks")

    def testSaveHKL(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name+'.hkl')
        SaveHKLCW(self._ws_name+"_peaks", output_file)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 5)
        self.assertEqual(lines[0], 'Single crystal data\n')
        self.assertEqual(lines[1], '(3i4,2f8.2,i4)\n')
        self.assertEqual(lines[2], '0.66667  0   0\n')
        self.assertEqual(lines[3], '   1   1   1    0.00    0.00   1\n')
        self.assertEqual(lines[4], '   1  -1   1    0.00    0.00   1\n')

    def testSaveHKL_no_header(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name+'_no_header.hkl')
        SaveHKLCW(self._ws_name+"_peaks", output_file, Header=False)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 2)
        self.assertEqual(lines[0], '   1   1   1    0.00    0.00   1\n')
        self.assertEqual(lines[1], '   1  -1   1    0.00    0.00   1\n')

    def testSaveHKL_direction_cosines(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name+'_dc.hkl')
        SaveHKLCW(self._ws_name+"_peaks", output_file, DirectionCosines=True)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 5)
        self.assertEqual(lines[0], 'Single crystal data\n')
        self.assertEqual(lines[1], '(3i4,2f8.2,i4,6f8.5)\n')
        self.assertEqual(lines[2], '0.66667  0   0\n')
        self.assertEqual(lines[3], '   1   1   1    0.00    0.00   1-1.00000 0.33333 0.00000-0.66667 0.00000-0.66667\n')
        self.assertEqual(lines[4], '   1  -1   1    0.00    0.00   1-1.00000 0.33333 0.00000 0.66667 0.00000-0.66667\n')

    def testSaveHKL_direction_cosines_leanelasticpeak(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name+'_dc_leanelastic.hkl')
        SaveHKLCW(self._ws_name+"_leanelasticpeaks", output_file, DirectionCosines=True)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 5)
        self.assertEqual(lines[0], 'Single crystal data\n')
        self.assertEqual(lines[1], '(3i4,2f8.2,i4,6f8.5)\n')
        self.assertEqual(lines[2], '0.66667  0   0\n')
        self.assertEqual(lines[3], '   1   1   1    0.00    0.00   1-1.00000 0.33333 0.00000-0.66667 0.00000-0.66667\n')
        self.assertEqual(lines[4], '   1  -1   1    0.00    0.00   1-1.00000 0.33333 0.00000 0.66667 0.00000-0.66667\n')

    def testSaveHKL_direction_cosines_no_header(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name+'_dc_no_header.hkl')
        SaveHKLCW(self._ws_name+"_peaks", output_file, DirectionCosines=True, Header=False)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 2)
        self.assertEqual(lines[0], '   1   1   1    0.00    0.00   1-1.00000 0.33333 0.00000-0.66667 0.00000-0.66667\n')
        self.assertEqual(lines[1], '   1  -1   1    0.00    0.00   1-1.00000 0.33333 0.00000 0.66667 0.00000-0.66667\n')

    def test_DEMAND_direction_cosine(self):
        # This will compare the direction cosine calculation to a
        # calculation done by different software, the DEMAND
        # (FOUR-CIRCLE) software. This same test exist in the test for
        # SaveHKL
        LoadEmptyInstrument(InstrumentName='HB3A', OutputWorkspace='DEMAND')
        AddSampleLog(Workspace='DEMAND', LogName='2theta', LogText='58.0595',
                     LogType='Number Series', LogUnit='degree', NumberType='Double')
        AddSampleLog(Workspace='DEMAND', LogName='det_trans', LogText='399.9955',
                     LogType='Number Series', LogUnit='millimeter', NumberType='Double')
        LoadInstrument(Workspace='DEMAND', InstrumentName='HB3A', RewriteSpectraMap=False)
        peaks = CreatePeaksWorkspace(InstrumentWorkspace='DEMAND', NumberOfPeaks=0)
        SetUB(peaks, UB="-0.009884, -0.016780, 0.115725, 0.112280, 0.002840, 0.011331, -0.005899, 0.081084, 0.023625")
        SetGoniometer(peaks,
                      Axis0='29.0295, 0, 1, 0, -1',
                      Axis1='15.1168, 0, 0, 1, -1',
                      Axis2='4.7395, 0, 1, 0, -1')
        peaks.addPeak(peaks.createPeakHKL([1, -2, 5]))

        output_file = os.path.join(self._tmp_directory, 'SaveHKLCW_demand.hkl')
        SaveHKLCW(peaks, output_file, DirectionCosines=True, Header=False)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 1)
        self.assertEqual(lines[0], '   1  -2   5    0.00    0.00   1-0.03516-0.13886-0.71007 0.96633-0.70368-0.21549\n')

    def test_TOPAZ_ANVRED_direction_cosine(self):

        LoadEmptyInstrument(InstrumentName='TOPAZ', OutputWorkspace='TOPAZ')
        peaks = CreatePeaksWorkspace(InstrumentWorkspace='TOPAZ', NumberOfPeaks=0)
        SetUB(peaks, UB="0.07817488, -0.04974505, 0.0216728, 0.19329136, -0.03056769, -0.01126276, 0.17652773, 0.26605919, 0.00265988")
        SetGoniometer(peaks,
                      Axis0='159.99, 0, 1, 0, 1',
                      Axis1='0.00, 0, 0, 1, 1',
                      Axis2='0.00, 0, 1, 0, 1')
        L2, two_theta, az_phi, wl = 0.45577, 1.84809, -0.70686, 1.248933

        peaks.addPeak(peaks.createPeak(V3D(-2*np.pi/wl*np.sin(two_theta)*np.cos(az_phi),-2*np.pi/wl*np.sin(two_theta)*np.sin(az_phi),-2*np.pi/wl*(np.cos(two_theta)-1)), L2))

        UB = peaks.getPeak(0).getGoniometerMatrix() @ peaks.sample().getOrientedLattice().getUB()

        up = np.array([0,0,-1])
        us = np.array([np.sin(two_theta)*np.cos(az_phi),np.sin(two_theta)*np.sin(az_phi),np.cos(two_theta)])
        Q = np.dot(UB, [1,0,0])
        print(np.dot(up,Q)/np.linalg.norm(Q))
        Q = np.dot(UB, [0,1,0])
        print(np.dot(up,Q)/np.linalg.norm(Q))
        Q = np.dot(UB, [0,0,1])
        print(np.dot(up,Q)/np.linalg.norm(Q))
#    1  -3  43 1518.15 37.2891   1 1.24893 0.11557  0.51098 -0.67834 -0.21000 -0.63620  0.67201  0.65212 44454      4 0.9711  13  1.84809  0.7825  45.00  88.00
        output_file = os.path.join(self._tmp_directory, 'SaveHKLCW_topaz.hkl')
        SaveHKLCW(peaks, output_file, DirectionCosines=True, Header=False)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 1)
        self.assertEqual(lines[0], '    9   5  -3    0.00    0.00   1-0.65408-0.68392-0.70174-0.04159-0.28238 0.72837\n')

if __name__ == '__main__':
    unittest.main()
