from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import CreateWorkspace,DeleteWorkspace
import mantid.plots._functions as funcs
import numpy as np
from mantid.kernel import logger,config

class PlotsFunctionsTest(unittest.TestCase):

    def test_getWkspIndexDistAndLabel(self):
        dataX=[1,2,3,1,2,3]
        dataY=[2,3,4,5]
        ws = CreateWorkspace(DataX=dataX, 
                             DataY=dataY,
                             NSpec=2,
                             Distribution=True)
        self.assertRaises(RuntimeError,funcs._getWkspIndexDistAndLabel,ws)
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(ws,specNum=2)
        self.assertEquals(index,1)
        self.assertTrue(dist)
        self.assertEquals(kwargs['label'],'spec 2')
        DeleteWorkspace(ws)
        ws = CreateWorkspace(DataX=[1,2],
                             DataY=[1,2],
                             NSpec=1,
                             Distribution=False)
        index, dist, kwargs = funcs._getWkspIndexDistAndLabel(ws)
        self.assertEquals(index,0)
        self.assertFalse(dist)
        self.assertEquals(kwargs['label'],'spec 1')
        DeleteWorkspace(ws)

    def test_getAxesLabels(self):
        dataX=[1,2,3,1,2,3]
        dataY=[2,3,4,5]
        ws = CreateWorkspace(DataX=dataX,
                             DataY=dataY,
                             NSpec=2,
                             Distribution=True,
                             UnitX='Wavelength',
                             VerticalAxisUnit='DeltaE',
                             VerticalAxisValues=[4,6],)
        axs=funcs.getAxesLabels(ws)
        self.assertEquals(axs,('', 'Wavelength ($\\AA$)', 'Energy transfer ($meV$)'))

    def test_boundaries_from_points(self):
        centers=np.array([1.,2.,4.,8.])
        bounds=funcs.boundaries_from_points(centers)
        self.assertTrue(np.array_equal(bounds,np.array([0.5,1.5,3,6,10])))

    def test_points_from_boundaries(self):
        bounds=np.array([1.,3,4,10])
        centers=funcs.points_from_boundaries(bounds)
        self.assertTrue(np.array_equal(centers,np.array([2.,3.5,7])))

    def test_getSpectrum(self):
        g1da=config['graph1d.autodistribution']
        config['graph1d.autodistribution']='On'
        dataX=[10,20,30,10,20,30]
        dataY=[2,3,4,5]
        dataE=[1,2,3,4]
        ws = CreateWorkspace(DataX=dataX,
                             DataY=dataY,
                             DataE=dataE,
                             NSpec=2,
                             Distribution=True)
        x,y,dy,dx=funcs._getSpectrum(ws, 1, False,withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x,np.array([15.,25.])))
        self.assertTrue(np.array_equal(y,np.array([.4,.5])))
        self.assertTrue(np.array_equal(dy,np.array([.3,.4])))
        self.assertEquals(dx,None)
        x,y,dy,dx=funcs._getSpectrum(ws, 0, True, withDy=True, withDx=True)
        self.assertTrue(np.array_equal(x,np.array([15.,25.])))
        self.assertTrue(np.array_equal(y,np.array([2,3])))
        self.assertTrue(np.array_equal(dy,np.array([1,2])))
        self.assertEquals(dx,None)
        config['graph1d.autodistribution']=g1da
        self.assertRaises(RuntimeError,funcs._getSpectrum, ws, 10, True)
        DeleteWorkspace(ws)

if __name__ == '__main__':
    unittest.main()
