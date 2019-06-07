# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import collections
from mantid.api import mtd
import numpy
import numpy.testing
from testhelpers import illhelpers, run_algorithm
import unittest


class DirectILLReductionTest(unittest.TestCase):
    _BKG_LEVEL = 0.0
    _EPP_WS_NAME = 'eppWS_'
    _TEST_WS_NAME = 'testWS_'
    _VANADIUM_WS_NAME = 'vanadiumWS_'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        self._testIN5WS = None

    def setUp(self):
        if not self._testIN5WS:
            self._testIN5WS = illhelpers.create_poor_mans_in5_workspace(self._BKG_LEVEL,
                                                                        illhelpers.default_test_detectors)
        inWSName = 'inputWS'
        mtd.addOrReplace(inWSName, self._testIN5WS)
        kwargs = {
            'InputWorkspace': self._testIN5WS,
            'OutputWorkspace': self._TEST_WS_NAME,
            'OutputEPPWorkspace': self._EPP_WS_NAME
        }
        run_algorithm('DirectILLCollectData', **kwargs)
        kwargs = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': self._VANADIUM_WS_NAME,
            'EPPWorkspace': self._EPP_WS_NAME
        }
        run_algorithm('DirectILLIntegrateVanadium', **kwargs)
        vanadiumWS = mtd[self._VANADIUM_WS_NAME]
        for i in range(vanadiumWS.getNumberHistograms()):
            vanadiumYs = vanadiumWS.dataY(i)
            vanadiumYs.fill(1.0)
        mtd.remove(inWSName)

    def tearDown(self):
        mtd.clear()

    def testSuccessfulRun(self):
        _add_natural_angle_step_parameter(self._TEST_WS_NAME)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'SubalgorithmLogging': 'Logging ON',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))

    def testDetectorGrouping(self):
        ws = illhelpers.create_poor_mans_in5_workspace(0.0, _groupingTestDetectors)
        originalNDetectors = ws.getNumberHistograms()
        detectorIds = list()
        for i in range(originalNDetectors):
            detectorIds.append(ws.getDetector(i).getID())
        _add_natural_angle_step_parameter(ws)
        mtd.addOrReplace('inWS', ws)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': ws,
            'OutputWorkspace': outWSName,
            'Cleanup': 'Cleanup OFF',
            'Transposing': 'Transposing OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        groupedWSName = outWSName + '_grouped_detectors_'
        self.assertTrue(groupedWSName in mtd)
        groupedWS = mtd[groupedWSName]
        self.assertEqual(groupedWS.getNumberHistograms(), 2)
        groupIds = list(groupedWS.getDetector(0).getDetectorIDs())
        groupIds += groupedWS.getDetector(1).getDetectorIDs()
        self.assertEqual(collections.Counter(detectorIds), collections.Counter(groupIds))

    def testDetectorGroupingWithUserGivenAngleStep(self):
        ws = illhelpers.create_poor_mans_in5_workspace(0.0, _groupingTestDetectors)
        nhisto = ws.getNumberHistograms()
        spectrumInfo = ws.spectrumInfo()
        minAngle = 180.
        maxAngle = 0.
        for i in range(nhisto):
            angle = numpy.rad2deg(spectrumInfo.twoTheta(i))
            minAngle = min(minAngle, angle)
            maxAngle = max(maxAngle, angle)
        mtd.addOrReplace('inWS', ws)
        outWSName = 'unused'
        outSThetaWName = 'SofThetaW'
        angleStep = 0.2
        algProperties = {
            'InputWorkspace': ws,
            'OutputWorkspace': outWSName,
            'GroupingAngleStep': angleStep,
            'OutputSofThetaEnergyWorkspace': outSThetaWName,
            'Transposing': 'Transposing OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(outSThetaWName in mtd)
        SThetaWWS = mtd[outSThetaWName]
        spectrumInfo = SThetaWWS.spectrumInfo()
        firstAngleBin = int(minAngle / angleStep)
        lastAngleBin = int(maxAngle / angleStep) + 1
        expected = lastAngleBin - firstAngleBin
        self.assertEqual(spectrumInfo.size(), expected)

    def testOutputIsDistribution(self):
        _add_natural_angle_step_parameter(self._TEST_WS_NAME)
        outWSName = 'outWS'
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'OutputSofThetaEnergyWorkspace': 'SofThetaE',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertTrue(ws.isDistribution())
        self.assertTrue(mtd.doesExist('SofThetaE'))
        ws = mtd['SofThetaE']
        self.assertTrue(ws.isDistribution())

    def testERebinning(self):
        _add_natural_angle_step_parameter(self._TEST_WS_NAME)
        outWSName = 'outWS'
        E0 = -2.
        dE = 0.13
        E1 = E0 + 40 * dE
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EnergyRebinningParams': [E0, dE, E1],
            'Transposing': 'Transposing OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'DeltaE')
        xs = ws.readX(0)
        numpy.testing.assert_almost_equal(xs, numpy.arange(E0, E1 + 0.01, dE))

    def testHybridERebinningSingleUserRange(self):
        outWSName = 'outWS'
        E0 = -2.
        dE = 0.13
        E1 = E0 + 40 * dE
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EnergyRebinning': '{},{},{}'.format(E0, dE, E1),
            'Transposing': 'Transposing OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'DeltaE')
        xs = ws.readX(0)
        numpy.testing.assert_almost_equal(xs, numpy.arange(E0, E1 + 0.01, dE))

    def testHybridERebinningUserConstrainedAutoRange(self):
        outWSName = 'outWS'
        E0 = -0.23
        E1 = 0.32
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'EnergyRebinning': '{},a,{}'.format(E0, E1),
            'Transposing': 'Transposing OFF',
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'DeltaE')
        xs = ws.readX(0)
        self.assertEqual(xs[0], E0)
        self.assertEqual(xs[-1], E1)

    def testQRebinning(self):
        _add_natural_angle_step_parameter(self._TEST_WS_NAME)
        outWSName = 'outWS'
        Q0 = 2.3
        dQ = 0.1
        Q1 = 2.7
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'QBinningParams': [Q0, dQ, Q1],
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'MomentumTransfer')
        xs = ws.readX(0)
        numpy.testing.assert_almost_equal(xs, numpy.arange(Q0, Q1, dQ))

    def testQRebinningBinWidthOnly(self):
        _add_natural_angle_step_parameter(self._TEST_WS_NAME)
        outWSName = 'outWS'
        dQ = 0.1
        algProperties = {
            'InputWorkspace': self._TEST_WS_NAME,
            'OutputWorkspace': outWSName,
            'QBinningParams': [dQ],
            'rethrow': True
        }
        run_algorithm('DirectILLReduction', **algProperties)
        self.assertTrue(mtd.doesExist(outWSName))
        ws = mtd[outWSName]
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'MomentumTransfer')
        xs = ws.readX(0)
        self.assertGreater(len(xs), 3)
        dx = xs[1:] - xs[:-1]
        # Bin widths may differ at the edges.
        numpy.testing.assert_almost_equal(dx[1:-1], 0.1)

    def _checkAlgorithmsInHistory(self, ws, *args):
        """Return true if algorithm names listed in *args are found in the
        workspace's history.
        """
        # This method is currently unused and kept here for nostalgic reasons.
        # However, it might be interesting to start using it again in tests.
        history = ws.getHistory()
        reductionHistory = history.getAlgorithmHistory(history.size() - 1)
        algHistories = reductionHistory.getChildHistories()
        algNames = [alg.name() for alg in algHistories]
        for algName in args:
            return algName in algNames


def _add_natural_angle_step_parameter(ws, step=1.0):
    kwargs = {
        'Workspace': ws,
        'ParameterName': 'natural-angle-step',
        'ParameterType': 'Number',
        'Value': str(step)
    }
    run_algorithm('SetInstrumentParameter', **kwargs)


def _groupingTestDetectors(ws):
    """Mask detectors for detector grouping tests."""
    indexBegin = 63105  # Detector at L2 and at 2theta = 40.6.
    kwargs = {
        'Workspace': ws,
        'StartWorkspaceIndex': 0,
        'EndWorkspaceIndex': indexBegin - 1,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    spectrumInfo = ws.spectrumInfo()
    reference2Theta1 = spectrumInfo.twoTheta(indexBegin)
    reference2Theta2 = spectrumInfo.twoTheta(indexBegin + 256)
    mask = list()
    tolerance = numpy.deg2rad(0.3724)
    for i in range(indexBegin + 1, indexBegin + 10000):
        twoTheta = spectrumInfo.twoTheta(i)
        if abs(reference2Theta1 - twoTheta) >= tolerance and abs(reference2Theta2 - twoTheta) >= tolerance:
            mask.append(i)
    kwargs = {
        'Workspace': ws,
        'WorkspaceIndexList': mask,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    kwargs = {
        'Workspace': ws,
        'StartWorkspaceIndex': indexBegin + 10000,
        'child': True
    }
    run_algorithm('MaskDetectors', **kwargs)
    return ws

if __name__ == '__main__':
    unittest.main()
