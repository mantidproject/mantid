from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import LoadMergeRuns, config, mtd


class LoadMergeRunsTest(unittest.TestCase):

    def setUp(self):
        config.setFacility('ILL')
        config.appendDataSearchSubDir('ILL/IN16B/')
        config.appendDataSearchSubDir('ILL/D20/')

    def test_single_run(self):
        out1 = LoadMergeRuns(Filename='170257')
        self.assertTrue(out1)
        self.assertEquals(out1.getName(), 'out1')
        self.assertTrue(isinstance(out1, MatrixWorkspace))
        mtd.clear()

    def test_many_runs_summed(self):
        out2 = LoadMergeRuns(Filename='170257+170258')
        self.assertTrue(out2)
        self.assertEquals(out2.getName(), 'out2')
        self.assertTrue(isinstance(out2, MatrixWorkspace))
        mtd.clear()

    def test_many_runs_listed(self):
        out3 = LoadMergeRuns(Filename='170257,170258')
        self.assertTrue(out3)
        self.assertEquals(out3.getName(), 'out3')
        self.assertTrue(isinstance(out3, WorkspaceGroup))
        self.assertEquals(out3.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out3.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out3.getItem(1), MatrixWorkspace))
        self.assertEquals(out3.getItem(0).getName(),'170257')
        self.assertEquals(out3.getItem(1).getName(),'170258')
        mtd.clear()

    def test_many_runs_mixed(self):
        out4 = LoadMergeRuns(Filename='170257+170258,170300+170302')
        self.assertTrue(out4)
        self.assertEquals(out4.getName(), 'out4')
        self.assertTrue(isinstance(out4, WorkspaceGroup))
        self.assertEquals(out4.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out4.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out4.getItem(1), MatrixWorkspace))
        self.assertEquals(out4.getItem(0).getName(),'170257_170258')
        self.assertEquals(out4.getItem(1).getName(),'170300_170302')
        mtd.clear()

    def test_merge_options(self):
        self.assertRaises(RuntimeError,LoadMergeRuns,
                          Filename='170300+170301',
                          OutputWorkspace='out5',
                          MergeRunsOptions=dict({'FailBehaviour':'Stop'}))

    def test_specific_loader(self):
        out5 = LoadMergeRuns(Filename='170257',LoaderName='LoadILLIndirect',LoaderVersion=2)
        self.assertTrue(out5)
        self.assertEquals(out5.getName(), 'out5')
        self.assertTrue(isinstance(out5, MatrixWorkspace))
        mtd.clear()

    def test_loader_option(self):
        out6 = LoadMergeRuns(Filename='967101',LoaderOptions=dict({'DataType':'Raw'}))
        self.assertTrue(out6)
        self.assertEquals(out6.getName(), 'out6')
        self.assertTrue(isinstance(out6, MatrixWorkspace))
        mtd.clear()

    def test_output_hidden(self):
        LoadMergeRuns(Filename='170257+170258,170300+170302', OutputWorkspace='__out')
        self.assertTrue(mtd['__out'])
        self.assertTrue(isinstance(mtd['__out'], WorkspaceGroup))
        self.assertEquals(mtd['__out'].getNumberOfEntries(), 2)
        self.assertTrue(isinstance(mtd['__out'].getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(mtd['__out'].getItem(1), MatrixWorkspace))
        self.assertEquals(mtd['__out'].getItem(0).getName(),'__170257_170258')
        self.assertEquals(mtd['__out'].getItem(1).getName(),'__170300_170302')
        mtd.clear()

    def test_non_ill(self):
        out7 = LoadMergeRuns(Filename='IRS26173,26174.RAW')
        self.assertTrue(out7)
        self.assertTrue(isinstance(out7, WorkspaceGroup))
        self.assertEquals(out7.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out7.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out7.getItem(1), MatrixWorkspace))
        self.assertEquals(out7.getItem(0).getName(),'IRS26173')
        self.assertEquals(out7.getItem(1).getName(),'IRS26174')
        mtd.clear()

if __name__ == "__main__":
    unittest.main()
