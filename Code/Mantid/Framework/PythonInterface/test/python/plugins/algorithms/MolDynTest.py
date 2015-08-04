#pylint: disable=too-many-public-methods,invalid-name

import unittest
from mantid.simpleapi import *
from mantid.api import *


class MolDynTest(unittest.TestCase):

    def test_loadSqwWithEMax(self):
        # Load an Sqw function from a nMOLDYN file
        moldyn_group = MolDyn(Filename='NaF_DISF.cdl',
                              Functions=['Sqw-total'],
                              MaxEnergy="1.0")

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), 'NaF_DISF_Sqw-total')

        # Get max enery from result workspace
        x_data = moldyn_group[0].dataX(0)
        x_max = x_data[len(x_data) - 1]

        # Check that it is less that what was passed to algorithm
        self.assertTrue(x_max <= 1.0)


    def test_loadSqwWithSymm(self):
        # Load an Sqw function from a nMOLDYN file
        moldyn_group = MolDyn(Filename='NaF_DISF.cdl',
                              Functions=['Sqw-total'],
                              SymmetriseEnergy=True)

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), 'NaF_DISF_Sqw-total')

        # Get max and min energy from result workspace
        x_data = moldyn_group[0].dataX(0)
        x_max = x_data[len(x_data) - 1]
        x_min = x_data[0]

        # abs(min) should equal abs(max)
        self.assertEqual(x_max, -x_min)


    def test_loadSqwWithRes(self):
        # Create a sample workspace thet looks like an instrument resolution
        sample_res = CreateSampleWorkspace(NumBanks=1,
                                           BankPixelWidth=1,
                                           XUnit='Energy',
                                           XMin=-10,
                                           XMax=10,
                                           BinWidth=0.1)

        # Load an Sqw function from a nMOLDYN file
        moldyn_group = MolDyn(Filename='NaF_DISF.cdl',
                              Functions=['Sqw-total'],
                              MaxEnergy="1.0",
                              SymmetriseEnergy=True,
                              Resolution=sample_res)

        self.assertTrue(isinstance(moldyn_group, WorkspaceGroup))
        self.assertEqual(len(moldyn_group), 1)
        self.assertEqual(moldyn_group[0].name(), 'NaF_DISF_Sqw-total')


    def test_loadSqwWithResWithNoEMaxFails(self):
        """
        Tests that trying to use an instrument resolution without a Max Energy will fail.
        """

        # Create a sample workspace thet looks like an instrument resolution
        sample_res = CreateSampleWorkspace(NumBanks=1,
                                           BankPixelWidth=1,
                                           XUnit='Energy',
                                           XMin=-10,
                                           XMax=10,
                                           BinWidth=0.1)

        # Load an Sqw function from a nMOLDYN file
        self.assertRaises(RuntimeError, MolDyn,
                          Filename='NaF_DISF.cdl',
                          Functions=['Sqw-total'],
                          Resolution=sample_res,
                          OutputWorkspace='moldyn_group')


if __name__ == '__main__':
    unittest.main()
