import unittest
from mantid.simpleapi import *
from mantid.api import *


class ISISIndirectEnergyTransferTest(unittest.TestCase):

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        ws = ISISIndirectEnergyTransfer(InputFiles=['IRS26176.RAW'],
                                        Instrument='IRIS',
                                        Analyser='graphite',
                                        Reflection='002',
                                        SpectraRange=[3, 53])

        self.assertTrue(isinstance(ws, WorkspaceGroup), 'Result workspace should be a workspace group.')
        self.assertEqual(ws.getNames()[0], 'IRS26176_graphite002_red')


    def test_instrument_validation_failure(self):
        """
        Tests that an invalid instrument configuration causes the validation to fail.
        """

        self.assertRaises(RuntimeError,
                          ISISIndirectEnergyTransfer,
                          OutputWorkspace='__ISISIndirectEnergyTransferTest_ws',
                          InputFiles=['IRS26176.RAW'],
                          Instrument='IRIS',
                          Analyser='graphite',
                          Reflection='006',
                          SpectraRange=[3, 53])


    def test_group_workspace_validation_failure(self):
        """
        Tests that validation fails when Workspace is selected as the GroupingMethod
        but no workspace is provided.
        """

        self.assertRaises(RuntimeError,
                          ISISIndirectEnergyTransfer,
                          OutputWorkspace='__ISISIndirectEnergyTransferTest_ws',
                          InputFiles=['IRS26176.RAW'],
                          Instrument='IRIS',
                          Analyser='graphite',
                          Reflection='002',
                          SpectraRange=[3, 53],
                          GroupingMethod='Workspace')


if __name__ == '__main__':
    unittest.main()
