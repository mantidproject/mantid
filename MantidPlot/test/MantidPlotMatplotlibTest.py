# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test matplotlib plotting for MantidPlot. Without our custom backend matplotlib graphs crash MantidPlot.
"""
from __future__ import print_function

import mantidplottests
from mantidplottests import *
import time
import numpy as np
from PyQt4 import QtCore

try:
    import matplotlib as mpl
    import matplotlib.pyplot as plt
    import matplotlib.cm as cm
    import matplotlib.mlab as mlab
    HAVE_MPL = True
except ImportError:
    HAVE_MPL = False


def bivariate_normal(X, Y, sigmax=1.0, sigmay=1.0,
                     mux=0.0, muy=0.0, sigmaxy=0.0):
    """
    Bivariate Gaussian distribution for equal shape *X*, *Y*.
    See `bivariate normal
    <http://mathworld.wolfram.com/BivariateNormalDistribution.html>`_
    at mathworld.
    """
    Xmu = X-mux
    Ymu = Y-muy

    rho = sigmaxy/(sigmax*sigmay)
    z = Xmu**2/sigmax**2 + Ymu**2/sigmay**2 - 2*rho*Xmu*Ymu/(sigmax*sigmay)
    denom = 2*np.pi*sigmax*sigmay*np.sqrt(1-rho**2)
    return np.exp(-z/(2*(1-rho**2))) / denom


class MantidPlotMatplotlibTest(unittest.TestCase):

    def setUp(self):
        # A deliberate pause so that the tests wait a small
        # moment for MantidPlot to initialize properly
        # It seems to stop the occasional lockup
        time.sleep(0.2)

    def test_1d_plot(self):
        x, y = np.arange(1.0,10.0), np.arange(1.0,10.)
        ax = plt.plot(x,y)
        self.assertNotEqual(ax, None)
        plt.show()
        time.sleep(0.2)
        plt.close()

    def test_image_plot(self):
        delta = 0.025
        x = y = np.arange(-3.0, 3.0, delta)
        X, Y = np.meshgrid(x, y)
        Z1 = bivariate_normal(X, Y, 1.0, 1.0, 0.0, 0.0)
        Z2 = bivariate_normal(X, Y, 1.5, 0.5, 1, 1)
        Z = Z2 - Z1  # difference of Gaussians

        im = plt.imshow(Z, interpolation='bilinear', cmap=cm.RdYlGn,
                        origin='lower', extent=[-3, 3, -3, 3],
                        vmax=abs(Z).max(), vmin=-abs(Z).max())
        plt.show()
        time.sleep(0.2)
        plt.close()

# Run the unit tests
if HAVE_MPL:
    mantidplottests.runTests(MantidPlotMatplotlibTest)
else:
    print("Matplotlib unavailable. Tests skipped.")
