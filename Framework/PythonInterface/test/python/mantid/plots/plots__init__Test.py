from __future__ import (absolute_import, division, print_function)

import matplotlib
matplotlib.use('AGG')
import matplotlib.pyplot as plt
import numpy as np
import unittest

from mantid.simpleapi import CreateWorkspace, DeleteWorkspace


class Plots__init__Test(unittest.TestCase):
    '''
    Just test if mantid projection works
    '''
    @classmethod
    def setUpClass(cls):
        cls.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30, 10, 20, 30],
                                         DataY=[2, 3, 4, 5, 3, 5],
                                         DataE=[1, 2, 3, 4, 1, 1],
                                         NSpec=3,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4, 6, 8],
                                         OutputWorkspace='ws2d_histo')

    @classmethod
    def tearDownClass(cls):
        DeleteWorkspace('ws2d_histo')

    def test_1d_plots(self):
        fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
        # ax.plot(self.ws2d_histo, 'rs', specNum=1)
        ax.plot(self.ws2d_histo, specNum=2, linewidth=6)
        ax.plot(np.arange(10), np.arange(10), 'bo-')

    def test_3d_plots(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='mantid3d')
        ax.plot(self.ws2d_histo, specNum=1)
        ax.plot(np.arange(10), np.arange(10), np.arange(10))
        ax.plot_wireframe(self.ws2d_histo)

    def test_fail(self):
        fig, ax = plt.subplots()
        self.assertRaises(Exception, ax.plot, self.ws2d_histo, 'rs', specNum=1)
        self.assertRaises(Exception, ax.pcolormesh, self.ws2d_histo)

    def test_fail_3d(self):
        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')
        self.assertRaises(Exception, ax.plot_wireframe, self.ws2d_histo)
        self.assertRaises(Exception, ax.plot_surface, self.ws2d_histo)


if __name__ == '__main__':
    unittest.main()
