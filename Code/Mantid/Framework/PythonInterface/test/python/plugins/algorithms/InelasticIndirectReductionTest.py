import unittest
from mantid import mtd
from mantid.simpleapi import InelasticIndirectReduction


class InelasticIndirectReductionTest(unittest.TestCase):

    def test_basic(self):
        InelasticIndirectReduction(InputFiles='IRS26176.RAW',
                                   OutputWorkspace='IndirectReductions',
                                   Instrument='IRIS',
                                   Analyser='graphite',
                                   Reflection='002',
                                   DetectorRange=[3, 53])

        reduction_workspace = mtd['IndirectReductions'].getItem(0)
        self.assertEquals(reduction_workspace.getName(), 'irs26176_graphite002_red')


if __name__ == "__main__":
    unittest.main()
