# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid import config, FileFinder
from mantid.api import AnalysisDataService
from mantid.simpleapi import AddSampleLog, AddTimeSeriesLog, CreateSampleWorkspace, CropWorkspace, GroupWorkspaces, Rebin
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
            "TOF_38415_sliced",
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
            "TOF_38415_sliced",
            "TOF_38415_sliced_0_210",
            "TOF_38415_sliced_210_420",
            "TOF_38415_sliced_420_610",
            "TOF",
        ]

    def tearDown(self):
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument

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
        args["FirstTransmissionRunList"] = "13460"
        args["ProcessingInstructions"] = "4"
        args["HideInputWorkspaces"] = True
        outputs = ["IvsQ_13460", "IvsQ_binned_13460", "TRANS_LAM_13460"]
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
        outputs = ["IvsQ_13461+13462", "IvsQ_binned_13461+13462", "TOF_13461", "TOF_13462", "TOF_13461+13462", "TOF"]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["MergeRuns", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_13461+13462"), history)

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
            "TRANS_13463+13464",
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
            "38415_sliced",
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
        self._create_workspace(38415, "TOF_")
        args = self._default_options
        args.update(self._default_slice_options_real_run)
        args["InputRunList"] = "38415"
        args["ProcessingInstructions"] = "4"
        outputs = self._expected_real_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415_sliced_0_210"), history, False)

    def test_slicing_loads_input_run_if_monitor_ws_not_in_ADS(self):
        self._create_event_workspace(38415, "TOF_", False)
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
            "TOF_38415_sliced",
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
            "TOF_38415_sliced",
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
            "TOF_38415_sliced",
            "TOF_38415" + sliceName,
        ]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ["ReflectometryReductionOneAuto", "GroupWorkspaces"]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_38415" + sliceName), history, False)

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
            "ReflectometryReductionOneAuto",
            "GroupWorkspaces",
        ]
        self._check_history(AnalysisDataService.retrieve("IvsQ_binned_12345_1"), history, False)

    def test_TOF_input_workspace_groups_collapsed(self):
        self._create_workspace_group(13460, 2, "TOF_")
        self._create_workspace(13463, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460, 13463"
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460+13463", "TOF_13460_1", "TOF_13460_2", "TOF_13463"]

        self._assert_run_algorithm_succeeds(args, outputs)

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
        outputs = ["no_TOF_group", "TOF_12345+67890", "12345", "67890"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_group_TOF_workspaces_succeeds_with_only_TOF_workspaces(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460, 13463"
        args["GroupTOFWorkspaces"] = True
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460+13463", "TOF_13460", "TOF_13463"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_group_TOF_workspaces_succeeds_with_TOF_group(self):
        self._create_workspace_group(13460, 2, "TOF_")
        self._create_workspace(13463, "TOF_")
        args = self._default_options
        args["InputRunList"] = "13460, 13463"
        args["GroupTOFWorkspaces"] = True
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460+13463", "TOF_13460_1", "TOF_13460_2", "TOF_13463"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_group_TOF_workspaces_fails_with_mixed_unit_workspace_group(self):
        self._create_workspace(13460, "TOF_")
        self._create_workspace(13463, "TOF_")
        self._create_workspace_wavelength(12345)
        GroupWorkspaces("TOF_13463, 12345", OutputWorkspace="mixed_unit_group")
        args = self._default_options
        args["InputRunList"] = "13460, mixed_unit_group"
        args["GroupTOFWorkspaces"] = True
        outputs = ["IvsQ_13460+13463", "IvsQ_binned_13460+13463", "TOF", "TOF_13460+13463", "TOF_13460", "TOF_13463", "12345"]
        self._assert_run_algorithm_fails(args, outputs)

    def test_group_TOF_workspaces_succeeds_with_no_TOF_input_workspace_groups_which_remain_unchanged(self):
        self._create_workspace_wavelength(12345)
        self._create_workspace_wavelength(67890)
        GroupWorkspaces("12345, 67890", OutputWorkspace="no_TOF_group")
        args = self._default_options
        args["InputRunList"] = "12345, 67890"
        args["GroupTOFWorkspaces"] = True
        outputs = ["no_TOF_group", "TOF_12345+67890", "12345", "67890"]
        self._assert_run_algorithm_succeeds(args, outputs)

    def test_sum_banks_runs_for_2D_detector_workspace_when_detector_ROI_specified(self):
        self._create_2D_detector_workspace(38415, "TOF_")

        def _check_num_histograms_and_first_y_value(ws_name, expected_num_histograms, expected_y_value):
            ws = AnalysisDataService.retrieve(ws_name)
            self.assertTrue(ws.getNumberHistograms(), expected_num_histograms)
            self._assert_delta(ws.readY(2)[0], expected_y_value)

        _check_num_histograms_and_first_y_value("TOF_38415", 6, 0.3)
        args = self._default_options
        args["InputRunList"] = "38415"
        args["ROIDetectorIDs"] = self._TEST_2D_WS_DETECTOR_ROI
        outputs = ["IvsQ_38415", "IvsQ_binned_38415", "TOF", "TOF_38415", "TOF_38415_summed_segment"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_sum_banks(AnalysisDataService.retrieve("IvsQ_binned_38415"), is_summed=True)
        _check_num_histograms_and_first_y_value("TOF_38415_summed_segment", 4, 0.6)

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

    def test_calibration_is_skipped_if_file_not_provided(self):
        args = self._default_options
        args["InputRunList"] = "INTER45455"
        del args["ProcessingInstructions"]
        args["AnalysisMode"] = "MultiDetectorAnalysis"
        args["ROIDetectorIDs"] = self._INTER45455_DETECTOR_ROI
        outputs = ["IvsQ_45455", "IvsQ_binned_45455", "TOF", "TOF_45455", "TOF_45455_summed_segment"]
        self._assert_run_algorithm_succeeds(args, outputs)
        self._check_calibration(AnalysisDataService.retrieve("IvsQ_binned_45455"), is_calibrated=False)

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

    # TODO test if no runNumber is on the WS

    def _create_workspace(self, run_number, prefix="", suffix=""):
        name = prefix + str(run_number) + suffix
        ws = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=1, NumMonitors=2, BankPixelWidth=1, XMin=200, OutputWorkspace=name)
        AddSampleLog(Workspace=ws, LogName="run_number", LogText=str(run_number))

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

    def _create_event_workspace(self, run_number, prefix="", includeMonitors=True):
        name = prefix + str(run_number)
        CreateSampleWorkspace(WorkspaceType="Event", NumBanks=1, NumMonitors=3, BankPixelWidth=1, XMin=200, OutputWorkspace=name)
        if includeMonitors:
            CropWorkspace(InputWorkspace=name, StartWorkspaceIndex=0, EndWorkspaceIndex=2, OutputWorkspace=name + "_monitors")
            Rebin(InputWorkspace=name + "_monitors", Params="0,200,20000", OutputWorkspace=name + "_monitors", PreserveEvents=False)
        CropWorkspace(InputWorkspace=name, StartWorkspaceIndex=3, EndWorkspaceIndex=3, OutputWorkspace=name)
        AddSampleLog(Workspace=name, LogName="run_number", LogText=str(run_number))
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)

    def _create_workspace_group(self, run_number, number_of_items, prefix=""):
        group_name = prefix + str(run_number)
        child_names = list()
        for index in range(0, number_of_items):
            child_suffix = "_" + str(index + 1)
            child_name = group_name + child_suffix
            self._create_workspace(run_number, prefix, child_suffix)
            child_names.append(child_name)
        GroupWorkspaces(InputWorkspaces=",".join(child_names), OutputWorkspace=group_name)

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
            algNames = [alg.name() for alg in history]
        self.assertEqual(algNames, expected)

    def _check_calibration(self, ws, is_calibrated):
        """Check whether the calibration algorithm has run by checking the workspace history"""
        self._check_child_history(ws, 0, "ReflectometryISISCalibration", is_calibrated)

    def _check_sum_banks(self, ws, is_summed):
        """Check whether the sum banks algorithm has run by checking the workspace history"""
        self._check_child_history(ws, 0, "ReflectometryISISSumBanks", is_summed)

    def _check_child_history(self, ws, child_idx, alg_name, is_present):
        """Check whether the given algorithm has run by checking the workspace child algorithm history"""
        history = ws.getHistory()
        reductionHistory = history.getAlgorithmHistory(history.size() - 1)
        childHistory = reductionHistory.getChildHistories()[child_idx]
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
            self.assertEqual(set(actual), set(expected))

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
