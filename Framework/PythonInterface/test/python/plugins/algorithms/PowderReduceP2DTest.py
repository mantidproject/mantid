# The following line helps with future compatibility with Python 3
# print must now be used as a function, e.g print('Hello','World')
from __future__ import (absolute_import, division, print_function, unicode_literals)
# import mantid algorithms, numpy and matplotlib
from mantid.simpleapi import *
import matplotlib.pyplot as plt
import numpy as np


'''
ws = CreateSampleWorkspace('Event', 'Powder Diffraction')
AddSampleLog('ws', 'gd_prtn_chrg', '305.718178022222', 'Number')
for i in range(0,500):
    AddTimeSeriesLog('ws', 'proton_charge', '2010-01-01T00:00:00', '20346430.0')
    AddTimeSeriesLog('ws', 'frequency', '2010-01-01T00:00:00', '60.0')
'''

import unittest
import tempfile
import shutil
import os
import numpy as np
from mantid.simpleapi import PowderReduceP2D


class PowderReduceP2DTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._test_dir = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls._test_dir)

    def setUp(self):
        self._sample = self._sampleEventData()
        self._vanadium = self._vanadiumEventData()
        self._empty = self._emptyEventData()
        self._reference = self._loadReference()

    def tearDown(self):
        DeleteWorkspace(self._sample)
        DeleteWorkspace(self._vanadium)
        DeleteWorkspace(self._empty)

    def _sampleEventData(self):
        """path to sample event data used for testing the algorithm"""
        return "Path/to/sample/event/data.nxs"

    def _vanadiumEventData(self):
        """path to vanadium event data used for testing the algorithm"""
        return "Path/to/vanadium/event/data.nxs"

    def _emptyEventData(self):
        """path to empty event data used for testing the algorithm"""
        return "Path/to/empty/event/data.nxs"

    def _loadReference(self):
        return "Path/to/reference/file.p2d"

    def test_PowderReduceP2D_sam(self):
        PowderReduceP2D(SampleData=self._sample, OutputFile=self._test_dir + '/PowderReduceP2D_sam', DoIntensityCorrection = True, VanaData = self._vanadium, DoBackgroundCorrection = True, EmptyData = self._empty, DoEdgebinning = False, dSpaceBinning = 0.01, dPerpendicularBinning = 0.01, CalFile = 'Path/to/calibration/file.cal')
        self._assert_workspace_positive(
            os.path.join(self._test_dir, "Path/to/reference/file.p2d"),
            os.path.join(self._test_dir, "PowderReduceP2D_sam.p2d"))

    def _assert_workspace_positive(self, reference_file, result_file):
        with open(reference_file, 'r') as ref_file:
            reference = ref_file.read()
        with open(result_file, 'r') as res_file:
            result = res_file.read()
            self.maxDiff = 10000
        self.assertMultiLineEqual(reference, result)


if __name__ == '__main__':
    unittest.main()