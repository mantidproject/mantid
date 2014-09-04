import unittest
import numpy as np
from mantid.simpleapi import *
from mantid.api import *


class SymmetriseTest(unittest.TestCase):

    def setUp(self):
        self._sample_ws = self._generate_sample_ws('symm_test_sample_ws')

    def test_basic(self):
        symm_test_out_ws= Symmetrise(Sample=self._sample_ws, XCut=0.05)

    def _rayleigh(self, x, sigma):
        return (x / sigma ** 2) * np.exp(-x ** 2 / (2 * sigma ** 2))

    def _generate_sample_ws(self, ws_name):
        data_x = np.arange(0, 10, 0.01)
        data_y = self._rayleigh(data_x, 1)

        CreateWorkspace(DataX=data_x, DataY=data_y, OutputWorkspace=ws_name)
        ScaleX(InputWorkspace=ws_name, Factor=-1, Operation="Add", OutputWorkspace=ws_name)  # centre the peak over 0

        return mtd[ws_name]


if __name__ == '__main__':
    unittest.main()
