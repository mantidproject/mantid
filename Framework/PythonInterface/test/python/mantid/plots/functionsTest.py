from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CreateWorkspace,DeleteWorkspace,CreateMDHistoWorkspace, ConjoinWorkspaces
import mantid.plots.functions as funcs
import mantid.api
import numpy as np
from mantid.kernel import config
import matplotlib.pyplot as plt


class PlotsFunctionsTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.g1da=config['graph1d.autodistribution']
        config['graph1d.autodistribution']='On'
        cls.ws2d_histo = CreateWorkspace(DataX=[10,20,30,10,20,30],
                                         DataY=[2,3,4,5],
                                         DataE=[1,2,3,4],
                                         NSpec=2,
                                         Distribution=True,
                                         UnitX='Wavelength',
                                         VerticalAxisUnit='DeltaE',
                                         VerticalAxisValues=[4,6,8],
                                         OutputWorkspace='ws2d_histo')
        cls.ws2d_point = CreateWorkspace(DataX=[1,2,3,4,1,2,3,4,1,2,3,4],
                                         DataY=[2]*12,
                                         NSpec=3,
                                         OutputWorkspace='ws2d_point')
        cls.ws1d_point = CreateWorkspace(DataX=[1,2],
                                         DataY=[1,2],
                                         NSpec=1,
                                         Distribution=False,
                                         OutputWorkspace='ws1d_point')
        cls.ws2d_histo_rag = CreateWorkspace(DataX=[1,2,3,4,5,2,4,6,8,10],
                                             DataY=[2]*8,
                                             NSpec=2,
                                             VerticalAxisUnit='DeltaE',
                                             VerticalAxisValues=[5,7,9],
                                             OutputWorkspace='ws2d_histo_rag')
        cls.ws2d_point_rag = CreateWorkspace(DataX=[1,2,3,4,2,4,6,8],
                                             DataY=[2]*8,
                                             NSpec=2,
                                             OutputWorkspace='ws2d_point_rag')
        cls.ws_MD_2d = CreateMDHistoWorkspace(Dimensionality=3,
                                              Extents='-3,3,-10,10,-1,1',
                                              SignalInput=range(25),
                                              ErrorInput=range(25),
                                              NumberOfEvents=10*np.ones(25),
                                              NumberOfBins='5,5,1',
                                              Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                              OutputWorkspace='ws_MD_2d')
        cls.ws_MD_1d = CreateMDHistoWorkspace(Dimensionality=3,
                                              Extents='-3,3,-10,10,-1,1',
                                              SignalInput=range(5),
                                              ErrorInput=range(5),
                                              NumberOfEvents=10*np.ones(5),
                                              NumberOfBins='1,5,1',
                                              Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                              OutputWorkspace='ws_MD_1d')
        cls.ws2d_point_uneven=CreateWorkspace(DataX=[10,20,30],
                                              DataY=[1,2,3],
                                              NSpec=1,
                                              OutputWorkspace='ws2d_point_uneven')
        wp=CreateWorkspace(DataX=[15,25,35,45],DataY=[1,2,3,4],NSpec=1)
        ConjoinWorkspaces(cls.ws2d_point_uneven,wp,CheckOverlapping=False)
        cls.ws2d_point_uneven=mantid.mtd['ws2d_point_uneven']
        cls.ws2d_histo_uneven=CreateWorkspace(DataX=[10,20,30,40],
                                              DataY=[1,2,3],
                                              NSpec=1,
                                              OutputWorkspace='ws2d_histo_uneven')
        wp=CreateWorkspace(DataX=[15,25,35,45,55],DataY=[1,2,3,4],NSpec=1)
        ConjoinWorkspaces(cls.ws2d_histo_uneven,wp,CheckOverlapping=False)
        cls.ws2d_histo_uneven=mantid.mtd['ws2d_histo_uneven']
        newYAxis=mantid.api.NumericAxis.create(3)
        newYAxis.setValue(0,10)
        newYAxis.setValue(1,15)
        newYAxis.setValue(2,25)
        cls.ws2d_histo_uneven.replaceAxis(1,newYAxis)

    @classmethod
    def tearDownClass(cls):
        config['graph1d.autodistribution']=cls.g1da
        DeleteWorkspace('ws2d_histo')
        DeleteWorkspace('ws2d_point')
        DeleteWorkspace('ws1d_point')
        DeleteWorkspace('ws_MD_2d')
        DeleteWorkspace('ws_MD_1d')
        DeleteWorkspace('ws2d_histo_rag')
        DeleteWorkspace('ws2d_point_rag')
        DeleteWorkspace('ws2d_point_uneven')
        DeleteWorkspace('ws2d_histo_uneven')

    def test_getWkspIndexDistAndLabel(self):
        #fail case
        self.assertRaises(RuntimeError,funcs._getWkspIndexDistAndLabel,self.ws2d_histo)
        #get info from a 2d workspace
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(self.ws2d_histo,specNum=2)
        self.assertEqual(index,1)
        self.assertTrue(dist)
        self.assertEqual(kwargs['label'],'spec 2')
        #get info from default spectrum in the 1d case
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(self.ws1d_point)
        self.assertEqual(index,0)
        self.assertFalse(dist)
        self.assertEqual(kwargs['label'],'spec 1')

    def test_getAxesLabels(self):
        axs=funcs.getAxesLabels(self.ws2d_histo)
        self.assertEqual(axs,('', 'Wavelength ($\\AA$)', 'Energy transfer ($meV$)'))

    def test_getAxesLabeld_MDWS(self):
        axs=funcs.getAxesLabels(self.ws_MD_2d)
        #should get the first two dimension labels only
        self.assertEqual(axs,('Intensity', 'Dim1 ($\\AA^{-1}$)', 'Dim2 (EnergyTransfer)'))

    def test_getDataUnevenFlag(self):
        flag,kwargs=funcs._getDataUnevenFlag(self.ws2d_histo_rag, axisaligned=True, other_kwarg=1)
        self.assertTrue(flag)
        self.assertEquals(kwargs,{'other_kwarg':1})
        flag,kwargs=funcs._getDataUnevenFlag(self.ws2d_histo_rag, other_kwarg=2)
        self.assertFalse(flag)
        self.assertEquals(kwargs,{'other_kwarg':2})
        flag,kwargs=funcs._getDataUnevenFlag(self.ws2d_histo_uneven, axisaligned=False, other_kwarg=3)
        self.assertTrue(flag)
        self.assertEquals(kwargs,{'other_kwarg':3})

    def test_boundaries_from_points(self):
        centers=np.array([1.,2.,4.,8.])
        bounds=funcs.boundaries_from_points(centers)
        self.assertTrue(np.array_equal(bounds,np.array([0.5,1.5,3,6,10])))

    def test_points_from_boundaries(self):
        bounds=np.array([1.,3,4,10])
        centers=funcs.points_from_boundaries(bounds)
        self.assertTrue(np.array_equal(centers,np.array([2.,3.5,7])))

    def test_getSpectrum(self):
        #get data divided by bin width
        x,y,dy,dx=funcs._getSpectrum(self.ws2d_histo, 1, False,withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x,np.array([15.,25.])))
        self.assertTrue(np.array_equal(y,np.array([.4,.5])))
        self.assertTrue(np.array_equal(dy,np.array([.3,.4])))
        self.assertEqual(dx,None)
        #get data not divided by bin width
        x,y,dy,dx=funcs._getSpectrum(self.ws2d_histo, 0, True, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x,np.array([15.,25.])))
        self.assertTrue(np.array_equal(y,np.array([2,3])))
        self.assertTrue(np.array_equal(dy,np.array([1,2])))
        self.assertEqual(dx,None)
        #fail case - try to find spectrum out of range
        self.assertRaises(RuntimeError,funcs._getSpectrum, self.ws2d_histo, 10, True)

    def test_getMDData2D_bin_bounds(self):
        x,y,data=funcs._getMDData2D_bin_bounds(self.ws_MD_2d,mantid.api.MDNormalization.NoNormalization)
        #logger.error(str(coords))
        np.testing.assert_allclose(x,np.array([-3,-1.8,-0.6,0.6,1.8,3]),atol=1e-10)
        np.testing.assert_allclose(y,np.array([-10,-6,-2,2,6,10.]),atol=1e-10)
        np.testing.assert_allclose(data,np.arange(25).reshape(5,5),atol=1e-10)

    def test_getMDData2D_bin_centers(self):
        x,y,data=funcs._getMDData2D_bin_centers(self.ws_MD_2d,mantid.api.MDNormalization.NumEventsNormalization)
        np.testing.assert_allclose(x,np.array([-2.4,-1.2,0,1.2,2.4]),atol=1e-10)
        np.testing.assert_allclose(y,np.array([-8,-4,0,4,8]),atol=1e-10)
        np.testing.assert_allclose(data,np.arange(25).reshape(5,5)*0.1,atol=1e-10)

    def test_getMDData1D(self):
        coords,data,err=funcs._getMDData1D(self.ws_MD_1d,mantid.api.MDNormalization.NumEventsNormalization)
        np.testing.assert_allclose(coords,np.array([-8,-4,0,4,8]),atol=1e-10)

    def test_getMatrix2DData_rect(self):
        #contour from aligned point data
        x,y,z=funcs._getMatrix2DData(self.ws2d_point,True,histogram2D=False)
        np.testing.assert_allclose(x,np.array([[1,2,3,4],[1,2,3,4],[1,2,3,4]]))
        np.testing.assert_allclose(y,np.array([[1,1,1,1],[2,2,2,2],[3,3,3,3]]))
        #mesh from aligned point data
        x,y,z=funcs._getMatrix2DData(self.ws2d_point,True,histogram2D=True)
        np.testing.assert_allclose(x,np.array([[0.5,1.5,2.5,3.5,4.5],[0.5,1.5,2.5,3.5,4.5],[0.5,1.5,2.5,3.5,4.5],[0.5,1.5,2.5,3.5,4.5]]))
        np.testing.assert_allclose(y,np.array([[0.5,0.5,0.5,0.5,0.5],[1.5,1.5,1.5,1.5,1.5],[2.5,2.5,2.5,2.5,2.5],[3.5,3.5,3.5,3.5,3.5]]))        
        #contour from aligned histo data
        x,y,z=funcs._getMatrix2DData(self.ws2d_histo,True,histogram2D=False)
        np.testing.assert_allclose(x,np.array([[15,25],[15,25]]))
        np.testing.assert_allclose(y,np.array([[5,5],[7,7]]))
        #mesh from aligned histo data
        x,y,z=funcs._getMatrix2DData(self.ws2d_histo,True,histogram2D=True)
        np.testing.assert_allclose(x,np.array([[10,20,30],[10,20,30],[10,20,30]]))
        np.testing.assert_allclose(y,np.array([[4,4,4],[6,6,6],[8,8,8]]))

    def test_getMatrix2DData_rag(self):
        #contour from ragged point data
        x,y,z=funcs._getMatrix2DData(self.ws2d_point_rag,True,histogram2D=False)
        np.testing.assert_allclose(x,np.array([[1,2,3,4],[2,4,6,8]]))
        np.testing.assert_allclose(y,np.array([[1,1,1,1],[2,2,2,2]]))
        #contour from ragged histo data
        x,y,z=funcs._getMatrix2DData(self.ws2d_histo_rag,True,histogram2D=False)
        np.testing.assert_allclose(x,np.array([[1.5,2.5,3.5,4.5],[3,5,7,9]]))
        np.testing.assert_allclose(y,np.array([[6,6,6,6],[8,8,8,8]]))
        #mesh from ragged point data
        x,y,z=funcs._getMatrix2DData(self.ws2d_point_rag,True,histogram2D=True)
        np.testing.assert_allclose(x,np.array([[0.5,1.5,2.5,3.5,4.5],[1,3,5,7,9],[1,3,5,7,9]]))
        np.testing.assert_allclose(y,np.array([[0.5,0.5,0.5,0.5,0.5],[1.5,1.5,1.5,1.5,1.5],[2.5,2.5,2.5,2.5,2.5]]))
        #mesh from ragged histo data
        x,y,z=funcs._getMatrix2DData(self.ws2d_histo_rag,True,histogram2D=True)
        np.testing.assert_allclose(x,np.array([[1,2,3,4,5],[2,4,6,8,10],[2,4,6,8,10]]))
        np.testing.assert_allclose(y,np.array([[5,5,5,5,5],[7,7,7,7,7],[9,9,9,9,9]]))
        #check that fails for uneven data
        self.assertRaises(ValueError,funcs._getMatrix2DData, self.ws2d_point_uneven,True)

    def test_getUnevenData(self):
        #even points
        x,y,z=funcs._getUnevenData(self.ws2d_point_rag,True)
        np.testing.assert_allclose(x[0],np.array([0.5,1.5,2.5,3.5,4.5]))
        np.testing.assert_allclose(x[1],np.array([1,3,5,7,9]))
        np.testing.assert_allclose(y[0],np.array([0.5,1.5]))
        np.testing.assert_allclose(y[1],np.array([1.5,2.5]))
        np.testing.assert_allclose(z[0],np.array([2,2,2,2]))
        np.testing.assert_allclose(z[1],np.array([2,2,2,2]))
        #even histo
        x,y,z=funcs._getUnevenData(self.ws2d_histo_rag,True)
        np.testing.assert_allclose(x[0],np.array([1,2,3,4,5]))
        np.testing.assert_allclose(x[1],np.array([2,4,6,8,10]))
        np.testing.assert_allclose(y[0],np.array([5,7]))
        np.testing.assert_allclose(y[1],np.array([7,9]))
        np.testing.assert_allclose(z[0],np.array([2,2,2,2]))
        np.testing.assert_allclose(z[1],np.array([2,2,2,2]))
        #uneven points
        x,y,z=funcs._getUnevenData(self.ws2d_point_uneven,True)
        np.testing.assert_allclose(x[0],np.array([5,15,25,35]))
        np.testing.assert_allclose(x[1],np.array([10,20,30,40,50]))
        np.testing.assert_allclose(y[0],np.array([0.5,1.5]))
        np.testing.assert_allclose(y[1],np.array([1.5,2.5]))
        np.testing.assert_allclose(z[0],np.array([1,2,3]))
        np.testing.assert_allclose(z[1],np.array([1,2,3,4]))
        #uneven histo
        x,y,z=funcs._getUnevenData(self.ws2d_histo_uneven,True)
        np.testing.assert_allclose(x[0],np.array([10,20,30,40]))
        np.testing.assert_allclose(x[1],np.array([15,25,35,45,55]))
        np.testing.assert_allclose(y[0],np.array([10,15]))
        np.testing.assert_allclose(y[1],np.array([15,25]))
        np.testing.assert_allclose(z[0],np.array([1,2,3]))
        np.testing.assert_allclose(z[1],np.array([1,2,3,4]))

    def test_1dplots(self):
        fig, ax = plt.subplots()
        funcs.plot(ax,self.ws2d_histo,'rs',specNum=1)
        funcs.plot(ax,self.ws2d_histo,specNum=2,linewidth=6)
        funcs.plot(ax,self.ws_MD_1d,'bo')

    def test_1derrorbars(self):
        fig, ax = plt.subplots()
        funcs.errorbar(ax,self.ws2d_histo,'rs',specNum=1)
        funcs.errorbar(ax,self.ws2d_histo,specNum=2,linewidth=6)
        funcs.errorbar(ax,self.ws_MD_1d,'bo')

    def test_1dscatter(self):
        fig, ax = plt.subplots()
        funcs.scatter(ax,self.ws2d_histo,specNum=1)
        funcs.scatter(ax,self.ws2d_histo,specNum=2)
        funcs.scatter(ax,self.ws_MD_1d)

    def test_2dcontours(self):
        fig, ax = plt.subplots()
        funcs.contour(ax,self.ws2d_histo_rag)
        funcs.contourf(ax,self.ws2d_histo,vmin=0)
        funcs.tricontour(ax,self.ws_MD_2d)
        funcs.tricontourf(ax,self.ws_MD_2d)
        self.assertRaises(ValueError,funcs.contour, ax, self.ws2d_point_uneven)

    def test_2dpcolors(self):
        fig, ax = plt.subplots()
        funcs.pcolor(ax,self.ws2d_histo_rag)
        funcs.tripcolor(ax,self.ws2d_histo,vmin=0)
        funcs.pcolormesh(ax,self.ws_MD_2d)
        funcs.pcolorfast(ax,self.ws2d_point_uneven,vmin=-1)

if __name__ == '__main__':
    unittest.main()
