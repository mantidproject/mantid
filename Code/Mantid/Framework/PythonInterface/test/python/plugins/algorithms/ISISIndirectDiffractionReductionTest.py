import unittest
from mantid.simpleapi import *
from mantid.api import *


class ISISIndirectDiffractionReductionTest(unittest.TestCase):

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        ws = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                              Instrument='IRIS',
                                              Mode='diffspec',
                                              SpectraRange=[105, 112])

        self.assertTrue(isinstance(ws, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(ws.getNames()[0], 'IRS26176_diffspec_red')


    def test_reduction_with_container_completes(self):
        """
        Test to ensure that reduction with container subtraction works.
        """

        ws = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                              ContainerFiles=['IRS26173.RAW'],
                                              Instrument='IRIS',
                                              Mode='diffspec',
                                              SpectraRange=[105, 112])

        self.assertTrue(isinstance(ws, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(ws.getNames()[0], 'IRS26176_diffspec_red')


    def test_reduction_with_container_and_scale_completes(self):
        """
        Test to ensure that reduction with container subtraction works.
        """

        ws = ISISIndirectDiffractionReduction(InputFiles=['IRS26176.RAW'],
                                              ContainerFiles=['IRS26173.RAW'],
                                              ContainerScaleFactor=0.5,
                                              Instrument='IRIS',
                                              Mode='diffspec',
                                              SpectraRange=[105, 112])

        self.assertTrue(isinstance(ws, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(ws.getNames()[0], 'IRS26176_diffspec_red')


if __name__ == '__main__':
    unittest.main()
