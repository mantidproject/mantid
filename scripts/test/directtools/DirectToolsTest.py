# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

# Set matplotlib backend to AGG before anything else. Otherwise some build servers
# need to have extra packages (tkinter) installed.
import matplotlib
matplotlib.use('AGG')

import directtools
from mantid.api import mtd
from mantid.simpleapi import (AddSampleLog, CloneWorkspace, ComputeIncoherentDOS, ConvertSpectrumAxis, CreateSampleWorkspace,
                              CreateWorkspace, DirectILLCollectData, DeleteWorkspace, DirectILLReduction, LoadILLTOF,
                              MoveInstrumentComponent, SetInstrumentParameter)
import numpy
import numpy.testing
import testhelpers
import unittest


class DirectToolsTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        workspace = DirectILLCollectData('ILL/IN4/084446.nxs', EPPCreationMethod='Calculate EPP',
                                         IncidentEnergyCalibration='Energy Calibration OFF',
                                         FlatBkg='Flat Bkg OFF', Normalisation='Normalisation OFF',
                                         StoreInADS=False)
        cls._sqw = DirectILLReduction(workspace, OutputWorkspace='unused', StoreInADS=False)

    def tearDown(self):
        mtd.clear()

    def _box2DSetup(self):
        xs = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), (3, 1))
        vertAxis = numpy.array([-2, -1, 0, 1])
        return xs, vertAxis

    def test_box2D_defaults(self):
        xs, vertAxis = self._box2DSetup()
        vertAxis = numpy.array([-2, -1, 0, 1])
        box = directtools.box2D(xs, vertAxis)
        numpy.testing.assert_equal(xs[box], xs)

    def test_box2D_horMin(self):
        xs, vertAxis = self._box2DSetup()
        box = directtools.box2D(xs, vertAxis, horMin=0)
        expected = numpy.tile(numpy.array([0, 2, 4, 5]), (3, 1))
        numpy.testing.assert_equal(xs[box], expected)

    def test_box2D_horMax(self):
        xs, vertAxis = self._box2DSetup()
        box = directtools.box2D(xs, vertAxis, horMax=5)
        expected = numpy.tile(numpy.array([-1, 0, 2, 4]), (3, 1))
        numpy.testing.assert_equal(xs[box], expected)

    def test_box2D_vertMin(self):
        xs, vertAxis = self._box2DSetup()
        box = directtools.box2D(xs, vertAxis, vertMin=-1)
        expected = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), (2, 1))
        numpy.testing.assert_equal(xs[box], expected)

    def test_box2D_vertMax(self):
        xs, vertAxis = self._box2DSetup()
        box = directtools.box2D(xs, vertAxis, vertMax=-1)
        expected = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), (1, 1))
        numpy.testing.assert_equal(xs[box], expected)

    def test_configurematplotlib(self):
        defaultParams = directtools.defaultrcparams()
        directtools._configurematplotlib(defaultParams)
        for key in defaultParams:
            self.assertTrue(key in matplotlib.rcParams)
            self.assertEqual(matplotlib.rcParams[key], defaultParams[key])

    def test_defaultrcParams(self):
        result = directtools.defaultrcparams()
        self.assertEqual(result, {'legend.numpoints': 1})

    def test_dynamicsusceptibility(self):
        xs = numpy.array([-1, 0, 1])
        ys = numpy.array([1, 1])
        vertX = numpy.array([-1, 1])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=1, UnitX='DeltaE', VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=vertX,
                             StoreInADS=False)
        wsOut = directtools.dynamicsusceptibility(ws, 100.)
        self.assertEqual(wsOut.YUnitLabel(), 'Dynamic susceptibility')
        xs = numpy.array([0, 1, 0, 1])
        ys = numpy.array([1, 1])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=2, UnitX='MomentumTransfer', VerticalAxisUnit='DeltaE', VerticalAxisValues=vertX,
                             StoreInADS=False)
        wsOut = directtools.dynamicsusceptibility(ws, 100.)
        self.assertEqual(wsOut.YUnitLabel(), 'Dynamic susceptibility')

    def test_dynamicsusceptibility_removesingularity(self):
        xs = numpy.array([-0.7, -0.4, -0.1, 0.2, 0.5])
        ys = numpy.array([2, 2, 2, 2])
        es = numpy.sqrt(ys)
        vertX = numpy.array([-1, 1])
        ws = CreateWorkspace(DataX=xs, DataY=ys, DataE=es, NSpec=1, UnitX='DeltaE', VerticalAxisUnit='MomentumTransfer',
                             VerticalAxisValues=vertX, StoreInADS=False)
        wsOut = directtools.dynamicsusceptibility(ws, 100., zeroEnergyEpsilon=0.13)
        numpy.testing.assert_equal(wsOut.readX(0), xs)
        outYs = wsOut.readY(0)
        outEs = wsOut.readE(0)
        self.assertEqual(outYs[2], 0.)
        self.assertEqual(outEs[2], 0.)

    def test_mantidsubplotsetup(self):
        result = directtools._mantidsubplotsetup()
        self.assertEqual(result, {'projection': 'mantid'})

    def _nanminmaxSetup(self):
        xs = numpy.tile(numpy.array([-1, 0, 2, 4, 5]), 3)
        ys = numpy.linspace(-5, 3, 4 * 3)
        vertAxis = numpy.array([-3, -1, 2, 4])
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=3, VerticalAxisUnit='Degrees', VerticalAxisValues=vertAxis, StoreInADS=False)
        return ws

    def test_nanminmax_defaults(self):
        ws = self._nanminmaxSetup()
        ys = ws.extractY()
        cMin, cMax = directtools.nanminmax(ws)
        self.assertEqual(cMin, ys[0, 0])
        self.assertEqual(cMax, ys[2, -1])

    def test_nanminmax_horMin(self):
        ws = self._nanminmaxSetup()
        ys = ws.extractY()
        cMin, cMax = directtools.nanminmax(ws, horMin=0)
        self.assertEqual(cMin, ys[0, 1])
        self.assertEqual(cMax, ys[2, -1])

    def test_nanminmax_horMax(self):
        ws = self._nanminmaxSetup()
        ys = ws.extractY()
        cMin, cMax = directtools.nanminmax(ws, horMax=4)
        self.assertEqual(cMin, ys[0, 0])
        self.assertEqual(cMax, ys[2, -2])

    def test_nanminmax_vertMin(self):
        ws = self._nanminmaxSetup()
        ys = ws.extractY()
        cMin, cMax = directtools.nanminmax(ws, vertMin=-1)
        self.assertEqual(cMin, ys[1, 0])
        self.assertEqual(cMax, ys[2, -1])

    def test_nanminmax_vertMax(self):
        ws = self._nanminmaxSetup()
        ys = ws.extractY()
        cMin, cMax = directtools.nanminmax(ws, vertMax=2)
        self.assertEqual(cMin, ys[0, 0])
        self.assertEqual(cMax, ys[1, -1])

    def test_plotconstE_nonListArgsExecutes(self):
        kwargs = {
            'workspaces': self._sqw,
            'E' : -1.,
            'dE' : 1.5
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)

    def test_plotconstE_wsListExecutes(self):
        kwargs = {
            'workspaces': [self._sqw, self._sqw],
            'E' : -2.,
            'dE' : 1.5,
            'style' : 'l'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)

    def test_plotconstE_EListExecutes(self):
        kwargs = {

            'workspaces': self._sqw,
            'E' : [-3., 4.],
            'dE' : 1.5,
            'style' : 'm'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)

    def test_plotconstE_dEListExecutes(self):
        kwargs = {
            'workspaces': self._sqw,
            'E' : 3.,
            'dE' : [1.5, 15.],
            'style' : 'lm'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)

    def test_plotconstE_loglog(self):
        kwargs = {
            'workspaces': self._sqw,
            'E' : -10.,
            'dE' : 1.5,
            'xscale': 'log',
            'yscale': 'log'
        }
        figure, axes, cuts = testhelpers.assertRaisesNothing(self, directtools.plotconstE, **kwargs)
        self.assertEquals(axes.get_xscale(), 'log')
        self.assertEquals(axes.get_yscale(), 'log')

    def test_plotconstQ_nonListArgsExecutes(self):
        kwargs = {
            'workspaces': self._sqw,
            'Q' : 2.3,
            'dQ' : 0.3
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)

    def test_plotconstQ_wsListExecutes(self):
        kwargs = {
            'workspaces': [self._sqw, self._sqw],
            'Q' : 2.4,
            'dQ' : 0.42,
            'style' : 'l'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)

    def test_plotconstQ_QListExecutes(self):
        kwargs = {
            'workspaces': self._sqw,
            'Q' : [1.8, 3.1],
            'dQ' : 0.32,
            'style' : 'm'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)

    def test_plotconstQ_dQListExecutes(self):
        kwargs = {
            'workspaces': self._sqw,
            'Q' : 1.9,
            'dQ' : [0.2, 0.4],
            'style' : 'ml'
        }
        testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)

    def test_plotconstQ_loglog(self):
        kwargs = {
            'workspaces': self._sqw,
            'Q' : 2.6,
            'dQ' : 0.1,
            'xscale': 'log',
            'yscale': 'log'
        }
        figure, axes, cuts = testhelpers.assertRaisesNothing(self, directtools.plotconstQ, **kwargs)
        self.assertEquals(axes.get_xscale(), 'log')
        self.assertEquals(axes.get_yscale(), 'log')

    def test_plotconstE_and_plotconstQ_plot_equal_value_at_crossing(self):
        Q = 2.512
        figure, axes, cuts = directtools.plotconstQ(self._sqw, Q, 0.01)
        lineDataQ = axes.get_lines()[0].get_data()
        E = 2.2
        figure, axes, cuts = directtools.plotconstE(self._sqw, E, 0.01)
        lineDataE = axes.get_lines()[0].get_data()
        indexE = numpy.argmin(numpy.abs(lineDataQ[0] - E))
        indexQ = numpy.argmin(numpy.abs(lineDataE[0] - Q))
        self.assertEquals(lineDataQ[1][indexE], lineDataE[1][indexQ])

    def test_plotcuts_keepCutWorkspaces(self):
        kwargs = {
            'direction' : 'Vertical',
            'workspaces' : self._sqw,
            'cuts' : 1.9,
            'widths': 0.8,
            'quantity': 'TOF',
            'unit': 'microseconds',
            'keepCutWorkspaces': True
        }
        self.assertEquals(mtd.size(), 0)
        figure, axes, cuts = testhelpers.assertRaisesNothing(self, directtools.plotcuts, **kwargs)
        self.assertEquals(len(cuts), 1)
        self.assertEquals(mtd.size(), 1)

    def test_plotcuts_doNotKeepCutWorkspaces(self):
        kwargs = {
            'direction' : 'Vertical',
            'workspaces' : self._sqw,
            'cuts' : 2.0,
            'widths': 0.7,
            'quantity': 'TOF',
            'unit': 'microseconds',
            'keepCutWorkspaces': False
        }
        self.assertEquals(mtd.size(), 0)
        figure, axes, cuts = testhelpers.assertRaisesNothing(self, directtools.plotcuts, **kwargs)
        self.assertEquals(len(cuts), 0)
        self.assertEquals(mtd.size(), 0)

    def test_plotcuts_loglog(self):
        kwargs = {
            'direction' : 'Vertical',
            'workspaces' : self._sqw,
            'cuts' : 2.1,
            'widths': 0.6,
            'quantity': 'TOF',
            'unit': 'microseconds',
            'xscale': 'log',
            'yscale': 'log'
        }
        self.assertEquals(mtd.size(), 0)
        figure, axes, cuts = testhelpers.assertRaisesNothing(self, directtools.plotcuts, **kwargs)
        self.assertEquals(axes.get_xscale(), 'log')
        self.assertEquals(axes.get_yscale(), 'log')

    def test_plotprofiles_noXUnitsExecutes(self):
        xs = numpy.linspace(-3., 10., 12)
        ys = numpy.tile(1., len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=1, StoreInADS=False)
        kwargs = {'workspaces': ws}
        figure, axes = testhelpers.assertRaisesNothing(self, directtools.plotprofiles, **kwargs)
        self.assertEquals(axes.get_xlabel(), '')
        self.assertEquals(axes.get_ylabel(), r'$S(Q,E)$')
        numpy.testing.assert_equal(axes.get_lines()[0].get_data()[0], (xs[1:] + xs[:-1])/2)
        numpy.testing.assert_equal(axes.get_lines()[0].get_data()[1], ys)

    def test_plotprofiles_DeltaEXUnitsExecutes(self):
        xs = numpy.linspace(-3., 10., 12)
        ys = numpy.tile(1., len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=1, UnitX='DeltaE', StoreInADS=False)
        kwargs = {'workspaces': ws}
        figure, axes = testhelpers.assertRaisesNothing(self, directtools.plotprofiles, **kwargs)
        self.assertEquals(axes.get_xlabel(), 'Energy (meV)')
        self.assertEquals(axes.get_ylabel(), r'$S(Q,E)$')
        numpy.testing.assert_equal(axes.get_lines()[0].get_data()[0], (xs[1:] + xs[:-1])/2)
        numpy.testing.assert_equal(axes.get_lines()[0].get_data()[1], ys)

    def test_plotprofiles_MomentumTransferXUnitsExecutes(self):
        xs = numpy.linspace(-3., 10., 12)
        ys = numpy.tile(1., len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=1, UnitX='MomentumTransfer', StoreInADS=False)
        kwargs = {'workspaces': ws}
        figure, axes = testhelpers.assertRaisesNothing(self, directtools.plotprofiles, **kwargs)
        self.assertEquals(axes.get_xlabel(), u'$Q$ (\u00c5$^{-1}$)')
        self.assertEquals(axes.get_ylabel(), '$S(Q,E)$')
        numpy.testing.assert_equal(axes.get_lines()[0].get_data()[0], (xs[1:] + xs[:-1])/2)
        numpy.testing.assert_equal(axes.get_lines()[0].get_data()[1], ys)

    def test_plotprofiles_loglog(self):
        xs = numpy.linspace(-3., 10., 12)
        ys = numpy.tile(1., len(xs) - 1)
        ws = CreateWorkspace(DataX=xs, DataY=ys, NSpec=1, UnitX='MomentumTransfer', StoreInADS=False)
        kwargs = {'workspaces': ws, 'xscale': 'log', 'yscale': 'log'}
        figure, axes = testhelpers.assertRaisesNothing(self, directtools.plotprofiles, **kwargs)
        self.assertEquals(axes.get_xscale(), 'log')
        self.assertEquals(axes.get_yscale(), 'log')

    def test_plotDOS_PlotSingle(self):
        ws = CreateSampleWorkspace(NumBanks=1, XUnit='DeltaE', XMin=-12., XMax=12., BinWidth=0.2, StoreInADS=False)
        MoveInstrumentComponent(ws, 'bank1', X=-0.5, StoreInADS=False)
        ws = ConvertSpectrumAxis(ws, 'Theta', 'Direct', 14., StoreInADS=False)
        SetInstrumentParameter(ws, ParameterName='deltaE-mode', Value='direct', StoreInADS=False)
        AddSampleLog(ws, LogName='Ei', LogText=str(14.), LogType='Number', LogUnit='meV', StoreInADS=False)
        stw = ComputeIncoherentDOS(ws, StoreInADS=False)
        kwargs = {'workspaces': stw}
        figure, axes = testhelpers.assertRaisesNothing(self, directtools.plotDOS, **kwargs)
        self.assertEquals(axes.get_xlabel(), u'Energy transfer ($meV$)')
        self.assertEquals(axes.get_ylabel(), u'$g(E)$')

    def test_plotDOS_PlotMultiple(self):
        ws = CreateSampleWorkspace(NumBanks=1, XUnit='DeltaE', XMin=-12., XMax=12., BinWidth=0.2, StoreInADS=False)
        MoveInstrumentComponent(ws, 'bank1', X=-0.5, StoreInADS=False)
        ws = ConvertSpectrumAxis(ws, 'Theta', 'Direct', 14., StoreInADS=False)
        SetInstrumentParameter(ws, ParameterName='deltaE-mode', Value='direct', StoreInADS=False)
        AddSampleLog(ws, LogName='Ei', LogText=str(14.), LogType='Number', LogUnit='meV', StoreInADS=False)
        stw = ComputeIncoherentDOS(ws)
        kwargs = {'workspaces': [stw, 'stw']}
        figure, axes = testhelpers.assertRaisesNothing(self, directtools.plotDOS, **kwargs)
        self.assertEquals(axes.get_xlabel(), u'Energy transfer ($meV$)')
        self.assertEquals(axes.get_ylabel(), u'$g(E)$')

    def test_plotSofQW(self):
        wsName = 'ws'
        CloneWorkspace(self._sqw, OutputWorkspace=wsName)
        kwargs = {'workspace': wsName}
        testhelpers.assertRaisesNothing(self, directtools.plotSofQW, **kwargs)
        DeleteWorkspace(wsName)
        kwargs = {'workspace': self._sqw}
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
        testhelpers.assertRaisesNothing(self, directtools.wsreport, **{'workspace': self._sqw})
        in5WS = LoadILLTOF('ILL/IN5/104007.nxs', StoreInADS=False)
        testhelpers.assertRaisesNothing(self, directtools.wsreport, **{'workspace': in5WS})
        in6WS = LoadILLTOF('ILL/IN6/164192.nxs', StoreInADS=False)
        testhelpers.assertRaisesNothing(self, directtools.wsreport, **{'workspace': in6WS})

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
