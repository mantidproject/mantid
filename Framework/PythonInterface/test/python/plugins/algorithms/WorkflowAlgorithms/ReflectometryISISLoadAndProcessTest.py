# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import math
import numpy as np
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import MagicMock, patch

from mantid import config, FileFinder
from mantid.api import AnalysisDataService
from mantid.simpleapi import (
    AddSampleLog,
    AddTimeSeriesLog,
    CreateSampleWorkspace,
    CreateWorkspace,
    CropWorkspace,
    GroupWorkspaces,
    JoinISISPolarizationEfficiencies,
    LoadEmptyInstrument,
    Rebin,
)
from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISLoadAndProcess import ReflectometryISISLoadAndProcess
from testhelpers import assertRaisesNothing, create_algorithm


class ReflectometryISISLoadAndProcessTest(unittest.TestCase):
    _CALIBRATION_TEST_DATA = FileFinder.getFullPath("ISISReflectometry/calibration_test_data_INTER45455.dat")
    _INTER45455_DETECTOR_ROI = "6-62"
    _TEST_2D_WS_DETECTOR_ROI = "4-7"

    def setUp(self):
        self._oldFacility = config["default.facility"]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        self._oldInstrument = config["default.instrument"]
        config["default.facility"] = "ISIS"
        config["default.instrument"] = "INTER"
        # Set some commonly used properties
        self._default_options = {
            "ProcessingInstructions": "3",
            "ThetaIn": 0.5,
            "WavelengthMin": 2,
            "WavelengthMax": 5,
            "I0MonitorIndex": 1,
            "MomentumTransferStep": 0.02,
        }
        self._mandatory_output_names = {"OutputWorkspace": "testIvsQ", "OutputWorkspaceBinned": "testIvsQBin"}
        self._all_output_names = {
            "OutputWorkspace": "testIvsQ",
            "OutputWorkspaceBinned": "testIvsQBin",
            "OutputWorkspaceWavelength": "testIvsLam",
        }
        self._default_slice_options_dummy_run = {"SliceWorkspace": True, "TimeInterval": 1200}
        self._default_slice_options_real_run = {"SliceWorkspace": True, "TimeInterval": 210}
        self._expected_dummy_time_sliced_outputs = [
            "IvsQ_38415",
            "IvsQ_38415_sliced_0_1200",
            "IvsQ_38415_sliced_1200_2400",
            "IvsQ_38415_sliced_2400_3600",
            "IvsQ_binned_38415",
            "IvsQ_binned_38415_sliced_0_1200",
            "IvsQ_binned_38415_sliced_1200_2400",
            "IvsLam_38415",
            "IvsLam_38415_sliced_0_1200",
            "IvsLam_38415_sliced_1200_2400",
            "IvsLam_38415_sliced_2400_3600",
            "IvsQ_binned_38415_sliced_2400_3600",
            "TOF_38415",
            "TOF_38415_monitors",
            "TOF_38415_sliced_0_1200",
            "TOF_38415_sliced_1200_2400",
            "TOF_38415_sliced_2400_3600",
            "TOF",
        ]

        self._expected_real_time_sliced_outputs = [
            "IvsQ_38415",
            "IvsQ_38415_sliced_0_210",
            "IvsQ_38415_sliced_210_420",
            "IvsQ_38415_sliced_420_610",
            "IvsQ_binned_38415",
            "IvsQ_binned_38415_sliced_0_210",
            "IvsQ_binned_38415_sliced_210_420",
            "IvsLam_38415",
            "IvsLam_38415_sliced_0_210",
            "IvsLam_38415_sliced_210_420",
            "IvsLam_38415_sliced_420_610",
            "IvsQ_binned_38415_sliced_420_610",
            "TOF_38415",
            "TOF_38415_monitors",
            "TOF_38415_sliced_0_210",
            "TOF_38415_sliced_210_420",
            "TOF_38415_sliced_420_610",
            "TOF",
        ]

        self._expected_time_sliced_group_outputs = [
            "TOF",
            "TOF_38426_1",
            "TOF_38426_2",
            "TOF_38426_2_monitors",
            "TOF_38426_1_monitors",
            "TOF_38426_1_TOF_38426_1_monitors_TOF_38426_sliced_0_0_1800",
            "TOF_38426_2_TOF_38426_2_monitors_TOF_38426_sliced_1_0_1800",
            "TOF_38426_1_TOF_38426_1_monitors_TOF_38426_sliced_0_1800_3600",
            "TOF_38426_2_TOF_38426_2_monitors_TOF_38426_sliced_1_1800_3600",
            "IvsQ_TOF_38426_sliced_0_1800",
            "IvsQ_TOF_38426_sliced_0_1800_1_TOF_38426_1_monitors_TOF_38426_sliced_0_0_1800",
            "IvsQ_TOF_38426_sliced_0_1800_2_TOF_38426_2_monitors_TOF_38426_sliced_1_0_1800",
            "IvsQ_TOF_38426_sliced_1800_3600",
            "IvsQ_TOF_38426_sliced_1800_3600_1_TOF_38426_1_monitors_TOF_38426_sliced_0_1800_3600",
            "IvsQ_TOF_38426_sliced_1800_3600_2_TOF_38426_2_monitors_TOF_38426_sliced_1_1800_3600",
            "IvsLam_TOF_38426_sliced_0_1800",
            "IvsLam_TOF_38426_sliced_0_1800_2_TOF_38426_2_monitors_TOF_38426_sliced_1_0_1800",
            "IvsLam_TOF_38426_sliced_0_1800_1_TOF_38426_1_monitors_TOF_38426_sliced_0_0_1800",
            "IvsLam_TOF_38426_sliced_1800_3600",
            "IvsLam_TOF_38426_sliced_1800_3600_1_TOF_38426_1_monitors_TOF_38426_sliced_0_1800_3600",
            "IvsLam_TOF_38426_sliced_1800_3600_2_TOF_38426_2_monitors_TOF_38426_sliced_1_1800_3600",
            "IvsQ_binned_TOF_38426_sliced_0_1800",
            "IvsQ_binned_TOF_38426_sliced_0_1800_1_TOF_38426_1_monitors_TOF_38426_sliced_0_0_1800",
            "IvsQ_binned_TOF_38426_sliced_0_1800_2_TOF_38426_2_monitors_TOF_38426_sliced_1_0_1800",
            "IvsQ_binned_TOF_38426_sliced_1800_3600",
            "IvsQ_binned_TOF_38426_sliced_1800_3600_1_TOF_38426_1_monitors_TOF_38426_sliced_0_1800_3600",
            "IvsQ_binned_TOF_38426_sliced_1800_3600_2_TOF_38426_2_monitors_TOF_38426_sliced_1_1800_3600",
        ]

    def tearDown(self):
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument

    def test_detector_mask_is_preserved_in_sliced_results_without_masking_cached_tof(self):
        self._create_event_workspace(38415, "TOF_", bank_pixel_width=2)
        cached = AnalysisDataService.retrieve("TOF_38415")
        detector_id = next(iter(cached.getSpectrum(0).getDetectorIDs()))
        args = dict(
            self._default_options,
            **self._default_slice_options_dummy_run,
            InputRunList="38415",
            ProcessingInstructions="4-7",
            DetectorMask=str(detector_id),
        )
        self._assert_run_algorithm_succeeds(args)

        for name in ("TOF_processed", "IvsQ_38415", "IvsQ_binned_38415", "IvsLam_38415"):
            for workspace in AnalysisDataService.retrieve(name):
                detectors = workspace.detectorInfo()
                self.assertTrue(detectors.isMasked(detectors.indexOf(detector_id)))
        cached = AnalysisDataService.retrieve("TOF_38415")
        self.assertFalse(cached.spectrumInfo().isMasked(0))

    def test_all_property_settings_can_be_evaluated(self):
        alg = create_algorithm("ReflectometryISISLoadAndProcess")
        for prop in alg.getProperties():
            settings = prop.settings
            if settings is None:
                continue
            if not isinstance(settings, list):
                settings = [settings]
            for setting in settings:
                with self.subTest(property=prop.name):
                    self.assertIsInstance(setting.isEnabled(alg), bool)
                    self.assertIsInstance(setting.isVisible(alg), bool)

    def test_missing_input_runs(self):
        self._assert_run_algorithm_throws()

    def test_input_run_is_loaded_if_not_in_ADS(self):
        args = self._default_options
        args["InputRunList"] = "13460"
        args["ProcessingInstructions"] = "4"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryISISPreprocess", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_input_run_that_is_in_ADS_with_prefixed_name_is_not_reloaded(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_cache_reuse_does_not_depend_on_calibration_logs(self):
        self._create_workspace(13460, "TOF_")
        AddSampleLog(
            Workspace="TOF_13460",
            LogName="reflectometry_calibration_file",
            LogText=self._CALIBRATION_TEST_DATA,
        )
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("CalibrationFile", self._CALIBRATION_TEST_DATA)

        workspace_name = alg._getRunFromADSOrNone("13460", False)

        self.assertEqual("TOF_13460", workspace_name)

    def test_input_run_that_is_in_ADS_without_prefix_is_not_reloaded(self):
        self._create_workspace(13460)
        args = self._default_options
        args["InputRunList"] = "13460"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_input_run_is_reloaded_if_in_ADS_with_unknown_prefix(self):
        self._create_workspace(13460, "TEST_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["ProcessingInstructions"] = "4"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TEST_13460", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryISISPreprocess", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_existing_workspace_is_used_when_name_passed_in_input_list(self):
        self._create_workspace(13460, "TEST_")
        args = self._default_options
        args["InputRunList"] = "TEST_13460"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TEST_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_loading_run_with_instrument_prefix_in_name(self):
        args = self._default_options
        args["InputRunList"] = "INTER13460"
        args["ProcessingInstructions"] = "4"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryISISPreprocess", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_loading_workspace_group(self):
        args = self._default_options
        args["InputRunList"] = "POLREF14966"
        args["ProcessingInstructions"] = "4"
        outputs = [
            "IvsLam_14966",
            "IvsLam_14966_1",
            "IvsLam_14966_2",
            "IvsQ_14966",
            "IvsQ_14966_1",
            "IvsQ_14966_2",
            "IvsQ_binned_14966",
            "IvsQ_binned_14966_1",
            "IvsQ_binned_14966_2",
            "TOF_14966_1",
            "TOF_14966_2",
            "TOF",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        # RROA is called for each member of the group and then they are grouped together to form the output group
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_14966_1"), history, False)

    def test_overriding_output_names(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args.update(self._mandatory_output_names)
        outputs = ["testIvsQ", "testIvsQBin", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("testIvsQBin"), history)

    def test_overriding_output_names_includes_all_specified_outputs(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args.update(self._all_output_names)
        # Current behaviour is that the optional workspaces are output even if
        # Debug is not set
        outputs = ["testIvsQ", "testIvsQBin", "testIvsLam", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("testIvsQBin"), history)

    def test_hide_workspaces_option_hides_input_workspaces(self):
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        args["ProcessingInstructions"] = "4"
        args["HideInputWorkspaces"] = True
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TRANS_LAM_13463"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryISISPreprocess", "ReflectometryISISPreprocess", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_debug_option_outputs_extra_workspaces(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["Debug"] = True
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "IvsLam_13460", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_debug_option_outputs_extra_workspaces_with_overridden_names(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["Debug"] = True
        args.update(self._all_output_names)
        outputs = ["testIvsQ", "testIvsQBin", "testIvsLam", "TOF_13460", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("testIvsQBin"), history)

    def test_multiple_input_runs_are_summed(self):
        self._create_workspace(13461, "TOF_")
        self._create_workspace(13462, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13461, 13462"
        outputs = ["IvsQ_13461+13462", "IvsQ_binned_13461+13462", "TOF_13461", "TOF_13462", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["MergeRuns", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13461+13462"), history)

        merged_history = AnalysisDataService.retrieve("IvsQ_binned_13461+13462").getHistory()
        self.assertEqual(self._count_clones(merged_history.getAlgorithmHistory(merged_history.size() - 1)), 0)

    def test_trans_run_is_not_reloaded_if_in_ADS(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TRANS_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TRANS_13463", "TRANS_LAM_13463", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_trans_runs_are_loaded_if_not_in_ADS(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        args["SecondTransmissionRunList"] = "13464"
        # Expect IvsQ outputs from the reduction, and intermediate LAM outputs from
        # creating the stitched transmission run
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TRANS_13463", "TRANS_13464", "TRANS_LAM_13463_13464", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryISISPreprocess", "ReflectometryISISPreprocess", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_trans_runs_are_not_loaded_if_in_ADS_without_prefix(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463)
        self._create_workspace(13464)
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        args["SecondTransmissionRunList"] = "13464"
        # Expect IvsQ outputs from the reduction, and initermediate LAM outputs from
        # creating the stitched transmission run
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "13463", "13464", "TRANS_LAM_13463_13464", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_trans_runs_are_reloaded_if_in_ADS_with_unknown_prefix(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TEST_")
        self._create_workspace(13464, "TEST_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        args["SecondTransmissionRunList"] = "13464"
        # Expect IvsQ outputs from the reduction, and initermediate LAM outputs from
        # creating the stitched transmission run
        outputs = [
            "IvsQ_13460",
            "IvsQ_binned_13460",
            "TOF_13460",
            "TRANS_13463",
            "TRANS_13464",
            "TEST_13463",
            "TEST_13464",
            "TRANS_LAM_13463_13464",
            "TOF",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryISISPreprocess", "ReflectometryISISPreprocess", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_existing_workspace_is_used_for_trans_runs_when_name_passed_in_input_list(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TEST_")
        self._create_workspace(13464, "TEST_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "TEST_13463"
        args["SecondTransmissionRunList"] = "TEST_13464"
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TEST_13463", "TEST_13464", "TRANS_LAM_13463_13464", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_first_and_second_trans_runs_are_combined(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TRANS_")
        self._create_workspace(13464, "TRANS_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        args["SecondTransmissionRunList"] = "13464"
        # The intermediate LAM workspaces are output by the algorithm that combines the runs
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TOF_13460", "TRANS_13463", "TRANS_13464", "TRANS_LAM_13463_13464", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_first_and_second_trans_runs_are_combined_with_debug_output(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TRANS_")
        self._create_workspace(13464, "TRANS_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["Debug"] = True
        args["FirstTransmissionRunList"] = "13463"
        args["SecondTransmissionRunList"] = "13464"
        # The intermediate LAM workspaces are output by the algorithm that combines the runs.
        # The stitched TRANS_LAM_13463_13464 is only output with Debug on.
        outputs = [
            "IvsQ_13460",
            "IvsQ_binned_13460",
            "IvsLam_13460",
            "TOF_13460",
            "TRANS_13463",
            "TRANS_13464",
            "TRANS_LAM_13463",
            "TRANS_LAM_13464",
            "TRANS_LAM_13463_13464",
            "TOF",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_multiple_trans_runs_are_summed(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TRANS_")
        self._create_workspace(13464, "TRANS_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463, 13464"
        outputs = [
            "IvsQ_13460",
            "IvsQ_binned_13460",
            "TOF_13460",
            "TRANS_13463",
            "TRANS_13464",
            "TRANS_LAM_13463+13464",
            "TOF",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["MergeRuns", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13460"), history)

    def test_slicing_is_disallowed_if_summing_input_runs(self):
        self._create_workspace(13460, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["SliceWorkspace"] = True
        self._assert_run_algorithm_fails(args)

    def test_slicing_uses_run_in_ADS_with_correct_prefix(self):
        self._create_event_workspace(38415, "TOF_")
        args = self._default_options.copy()
        args.update(self._default_slice_options_dummy_run)
        args["InputRunList"] = "38415"
        args["ProcessingInstructions"] = "4"
        outputs = self._expected_dummy_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        # Note that the child sliced workspaces don't include the full history - this
        # might be something we want to change in the underlying algorithms at some point
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_1200"), history, False)

    def test_slicing_uses_run_in_ADS_with_no_prefix(self):
        self._create_event_workspace(38415)
        args = self._default_options
        args.update(self._default_slice_options_dummy_run)
        args["InputRunList"] = "38415"
        args["ProcessingInstructions"] = "4"
        outputs = [
            "IvsQ_38415",
            "IvsQ_38415_1",
            "IvsQ_38415_2",
            "IvsQ_38415_3",
            "IvsQ_binned_38415",
            "IvsQ_binned_38415_1",
            "IvsQ_binned_38415_2",
            "IvsLam_38415",
            "IvsLam_38415_1",
            "IvsLam_38415_2",
            "IvsLam_38415_3",
            "IvsQ_binned_38415_3",
            "38415",
            "38415_monitors",
            "38415_sliced_0_1200",
            "38415_sliced_1200_2400",
            "38415_sliced_2400_3600",
            "TOF",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_1"), history, False)

    def test_slicing_loads_input_run_if_not_in_ADS(self):
        args = self._default_options
        args.update(self._default_slice_options_real_run)
        args["InputRunList"] = "38415"
        args["ProcessingInstructions"] = "4"
        outputs = self._expected_real_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_210"), history, False)

    def test_slicing_reloads_input_run_if_workspace_is_incorrect_type(self):
        for grouped in (False, True):
            with self.subTest(grouped=grouped):
                AnalysisDataService.clear()
                if grouped:
                    self._create_workspace_group(38415, 2, "TOF_")
                    alg = ReflectometryISISLoadAndProcess()
                    alg.initialize()
                    alg._getInputWorkspaces(["38415"], False)
                    alg._groupTOFWorkspaces(["TOF_38415"])
                else:
                    self._create_workspace(38415, "TOF_")
                args = dict(self._default_options, **self._default_slice_options_real_run, InputRunList="38415", ProcessingInstructions="4")
                self._assert_run_algorithm_succeeds(args, self._expected_real_time_sliced_outputs)
                history = [
                    "ReflectometryReductionOneAuto",
                    "ReflectometryReductionOneAuto",
                    "ReflectometryReductionOneAuto",
                    "GroupWorkspaces",
                ]
                self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_210"), history, False)

    def test_slicing_loads_input_run_if_monitor_ws_not_in_ADS(self):
        self._create_event_workspace(38415, "TOF_", includeMonitors=False)
        args = self._default_options
        args.update(self._default_slice_options_real_run)
        args["InputRunList"] = "38415"
        args["ProcessingInstructions"] = "4"
        outputs = self._expected_real_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_210"), history, False)

    def test_slicing_with_no_interval_returns_single_slice(self):
        self._create_event_workspace(38415, "TOF_")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ProcessingInstructions"] = "4"
        args["SliceWorkspace"] = True
        outputs = [
            "IvsQ_38415",
            "IvsQ_38415_sliced_0_4200",
            "IvsQ_binned_38415",
            "IvsQ_binned_38415_sliced_0_4200",
            "IvsLam_38415",
            "IvsLam_38415_sliced_0_4200",
            "TOF_38415",
            "TOF_38415_monitors",
            "TOF_38415_sliced_0_4200",
            "TOF",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_4200"), history, False)

    def test_slicing_by_number_of_slices(self):
        self._create_event_workspace(38415, "TOF_")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["SliceWorkspace"] = True
        args["NumberOfSlices"] = 3
        args["ProcessingInstructions"] = "4"
        outputs = self._expected_dummy_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_1200"), history, False)

    def test_slicing_by_log_value(self):
        self._create_event_workspace(38415, "TOF_")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["SliceWorkspace"] = True
        args["LogName"] = "proton_charge"
        args["LogValueInterval"] = 60
        args["ProcessingInstructions"] = "4"
        slice1 = "_sliced_Log.proton_charge.From.-30.To.30.Value-change-direction:both"
        slice2 = "_sliced_Log.proton_charge.From.30.To.90.Value-change-direction:both"
        slice3 = "_sliced_Log.proton_charge.From.90.To.150.Value-change-direction:both"
        outputs = [
            "IvsQ_38415",
            "IvsQ_38415" + slice1,
            "IvsQ_38415" + slice2,
            "IvsQ_38415" + slice3,
            "IvsQ_binned_38415",
            "IvsQ_binned_38415" + slice1,
            "IvsQ_binned_38415" + slice2,
            "IvsQ_binned_38415" + slice3,
            "IvsLam_38415",
            "IvsLam_38415" + slice1,
            "IvsLam_38415" + slice2,
            "IvsLam_38415" + slice3,
            "TOF",
            "TOF_38415",
            "TOF_38415_monitors",
            "TOF_38415" + slice1,
            "TOF_38415" + slice2,
            "TOF_38415" + slice3,
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415" + slice1), history, False)

    def test_slicing_by_log_value_with_no_interval_returns_single_slice(self):
        self._create_event_workspace(38415, "TOF_")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["SliceWorkspace"] = True
        args["LogName"] = "proton_charge"
        args["ProcessingInstructions"] = "4"
        sliceName = "_sliced_Log.proton_charge.From.0.To.100.Value-change-direction:both"
        outputs = [
            "IvsQ_38415",
            "IvsQ_38415" + sliceName,
            "IvsQ_binned_38415",
            "IvsQ_binned_38415" + sliceName,
            "IvsLam_38415",
            "IvsLam_38415" + sliceName,
            "TOF",
            "TOF_38415",
            "TOF_38415_monitors",
            "TOF_38415" + sliceName,
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415" + sliceName), history, False)

    def test_slicing_by_number_of_slices_with_group_maintains_history(self):
        self._create_workspace_group(38426, 2, "TOF_", self._create_event_workspace, has_monitors=True)
        args = self._default_options
        args["InputRunList"] = "38426"
        args["SliceWorkspace"] = True
        args["NumberOfSlices"] = 2
        args["ProcessingInstructions"] = "4"
        with patch.object(ReflectometryISISLoadAndProcess, "_loadRun", side_effect=AssertionError("Cache should be reused")):
            for _ in range(2):
                self._assert_run_algorithm_succeeds(args, self._expected_time_sliced_group_outputs)
        self.assertEqual(
            list(AnalysisDataService.retrieve("TOF").getNames()),
            ["TOF_38426_1", "TOF_38426_2", "TOF_38426_1_monitors", "TOF_38426_2_monitors"],
        )
        processed_group = AnalysisDataService.retrieve("TOF_processed")
        slice_members = {name for name in self._expected_time_sliced_group_outputs if name.startswith("TOF_38426") and "_sliced_" in name}
        self.assertTrue(slice_members.issubset(set(processed_group.getNames())))
        self.assertTrue(all(not workspace.isGroup() for workspace in processed_group))
        self.assertFalse(AnalysisDataService.doesExist("TOF_38426_sliced_0_1800"))
        self.assertFalse(AnalysisDataService.doesExist("TOF_38426_sliced_1800_3600"))
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(
            AnalysisDataService.retrieve("IvsQ_binned_TOF_38426_sliced_0_1800_2_TOF_38426_2_monitors_TOF_38426_sliced_1_0_1800"),
            history,
            False,
        )

    def test_with_input_workspace_group(self):
        self._create_workspace_group(12345, 2, "TOF_")
        args = self._default_options
        args["InputRunList"] = "12345"
        outputs = [
            "IvsQ_12345",
            "IvsQ_12345_1",
            "IvsQ_12345_2",
            "IvsQ_binned_12345",
            "IvsQ_binned_12345_2",
            "IvsQ_binned_12345_1",
            "IvsLam_12345",
            "IvsLam_12345_1",
            "IvsLam_12345_2",
            "TOF",
            "TOF_12345_1",
            "TOF_12345_2",
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = [
            "CreateSampleWorkspace",
            "AddSampleLog",
            "CreateSampleWorkspace",
            "AddSampleLog",
            "GroupWorkspaces",
            "ReflectometryReductionOneAuto",
            "GroupWorkspaces",
        ]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_12345_1"), history, False)

    def test_TOF_input_workspace_groups_collapsed(self):
        self._create_workspace_group(13460, 2, "TOF_")
        GroupWorkspaces("TOF_13460_2,TOF_13460_1", OutputWorkspace="TOF_13460")
        self._create_workspace(13463, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460, 13463"
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460_1", "TOF_13460_2", "TOF_13463"]

        with patch.object(ReflectometryISISLoadAndProcess, "_loadRun", side_effect=AssertionError("Cache should be reused")):
            for _ in range(2):
                self._assert_run_algorithm_succeeds(args, outputs)
                self.assertEqual(list(AnalysisDataService.retrieve("TOF").getNames()), ["TOF_13460_2", "TOF_13460_1", "TOF_13463"])
                for name in ("TOF_13460_2", "TOF_13460_1"):
                    self.assertEqual(
                        AnalysisDataService.retrieve(name).run().getProperty("reflectometry_cached_group_name").value, "TOF_13460"
                    )
                self.assertFalse(any("_refl_work_" in name for name in AnalysisDataService.getObjectNames()))

    def test_fails_with_mixed_unit_input_workspace_group(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TOF_")
        self._create_workspace_wavelength(12345)
        GroupWorkspaces("TOF_13463, 12345", OutputWorkspace="mixed_unit_group")
        args = self._default_options
        args["InputRunList"] = "13460, mixed_unit_group"
        self._assert_run_algorithm_throws(args)

    def test_no_TOF_input_workspace_groups_remain_unchanged(self):
        self._create_workspace_wavelength(12345)
        self._create_workspace_wavelength(67890)
        GroupWorkspaces("12345, 67890", OutputWorkspace="no_TOF_group")
        args = self._default_options
        args["InputRunList"] = "12345, 67890"
        outputs = ["no_TOF_group", "12345", "67890"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_group_TOF_workspaces_succeeds_with_only_TOF_workspaces(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460, 13463"
        args["GroupTOFWorkspaces"] = True
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460", "TOF_13463"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_group_TOF_workspaces_succeeds_with_TOF_group(self):
        self._create_workspace_group(13460, 2, "TOF_")
        self._create_workspace(13463, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460, 13463"
        args["GroupTOFWorkspaces"] = True
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460_1", "TOF_13460_2", "TOF_13463"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_group_TOF_workspaces_fails_with_mixed_unit_workspace_group(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TOF_")
        self._create_workspace_wavelength(12345)
        GroupWorkspaces("TOF_13463, 12345", OutputWorkspace="mixed_unit_group")
        args = self._default_options
        args["InputRunList"] = "13460, mixed_unit_group"
        args["GroupTOFWorkspaces"] = True
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460", "TOF_13463", "12345"]
        self._assert_run_algorithm_fails(args, outputs)

    def test_group_TOF_workspaces_succeeds_with_no_TOF_input_workspace_groups_which_remain_unchanged(self):
        self._create_workspace_wavelength(12345)
        self._create_workspace_wavelength(67890)
        GroupWorkspaces("12345, 67890", OutputWorkspace="no_TOF_group")
        args = self._default_options
        args["InputRunList"] = "12345, 67890"
        args["GroupTOFWorkspaces"] = True
        outputs = ["no_TOF_group", "12345", "67890"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_sum_banks_runs_for_2D_detector_workspace_when_detector_ROI_specified(self):
        self._create_2D_detector_workspace(38415, "TOF_")

        def _check_num_histograms_and_first_y_value(ws_name, expected_num_histograms, expected_y_value):
            ws = AnalysisDataService.retrieve(ws_name)
            self.assertTrue(ws.getNumberHistograms(), expected_num_histograms)
            self._assert_delta(ws.y(2)[0], expected_y_value)

        _check_num_histograms_and_first_y_value("TOF_38415", 6, 0.3)
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        outputs = ["IvsQ_38415", "IvsQ_binned_38415", "TOF", "TOF_38415", "TOF_38415_summed_segment"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_sum_banks(AnalysisDataService.retrieve("IvsQ_binned_38415"), is_summed=True)
        _check_num_histograms_and_first_y_value("TOF_38415_summed_segment", 4, 0.6)
        self.assertIn("TOF_38415_summed_segment", AnalysisDataService.retrieve("TOF_processed").getNames())
        self.assertEqual(set(AnalysisDataService.retrieve("TOF").getNames()), {"TOF_38415"})

    def test_sum_banks_runs_for_each_2D_detector_workspace_group_member(self):
        self._create_workspace_group(38415, 2, "TOF_", self._create_2D_detector_workspace)
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        outputs = [
            "IvsLam_38415",
            "IvsLam_38415_1",
            "IvsLam_38415_2",
            "IvsQ_38415",
            "IvsQ_38415_1",
            "IvsQ_38415_2",
            "IvsQ_binned_38415",
            "IvsQ_binned_38415_1",
            "IvsQ_binned_38415_2",
            "TOF",
            "TOF_38415_1",
            "TOF_38415_1_summed_segment",
            "TOF_38415_2",
            "TOF_38415_2_summed_segment",
            "TOF_summed_segment",
        ]

        self._assert_run_algorithm_succeeds(args, outputs)

        self.assertEqual(list(AnalysisDataService.retrieve("IvsQ_38415").getNames()), ["IvsQ_38415_1", "IvsQ_38415_2"])
        for index in (1, 2):
            summed = AnalysisDataService.retrieve(f"TOF_38415_{index}_summed_segment")
            self.assertEqual(summed.getNumberHistograms(), 4)
            self._check_sum_banks(AnalysisDataService.retrieve(f"IvsQ_binned_38415_{index}"), is_summed=True)

    def test_reduction_algorithm_sums_each_2D_detector_workspace_group_member(self):
        self._create_workspace_group(38415, 2, "TOF_", self._create_2D_detector_workspace)
        alg = create_algorithm(
            "ReflectometryReductionOneAuto",
            InputWorkspace="TOF_38415",
            ROIDetectorIDs=self._TEST_2D_WS_DETECTOR_ROI,
            **self._default_options,
        )

        assertRaisesNothing(self, alg.execute)

        self.assertEqual(
            list(AnalysisDataService.retrieve("TOF_summed_segment").getNames()),
            ["TOF_38415_1_summed_segment", "TOF_38415_2_summed_segment"],
        )
        self.assertEqual(list(AnalysisDataService.retrieve("IvsQ_38415").getNames()), ["IvsQ_38415_1", "IvsQ_38415_2"])

    def test_reduction_algorithm_preserves_informative_group_member_names_when_summing(self):
        self._create_2D_detector_workspace(38415, "TOF_", "_up")
        self._create_2D_detector_workspace(38415, "TOF_", "_down")
        GroupWorkspaces("TOF_38415_up,TOF_38415_down", OutputWorkspace="TOF_38415")
        alg = create_algorithm(
            "ReflectometryReductionOneAuto",
            InputWorkspace="TOF_38415",
            ROIDetectorIDs=self._TEST_2D_WS_DETECTOR_ROI,
            **self._default_options,
        )

        assertRaisesNothing(self, alg.execute)

        self.assertEqual(
            list(AnalysisDataService.retrieve("TOF_summed_segment").getNames()),
            ["TOF_38415_up_summed_segment", "TOF_38415_down_summed_segment"],
        )
        self.assertEqual(list(AnalysisDataService.retrieve("IvsQ_38415").getNames()), ["IvsQ_38415_up", "IvsQ_38415_down"])

    def test_sum_banks_rejects_workspace_group_with_mixed_detector_types(self):
        self._create_2D_detector_workspace(38415, "TOF_", "_1")
        self._create_workspace(38415, "TOF_", "_2")
        GroupWorkspaces("TOF_38415_1,TOF_38415_2", OutputWorkspace="TOF_38415")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI

        self._assert_run_algorithm_throws_with_correct_msg(
            args, "some but not all input and transmission workspaces require summing across banks"
        )

    def test_sum_banks_runs_for_polarized_2D_detector_workspace_group(self):
        self._create_workspace_group(38415, 4, "TOF_", self._create_2D_detector_workspace)
        self._create_polarization_efficiencies_workspace("efficiencies")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        args["PolarizationAnalysis"] = "1"
        args["PolarizationEfficiencies"] = "efficiencies"

        self._assert_run_algorithm_succeeds(args)

        self.assertEqual(
            list(AnalysisDataService.retrieve("IvsQ_38415").getNames()),
            ["IvsQ_38415_1", "IvsQ_38415_2", "IvsQ_38415_3", "IvsQ_38415_4"],
        )
        for index in range(1, 5):
            self._check_sum_banks(AnalysisDataService.retrieve(f"IvsQ_binned_38415_{index}"), is_summed=True)

    def test_sum_banks_preserves_group_output_names_when_summed_workspaces_are_hidden(self):
        self._create_workspace_group(38415, 2, "__TOF_", self._create_2D_detector_workspace)
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        args["HideInputWorkspaces"] = True

        self._assert_run_algorithm_succeeds(args)

        self.assertEqual(list(AnalysisDataService.retrieve("IvsQ_38415").getNames()), ["IvsQ_38415_1", "IvsQ_38415_2"])
        self.assertEqual(
            list(AnalysisDataService.retrieve("__TOF_summed_segment").getNames()),
            ["__TOF_38415_1_summed_segment", "__TOF_38415_2_summed_segment"],
        )
        self.assertTrue(AnalysisDataService.doesExist("__TOF_38415_1_summed_segment"))
        self.assertTrue(AnalysisDataService.doesExist("__TOF_38415_2_summed_segment"))

    def test_sum_banks_uses_only_first_transmission_group_member(self):
        self._create_2D_detector_workspace(38415, "TOF_")
        self._create_2D_detector_workspace(38416, "TRANS_", "_1")
        self._create_workspace(38416, "TRANS_", "_2")
        GroupWorkspaces("TRANS_38416_1,TRANS_38416_2", OutputWorkspace="TRANS_38416")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["FirstTransmissionRunList"] = "38416"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI

        self._assert_run_algorithm_succeeds(args)

        self._check_sum_banks(AnalysisDataService.retrieve("IvsQ_binned_38415"), is_summed=True)
        self.assertEqual(AnalysisDataService.retrieve("TRANS_38416_2").getNumberHistograms(), 3)

    def test_sum_banks_not_run_for_2D_detector_workspace_when_no_detector_ROI(self):
        self._create_2D_detector_workspace(38415, "TOF_")
        args = self._default_options
        args["InputRunList"] = "38415"
        outputs = ["IvsQ_38415", "IvsQ_binned_38415", "TOF", "TOF_38415"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_sum_banks(AnalysisDataService.retrieve("IvsQ_binned_38415"), is_summed=False)

    def test_sum_banks_is_not_run_for_linear_detector_workspace(self):
        self._create_workspace(38415, "TOF_")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = "1"
        outputs = ["IvsQ_38415", "IvsQ_binned_38415", "TOF", "TOF_38415"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_sum_banks(AnalysisDataService.retrieve("IvsQ_binned_38415"), is_summed=False)

    def test_sum_banks_not_run_for_mix_of_workspace_types(self):
        self._create_2D_detector_workspace(13460, "TOF_")
        self._create_workspace(13463, "TRANS_")
        args = self._default_options
        args["InputRunList"] = "13460"
        args["FirstTransmissionRunList"] = "13463"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        self._assert_run_algorithm_throws_with_correct_msg(args, "some but not all input and transmission workspaces require summing")

    def test_summed_workspaces_hidden_when_selected(self):
        self._create_2D_detector_workspace(38415, "__TOF_")
        self._create_2D_detector_workspace(38416, "__TRANS_")
        self._create_2D_detector_workspace(38417, "__TRANS_")
        args = self._default_options
        args["InputRunList"] = "38415"
        args["FirstTransmissionRunList"] = "38416"
        args["SecondTransmissionRunList"] = "38417"
        args["HideInputWorkspaces"] = True
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        outputs = ["IvsQ_38415", "IvsQ_binned_38415", "TRANS_LAM_38416_38417"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_sum_banks(AnalysisDataService.retrieve("IvsQ_binned_38415"), is_summed=True)

    def test_calibration_file_is_applied_when_provided(self):
        args = self._default_options
        args["InputRunList"] = "INTER45455"
        del args["ProcessingInstructions"]
        args["CalibrationFile"] = self._CALIBRATION_TEST_DATA
        args["AnalysisMode"] = "MultiDetectorAnalysis"
        args["ROIDetectorIDs"] = self._INTER45455_DETECTOR_ROI
        outputs = ["IvsQ_45455", "IvsQ_binned_45455", "TOF", "TOF_45455", "TOF_45455_summed_segment"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_calibration(AnalysisDataService.retrieve("IvsQ_binned_45455"), is_calibrated=True)

    def test_nominal_specular_pixel_is_unset_by_default(self):
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()

        self.assertTrue(alg.getProperty("NominalSpecularPixelSpectrumNo").isDefault)

    def test_nominal_specular_pixel_is_forwarded_to_polref_calibration(self):
        workspace = MagicMock()
        workspace.getInstrumentName.return_value = "POLREF"
        calibration_alg = MagicMock()
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("CorrectDetectors", False)
        alg.setProperty("NominalSpecularPixelSpectrumNo", 281.5)

        with (
            patch.object(alg, "_find_specular_pixel_spectrum_no", return_value=282.0),
            patch.object(alg, "_experiment_angle", return_value=0.5),
        ):
            alg._setInstrumentSpecificProperties(calibration_alg, workspace, adjust_theta=True)

        calibration_alg.setProperty.assert_any_call("NominalSpecularPixelSpectrumNo", 281.5)
        calibration_alg.setProperty.assert_any_call("SpecularPixelSpectrumNo", 282.0)
        calibration_alg.setProperty.assert_any_call("AdjustExperimentAngle", True)

    def test_actual_specular_pixel_is_unset_by_default(self):
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()

        self.assertTrue(alg.getProperty("ActualSpecularPixelSpectrumNo").isDefault)

    def test_actual_specular_pixel_rejects_negative_values(self):
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()

        with self.assertRaises(ValueError):
            alg.setProperty("ActualSpecularPixelSpectrumNo", -0.5)

    def test_actual_specular_pixel_bypasses_fitting_and_controls_angle_correction(self):
        for nominal, expected_nominal, correct_angle in [(None, 282.5, False), (282.5, 282.5, False), (281.0, 281.0, True)]:
            with self.subTest(nominal=nominal):
                workspace = MagicMock()
                workspace.getInstrumentName.return_value = "POLREF"
                calibration_alg = MagicMock()
                alg = ReflectometryISISLoadAndProcess()
                alg.initialize()
                alg.setProperty("CorrectDetectors", False)
                alg.setProperty("ActualSpecularPixelSpectrumNo", 282.5)
                if nominal is not None:
                    alg.setProperty("NominalSpecularPixelSpectrumNo", nominal)

                alg._validate_instrument_specific_calibration_properties(workspace)
                with patch.object(alg, "_find_specular_pixel_spectrum_no") as fit:
                    angle, has_geometry = alg._setInstrumentSpecificProperties(calibration_alg, workspace, True, experiment_angle=0.5)

                fit.assert_not_called()
                self.assertEqual(0.5, angle)
                self.assertTrue(has_geometry)
                calibration_alg.setProperty.assert_any_call("SpecularPixelSpectrumNo", 282.5)
                calibration_alg.setProperty.assert_any_call("NominalSpecularPixelSpectrumNo", expected_nominal)
                calibration_alg.setProperty.assert_any_call("AdjustExperimentAngle", correct_angle)

    def test_polref_specular_pixel_inputs_produce_expected_angle_and_detector_alignment(self):
        cases = [(280.5, None, 0.5), (280.5, 280.5, 0.5), (280.5, 280.0, 0.55), (None, 280.0, 0.55)]
        with TemporaryDirectory() as directory:
            calibration_file = Path(directory) / "calibration.dat"
            calibration_file.write_text("spectrumnumber angle\n279 0.2\n280 0.1\n281 0.0\n")
            for actual, nominal, expected_angle in cases:
                with self.subTest(actual=actual, nominal=nominal):
                    LoadEmptyInstrument(InstrumentName="POLREF", OutputWorkspace="TOF_1")
                    alg = ReflectometryISISLoadAndProcess()
                    alg.initialize()
                    alg.setProperty("CalibrationFile", str(calibration_file))
                    alg.setProperty("CorrectDetectors", False)
                    alg.setProperty("ThetaIn", 0.5)
                    if actual is not None:
                        alg.setProperty("ActualSpecularPixelSpectrumNo", actual)
                    if nominal is not None:
                        alg.setProperty("NominalSpecularPixelSpectrumNo", nominal)

                    with patch.object(alg, "_find_specular_pixel_spectrum_no", return_value=280.5) as fit:
                        _, angle = alg._calibrate_workspace(AnalysisDataService.retrieve("TOF_1"), adjust_theta=True)

                    if actual is None:
                        fit.assert_called_once()
                    else:
                        fit.assert_not_called()
                    self.assertAlmostEqual(expected_angle, angle)
                    output = AnalysisDataService.retrieve("TOF_1")
                    self.assertEqual(expected_angle != 0.5, output.run().hasProperty("reflectometry_adjusted_theta"))
                    indices = [output.getIndexFromSpectrumNumber(number) for number in (280, 281)]
                    actual_two_theta = sum(math.degrees(output.spectrumInfo().signedTwoTheta(index)) for index in indices) / 2.0
                    self.assertAlmostEqual(2.0 * expected_angle, actual_two_theta)

    def test_polref_calibration_requires_actual_or_nominal_specular_pixel(self):
        workspace = MagicMock()
        workspace.getInstrumentName.return_value = "POLREF"
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("CorrectDetectors", False)

        with self.assertRaisesRegex(RuntimeError, "ActualSpecularPixelSpectrumNo or NominalSpecularPixelSpectrumNo must be provided"):
            alg._validate_instrument_specific_calibration_properties(workspace)

    def test_transmission_calibration_uses_nominal_pixel_without_fitting_or_angle_correction(self):
        workspace = MagicMock()
        workspace.getInstrumentName.return_value = "POLREF"
        calibration_alg = MagicMock()
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("ActualSpecularPixelSpectrumNo", 282.5)
        alg.setProperty("NominalSpecularPixelSpectrumNo", 280.0)

        with patch.object(alg, "_find_specular_pixel_spectrum_no") as fit:
            alg._setInstrumentSpecificProperties(calibration_alg, workspace, False, experiment_angle=0.0, specular_spectrum_no=280.0)

        fit.assert_not_called()
        calibration_alg.setProperty.assert_any_call("SpecularPixelSpectrumNo", 280.0)
        calibration_alg.setProperty.assert_any_call("ExperimentAngle", 0.0)
        calibration_alg.setProperty.assert_any_call("AdjustExperimentAngle", False)

    def test_sample_calibration_returns_experiment_angle_when_angle_correction_is_disabled(self):
        self._create_workspace(1, "TOF_")
        workspace = AnalysisDataService.retrieve("TOF_1")
        calibration_alg = MagicMock()
        calibration_alg.getProperty.return_value.value = workspace
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("CalibrationFile", self._CALIBRATION_TEST_DATA)

        with (
            patch.object(alg, "createChildAlgorithm", return_value=calibration_alg),
            patch.object(alg, "_setInstrumentSpecificProperties", return_value=(0.5, True)),
        ):
            _, angle = alg._calibrate_workspace(AnalysisDataService.retrieve("TOF_1"), adjust_theta=True)

        self.assertEqual(0.5, angle)
        self.assertFalse(workspace.run().hasProperty("reflectometry_adjusted_theta"))

    def test_polref_calibration_rejects_detector_correction(self):
        workspace = MagicMock()
        workspace.getInstrumentName.return_value = "POLREF"
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()

        with self.assertRaisesRegex(RuntimeError, "CorrectDetectors must be False when CalibrationFile is provided for POLREF data"):
            alg._validate_instrument_specific_calibration_properties(workspace)

    def test_non_polref_calibration_does_not_reject_detector_correction(self):
        workspace = MagicMock()
        workspace.getInstrumentName.return_value = "INTER"
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()

        alg._validate_instrument_specific_calibration_properties(workspace)

    def test_multiple_input_runs_are_calibrated_once_after_summing(self):
        args = self._default_options
        args["InputRunList"] = "INTER45455, INTER45455"
        del args["ProcessingInstructions"]
        args["CalibrationFile"] = self._CALIBRATION_TEST_DATA
        args["AnalysisMode"] = "MultiDetectorAnalysis"
        args["ROIDetectorIDs"] = self._INTER45455_DETECTOR_ROI

        self._assert_run_algorithm_succeeds(args)

        output_ws = AnalysisDataService.retrieve("IvsQ_binned_45455+45455")
        workflow_history = output_ws.getHistory().getAlgorithmHistory(output_ws.getHistory().size() - 1)
        child_names = [alg.name() for alg in workflow_history.getChildHistories()]
        self.assertEqual(1, child_names.count("ReflectometryISISCalibration"))
        self.assertLess(child_names.index("MergeRuns"), child_names.index("ReflectometryISISCalibration"))

    def test_transmission_calibration_does_not_apply_angle_correction(self):
        self._create_workspace(1, "TOF_")
        workspace = AnalysisDataService.retrieve("TOF_1")
        calibration_alg = MagicMock()
        calibration_alg.getProperty.return_value.value = workspace
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("CalibrationFile", self._CALIBRATION_TEST_DATA)

        with (
            patch.object(alg, "createChildAlgorithm", return_value=calibration_alg),
            patch.object(alg, "_setInstrumentSpecificProperties", return_value=(0.0, True)),
        ):
            alg._calibrate_workspace(
                AnalysisDataService.retrieve("TOF_1"), adjust_theta=False, experiment_angle=0.0, specular_spectrum_no=280.0
            )

        self.assertTrue(workspace.run().hasProperty("reflectometry_calibration_file"))
        self.assertFalse(workspace.run().hasProperty("reflectometry_adjusted_theta"))

    def test_processed_tof_retains_full_detector_data_for_single_summed_and_grouped_inputs(self):
        cases = (
            (False, False, {}),
            (False, True, {}),
            (True, False, {}),
            (False, False, {"HideInputWorkspaces": True}),
            (False, False, {"GroupTOFWorkspaces": False}),
            (True, False, {"HideInputWorkspaces": True}),
            (True, False, {"GroupTOFWorkspaces": False}),
        )
        for grouped, summed, options in cases:
            with self.subTest(grouped=grouped, summed=summed, options=options):
                AnalysisDataService.clear()
                input_prefix = "__TOF_" if options.get("HideInputWorkspaces") else "TOF_"
                if grouped:
                    self._create_workspace_group(1, 2, input_prefix)
                else:
                    self._create_workspace(1, input_prefix)
                if summed:
                    self._create_workspace(2, input_prefix)
                input_workspace = AnalysisDataService.retrieve(input_prefix + "1")
                original = input_workspace[0] if grouped else input_workspace
                original_y = original.extractY().copy()
                alg = create_algorithm(
                    "ReflectometryISISLoadAndProcess",
                    **dict(self._default_options, **self._mandatory_output_names, **options, InputRunList="1,2" if summed else "1"),
                )
                alg.setRethrows(True)
                self.assertTrue(alg.execute())
                if grouped:
                    for member in input_workspace:
                        self.assertEqual(member.run().getProperty("reflectometry_cached_group_name").value, input_prefix + "1")
                output = AnalysisDataService.retrieve("testIvsQBin")
                output = output[0] if grouped else output
                processed_id = output.run().getProperty("reflectometry_processed_tof_id").value
                group_name = "__TOF_processed" if options.get("HideInputWorkspaces") else "TOF_processed"
                if options.get("GroupTOFWorkspaces", True):
                    processed_members = [
                        workspace
                        for workspace in AnalysisDataService.retrieve(group_name)
                        if workspace.run().getProperty("reflectometry_processed_tof_id").value == processed_id
                    ]
                    expected_names = [f"{processed_id}_1", f"{processed_id}_2"] if grouped else [processed_id]
                    self.assertEqual(list(AnalysisDataService.retrieve(group_name).getNames()), expected_names)
                    if grouped:
                        self.assertFalse(AnalysisDataService.doesExist(processed_id))
                else:
                    self.assertFalse(AnalysisDataService.doesExist(group_name))
                    processed = AnalysisDataService.retrieve(processed_id)
                    processed_members = list(processed) if grouped else [processed]
                self.assertEqual(len(processed_members), 2 if grouped else 1)
                processed = processed_members[0]
                np.testing.assert_array_equal(original.extractY(), original_y)
                np.testing.assert_allclose(processed.extractY(), original_y * (2 if summed else 1))
                self.assertEqual(processed.getNumberHistograms(), original.getNumberHistograms())
                with patch.object(ReflectometryISISLoadAndProcess, "_loadRun", side_effect=AssertionError("Cache should be reused")):
                    self.assertTrue(alg.execute())
                output = AnalysisDataService.retrieve("testIvsQBin")
                if grouped:
                    self.assertEqual(len(output), 2)
                else:
                    self.assertFalse(output.isGroup())
                np.testing.assert_array_equal(original.extractY(), original_y)

    def test_single_input_working_copy_stays_out_of_ads(self):
        self._create_workspace(13460, "TOF_")
        original_reduce = ReflectometryISISLoadAndProcess._reduce

        def reduce(alg, cloned_workspaces, *args):
            self.assertEqual(cloned_workspaces[0][1].name(), "")
            return original_reduce(alg, cloned_workspaces, *args)

        with patch.object(ReflectometryISISLoadAndProcess, "_reduce", new=reduce):
            self._assert_run_algorithm_succeeds(dict(self._default_options, InputRunList="13460"))
        history = AnalysisDataService.retrieve("IvsQ_binned_13460").getHistory()
        self.assertEqual(self._count_clones(history.getAlgorithmHistory(history.size() - 1)), 1)

    @staticmethod
    def _count_clones(history):
        return int(history.name() == "CloneWorkspace") + sum(
            ReflectometryISISLoadAndProcessTest._count_clones(child) for child in history.getChildHistories()
        )

    def test_repeated_calibration_uses_initial_cache_without_reloading(self):
        args = dict(
            self._default_options,
            InputRunList="INTER45455",
            CalibrationFile=self._CALIBRATION_TEST_DATA,
            AnalysisMode="MultiDetectorAnalysis",
            ROIDetectorIDs=self._INTER45455_DETECTOR_ROI,
        )
        del args["ProcessingInstructions"]
        self._assert_run_algorithm_succeeds(args)
        output_history = AnalysisDataService.retrieve("IvsQ_binned_45455").getHistory()
        self.assertEqual(self._count_clones(output_history.getAlgorithmHistory(output_history.size() - 1)), 1)
        cached = AnalysisDataService.retrieve("TOF_45455")
        original_y = cached.extractY().copy()
        positions = [cached.detectorInfo().position(i) for i in range(cached.detectorInfo().size())]
        baseline = AnalysisDataService.retrieve("IvsQ_binned_45455").extractY().copy()
        with patch.object(ReflectometryISISLoadAndProcess, "_loadRun", side_effect=AssertionError("Cache should be reused")):
            self._assert_run_algorithm_succeeds(dict(args, CalibrationFile=""))
            self._assert_run_algorithm_succeeds(args)
        np.testing.assert_allclose(AnalysisDataService.retrieve("IvsQ_binned_45455").extractY(), baseline)
        np.testing.assert_array_equal(cached.extractY(), original_y)
        self.assertFalse(cached.run().hasProperty("reflectometry_calibration_file"))
        self.assertEqual([cached.detectorInfo().position(i) for i in range(cached.detectorInfo().size())], positions)

    def test_event_reduction_preserves_cached_events_and_monitors(self):
        self._create_event_workspace(38415, "TOF_", bank_pixel_width=2)
        cached = AnalysisDataService.retrieve("TOF_38415")
        monitors = AnalysisDataService.retrieve("TOF_38415_monitors")
        event_count = cached.getNumberEvents()
        monitor_y = monitors.extractY().copy()
        args = dict(
            self._default_options,
            **self._default_slice_options_dummy_run,
            InputRunList="38415",
            ProcessingInstructions="4-7",
        )
        original_reduce = ReflectometryISISLoadAndProcess._reduce

        def reduce(alg, *inputs):
            self.assertFalse(any(name.startswith("_refl_work_") for name in AnalysisDataService.getObjectNames()))
            return original_reduce(alg, *inputs)

        with patch.object(ReflectometryISISLoadAndProcess, "_reduce", new=reduce):
            self._assert_run_algorithm_succeeds(args)
        slice_members = {name for name in self._expected_dummy_time_sliced_outputs if name.startswith("TOF_38415_sliced_")}
        self.assertFalse(AnalysisDataService.doesExist("TOF_38415_sliced"))
        diagnostic_members = [
            workspace
            for workspace in AnalysisDataService.retrieve("TOF_processed")
            if workspace.run().hasProperty("reflectometry_processed_tof_id")
        ]
        self.assertEqual(len(diagnostic_members), 1)
        self.assertTrue(slice_members.issubset(set(AnalysisDataService.retrieve("TOF_processed").getNames())))
        self.assertEqual(cached.getNumberEvents(), event_count)
        self.assertFalse(cached.spectrumInfo().isMasked(1))
        np.testing.assert_array_equal(monitors.extractY(), monitor_y)
        self.assertFalse(any(name.startswith("_refl_work_") for name in AnalysisDataService.getObjectNames()))

    def test_slicing_failure_removes_temporary_inputs(self):
        self._create_event_workspace(38415, "TOF_")
        input_workspace = AnalysisDataService.retrieve("TOF_38415")
        cached_names = set(AnalysisDataService.getObjectNames())
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        cloned_workspace = alg._clone_workspace(input_workspace)

        def fail(cloned_workspace_name, cloned_monitor_workspace_name, output_workspace_name):
            self.assertTrue(AnalysisDataService.doesExist(cloned_workspace_name))
            self.assertTrue(AnalysisDataService.doesExist(cloned_monitor_workspace_name))
            raise RuntimeError("Slicing failed")

        with patch.object(alg, "_runSliceAlgorithm", side_effect=fail):
            with self.assertRaisesRegex(RuntimeError, "Slicing failed"):
                alg._sliceWorkspace(cloned_workspace, "TOF_38415")
        self.assertEqual(set(AnalysisDataService.getObjectNames()), cached_names)

    def test_grouped_reduction_failure_removes_temporary_inputs(self):
        self._create_workspace_group(38415, 2, "TOF_", self._create_2D_detector_workspace)
        self._create_workspace_group(38416, 2, "TRANS_", self._create_2D_detector_workspace)
        cached_names = set(AnalysisDataService.getObjectNames())
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        cloned_workspace = alg._clone_workspace(AnalysisDataService.retrieve("TOF_38415"))
        cloned_transmission_workspace = alg._clone_workspace(AnalysisDataService.retrieve("TRANS_38416"))
        child = MagicMock()
        child.execute.side_effect = RuntimeError("Reduction failed")
        with patch.object(alg, "createChildAlgorithm", return_value=child):
            with self.assertRaisesRegex(RuntimeError, "Reduction failed"):
                alg._reduce(
                    [
                        ("TOF_38415", cloned_workspace),
                        ("TRANS_38416", cloned_transmission_workspace),
                        ("", None),
                    ]
                )
        self.assertEqual(set(AnalysisDataService.getObjectNames()), cached_names)

    def test_hidden_group_bank_summing_does_not_leave_working_copies(self):
        self._create_workspace_group(38415, 2, "__TOF_", self._create_2D_detector_workspace)
        args = dict(self._default_options, InputRunList="38415", HideInputWorkspaces=True, ROIDetectorIDs=self._TEST_2D_WS_DETECTOR_ROI)
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        original_finalize = ReflectometryISISLoadAndProcess._finalize

        def finalize(alg, finished):
            self.assertFalse(any("_refl_work_" in name for name in AnalysisDataService.getObjectNames()))
            return original_finalize(alg, finished)

        with (
            patch.object(ReflectometryISISLoadAndProcess, "_loadRun", side_effect=AssertionError("Cache should be reused")),
            patch.object(ReflectometryISISLoadAndProcess, "_finalize", new=finalize),
        ):
            for _ in range(2):
                assertRaisesNothing(self, alg.execute)
        self.assertEqual(list(AnalysisDataService.retrieve("__TOF").getNames()), ["__TOF_38415_1", "__TOF_38415_2"])
        self.assertTrue(AnalysisDataService.doesExist("__TOF_38415_1_summed_segment"))
        self.assertFalse(any("_refl_work_" in name for name in AnalysisDataService.getObjectNames()))

    def test_same_workspace_can_be_used_for_sample_and_both_transmissions(self):
        self._create_workspace(1, "shared_")
        input_workspace = AnalysisDataService.retrieve("shared_1")
        original_y = input_workspace.extractY().copy()
        original_positions = [input_workspace.detectorInfo().position(i) for i in range(input_workspace.detectorInfo().size())]
        args = dict(
            self._default_options,
            InputRunList="shared_1",
            FirstTransmissionRunList="shared_1",
            SecondTransmissionRunList="shared_1",
        )
        self._assert_run_algorithm_succeeds(args)
        np.testing.assert_array_equal(input_workspace.extractY(), original_y)
        self.assertEqual(
            [input_workspace.detectorInfo().position(i) for i in range(input_workspace.detectorInfo().size())], original_positions
        )

    def test_uncalibrated_workspace_in_ads_is_reused_when_calibration_file_is_provided(self):
        self._create_workspace(45455, "custom_")
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("CalibrationFile", self._CALIBRATION_TEST_DATA)

        workspace_name = alg._getRunFromADSOrNone("custom_45455", False)

        self.assertEqual("custom_45455", workspace_name)

    def test_calibration_is_skipped_if_file_not_provided(self):
        args = self._default_options
        args["InputRunList"] = "INTER45455"
        del args["ProcessingInstructions"]
        args["AnalysisMode"] = "MultiDetectorAnalysis"
        args["ROIDetectorIDs"] = self._INTER45455_DETECTOR_ROI
        outputs = ["IvsQ_45455", "IvsQ_binned_45455", "TOF", "TOF_45455", "TOF_45455_summed_segment"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_calibration(AnalysisDataService.retrieve("IvsQ_binned_45455"), is_calibrated=False)

    def test_experiment_angle_uses_theta_in_when_provided(self):
        ws = CreateSampleWorkspace()
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("ThetaIn", 1.2)

        self.assertEqual(1.2, alg._experiment_angle(ws))

    def test_experiment_angle_uses_last_time_series_log_value(self):
        ws = CreateSampleWorkspace()
        AddTimeSeriesLog(Workspace=ws, Name="theta", Time="2010-01-01T00:00:00", Value=0.5)
        AddTimeSeriesLog(Workspace=ws, Name="theta", Time="2010-01-01T00:10:00", Value=0.7)
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("ThetaLogName", "theta")

        self.assertEqual(0.7, alg._experiment_angle(ws))

    def test_experiment_angle_uses_scalar_log_value(self):
        ws = CreateSampleWorkspace()
        AddSampleLog(Workspace=ws, LogName="theta", LogText="0.7", LogType="Number", NumberType="Double")
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()
        alg.setProperty("ThetaLogName", "theta")

        self.assertEqual(0.7, alg._experiment_angle(ws))

    def test_experiment_angle_throws_when_theta_is_not_available(self):
        ws = CreateSampleWorkspace()
        alg = ReflectometryISISLoadAndProcess()
        alg.initialize()

        with self.assertRaisesRegex(RuntimeError, "ThetaIn or ThetaLogName"):
            alg._experiment_angle(ws)

    def test_fractional_workspace_index_is_converted_to_fractional_spectrum_number(self):
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=2)
        ws.getSpectrum(0).setSpectrumNo(100)
        ws.getSpectrum(1).setSpectrumNo(104)

        spectrum_number = ReflectometryISISLoadAndProcess._spectrum_number_for_workspace_index(ws, 0.25)

        self.assertEqual(101.0, spectrum_number)

    def test_invalid_polarization_efficiency_file_name_raises_error(self):
        args = self._default_options
        args["InputRunList"] = "POLREF14966"
        args["ProcessingInstructions"] = "4"
        filename = "efficiencies"
        args["PolarizationEfficiencies"] = filename
        self._assert_run_algorithm_throws_with_correct_msg(
            args, f'Could not load polarization efficiency information from file "{filename}"'
        )

    def test_polarization_efficiency_workspace_from_file_is_passed_to_reduction(self):
        args = self._default_options
        args["InputRunList"] = "POLREF14966"
        args["ProcessingInstructions"] = "4"
        # The workspace we're providing for the efficiencies isn't in the correct format so the reduction will throw an
        # error, but this is sufficient to confirm that the parameter was passed through successfully
        args["PolarizationEfficiencies"] = "INTER38415"
        args["PolarizationAnalysis"] = "1"
        self._assert_run_algorithm_throws_with_correct_msg(args, "Efficiencies workspace is not in a supported format")

    def test_polarization_efficiency_workspace_from_ADS_is_passed_to_reduction(self):
        args = self._default_options
        args["InputRunList"] = "POLREF14966"
        args["ProcessingInstructions"] = "4"
        # The workspace we're providing for the efficiencies isn't in the correct format so the reduction will throw an
        # error, but this is sufficient to confirm that the parameter was passed through successfully
        self._create_workspace("test_corrections")
        args["PolarizationEfficiencies"] = "test_corrections"
        args["PolarizationAnalysis"] = "1"
        self._assert_run_algorithm_throws_with_correct_msg(args, "Efficiencies workspace is not in a supported format")

    def test_group_outputs_record_history_with_polarization_analysis(self):
        self._create_polarization_efficiencies_workspace("efficiencies")
        args = self._default_options
        args["InputRunList"] = "POLREF14966"
        args["ProcessingInstructions"] = "4"
        args["PolarizationAnalysis"] = "1"
        args["PolarizationEfficiencies"] = "efficiencies"
        outputs = [
            "Alpha",
            "Ap",
            "efficiencies",
            "IvsLam_14966",
            "IvsLam_14966_1",
            "IvsLam_14966_2",
            "IvsQ_14966",
            "IvsQ_14966_1",
            "IvsQ_14966_2",
            "IvsQ_binned_14966",
            "IvsQ_binned_14966_1",
            "IvsQ_binned_14966_2",
            "Pp",
            "Rho",
            "TOF_14966_1",
            "TOF_14966_2",
            "TOF",
        ]

        self._assert_run_algorithm_succeeds(args, outputs)

        self._check_polarized_group_history(AnalysisDataService.retrieve("IvsQ_binned_14966_1"))
        self._check_polarized_group_history(AnalysisDataService.retrieve("IvsQ_14966_1"))
        self._check_polarized_group_history(AnalysisDataService.retrieve("IvsLam_14966_1"))

    # TODO test if no runNumber is on the WS

    def _create_workspace(self, run_number, prefix="", suffix=""):
        name = prefix + str(run_number) + suffix
        ws = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, NumMonitors=2, BankPixelWidth=1, XMin=200, OutputWorkspace=name)
        AddSampleLog(Workspace=ws, LogName="run_number", LogText=str(run_number))

    def _create_polarization_efficiencies_workspace(self, output_workspace):
        wavelength = [0, 3, 6, 10, 15, 20]
        efficiencies = {
            "Pp": [0.1] * 6,
            "Ap": [0.8] * 6,
            "Rho": [0.778] * 6,
            "Alpha": [0.75] * 6,
        }
        for name, values in efficiencies.items():
            CreateWorkspace(DataX=wavelength, DataY=values, DataE=[0] * len(values), NSpec=1, OutputWorkspace=name)
        JoinISISPolarizationEfficiencies(Pp="Pp", Ap="Ap", Rho="Rho", Alpha="Alpha", OutputWorkspace=output_workspace)

    def _create_2D_detector_workspace(self, run_number, prefix="", suffix=""):
        name = prefix + str(run_number) + suffix
        ws = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, NumMonitors=2, BankPixelWidth=2, XMin=200, OutputWorkspace=name)
        AddSampleLog(Workspace=ws, LogName="run_number", LogText=str(run_number))

    def _create_workspace_wavelength(self, run_number, prefix="", suffix=""):
        name = prefix + str(run_number) + suffix
        ws = CreateSampleWorkspace(
            WorkspaceType="Histogram", NumBanks=1, NumMonitors=2, BankPixelWidth=1, XMin=200, XUnit="Wavelength", OutputWorkspace=name
        )
        AddSampleLog(Workspace=ws, LogName="run_number", LogText=str(run_number))

    def _create_event_workspace(self, run_number, prefix="", suffix="", includeMonitors=True, bank_pixel_width=1):
        name = prefix + str(run_number) + suffix
        CreateSampleWorkspace(
            WorkspaceType="Event", NumBanks=1, NumMonitors=3, BankPixelWidth=bank_pixel_width, XMin=200, OutputWorkspace=name
        )
        if includeMonitors:
            CropWorkspace(InputWorkspace=name, StartWorkspaceIndex=0, EndWorkspaceIndex=2, OutputWorkspace=name + "_monitors")
            Rebin(InputWorkspace=name + "_monitors", Params="0,200,20000", OutputWorkspace=name + "_monitors", PreserveEvents=False)
        CropWorkspace(InputWorkspace=name, StartWorkspaceIndex=3, EndWorkspaceIndex=2 + bank_pixel_width**2, OutputWorkspace=name)
        AddSampleLog(Workspace=name, LogName="run_number", LogText=str(run_number))
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)

    def _create_workspace_group(self, run_number, number_of_items, prefix="", workspace_creator=None, has_monitors=False):
        group_name = prefix + str(run_number)
        child_names = list()
        workspace_creator = workspace_creator or self._create_workspace
        for index in range(0, number_of_items):
            child_suffix = "_" + str(index + 1)
            child_name = group_name + child_suffix
            workspace_creator(run_number, prefix, child_suffix)
            child_names.append(child_name)
        GroupWorkspaces(InputWorkspaces=",".join(child_names), OutputWorkspace=group_name)
        if has_monitors:
            GroupWorkspaces(
                InputWorkspaces=",".join([child_name + "_monitors" for child_name in child_names]), OutputWorkspace=group_name + "_monitors"
            )

    def _check_history(self, ws, expected, unroll=True):
        """Return true if algorithm names listed in algorithmNames are found in the
        workspace's history. If unroll is true, checks the child histories, otherwise
        checks the top level history (the latter is required for sliced workspaces where
        the child workspaces have lost their parent's history)
        """
        history = ws.getHistory()
        if unroll:
            reductionHistory = history.getAlgorithmHistory(history.size() - 1)
            algHistories = reductionHistory.getChildHistories()
            algNames = [alg.name() for alg in algHistories]
        else:
            algNames = [alg.name() for alg in history.getAlgorithmHistories()]

        self.assertEqual(algNames.sort(), expected.sort())

    def _check_polarized_group_history(self, ws):
        history = ws.getHistory()
        self.assertGreater(history.size(), 0)
        parent_history = history.getAlgorithmHistory(history.size() - 1)
        self.assertEqual(parent_history.name(), "ReflectometryISISLoadAndProcess")
        reduction_histories = [alg for alg in parent_history.getChildHistories() if alg.name() == "ReflectometryReductionOneAuto"]
        self.assertGreater(len(reduction_histories), 0)
        core_reduction_histories = [alg for alg in reduction_histories[-1].getChildHistories() if alg.name() == "ReflectometryReductionOne"]
        self.assertGreater(len(core_reduction_histories), 0)

    def _check_calibration(self, ws, is_calibrated):
        """Check whether the calibration algorithm has run by checking the workspace history"""
        history = ws.getHistory()
        reduction_history = history.getAlgorithmHistory(history.size() - 1)
        alg_names = [alg.name() for alg in reduction_history.getChildHistories()]
        self.assertEqual("ReflectometryISISCalibration" in alg_names, is_calibrated)

    def _check_sum_banks(self, ws, is_summed):
        """Check whether the sum banks algorithm has run by checking the workspace history"""
        self._check_child_history(ws, 0, "ReflectometryISISSumBanks", is_summed)

    def _check_child_history(self, ws, child_idx, alg_name, is_present):
        """Check whether the given algorithm has run by checking the workspace child algorithm history"""
        history = ws.getHistory()
        reductionHistory = history.getAlgorithmHistory(history.size() - 1)
        childHistory = next(child for child in reductionHistory.getChildHistories() if child.name() == "ReflectometryReductionOneAuto")
        childAlgs = childHistory.getChildHistories()
        algNames = [alg.name() for alg in childAlgs]
        self.assertEqual(alg_name in algNames, is_present)

    def _check_history_algorithm_properties(self, ws, toplevel_idx, child_idx, property_values):
        parent_hist = ws.getHistory().getAlgorithmHistory(toplevel_idx)
        child_hist = parent_hist.getChildHistories()[child_idx]
        for prop, val in property_values.items():
            self.assertEqual(child_hist.getPropertyValue(prop), val)

    def _assert_run_algorithm_succeeds(self, args, expected=None):
        """Run the algorithm with the given args and check it succeeds,
        and that the additional workspaces produced match the expected list.
        Clear these additional workspaces from the ADS"""
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        assertRaisesNothing(self, alg.execute)
        actual = AnalysisDataService.getObjectNames()
        if expected is not None:
            # Processed diagnostic inputs are checked separately from the established reduction outputs.
            self.assertEqual(
                {name for name in actual if not name.startswith("__") and "_processed_tof" not in name and name != "TOF_processed"},
                set(expected),
            )

    def _assert_run_algorithm_fails(self, args, namesNotExpected=None):
        """Run the algorithm with the given args and check it fails to produce output"""
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        assertRaisesNothing(self, alg.execute)
        actual = AnalysisDataService.getObjectNames()
        if namesNotExpected is not None:
            self.assertNotEqual(set(actual), set(namesNotExpected))

    def _assert_run_algorithm_throws(self, args={}):
        """Run the algorithm with the given args and check it throws"""
        throws = False
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        alg.setRethrows(True)
        try:
            alg.execute()
        except:
            throws = True
        self.assertEqual(throws, True)

    def _assert_run_algorithm_throws_with_correct_msg(self, args, error_msg_regex):
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        alg.setRethrows(True)
        self.assertRaisesRegex(RuntimeError, error_msg_regex, alg.execute)

    def _assert_delta(self, value1, value2):
        self.assertEqual(round(value1, 6), round(value2, 6))

    def _clear(self, expected):
        for workspace in expected:
            AnalysisDataService.remove(workspace)


if __name__ == "__main__":
    unittest.main()
