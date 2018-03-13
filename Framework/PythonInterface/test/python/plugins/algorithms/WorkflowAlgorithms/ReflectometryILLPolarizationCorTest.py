# -*- coding: utf-8 -*-

from __future__ import (absolute_import, division, print_function)

from mantid.api import mtd
from testhelpers import (assertRaisesNothing, create_algorithm, illhelpers)
import unittest


class ReflectometryILLPolarizationCorTest(unittest.TestCase):

    def tearDown(self):
        mtd.clear()

    def testExecutes(self):
        ws = illhelpers.create_poor_mans_d17_workspace()
        illhelpers.add_flipper_configuration_D17(ws, 1, 1)
        mtd.add('ws', ws)
        self._createBeamPositionWS('beamPosWS', ws, 0., 128)
        ws = self._preprocess('ws', ws, 'beamPosWS')
        ws = self._sumInLambda('ws', ws)
        args = {
            'InputWorkspaces': 'ws',
            'OutputWorkspace': 'corrected',
            'EfficiencyFile': 'ILL/D17/PolarizationFactors.txt',
        }
        alg = create_algorithm('ReflectometryILLPolarizationCor', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertTrue(mtd.doesExist('corrected_++'))

    def _createBeamPositionWS(self, beamPosWSName, referenceWS, detectorAngle, beamCentre):
        args = {
            'OutputWorkspace': beamPosWSName
        }
        alg = create_algorithm('CreateEmptyTableWorkspace', **args)
        alg.execute()
        beamPos = mtd[beamPosWSName]
        beamPos.addColumn('double', 'DetectorAngle')
        beamPos.addColumn('double', 'DetectorDistance')
        beamPos.addColumn('double', 'PeakCentre')
        L2 = referenceWS.getInstrument().getComponentByName('detector').getPos().norm()
        beamPos.addRow((detectorAngle, L2, beamCentre))
        return beamPos

    def _preprocess(self, outputWSName, ws, beamPosWS):
        args = {
            'InputWorkspace': ws,
            'BeamPositionWorkspace': beamPosWS,
            'OutputWorkspace': outputWSName,
        }
        alg = create_algorithm('ReflectometryILLPreprocess', **args)
        alg.execute()
        return mtd[outputWSName]

    def _sumInLambda(self, outputWSName, ws):
        args = {
            'InputWorkspace': ws,
            'OutputWorkspace': outputWSName,
            'SummationType': 'SumInLambda'
        }
        alg = create_algorithm('ReflectometryILLSumForeground', **args)
        alg.execute()
        return mtd[outputWSName]


if __name__ == "__main__":
    unittest.main()
