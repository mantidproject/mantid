# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import LoadAndMerge, config, mtd


class LoadAndMergeTest(unittest.TestCase):
    _facility = None
    _data_search_dirs = None

    @classmethod
    def setUpClass(cls):
        cls._facility = config["default.facility"]
        cls._data_search_dirs = config["datasearch.directories"]
        config.appendDataSearchSubDir("ILL/IN16B/")
        config.appendDataSearchSubDir("ILL/D20/")
        config.appendDataSearchSubDir("ILL/D11/")
        config["default.facility"] = "ILL"
        config["default.instrument"] = "IN16B"

    def turnDown(self):
        mtd.clear()

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["datasearch.directories"] = cls._data_search_dirs

    def test_single_run_load(self):
        out1 = LoadAndMerge(Filename="170257")
        self.assertTrue(out1)
        self.assertEqual(out1.name(), "out1")
        self.assertTrue(isinstance(out1, MatrixWorkspace))

    def test_single_run_load_ads(self):
        out1 = LoadAndMerge(Filename="170257")
        ads_ws = mtd[out1.name()]
        self.assertTrue(ads_ws)
        self.assertEqual(ads_ws.name(), "out1")
        self.assertTrue(isinstance(ads_ws, MatrixWorkspace))

    def test_single_run_load_no_ads(self):
        out1 = LoadAndMerge(Filename="170257", StoreInADS=False)
        with self.assertRaisesRegex(KeyError, ".*does not exist."):
            _ = mtd["out1"]
        self.assertTrue(out1)
        self.assertEqual(out1.name(), "")
        self.assertTrue(isinstance(out1, MatrixWorkspace))

    def test_many_runs_summed(self):
        out2 = LoadAndMerge(Filename="170257+170258", LoaderName="LoadILLIndirect")
        self.assertTrue(out2)
        self.assertEqual(out2.name(), "out2")
        self.assertTrue(isinstance(out2, MatrixWorkspace))

    def test_many_runs_listed(self):
        out3 = LoadAndMerge(Filename="170257,170258", LoaderName="LoadILLIndirect")
        self.assertTrue(out3)
        self.assertEqual(out3.name(), "out3")
        self.assertTrue(isinstance(out3, WorkspaceGroup))
        self.assertEqual(out3.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out3.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out3.getItem(1), MatrixWorkspace))
        self.assertEqual(out3.getItem(0).name(), "170257")
        self.assertEqual(out3.getItem(1).name(), "170258")

    def test_many_runs_mixed(self):
        out4 = LoadAndMerge(Filename="170257+170258,170300+170302", LoaderName="LoadILLIndirect")
        self.assertTrue(out4)
        self.assertEqual(out4.name(), "out4")
        self.assertTrue(isinstance(out4, WorkspaceGroup))
        self.assertEqual(out4.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out4.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out4.getItem(1), MatrixWorkspace))
        self.assertEqual(out4.getItem(0).name(), "170257_170258")
        self.assertEqual(out4.getItem(1).name(), "170300_170302")

    def test_merge_options(self):
        self.assertRaises(
            RuntimeError,
            LoadAndMerge,
            Filename="170300+170301",
            OutputWorkspace="out5",
            LoaderName="LoadILLIndirect",
            MergeRunsOptions={"FailBehaviour": "Stop"},
        )

        out5 = LoadAndMerge(
            Filename="170300+170301",
            OutputWorkspace="out5",
            LoaderName="LoadILLIndirect",
            MergeRunsOptions={
                "SampleLogsFail": "Doppler.maximum_delta_energy,Doppler.mirror_sense,acquisition_mode",
                "SampleLogsFailTolerances": "10.0,10.0,10.0",
                "SampleLogsList": "Doppler.maximum_delta_energy",
            },
        )
        self.assertEqual(out5.getRun().getLogData("Doppler.maximum_delta_energy").value, "2, 0")

    @mock.patch("plugins.algorithms.LoadAndMerge.MergeRuns")
    def test_merge_runs_call(self, m_merge_runs):
        # This try/except block is compulsory as the use of the mocked MergeRuns inside the
        # LoadAndMerge algorithm triggers errors further
        try:
            LoadAndMerge(
                Filename="170300+170301",
                OutputWorkspace="out5",
                LoaderName="LoadILLIndirect",
                MergeRunsOptions={
                    "SampleLogsFail": "Doppler.maximum_delta_energy,Doppler.mirror_sense,acquisition_mode",
                    "SampleLogsFailTolerances": "10.0,10.0,10.0",
                    "SampleLogsWarn": "Doppler.maximum_delta_energy,Doppler.mirror_sense,acquisition_mode",
                    "SampleLogsWarnTolerances": "0.0,0.0,0.0",
                    "SampleLogsList": "Doppler.maximum_delta_energy",
                },
            )
        except RuntimeError:
            pass

        m_merge_runs.assert_called_with(
            InputWorkspaces=["170300", "170301"],
            OutputWorkspace="__tmp_170300",
            SampleLogsFail="Doppler.maximum_delta_energy,Doppler.mirror_sense,acquisition_mode",
            SampleLogsFailTolerances="10.0,10.0,10.0",
            SampleLogsWarn="Doppler.maximum_delta_energy,Doppler.mirror_sense,acquisition_mode",
            SampleLogsWarnTolerances="0.0,0.0,0.0",
            SampleLogsList="Doppler.maximum_delta_energy",
        )

    def test_specific_loader(self):
        out5 = LoadAndMerge(
            Filename="170257",
            LoaderName="LoadILLIndirect",
        )
        self.assertTrue(out5)
        self.assertEqual(out5.name(), "out5")
        self.assertTrue(isinstance(out5, MatrixWorkspace))

    def test_loader_option(self):
        out6 = LoadAndMerge(Filename="967101", LoaderName="LoadILLDiffraction", LoaderVersion=1)
        self.assertTrue(out6)
        self.assertEqual(out6.name(), "out6")
        self.assertTrue(isinstance(out6, MatrixWorkspace))

    def test_output_hidden(self):
        LoadAndMerge(Filename="170257+170258,170300+170302", LoaderName="LoadILLIndirect", OutputWorkspace="__out")
        self.assertTrue(mtd["__out"])
        self.assertTrue(isinstance(mtd["__out"], WorkspaceGroup))
        self.assertEqual(mtd["__out"].getNumberOfEntries(), 2)
        self.assertTrue(isinstance(mtd["__out"].getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(mtd["__out"].getItem(1), MatrixWorkspace))
        self.assertEqual(mtd["__out"].getItem(0).name(), "__170257_170258")
        self.assertEqual(mtd["__out"].getItem(1).name(), "__170300_170302")

    def test_non_ill_load(self):
        out7 = LoadAndMerge(Filename="IRS26173,26174.RAW")
        self.assertTrue(out7)
        self.assertTrue(isinstance(out7, WorkspaceGroup))
        self.assertEqual(out7.getNumberOfEntries(), 2)
        self.assertTrue(isinstance(out7.getItem(0), MatrixWorkspace))
        self.assertTrue(isinstance(out7.getItem(1), MatrixWorkspace))
        self.assertEqual(out7.getItem(0).name(), "IRS26173")
        self.assertEqual(out7.getItem(1).name(), "IRS26174")

    def test_multi_period_loader_list(self):
        out8 = LoadAndMerge(Filename="MUSR00015196,00015197.nxs")
        self.assertTrue(out8)
        self.assertTrue(isinstance(out8, WorkspaceGroup))
        self.assertEqual(out8.getNumberOfEntries(), 4)
        self.assertEqual(out8.getItem(0).name(), "MUSR00015196_1")
        self.assertEqual(out8.getItem(1).name(), "MUSR00015196_2")
        self.assertEqual(out8.getItem(2).name(), "MUSR00015197_1")
        self.assertEqual(out8.getItem(3).name(), "MUSR00015197_2")

    def test_multi_period_loader_sum(self):
        out9 = LoadAndMerge(Filename="MUSR00015196+00015197.nxs")
        self.assertTrue(out9)
        self.assertTrue(isinstance(out9, MatrixWorkspace))
        self.assertTrue("MUSR00015196" not in mtd)
        self.assertTrue("MUSR00015197" not in mtd)
        self.assertTrue("MUSR00015196_1" not in mtd)
        self.assertTrue("MUSR00015196_2" not in mtd)
        self.assertTrue("MUSR00015197_1" not in mtd)
        self.assertTrue("MUSR00015197_2" not in mtd)

    def test_concatenate_output(self):
        out = LoadAndMerge(Filename="010444:010446", OutputBehaviour="Concatenate")
        self.assertTrue(out)
        self.assertEqual(out.name(), "out")
        self.assertTrue(isinstance(out, MatrixWorkspace))
        self.assertEqual(out.blocksize(), 3)
        self.assertEqual(out.readX(0)[0], 0)
        self.assertEqual(out.readX(0)[1], 1)
        self.assertEqual(out.readX(0)[2], 2)
        # check if LoadAndMerge does not abandon intermediate workspaces in ADS:
        self.assertTrue("010444" not in mtd)
        self.assertTrue("010445" not in mtd)
        self.assertTrue("010446" not in mtd)

    def test_concatenate_output_with_log(self):
        out = LoadAndMerge(Filename="010444:010446", OutputBehaviour="Concatenate", SampleLogAsXAxis="sample.temperature")
        self.assertTrue(out)
        self.assertEqual(out.name(), "out")
        self.assertTrue(isinstance(out, MatrixWorkspace))
        self.assertEqual(out.blocksize(), 3)
        self.assertAlmostEqual(out.readX(0)[0], 297.6, 1)
        self.assertAlmostEqual(out.readX(0)[1], 297.7, 1)
        self.assertAlmostEqual(out.readX(0)[2], 297.7, 1)
        # check if LoadAndMerge does not abandon intermediate workspaces in ADS:
        self.assertTrue("010444" not in mtd)
        self.assertTrue("010445" not in mtd)
        self.assertTrue("010446" not in mtd)


if __name__ == "__main__":
    unittest.main()
