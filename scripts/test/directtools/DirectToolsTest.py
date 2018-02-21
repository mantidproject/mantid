from __future__ import (absolute_import, division, print_function)

import directtools
from mantid.api import mtd
from mantid.simpleapi import LoadILLTOF, CreateSampleWorkspace, CreateWorkspace
import matplotlib
import numpy
import numpy.testing
import testhelpers
import unittest


class DirectTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def test_box2D(self):
        xs = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), (3, 1))
        vertAxis = numpy.array([-2, -1, 0, 1])
        box = directtools.box2D(xs, vertAxis)
        numpy.testing.assert_equal(xs[box], xs)
        box = directtools.box2D(xs, vertAxis, horMin=0)
        expected = numpy.tile(numpy.array([0, 2, 4, 5]), (3, 1))
        numpy.testing.assert_equal(xs[box], expected)
        box = directtools.box2D(xs, vertAxis, horMax=5)
        expected = numpy.tile(numpy.array([-1, 0, 2, 4]), (3, 1))
        numpy.testing.assert_equal(xs[box], expected)
        box = directtools.box2D(xs, vertAxis, vertMin=-1)
        expected = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), (2, 1))
        numpy.testing.assert_equal(xs[box], expected)
        box = directtools.box2D(xs, vertAxis, vertMax=-1)
        expected = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), (1, 1))
        numpy.testing.assert_equal(xs[box], expected)

    def test_configurematplotlib(self):
        defaultParams = directtools.defaultrcParams()
        directtools.configurematplotlib(defaultParams)
        for key in defaultParams:
            self.assertTrue(key in matplotlib.rcParams)
            self.assertEqual(matplotlib.rcParams[key], defaultParams[key])

    def test_defaultrcParams(self):
        result = directtools.defaultrcParams()
        self.assertTrue(isinstance(result, dict))
        self.assertEqual(len(result), 2)
        self.assertTrue('legend.numpoints' in result)
        self.assertEqual(result['legend.numpoints'], 1)
        self.assertTrue('text.usetex' in result)
        self.assertEqual(result['text.usetex'], True)

    def test_dynamicsusceptibility(self):
        xs = numpy.array([-1, 0, 1])
        ys = numpy.array([1, 1])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=1, UnitX='DeltaE', StoreInADS=False)
        wsOut = directtools.dynamicsusceptibility(ws, 100.)
        self.assertEqual(wsOut.YUnit(), 'Dynamic susceptibility')
        xs = numpy.array([0, 1, 0, 1])
        ys = numpy.array([1, 1])
        vertX = numpy.array([-1, 1])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2, UnitX='MomentumTransfer', VerticalAxisUnit='DeltaE', VerticalAxisValues=vertX,
                             StoreInADS=False)
        wsOut = directtools.dynamicsusceptibility(ws, 100.)
        self.assertEqual(wsOut.YUnit(), 'Dynamic susceptibility')

    def test_mantidsubplotsetup(self):
        result = directtools.mantidsubplotsetup()
        self.assertEqual(len(result), 1)
        self.assertEqual(result['projection'], 'mantid')

    def test_nanminmax(self):
        xs = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), 3)
        ys = numpy.linspace(-5, 3, 4 * 3)
        vertAxis = numpy.array([-3, -1, 2, 4])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=3, VerticalAxisUnit='Degrees', VerticalAxisValues=vertAxis, StoreInADS=False)
        cMin, cMax = directtools.nanminmax(ws)
        self.assertEqual(cMin, ys[0])
        self.assertEqual(cMax, ys[-1])
        cMin, cMax = directtools.nanminmax(ws, horMin=0)
        self.assertEqual(cMin, ys[1])
        self.assertEqual(cMax, ys[-1])
        cMin, cMax = directtools.nanminmax(ws, horMax=4)
        self.assertEqual(cMin, ys[0])
        self.assertEqual(cMax, ys[-2])
        cMin, cMax = directtools.nanminmax(ws, vertMin=-1)
        self.assertEqual(cMin, ys[4])
        self.assertEqual(cMax, ys[-1])
        cMin, cMax = directtools.nanminmax(ws, vertMax=2)
        self.assertEqual(cMin, ys[0])
        self.assertEqual(cMax, ys[-5])

    def test_plotconstE(self):
        ws = LoadILLTOF('ILL/IN4/084446.nxs')
        kwargs = {
            'workspaces': ws,
            'E' : 13.,
            'dE' : 1.5
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)
        kwargs = {
            'workspaces': [ws, ws],
            'E' : 13.,
            'dE' : 1.5,
            'style' : 'l'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)
        kwargs = {
            'workspaces': ws,
            'E' : [13., 23.],
            'dE' : 1.5,
            'style' : 'm'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)
        kwargs = {
            'workspaces': ws,
            'E' : 13.,
            'dE' : [1.5, 15.],
            'style' : 'lm'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)

    def test_plotconstQ(self):
        ws = LoadILLTOF('ILL/IN4/084446.nxs')
        kwargs = {
            'workspaces': ws,
            'Q' : 523.,
            'dQ' : 42.
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)
        kwargs = {
            'workspaces': [ws, ws],
            'Q' : 523.,
            'dQ' : 42.,
            'style' : 'l'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)
        kwargs = {
            'workspaces': ws,
            'Q' : [472., 623.],
            'dQ' : 42.,
            'style' : 'm'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)
        kwargs = {
            'workspaces': ws,
            'Q' : 523.,
            'dQ' : [17., 2.],
            'style' : 'ml'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)

    def test_plotSofQW(self):
        ws = LoadILLTOF('ILL/IN4/084446.nxs')
        kwargs = {'workspace': 'ws'}
        testhelpers.assertRaisesNothing(self, directtools.plotSofQW, **kwargs)
        kwargs = {'workspace': ws}
        testhelpers.assertRaisesNothing(self, directtools.plotSofQW, **kwargs)

    def test_subplots(self):
        testhelpers.assertRaisesNothing(self, directtools.subplots)

    def test_validQ(self):
        xs = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), 3)
        nPoints = 4
        ys = numpy.tile(numpy.zeros(nPoints), 3)
        ys[nPoints] = numpy.nan
        ys[2 * nPoints - 1] = numpy.nan
        vertAxis = numpy.array([-3, -1, 2, 4])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=3, VerticalAxisUnit='Degrees', VerticalAxisValues=vertAxis, StoreInADS=False)
        qMin, qMax = directtools.validQ(ws, -2.5)
        self.assertEqual(qMin, xs[0])
        self.assertEqual(qMax, xs[-1])
        qMin, qMax = directtools.validQ(ws, 0)
        self.assertEqual(qMin, xs[1])
        self.assertEqual(qMax, xs[-2])

    def test_wsreport(self):
        ws = LoadILLTOF('ILL/IN4/084446.nxs')
        kwargs = {'workspace': ws}
        testhelpers.assertRaisesNothing(self, directtools.wsreport, **kwargs)

    def test_SampleLogs(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1)
        ws.mutableRun().addProperty('a', 7, True)
        ws.mutableRun().addProperty('b.c', 13, True)
        logs = directtools.SampleLogs(ws)
        self.assertTrue(hasattr(logs, 'a'))
        self.assertEqual(logs.a, 7)
        self.assertTrue(hasattr(logs, 'b'))
        self.assertTrue(hasattr(logs.b, 'c'))
        self.assertEqual(logs.b.c, 13)


if __name__ == '__main__':
    unittest.main()
