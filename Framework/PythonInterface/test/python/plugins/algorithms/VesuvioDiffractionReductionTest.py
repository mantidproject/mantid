#pylint: disable=too-many-public-methods,invalid-name

import unittest
from mantid.simpleapi import *
from mantid.api import *


class VesuvioDiffractionReductionTest(unittest.TestCase):

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        wks = VesuvioDiffractionReduction(InputFiles=['EVS15289.raw'],
                                      InstrumentParFIle='IP0005.dat')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)
        self.assertEqual(wks.getNames()[0], 'vesuvio15289_diffspec_red')

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 1)


    def test_grouping_individual(self):
        """
        Test setting individual grouping, one spectrum per detector.
        """

        wks = VesuvioDiffractionReduction(InputFiles=['EVS15289.raw'],
                                      GroupingPolicy='Individual',
                                      InstrumentParFIle='IP0005.dat')

        self.assertTrue(isinstance(wks, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(len(wks), 1)

        red_ws = wks[0]
        self.assertEqual(red_ws.getAxis(0).getUnit().unitID(), 'dSpacing')
        self.assertEqual(red_ws.getNumberHistograms(), 196)


if __name__ == '__main__':
    unittest.main()
