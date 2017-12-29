from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CreateWorkspace,DeleteWorkspace,CreateMDHistoWorkspace
import mantid.plots._functions as funcs
import mantid.api
import numpy as np
from mantid.kernel import logger,config

class PlotsFunctionsTest(unittest.TestCase):

    def setUp(self):
        self.g1da=config['graph1d.autodistribution']
        config['graph1d.autodistribution']='On'
        dataX=[10,20,30,10,20,30]
        dataY=[2,3,4,5]
        dataE=[1,2,3,4]
        self.ws2d_histo = CreateWorkspace(DataX=dataX,
                                          DataY=dataY,
                                          DataE=dataE,
                                          NSpec=2,
                                          Distribution=True,
                                          UnitX='Wavelength',
                                          VerticalAxisUnit='DeltaE',
                                          VerticalAxisValues=[4,6],
                                          OutputWorkspace='ws2d_histo')
        dataX_1=[1,2]
        dataY_1=[1,2]
        self.ws1d_point = CreateWorkspace(DataX=dataX_1,
                                          DataY=dataY_1,
                                          NSpec=1,
                                          Distribution=False,
                                          OutputWorkspace='ws1d_point')
        self.ws_MD_2d = CreateMDHistoWorkspace(Dimensionality=3,
                                               Extents='-3,3,-10,10,-1,1',
                                               SignalInput=range(25),
                                               ErrorInput=range(25),
                                               NumberOfEvents=10*np.ones(25),
                                               NumberOfBins='5,5,1',
                                               Names='Dim1,Dim2,Dim3',
                                               Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                               OutputWorkspace='ws_MD_2d')

    def tearDown(self):
        config['graph1d.autodistribution']=self.g1da
        DeleteWorkspace('ws2d_histo')
        DeleteWorkspace('ws1d_point')
        DeleteWorkspace('ws_MD_2d')


    def test_getWkspIndexDistAndLabel(self):
        #fail case
        self.assertRaises(RuntimeError,funcs._getWkspIndexDistAndLabel,self.ws2d_histo)
        #get info from a 2d workspace
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(self.ws2d_histo,specNum=2)
        self.assertEquals(index,1)
        self.assertTrue(dist)
        self.assertEquals(kwargs['label'],'spec 2')
        #get info from default spectrum in the 1d case
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(self.ws1d_point)
        self.assertEquals(index,0)
        self.assertFalse(dist)
        self.assertEquals(kwargs['label'],'spec 1')

    def test_getAxesLabels(self):
        axs=funcs.getAxesLabels(self.ws2d_histo)
        self.assertEquals(axs,('', 'Wavelength ($\\AA$)', 'Energy transfer ($meV$)'))

    def test_getAxesLabeld_MDWS(self):
        axs=funcs.getAxesLabels(self.ws_MD_2d)
        #should get the first two dimension labels only
        self.assertEquals(axs,('Intensity', 'Dim1 ($\\AA^{-1}$)', 'Dim2 (EnergyTransfer)'))

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
        self.assertEquals(dx,None)
        #get data not divided by bin width
        x,y,dy,dx=funcs._getSpectrum(self.ws2d_histo, 0, True, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x,np.array([15.,25.])))
        self.assertTrue(np.array_equal(y,np.array([2,3])))
        self.assertTrue(np.array_equal(dy,np.array([1,2])))
        self.assertEquals(dx,None)
        #fail case - try to find spectrum out of range
        self.assertRaises(RuntimeError,funcs._getSpectrum, self.ws2d_histo, 10, True)

    def test_getMDData2D_bin_bounds(self):
        coords,data,err=funcs._getMDData2D_bin_bounds(self.ws_MD_2d,mantid.api.MDNormalization.NoNormalization)
        #logger.error(str(coords))
        np.testing.assert_allclose(coords[0],np.array([-3,-1.8,-0.6,0.6,1.8,3]),atol=1e-10)
        np.testing.assert_allclose(coords[1],np.array([-10,-6,-2,2,6,10.]),atol=1e-10)
        np.testing.assert_allclose(data,np.arange(25).reshape(5,5).transpose(),atol=1e-10)

    def test_getMDData2D_bin_centers(self):
        coords,data,err=funcs._getMDData2D_bin_centers(self.ws_MD_2d,mantid.api.MDNormalization.NumEventsNormalization)
        np.testing.assert_allclose(coords[0],np.array([-2.4,-1.2,0,1.2,2.4]),atol=1e-10)
        np.testing.assert_allclose(coords[1],np.array([-8,-4,0,4,8]),atol=1e-10)
        np.testing.assert_allclose(data,np.arange(25).reshape(5,5).transpose()*0.1,atol=1e-10)

    def test_getMDData1D(self):
        pass

if __name__ == '__main__':
    unittest.main()
