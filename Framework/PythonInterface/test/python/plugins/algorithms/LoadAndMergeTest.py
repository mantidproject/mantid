# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import LoadAndMerge, config, mtd


class LoadAndMergeTest(unittest.TestCase):

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')
        config.appendDataSearchSubDir('ILL/D20/')

    def test_single_run_load(self):
        out1 = LoadAndMerge(Filename='170257')
        self.assertTrue(out1)
        self.assertEqual(out1.name(), 'out1')
        self.assertTrue(isinstance(out1, MatrixWorkspace))
        mtd.clear()

    def test_many_runs_summed(self):
        out2 = LoadAndMerge(Filename='170257+170258',LoaderName='LoadILLIndirect')
        self.assertTrue(out2)
        self.assertEqual(out2.name(), 'out2')
        self.assertTrue(isinstance(out2, MatrixWorkspace))
        mtd.clear()

    def test_many_runs_listed(self):
        out3 = LoadAndMerge(Filename='170257,170258',LoaderName='LoadILLIndirect')
        self.assertTrue(out3)
        self.assertEqual(out3.name(), 'out3')
        self.assertTrue(isinstance(out3, WorkspaceGroup))
        self.assertEqual(out3.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out3.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out3.getItem(1), MatrixWorkspace))
        self.assertEqual(out3.getItem(0).name(),'170257')
        self.assertEqual(out3.getItem(1).name(),'170258')
        mtd.clear()

    def test_many_runs_mixed(self):
        out4 = LoadAndMerge(Filename='170257+170258,170300+170302',LoaderName='LoadILLIndirect')
        self.assertTrue(out4)
        self.assertEqual(out4.name(), 'out4')
        self.assertTrue(isinstance(out4, WorkspaceGroup))
        self.assertEqual(out4.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out4.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out4.getItem(1), MatrixWorkspace))
        self.assertEqual(out4.getItem(0).name(),'170257_170258')
        self.assertEqual(out4.getItem(1).name(),'170300_170302')
        mtd.clear()

    def test_merge_options(self):
        self.assertRaises(RuntimeError,LoadAndMerge,Filename='170300+170301',
                          OutputWorkspace='out5',LoaderName='LoadILLIndirect',
                          MergeRunsOptions=dict({'FailBehaviour':'Stop'}))

    def test_specific_loader(self):
        out5 = LoadAndMerge(Filename='170257',LoaderName='LoadILLIndirect',)
        self.assertTrue(out5)
        self.assertEqual(out5.name(), 'out5')
        self.assertTrue(isinstance(out5, MatrixWorkspace))
        mtd.clear()

    def test_loader_option(self):
        out6 = LoadAndMerge(Filename='967101',LoaderName='LoadILLDiffraction',
                             LoaderVersion=1,LoaderOptions=dict({'DataType':'Raw'}))
        self.assertTrue(out6)
        self.assertEqual(out6.name(), 'out6')
        self.assertTrue(isinstance(out6, MatrixWorkspace))
        mtd.clear()

    def test_output_hidden(self):
        LoadAndMerge(Filename='170257+170258,170300+170302',LoaderName='LoadILLIndirect',
                      OutputWorkspace='__out')
        self.assertTrue(mtd['__out'])
        self.assertTrue(isinstance(mtd['__out'], WorkspaceGroup))
        self.assertEqual(mtd['__out'].getNumberOfEntries(), 2)
        self.assertTrue(isinstance(mtd['__out'].getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(mtd['__out'].getItem(1), MatrixWorkspace))
        self.assertEqual(mtd['__out'].getItem(0).name(),'__170257_170258')
        self.assertEqual(mtd['__out'].getItem(1).name(),'__170300_170302')
        mtd.clear()

    def test_non_ill_load(self):
        out7 = LoadAndMerge(Filename='IRS26173,26174.RAW')
        self.assertTrue(out7)
        self.assertTrue(isinstance(out7, WorkspaceGroup))
        self.assertEqual(out7.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out7.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out7.getItem(1), MatrixWorkspace))
        self.assertEqual(out7.getItem(0).name(),'IRS26173')
        self.assertEqual(out7.getItem(1).name(),'IRS26174')
        mtd.clear()

    def test_multi_period_loader_list(self):
        out8 = LoadAndMerge(Filename='MUSR00015196,00015197.nxs')
        self.assertTrue(out8)
        self.assertTrue(isinstance(out8, WorkspaceGroup))
        self.assertEqual(out8.getNumberOfEntries(), 4)
        self.assertEqual(out8.getItem(0).name(),'MUSR00015196_1')
        self.assertEqual(out8.getItem(1).name(),'MUSR00015196_2')
        self.assertEqual(out8.getItem(2).name(),'MUSR00015197_1')
        self.assertEqual(out8.getItem(3).name(),'MUSR00015197_2')
        mtd.clear()

    def test_multi_period_loader_sum(self):
        out9 = LoadAndMerge(Filename='MUSR00015196+00015197.nxs')
        self.assertTrue(out9)
        self.assertTrue(isinstance(out9, MatrixWorkspace))
        self.assertTrue('MUSR00015196' not in mtd)
        self.assertTrue('MUSR00015197' not in mtd)
        self.assertTrue('MUSR00015196_1' not in mtd)
        self.assertTrue('MUSR00015196_2' not in mtd)
        self.assertTrue('MUSR00015197_1' not in mtd)
        self.assertTrue('MUSR00015197_2' not in mtd)
        mtd.clear()

if __name__ == "__main__":
    unittest.main()
