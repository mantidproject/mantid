# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import Load, DeleteWorkspace, AddSampleLogMultiple, DeleteLog
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService


class TOFTOFMergeRunsTest(unittest.TestCase):
    def setUp(self):
        input_ws = Load(Filename="TOFTOFTestdata.nxs")
        self._input_ws_base = input_ws
        self._input_good = input_ws
        AddSampleLogMultiple(Workspace=self._input_good, LogNames=["run_number"], LogValues=["001"])

        self._input_bad_entry = input_ws + 0
        # remove a compulsory entry in Logs
        DeleteLog(self._input_bad_entry, "duration")

        self._input_bad_value = input_ws + 0
        AddSampleLogMultiple(Workspace=self._input_bad_value, LogNames=["wavelength"], LogValues=[0.0])

    def test_success(self):
        OutputWorkspaceName = "output_ws"
        Inputws = "%s, %s" % (self._input_ws_base.name(), self._input_good.name())

        alg_test = run_algorithm("TOFTOFMergeRuns", InputWorkspaces=Inputws, OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())

        wsoutput = AnalysisDataService.retrieve(OutputWorkspaceName)

        run_out = wsoutput.getRun()
        run_in = self._input_ws_base.getRun()
        self.assertAlmostEqual(run_out.getLogData("wavelength").value, run_in.getLogData("wavelength").value)
        self.assertEqual(run_out.getLogData("chopper_speed").value, run_in.getLogData("chopper_speed").value)
        self.assertEqual(run_out.getLogData("chopper_ratio").value, run_in.getLogData("chopper_ratio").value)
        self.assertEqual(run_out.getLogData("channel_width").value, run_in.getLogData("channel_width").value)
        self.assertAlmostEqual(run_out.getLogData("Ei").value, run_in.getLogData("Ei").value)
        self.assertEqual(run_out.getLogData("EPP").value, run_in.getLogData("EPP").value)
        self.assertEqual(run_out.getLogData("proposal_number").value, run_in.getLogData("proposal_number").value)
        self.assertEqual(run_out.getLogData("proposal_title").value, run_in.getLogData("proposal_title").value)
        self.assertEqual(run_out.getLogData("mode").value, run_in.getLogData("mode").value)
        self.assertEqual(run_out.getLogData("experiment_team").value, run_in.getLogData("experiment_team").value)

        run_in_good = self._input_good.getRun()
        self.assertEqual(
            run_out.getLogData("run_number").value, str([run_in.getLogData("run_number").value, run_in_good.getLogData("run_number").value])
        )

        self.assertAlmostEqual(run_out.getLogData("temperature").value, float(run_in.getLogData("temperature").value))
        self.assertEqual(
            run_out.getLogData("duration").value,
            float(run_in.getLogData("duration").value) + float(run_in_good.getLogData("duration").value),
        )
        self.assertEqual(run_out.getLogData("run_start").value, run_in.getLogData("run_start").value)
        self.assertEqual(run_out.getLogData("run_end").value, run_in.getLogData("run_end").value)
        self.assertEqual(run_out.getLogData("full_channels").value, run_in.getLogData("full_channels").value)
        self.assertEqual(run_out.getLogData("monitor_counts").value, 2 * int(run_in.getLogData("monitor_counts").value))
        # Dimension output workspace
        self.assertEqual(wsoutput.getNumberHistograms(), self._input_ws_base.getNumberHistograms())
        self.assertEqual(wsoutput.blocksize(), self._input_ws_base.blocksize())
        # check instrument
        self.assertEqual(wsoutput.getInstrument().getName(), "TOFTOF")

        AnalysisDataService.remove("output_ws")

    def test_failed(self):
        """
        Failed tests because of missing keys or different values
        """
        OutputWorkspaceName = "output_ws"
        Inputws_badvalue = "%s, %s" % (self._input_ws_base.name(), self._input_bad_value.name())
        self.assertRaisesRegex(
            RuntimeError,
            "Sample logs wavelength do not match!",
            run_algorithm,
            "TOFTOFMergeRuns",
            InputWorkspaces=Inputws_badvalue,
            OutputWorkspace=OutputWorkspaceName,
            rethrow=True,
        )

        Inputws_badentry = "%s, %s" % (self._input_ws_base.name(), self._input_bad_entry.name())
        self.assertRaisesRegex(
            RuntimeError,
            r"Workspace .* does not have property duration. Cannot merge.",
            run_algorithm,
            "TOFTOFMergeRuns",
            InputWorkspaces=Inputws_badentry,
            OutputWorkspace=OutputWorkspaceName,
            rethrow=True,
        )

        if AnalysisDataService.doesExist("output_ws"):
            AnalysisDataService.remove("output_ws")

    def test_single_ws(self):
        OutputWorkspaceName = "output_ws"
        Inputws = self._input_ws_base.name()
        alg_test = run_algorithm("TOFTOFMergeRuns", InputWorkspaces=Inputws, OutputWorkspace=OutputWorkspaceName)
        self.assertTrue(alg_test.isExecuted())

    def cleanUp(self):
        if self._input_ws_base is not None:
            DeleteWorkspace(self._input_ws_base)


if __name__ == "__main__":
    unittest.main()
