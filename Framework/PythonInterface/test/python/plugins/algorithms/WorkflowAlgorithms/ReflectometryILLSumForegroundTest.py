# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import numpy
import unittest
import ReflectometryILL_common as common


class ReflectometryILLSumForegroundTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testDirectBeamSummationExecutes(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testLinePositionFromSampleLogs(self):
        # We do not run the preprocess!
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws.getAxis(0).setUnit('Wavelength')
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_add_line_position(ws, 120.6)
        ws.run().addProperty('reduction.foreground.first_workspace_index', int(140), True)
        ws.run().addProperty('reduction.foreground.centre_workspace_index', int(150), True)
        ws.run().addProperty('reduction.foreground.last_workspace_index', int(160), True)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testReflectedBeamSumInLambdaExecutes(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 6.7)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        dirForeground = alg.getProperty('OutputWorkspace').value
        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(reflWS, 1.2)
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.03, 0.02)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 6.7)
        reflWS = illhelpers.refl_preprocess('refWS', reflWS)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInLambda',
            'DirectLineWorkspace': dirWS,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testReflectedBeamSumInQExecutes(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.02, 0.03)
        dirWS = illhelpers.refl_add_line_position(dirWS, 128.0)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 5.5)
        dirWS = illhelpers.refl_preprocess('dirWS', dirWS)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        dirForeground = alg.getProperty('OutputWorkspace').value
        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(reflWS, 1.2)
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.02, 0.03)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 6.7)
        reflWS = illhelpers.refl_preprocess('refWS', reflWS)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInQ',
            'DirectLineWorkspace': dirWS,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testFigaroSumInLambdaExecutes(self):
        args = {
            'Run': 'ILL/Figaro/000002.nxs',
            'OutputWorkspace': 'direct',
            'ForegroundHalfWidth': [6, 6],
            'FlatBackground': 'Background OFF',
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        args = {
            'InputWorkspace': 'direct',
            'OutputWorkspace': 'direct-fgd'
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        args = {
            'Run': 'ILL/Figaro/000002.nxs',
            'OutputWorkspace': 'reflected',
            'ForegroundHalfWidth': [6, 6],
            'FlatBackground': 'Background OFF',
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        args = {
            'InputWorkspace': 'reflected',
            'OutputWorkspace': 'reflected-fgd',
            'DirectForegroundWorkspace': 'direct-fgd',
            'DirectLineWorkspace': 'direct'
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())

    def testWavelengthRange(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        xMin = 2.3
        xMax = 4.2
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'WavelengthRange': [xMin, xMax],
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        out = alg.getProperty('OutputWorkspace').value
        self.assertEquals(out.getNumberHistograms(), 1)
        Xs = out.readX(0)
        self.assertGreater(len(Xs), 1)
        self.assertGreater(Xs[0], xMin)
        self.assertLess(Xs[-1], xMax)

    def testWavelengthRangeDefault(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        out = alg.getProperty('OutputWorkspace').value
        self.assertEquals(out.getNumberHistograms(), 1)
        Xs = out.readX(0)
        self.assertGreater(len(Xs), 1)
        self.assertGreater(Xs[0], 0.)
        self.assertLess(Xs[-1], 30.)

    def testNoDirectForegroundAndSumInQRaises(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'SummationType': 'SumInQ',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        self.assertRaisesRegexp(RuntimeError, 'Some invalid Properties found', alg.execute)
        self.assertTrue(alg.isExecuted)

    def testNotSummedDirectForegroundRaises(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(ws, 1.2)
        ws = illhelpers.refl_add_two_theta(ws, 5.5)
        ws = illhelpers.refl_preprocess('ws', ws)
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': ws,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        self.assertRaisesRegexp(RuntimeError, 'Some invalid Properties found', alg.execute)
        self.assertTrue(alg.isExecuted)

    def testReflectedBeamSumInLambdaNoRotation(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 6.7)
        dirWS = illhelpers.refl_preprocess_lineposition('dirWS', dirWS, 51)
        self.assertEquals(dirWS.run().getProperty(common.SampleLogs.TWO_THETA).value, 6.7)
        self.assertEquals(dirWS.run().getProperty(common.SampleLogs.LINE_POSITION).value, 51)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        dirForeground = alg.getProperty('OutputWorkspace').value
        self.assertEquals(dirForeground.run().getProperty(common.SampleLogs.TWO_THETA).value, 6.7)
        self.assertEquals(dirForeground.run().getProperty(common.SampleLogs.LINE_POSITION).value, 51)
        self.assertEquals(dirForeground.spectrumInfo().size(), 1)
        self.assertEquals(dirForeground.spectrumInfo().l2(0), dirWS.spectrumInfo().l2(51))
        self.assertEquals(dirForeground.spectrumInfo().twoTheta(0) * 180. / numpy.pi, 8.389135285788196)

    def testReflectedBeamSumInLambdaDetectorMovingAndRotation(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 6.7)
        dirWS = illhelpers.refl_preprocess_lineposition('dirWS', dirWS, 50.9)
        self.assertEquals(dirWS.run().getProperty(common.SampleLogs.TWO_THETA).value, 6.7)
        self.assertEquals(dirWS.run().getProperty(common.SampleLogs.LINE_POSITION).value, 50.9)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        dirForeground = alg.getProperty('OutputWorkspace').value
        self.assertEquals(dirForeground.run().getProperty(common.SampleLogs.TWO_THETA).value, 6.7)
        self.assertEquals(dirForeground.run().getProperty(common.SampleLogs.LINE_POSITION).value, 50.9)
        self.assertEquals(dirForeground.spectrumInfo().size(), 1)
        self.assertEquals(dirForeground.spectrumInfo().l2(0), 3.101234371122588)
        self.assertEquals(numpy.degrees(dirForeground.spectrumInfo().twoTheta(0)), 8.3914043569830614)

        reflWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.refl_rotate_detector(reflWS, 1.2)
        illhelpers.add_chopper_configuration_D17(reflWS)
        illhelpers.add_slit_configuration_D17(reflWS, 0.03, 0.02)
        reflWS = illhelpers.refl_add_two_theta(reflWS, 40.2)
        reflWS = illhelpers.refl_preprocess_lineposition('refWS', reflWS, 120.4)
        self.assertEquals(reflWS.run().getProperty(common.SampleLogs.TWO_THETA).value, 40.2)
        self.assertEquals(reflWS.run().getProperty(common.SampleLogs.LINE_POSITION).value, 120.4)
        args = {
            'InputWorkspace': reflWS,
            'OutputWorkspace': 'foreground',
            'DirectForegroundWorkspace': dirForeground,
            'SummationType': 'SumInLambda',
            'DirectLineWorkspace': dirWS,
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        out = alg.getProperty('OutputWorkspace').value
        self.assertEquals(out.run().getProperty(common.SampleLogs.TWO_THETA).value, 40.2)
        self.assertEquals(out.run().getProperty(common.SampleLogs.LINE_POSITION).value, 120.4)
        self.assertEquals(out.spectrumInfo().size(), 1)
        self.assertEquals(out.spectrumInfo().l2(0), 3.0992766423566)
        self.assertEquals(numpy.degrees(out.spectrumInfo().twoTheta(0)), 40.356848517834038)
        self.assertTrue(alg.isExecuted())

    def testReflectedBeamSumInLambdaDetectorMovingAndRotationD17(self):
        dirWS = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_chopper_configuration_D17(dirWS)
        illhelpers.add_slit_configuration_D17(dirWS, 0.03, 0.02)
        dirWS = illhelpers.refl_add_two_theta(dirWS, 6.7)
        dirWS = illhelpers.refl_preprocess_lineposition('dirWS', dirWS, 50.5)
        args = {
            'InputWorkspace': dirWS,
            'OutputWorkspace': 'dirForeground',
            'rethrow': True,
            'child': True
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(alg.isExecuted())
        #dirForeground = alg.getProperty('OutputWorkspace').value
        #dirSpectrumInfo = dirWS.spectrumInfo()
        #twoTheta50 = dirWS.spectrumInfo().twoTheta(50)
        #twoTheta51 = dirWS.spectrumInfo().twoTheta(51)
        #twoTheta = dirForeground.spectrumInfo().twoTheta(0)
        #self.assertEquals(numpy.degrees([(twoTheta51-twoTheta50)/2. + twoTheta50]), numpy.degrees([twoTheta]))

    '''
    def testReflectedBeamSumInLambdaDetectorMovingAndRotationFIGARO(self):
    '''

if __name__ == "__main__":
    unittest.main()
