from __future__ import (absolute_import, division, print_function)

import directtools
from mantid.simpleapi import CreateWorkspace
import numpy
import numpy.testing
import unittest

class DirectTest(unittest.TestCase):
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

if __name__ == '__main__':
    unittest.main()
