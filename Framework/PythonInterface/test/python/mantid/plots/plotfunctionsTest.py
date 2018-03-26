from __future__ import (absolute_import, division, print_function)

import numpy as np
import matplotlib
matplotlib.use('AGG')
import matplotlib.pyplot as plt
import unittest

import mantid.api
import mantid.plots.plotfunctions as funcs
from mantid.kernel import config
from mantid.simpleapi import CreateWorkspace, DeleteWorkspace, CreateMDHistoWorkspace,\
                             ConjoinWorkspaces, AddTimeSeriesLog




class PlotFunctionsTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.g1da = config['graph1d.autodistribution']
        config['graph1d.autodistribution'] = 'On'
        cls.ws2d_histo = CreateWorkspace(DataX=[10, 20, 30, 10, 20, 30],
                                         DataY=[2, 3, 4, 5],
                                         DataE=[1, 2, 3, 4],
                                         NSpec=2,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4, 6, 8],
                                         OutputWorkspace='ws2d_histo')
        cls.ws2d_histo_rag = CreateWorkspace(DataX=[1, 2, 3, 4, 5, 2, 4, 6, 8, 10],
                                             DataY=[2] * 8,
                                             NSpec=2,
                                             VerticalAxisUnit='DeltaE',
                                             VerticalAxisValues=[5, 7, 9],
                                             OutputWorkspace='ws2d_histo_rag')
        cls.ws_MD_2d = CreateMDHistoWorkspace(Dimensionality=3,
                                              Extents='-3,3,-10,10,-1,1',
                                              SignalInput=range(25),
                                              ErrorInput=range(25),
                                              NumberOfEvents=10 * np.ones(25),
                                              NumberOfBins='5,5,1',
                                              Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                              OutputWorkspace='ws_MD_2d')
        cls.ws_MD_1d = CreateMDHistoWorkspace(Dimensionality=3,
                                              Extents='-3,3,-10,10,-1,1',
                                              SignalInput=range(5),
                                              ErrorInput=range(5),
                                              NumberOfEvents=10 * np.ones(5),
                                              NumberOfBins='1,5,1',
                                              Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                              OutputWorkspace='ws_MD_1d')
        cls.ws2d_point_uneven = CreateWorkspace(DataX=[10, 20, 30],
                                                DataY=[1, 2, 3],
                                                NSpec=1,
                                                OutputWorkspace='ws2d_point_uneven')
        wp = CreateWorkspace(DataX=[15, 25, 35, 45], DataY=[1, 2, 3, 4], NSpec=1)
        ConjoinWorkspaces(cls.ws2d_point_uneven, wp, CheckOverlapping=False)
        cls.ws2d_point_uneven = mantid.mtd['ws2d_point_uneven']
        cls.ws2d_histo_uneven = CreateWorkspace(DataX=[10, 20, 30, 40],
                                                DataY=[1, 2, 3],
                                                NSpec=1,
                                                OutputWorkspace='ws2d_histo_uneven')
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:30:00", Value=15)
        AddTimeSeriesLog(cls.ws2d_histo, Name="my_log", Time="2010-01-01T00:50:00", Value=100.2)

    @classmethod
    def tearDownClass(cls):
        config['graph1d.autodistribution'] = cls.g1da
        DeleteWorkspace('ws2d_histo')
        DeleteWorkspace('ws_MD_2d')
        DeleteWorkspace('ws_MD_1d')
        DeleteWorkspace('ws2d_point_uneven')

    def test_1d_plots(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo, 'rs', specNum=1)
        funcs.plot(ax, self.ws2d_histo, specNum=2, linewidth=6)
        funcs.plot(ax, self.ws_MD_1d, 'bo')

    def test_1d_log(self):
        fig, ax = plt.subplots()
        funcs.plot(ax, self.ws2d_histo, LogName='my_log')
        ax1 = ax.twiny()
        funcs.plot(ax1, self.ws2d_histo, LogName='my_log', FullTime=True)

    def test_1d_errorbars(self):
        fig, ax = plt.subplots()
        funcs.errorbar(ax, self.ws2d_histo, 'rs', specNum=1)
        funcs.errorbar(ax, self.ws2d_histo, specNum=2, linewidth=6)
        funcs.errorbar(ax, self.ws_MD_1d, 'bo')

    def test_1d_scatter(self):
        fig, ax = plt.subplots()
        funcs.scatter(ax, self.ws2d_histo, specNum=1)
        funcs.scatter(ax, self.ws2d_histo, specNum=2)
        funcs.scatter(ax, self.ws_MD_1d)

    def test_2d_contours(self):
        fig, ax = plt.subplots()
        funcs.contour(ax, self.ws2d_histo_rag)
        funcs.contourf(ax, self.ws2d_histo, vmin=0)
        funcs.tricontour(ax, self.ws_MD_2d)
        funcs.tricontourf(ax, self.ws_MD_2d)
        self.assertRaises(ValueError, funcs.contour, ax, self.ws2d_point_uneven)

    def test_2d_pcolors(self):
        fig, ax = plt.subplots()
        funcs.pcolor(ax, self.ws2d_histo_rag)
        funcs.tripcolor(ax, self.ws2d_histo, vmin=0)
        funcs.pcolormesh(ax, self.ws_MD_2d)
        funcs.pcolorfast(ax, self.ws2d_point_uneven, vmin=-1)


if __name__ == '__main__':
    unittest.main()
