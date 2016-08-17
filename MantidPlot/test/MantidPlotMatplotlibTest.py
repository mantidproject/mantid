"""
Test matplotlib plotting for MantidPlot. Without our custom backend matplotlib graphs crash MantidPlot.
"""
from __future__ import print_function

import mantidplottests
from mantidplottests import *
import time
import numpy as np
from distutils.version import LooseVersion
from PyQt4 import QtCore

try:
    import matplotlib as mpl
    import matplotlib.pyplot as plt
    import matplotlib.cm as cm
    import matplotlib.mlab as mlab
    HAVE_MPL = True
except ImportError:
    HAVE_MPL = False

class MantidPlotMatplotlibTest(unittest.TestCase):

    def tearDown(self):
        if LooseVersion(mpl.__version__) >= '1.5.0':
             plt.close('all')
        else:
            # Old versions of matplotlib.pyplot.close work without this
            # in a normal script session but fail here. The workaround
            # is to ensure this happens on the same thread too.
            gui_cmd(plt.close, 'all')

    def test_1d_plot(self):
        x, y = np.arange(1.0,10.0), np.arange(1.0,10.)
        ax = plt.plot(x,y)
        self.assertTrue(ax is not None)
        plt.show()

    def test_image_plot(self):
        delta = 0.025
        x = y = np.arange(-3.0, 3.0, delta)
        X, Y = np.meshgrid(x, y)
        Z1 = mlab.bivariate_normal(X, Y, 1.0, 1.0, 0.0, 0.0)
        Z2 = mlab.bivariate_normal(X, Y, 1.5, 0.5, 1, 1)
        Z = Z2 - Z1  # difference of Gaussians

        im = plt.imshow(Z, interpolation='bilinear', cmap=cm.RdYlGn,
                        origin='lower', extent=[-3, 3, -3, 3],
                        vmax=abs(Z).max(), vmin=-abs(Z).max())
        plt.show()

# Run the unit tests
if HAVE_MPL:
    mantidplottests.runTests(MantidPlotMatplotlibTest)
else:
    print("Matplotlib unavailable. Tests skipped.")
