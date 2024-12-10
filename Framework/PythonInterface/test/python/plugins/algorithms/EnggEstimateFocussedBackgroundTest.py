# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import CreateWorkspace, EnggEstimateFocussedBackground, DeleteWorkspace
import numpy as np


def mgauss(x, p):
    """
    make gaussian peaks
    p = [height, cen, sig]
    """
    ngauss = len(p) // 3
    ht = p[0::3]
    cen = p[1::3]
    sig = p[2::3]
    y = np.zeros(x.shape)
    for ii in range(0, ngauss):
        y += ht[ii] * np.exp(-0.5 * ((np.array(x) - cen[ii]) / sig[ii]) ** 2)
    return y


class EnggEstimateFocussedBackground_Test(unittest.TestCase):
    def setUp(self):
        # create some fairly realistic fake data with peaks and background
        x = np.linspace(0, 160, 401)
        cens = [60, 70, 100]
        mask = np.ones(x.shape, dtype=bool)
        mask[(x > cens[0] - 4) & (x < cens[-1] + 4)] = False  # mask that includes xvalues at each end (away from peaks)
        pin = []
        for ipk in range(0, len(cens)):
            pin += [3, cens[ipk], 1]  # height, centre, sig
        pin += [3, 80, 20]  # broad peak as background

        y = mgauss(x, pin)
        np.random.seed(0)
        self.ws = CreateWorkspace(OutputWorkspace="ws", DataX=x, DataY=y + np.random.normal(y, 0.2 * np.sqrt(y)))
        self.mask = mask

    def tearDown(self):
        if self.ws is not None:
            DeleteWorkspace(self.ws)

    def test_subtraction(self):
        # get bg and subtract
        ws_bg = EnggEstimateFocussedBackground(InputWorkspace="ws", NIterations=20, XWindow=3)
        ws_diff = self.ws - ws_bg

        # test residuals to ensure background is well approximated
        self.assertAlmostEqual(np.mean(ws_diff.readY(0)[self.mask]), 0, delta=0.01)
        self.assertAlmostEqual(np.median(ws_diff.readY(0)[self.mask]), 0, delta=0.01)

    def test_window_validation(self):
        # test too small a window
        with self.assertRaisesRegex(RuntimeError, "Convolution window must have at least three points"):
            EnggEstimateFocussedBackground(InputWorkspace="ws", OutputWorkspace="ws_bg", NIterations=20, XWindow=0.1)
        # test too large a window
        with self.assertRaisesRegex(RuntimeError, "Data has must have at least the number of points as the convolution window"):
            EnggEstimateFocussedBackground(InputWorkspace="ws", OutputWorkspace="ws_bg", NIterations=20, XWindow=200)


if __name__ == "__main__":
    unittest.main()
