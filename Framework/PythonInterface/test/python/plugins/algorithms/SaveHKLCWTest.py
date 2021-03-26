# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
import tempfile
import os
from mantid.simpleapi import SaveHKLCW, CreateSampleWorkspace, CreatePeaksWorkspace, SetUB, DeleteWorkspace


class SaveHKLCWTest(unittest.TestCase):
    def setUp(self):
        self._tmp_directory = tempfile.gettempdir()
        self._ws_name = 'SaveHKLCWTest'
        CreateSampleWorkspace(OutputWorkspace=self._ws_name)
        peaks = CreatePeaksWorkspace(self._ws_name, NumberOfPeaks=0, OutputWorkspace=self._ws_name + "_peaks")
        SetUB(peaks)
        peaks.addPeak(peaks.createPeakHKL([1, 1, 1]))
        peaks.addPeak(peaks.createPeakHKL([1, -1, 1]))

    def tearDown(self):
        DeleteWorkspace(self._ws_name, self._ws_name + "_peaks")

    def testSaveHKL(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name + '.hkl')
        SaveHKLCW(self._ws_name + "_peaks", output_file)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 5)
        self.assertEqual(lines[0], 'Single crystal data\n')
        self.assertEqual(lines[1], '(3i4,2f8.2,i4)\n')
        self.assertEqual(lines[2], '0.66667  0   0\n')
        self.assertEqual(lines[3], '   1   1   1    0.00    0.00   1\n')
        self.assertEqual(lines[4], '   1  -1   1    0.00    0.00   1\n')

    def testSaveHKL_no_header(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name + '_no_header.hkl')
        SaveHKLCW(self._ws_name + "_peaks", output_file, Header=False)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 2)
        self.assertEqual(lines[0], '   1   1   1    0.00    0.00   1\n')
        self.assertEqual(lines[1], '   1  -1   1    0.00    0.00   1\n')

    def testSaveHKL_direction_cosines(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name + '_dc.hkl')
        SaveHKLCW(self._ws_name + "_peaks", output_file, DirectionCosines=True)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 5)
        self.assertEqual(lines[0], 'Single crystal data\n')
        self.assertEqual(lines[1], '(3i4,2f8.2,i4,6f8.5)\n')
        self.assertEqual(lines[2], '0.66667  0   0\n')
        self.assertEqual(lines[3], '   1   1   1    0.00    0.00   1-1.00000 0.33333 0.00000-0.66667 0.00000-0.66667\n')
        self.assertEqual(lines[4], '   1  -1   1    0.00    0.00   1-1.00000 0.33333 0.00000 0.66667 0.00000-0.66667\n')

    def testSaveHKL_direction_cosines_no_header(self):
        output_file = os.path.join(self._tmp_directory, self._ws_name + '_dc_no_header.hkl')
        SaveHKLCW(self._ws_name + "_peaks", output_file, DirectionCosines=True, Header=False)

        with open(output_file, 'r') as f:
            lines = f.readlines()

        self.assertEqual(len(lines), 2)
        self.assertEqual(lines[0], '   1   1   1    0.00    0.00   1-1.00000 0.33333 0.00000-0.66667 0.00000-0.66667\n')
        self.assertEqual(lines[1], '   1  -1   1    0.00    0.00   1-1.00000 0.33333 0.00000 0.66667 0.00000-0.66667\n')


if __name__ == '__main__':
    unittest.main()
