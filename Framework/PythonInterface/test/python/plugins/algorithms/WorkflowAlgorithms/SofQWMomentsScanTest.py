# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import SofQWMomentsScan, DeleteWorkspace
from mantid import mtd


class SofQWMomentsScanTest(unittest.TestCase):

    def test_sqw_moments_scan(self):
        SofQWMomentsScan(InputFiles='OSIRIS100320', Instrument='OSIRIS', Analyser='graphite', Reflection='002',
                                        SpectraRange='963,1004', QRange='0,0.1,2', EnergyRange='-0.4,0.01,0.4')

        sqw = mtd['Sqw'][0]
        self.assertEqual(sqw.getNumberHistograms(), 20)
        self.assertEqual(sqw.blocksize(), 80)

    def test_multiple_scan(self):
        SofQWMomentsScan(InputFiles='OSIRIS100320, OSIRIS100321', Instrument='OSIRIS', Analyser='graphite', Reflection='002',
                         SpectraRange='963,1004', QRange='0,0.1,2', EnergyRange='-0.4,0.01,0.4')

    def tearDown(self):
        """
        Remove workspaces from ADS.
        """

        DeleteWorkspace(mtd['reduced'])
        DeleteWorkspace(mtd['sqw'])

if __name__ == "__main__":
    unittest.main()
