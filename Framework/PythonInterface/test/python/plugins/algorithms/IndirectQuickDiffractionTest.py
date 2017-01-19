from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
from mantid.api import *


class IndirectQuickDiffractionTest(unittest.TestCase):
    def test_basic(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        IndirectQuickDiffraction(RunNumbers='26176', Instrument='IRIS', Analyser='graphite', Reflection='002', SpectraRange=[3, 53],
                                 ElasticRange=[-0.5, 0], InelasticRange=[0, 0.5], GroupingMethod='All', DiffractionSpectra=[105, 112])

        # Check Diffraction
        wks = mtd['iris26176_diff_scan']

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'iris26176_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)

        # Check Reduction

        wks = mtd['iris26176_ew_scan_red']
        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(wks.getNames()[0], 'iris26176_graphite002_red')

        red_ws = wks.getItem(0)
        self.assertEqual(red_ws.getNumberHistograms(), 1)
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'DeltaE')

        # Check ElasticWindowScan
        wks = mtd['iris26176_ew_scan_eisf']
        self.assertEqual(wks.dataX(0), 1.36593154)
