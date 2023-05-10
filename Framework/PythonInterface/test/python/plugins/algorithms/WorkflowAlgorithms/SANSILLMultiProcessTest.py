# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import SANSILLMultiProcess, config, mtd
from mantid.api import WorkspaceGroup, MatrixWorkspace, TextAxis


class SANSILLMultiProcessTest(unittest.TestCase):
    """
    The testee algorithm performs an entire experiment reduction.
    It takes too long to be unit tested in all the possible scenarios.
    Therefore, the functionality is covered by system tests instead.
    This unit test suite covers only the failure cases, i.e. the input validation,
    and the most basic reduction with default parameters.
    """

    @classmethod
    def setUpClass(cls):
        cls._data_search_dirs = config["datasearch.directories"]
        cls._facility = config["default.facility"]
        cls._instrument = config["default.instrument"]
        config.appendDataSearchSubDir("ILL/D11/")
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D11"

    def tearDown(self):
        mtd.clear()

    @classmethod
    def tearDownClass(cls):
        config["default.facility"] = cls._facility
        config["default.instrument"] = cls._instrument
        config["datasearch.directories"] = cls._data_search_dirs

    def test_minimal(self):
        ws = SANSILLMultiProcess(SampleRunsD1="010569")
        self.assertTrue(isinstance(ws, WorkspaceGroup))
        self.assertEqual(ws.getNumberOfEntries(), 2)
        realspace = ws.getItem(0)
        self.assertTrue(isinstance(realspace, MatrixWorkspace))
        self.assertEqual(realspace.getAxis(0).getUnit().unitID(), "Empty")
        self.assertEqual(realspace.getNumberHistograms(), 128 * 128 + 2)
        self.assertEqual(realspace.blocksize(), 1)
        self.assertFalse(realspace.isHistogramData())
        self.assertTrue(realspace.getInstrument())
        self.assertTrue(realspace.getRun())
        self.assertTrue(realspace.getHistory())
        iq = ws.getItem(1)
        self.assertTrue(isinstance(iq, MatrixWorkspace))
        self.assertEqual(iq.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertTrue(isinstance(iq.getAxis(1), TextAxis))
        self.assertEqual(iq.getNumberHistograms(), 1)
        self.assertEqual(iq.blocksize(), 88)
        self.assertFalse(iq.isHistogramData())
        self.assertTrue(iq.isDistribution())
        self.assertTrue(iq.getRun())
        self.assertTrue(iq.getHistory())

    def test_fail_d2_before_d1_samples(self):
        self.assertRaisesRegex(
            RuntimeError, "Samples have to be filled from D1 onwards", SANSILLMultiProcess, OutputWorkspace="out", SampleRunsD2="010569"
        )

    def test_fail_d2_skipped_samples(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Samples have to be filled from D1 onwards:",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleRunsD3="010455",
        )

    def test_fail_different_number_of_samples(self):
        self.assertRaisesRegex(
            RuntimeError,
            "SampleRunsD2 has 2 elements instead of 1",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleRunsD2="010455,010460",
        )

    def test_fail_transmission_wo_empty_beam(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Empty beam flux input workspace is mandatory for transmission calculation.",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleTrRunsW1="010585",
        )

    def test_fail_different_number_of_tr_runs(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Input and transmission workspaces have a different number of wavelength bins",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569,010460",
            SampleTrRunsW1="010585",
            TrEmptyBeamRuns="010414",
        )

    def test_fail_w2_before_w1_tr_runs(self):
        self.assertRaisesRegex(
            RuntimeError,
            "If there is one wavelength, transmissions must be filled in W1.",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleTrRunsW2="010585",
            TrEmptyBeamRuns="010414",
        )

    def test_fail_output_name_not_alphanumeric(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Output workspace name must be alphanumeric, it should start with a letter.",
            SANSILLMultiProcess,
            OutputWorkspace="7out",
            SampleRunsD1="010569",
        )

    def test_fail_wedges_and_panels(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Panels cannot be calculated together with the wedges, please choose one or the other.",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            OutputPanels=True,
            NumberOfWedges=2,
        )

    def test_fail_wrong_q_range(self):
        self.assertRaisesRegex(
            RuntimeError,
            "Number of Q binning parameter sets must be equal to the number of distances.",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            OutputBinning="0.05,0.3:",
        )

    def test_fail_wrong_tr_beam_radius_dimension(self):
        self.assertRaisesRegex(
            RuntimeError,
            "TrBeamRadius has 2 elements which does not match the number of distances 1",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleTrRunsW1="010585",
            TrEmptyBeamRuns="010414",
            TrBeamRadius=[0.1, 0.11],
        )

    def test_fail_wrong_sample_names_dimension(self):
        self.assertRaisesRegex(
            RuntimeError,
            "SampleNames has 2 elements which does not match the number of samples 1.",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleNamesFrom="User",
            SampleNames="apple, pear",
        )

    def test_fail_wrong_sample_thickness_dimension(self):
        self.assertRaisesRegex(
            RuntimeError,
            "SampleThickness has 2 elements which does not match the number of samples 1.",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            SampleThickness=[0.1, 0.11],
        )

    def test_fail_wrong_beam_radius_dimension(self):
        self.assertRaisesRegex(
            RuntimeError,
            "BeamRadius has 2 elements which does not match the number of distances 1",
            SANSILLMultiProcess,
            OutputWorkspace="out",
            SampleRunsD1="010569",
            BeamRadius=[0.1, 0.11],
        )


if __name__ == "__main__":
    unittest.main()
