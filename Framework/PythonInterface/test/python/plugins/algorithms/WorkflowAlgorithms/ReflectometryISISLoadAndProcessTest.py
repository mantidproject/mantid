# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid import config
from mantid.api import mtd
from mantid.simpleapi import (AddSampleLog, AddTimeSeriesLog, CreateSampleWorkspace,
                              CropWorkspace, GroupWorkspaces, Rebin)
from testhelpers import (assertRaisesNothing, create_algorithm)


class ReflectometryISISLoadAndProcessTest(unittest.TestCase):
    def setUp(self):
        self._oldFacility = config['default.facility']
        if self._oldFacility.strip() == '':
            self._oldFacility = 'TEST_LIVE'
        self._oldInstrument = config['default.instrument']
        config['default.facility'] = 'ISIS'
        config['default.instrument'] = 'INTER'
        # Set some commonly used properties
        self._default_options = {
            'ProcessingInstructions' : '4',
            'ThetaIn' : 0.5,
            'WavelengthMin' : 2,
            'WavelengthMax' : 5,
            'I0MonitorIndex' : 1,
            'MomentumTransferStep' : 0.02
            }
        self._mandatory_output_names = {
            'OutputWorkspace' : 'testIvsQ',
            'OutputWorkspaceBinned' : 'testIvsQBin'
            }
        self._all_output_names = {
            'OutputWorkspace' : 'testIvsQ',
            'OutputWorkspaceBinned' : 'testIvsQBin',
            'OutputWorkspaceWavelength' : 'testIvsLam'
            }
        self._default_slice_options_dummy_run = {
            'SliceWorkspace' : True,
            'TimeInterval' : 1200
        }
        self._default_slice_options_real_run = {
            'SliceWorkspace' : True,
            'TimeInterval' : 210
        }
        self._expected_dummy_time_sliced_outputs = [
            'IvsQ_38415', 'IvsQ_38415_sliced_0_1200', 'IvsQ_38415_sliced_1200_2400',
            'IvsQ_38415_sliced_2400_3600', 'IvsQ_binned_38415', 'IvsQ_binned_38415_sliced_0_1200',
            'IvsQ_binned_38415_sliced_1200_2400', 'IvsLam_38415',
            'IvsLam_38415_sliced_0_1200', 'IvsLam_38415_sliced_1200_2400',
            'IvsLam_38415_sliced_2400_3600', 'IvsQ_binned_38415_sliced_2400_3600', 'TOF_38415',
            'TOF_38415_monitors', 'TOF_38415_sliced', 'TOF_38415_sliced_0_1200',
            'TOF_38415_sliced_1200_2400', 'TOF_38415_sliced_2400_3600', 'TOF']
        
        self._expected_real_time_sliced_outputs = [
            'IvsQ_38415', 'IvsQ_38415_sliced_0_210', 'IvsQ_38415_sliced_210_420',
            'IvsQ_38415_sliced_420_610', 'IvsQ_binned_38415', 'IvsQ_binned_38415_sliced_0_210',
            'IvsQ_binned_38415_sliced_210_420',  'IvsLam_38415', 'IvsLam_38415_sliced_0_210',
            'IvsLam_38415_sliced_210_420', 'IvsLam_38415_sliced_420_610',
            'IvsQ_binned_38415_sliced_420_610', 'TOF_38415', 'TOF_38415_monitors',
            'TOF_38415_sliced', 'TOF_38415_sliced_0_210', 'TOF_38415_sliced_210_420',
            'TOF_38415_sliced_420_610', 'TOF']

    def tearDown(self):
        mtd.clear()
        config['default.facility'] = self._oldFacility
        config['default.instrument'] = self._oldInstrument

    def test_missing_input_runs(self):
        self._assert_run_algorithm_throws()

    def test_input_run_is_loaded_if_not_in_ADS(self):
        args = self._default_options
        args['InputRunList'] = '13460'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['LoadNexus', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_input_run_that_is_in_ADS_with_prefixed_name_is_not_reloaded(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_input_run_that_is_in_ADS_without_prefix_is_not_reloaded(self):
        self._create_workspace(13460)
        args = self._default_options
        args['InputRunList'] = '13460'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', '13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_input_run_is_reloaded_if_in_ADS_with_unknown_prefix(self):
        self._create_workspace(13460, 'TEST_')
        args = self._default_options
        args['InputRunList'] = '13460'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TEST_13460', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['LoadNexus', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_existing_workspace_is_used_when_name_passed_in_input_list(self):
        self._create_workspace(13460, 'TEST_')
        args = self._default_options
        args['InputRunList'] = 'TEST_13460'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TEST_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_loading_run_with_instrument_prefix_in_name(self):
        args = self._default_options
        args['InputRunList'] = 'INTER13460'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['LoadNexus', 'RenameWorkspace', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_overriding_output_names(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args.update(self._mandatory_output_names)
        outputs = ['testIvsQ', 'testIvsQBin', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['testIvsQBin'], history)

    def test_overriding_output_names_includes_all_specified_outputs(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args.update(self._all_output_names)
        # Current behaviour is that the optional workspaces are output even if
        # Debug is not set
        outputs = ['testIvsQ', 'testIvsQBin', 'testIvsLam', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['testIvsQBin'], history)

    def test_debug_option_outputs_extra_workspaces(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['Debug'] = True
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'IvsLam_13460', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_debug_option_outputs_extra_workspaces_with_overridden_names(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['Debug'] = True
        args.update(self._all_output_names)
        outputs = ['testIvsQ', 'testIvsQBin', 'testIvsLam', 'TOF_13460', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['testIvsQBin'], history)

    def test_multiple_input_runs_are_summed(self):
        self._create_workspace(13461, 'TOF_')
        self._create_workspace(13462, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13461, 13462'
        outputs = ['IvsQ_13461+13462', 'IvsQ_binned_13461+13462', 'TOF_13461', 'TOF_13462',
                   'TOF_13461+13462', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['MergeRuns', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13461+13462'], history)

    def test_trans_run_is_not_reloaded_if_in_ADS(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463, 'TRANS_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = '13463'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_trans_runs_are_loaded_if_not_in_ADS(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = '13463'
        args['SecondTransmissionRunList'] = '13464'
        # Expect IvsQ outputs from the reduction, and intermediate LAM outputs from
        # creating the stitched transmission run
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TRANS_13464',
                   'TRANS_LAM_13463', 'TRANS_LAM_13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['LoadNexus', 'LoadNexus', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_trans_runs_are_not_loaded_if_in_ADS_without_prefix(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463)
        self._create_workspace(13464)
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = '13463'
        args['SecondTransmissionRunList'] = '13464'
        # Expect IvsQ outputs from the reduction, and initermediate LAM outputs from
        # creating the stitched transmission run
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', '13463', '13464',
                   'TRANS_LAM_13463', 'TRANS_LAM_13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_trans_runs_are_reloaded_if_in_ADS_with_unknown_prefix(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463, 'TEST_')
        self._create_workspace(13464, 'TEST_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = '13463'
        args['SecondTransmissionRunList'] = '13464'
        # Expect IvsQ outputs from the reduction, and initermediate LAM outputs from
        # creating the stitched transmission run
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TRANS_13464',
                   'TEST_13463', 'TEST_13464', 'TRANS_LAM_13463', 'TRANS_LAM_13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['LoadNexus', 'LoadNexus', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_existing_workspace_is_used_for_trans_runs_when_name_passed_in_input_list(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463, 'TEST_')
        self._create_workspace(13464, 'TEST_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = 'TEST_13463'
        args['SecondTransmissionRunList'] = 'TEST_13464'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TEST_13463', 'TEST_13464',
                   'TRANS_LAM_13463', 'TRANS_LAM_13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_first_and_second_trans_runs_are_combined(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463, 'TRANS_')
        self._create_workspace(13464, 'TRANS_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = '13463'
        args['SecondTransmissionRunList'] = '13464'
        # The intermediate LAM workspaces are output by the algorithm that combines the runs
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TRANS_13464',
                   'TRANS_LAM_13463', 'TRANS_LAM_13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_first_and_second_trans_runs_are_combined_with_debug_output(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463, 'TRANS_')
        self._create_workspace(13464, 'TRANS_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['Debug'] = True
        args['FirstTransmissionRunList'] = '13463'
        args['SecondTransmissionRunList'] = '13464'
        # The intermediate LAM workspaces are output by the algorithm that combines the runs.
        # The stitched TRANS_LAM_13463_13464 is only output with Debug on.
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'IvsLam_13460', 'TOF_13460', 'TRANS_13463',
                   'TRANS_13464', 'TRANS_LAM_13463', 'TRANS_LAM_13464', 'TRANS_LAM_13463_13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_multiple_trans_runs_are_summed(self):
        self._create_workspace(13460, 'TOF_')
        self._create_workspace(13463, 'TRANS_')
        self._create_workspace(13464, 'TRANS_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['FirstTransmissionRunList'] = '13463, 13464'
        outputs = ['IvsQ_13460', 'IvsQ_binned_13460', 'TOF_13460', 'TRANS_13463', 'TRANS_13464',
                   'TRANS_13463+13464', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['MergeRuns', 'ReflectometryReductionOneAuto', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_13460'], history)

    def test_slicing_is_disallowed_if_summing_input_runs(self):
        self._create_workspace(13460, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '13460'
        args['SliceWorkspace'] = True
        self._assert_run_algorithm_fails(args)

    def test_slicing_uses_run_in_ADS_with_correct_prefix(self):
        self._create_event_workspace(38415, 'TOF_')
        args = self._default_options.copy()
        args.update(self._default_slice_options_dummy_run)
        args['InputRunList'] = '38415'
        outputs = self._expected_dummy_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        # Note that the child sliced workspaces don't include the full history - this
        # might be something we want to change in the underlying algorithms at some point
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_sliced_0_1200'], history, False)

    def test_slicing_uses_run_in_ADS_with_no_prefix(self):
        self._create_event_workspace(38415)
        args = self._default_options
        args.update(self._default_slice_options_dummy_run)
        args['InputRunList'] = '38415'
        outputs = [
             'IvsQ_38415', 'IvsQ_38415_1', 'IvsQ_38415_2', 'IvsQ_38415_3',
             'IvsQ_binned_38415', 'IvsQ_binned_38415_1', 'IvsQ_binned_38415_2',
             'IvsLam_38415', 'IvsLam_38415_1', 'IvsLam_38415_2', 'IvsLam_38415_3',
             'IvsQ_binned_38415_3', '38415', '38415_monitors', '38415_sliced',
             '38415_sliced_0_1200', '38415_sliced_1200_2400',
             '38415_sliced_2400_3600', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_1'], history, False)

    def test_slicing_loads_input_run_if_not_in_ADS(self):
        args = self._default_options
        args.update(self._default_slice_options_real_run)
        args['InputRunList'] = '38415'
        outputs = self._expected_real_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_sliced_0_210'], history, False)

    def test_slicing_reloads_input_run_if_workspace_is_incorrect_type(self):
        self._create_workspace(38415, 'TOF_')
        args = self._default_options
        args.update(self._default_slice_options_real_run)
        args['InputRunList'] = '38415'
        outputs = self._expected_real_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_sliced_0_210'], history, False)

    def test_slicing_loads_input_run_if_monitor_ws_not_in_ADS(self):
        self._create_event_workspace(38415, 'TOF_', False)
        args = self._default_options
        args.update(self._default_slice_options_real_run)
        args['InputRunList'] = '38415'
        outputs = self._expected_real_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_sliced_0_210'], history, False)

    def test_slicing_with_no_interval_returns_single_slice(self):
        self._create_event_workspace(38415, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '38415'
        args['SliceWorkspace'] = True
        outputs = ['IvsQ_38415', 'IvsQ_38415_sliced_0_4200', 'IvsQ_binned_38415',
                   'IvsQ_binned_38415_sliced_0_4200', 'IvsLam_38415',
                   'IvsLam_38415_sliced_0_4200', 'TOF_38415', 'TOF_38415_monitors',
                   'TOF_38415_sliced', 'TOF_38415_sliced_0_4200', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_sliced_0_4200'], history, False)

    def test_slicing_by_number_of_slices(self):
        self._create_event_workspace(38415, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '38415'
        args['SliceWorkspace'] = True
        args['NumberOfSlices'] = 3
        outputs = self._expected_dummy_time_sliced_outputs
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415_sliced_0_1200'], history, False)

    def test_slicing_by_log_value(self):
        self._create_event_workspace(38415, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '38415'
        args['SliceWorkspace'] = True
        args['LogName'] = 'proton_charge'
        args['LogValueInterval'] = 60
        slice1 = '_sliced_Log.proton_charge.From.-30.To.30.Value-change-direction:both'
        slice2 = '_sliced_Log.proton_charge.From.30.To.90.Value-change-direction:both'
        slice3 = '_sliced_Log.proton_charge.From.90.To.150.Value-change-direction:both'
        outputs = ['IvsQ_38415', 'IvsQ_38415'+slice1, 'IvsQ_38415'+slice2, 'IvsQ_38415'+slice3,
                   'IvsQ_binned_38415', 'IvsQ_binned_38415'+slice1,'IvsQ_binned_38415'+slice2,
                   'IvsQ_binned_38415'+slice3, 'IvsLam_38415', 'IvsLam_38415'+slice1,
                   'IvsLam_38415'+slice2, 'IvsLam_38415'+slice3, 'TOF', 'TOF_38415',
                   'TOF_38415_monitors', 'TOF_38415_sliced', 'TOF_38415'+slice1,
                   'TOF_38415'+slice2, 'TOF_38415'+slice3]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415'+slice1], history, False)

    def test_slicing_by_log_value_with_no_interval_returns_single_slice(self):
        self._create_event_workspace(38415, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '38415'
        args['SliceWorkspace'] = True
        args['LogName'] = 'proton_charge'
        sliceName = '_sliced_Log.proton_charge.From.0.To.100.Value-change-direction:both'
        outputs = ['IvsQ_38415', 'IvsQ_38415'+sliceName, 'IvsQ_binned_38415',
                   'IvsQ_binned_38415'+sliceName, 'IvsLam_38415', 'IvsLam_38415'+sliceName,
                   'TOF', 'TOF_38415', 'TOF_38415_monitors', 'TOF_38415_sliced',
                   'TOF_38415'+sliceName]
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['ReflectometryReductionOneAuto', 'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_38415'+sliceName], history, False)

    def test_with_input_workspace_group(self):
        self._create_workspace_group(12345, 2, 'TOF_')
        args = self._default_options
        args['InputRunList'] = '12345'
        outputs = ['IvsQ_12345', 'IvsQ_12345_1', 'IvsQ_12345_2', 'IvsQ_binned_12345',
                   'IvsQ_binned_12345_2', 'IvsQ_binned_12345_1', 'IvsLam_12345',
                   'IvsLam_12345_1', 'IvsLam_12345_2', 'TOF_12345_1', 'TOF_12345_2', 'TOF']
        self._assert_run_algorithm_succeeds(args, outputs)
        history = ['CreateSampleWorkspace', 'AddSampleLog',  'CreateSampleWorkspace', 'AddSampleLog',
                   'GroupWorkspaces', 'ReflectometryReductionOneAuto', 'ReflectometryReductionOneAuto',
                   'GroupWorkspaces', 'GroupWorkspaces']
        self._check_history(mtd['IvsQ_binned_12345_1'], history, False)

    def _create_workspace(self, run_number, prefix='', suffix=''):
        name = prefix + str(run_number) + suffix
        ws = CreateSampleWorkspace(WorkspaceType='Histogram',NumBanks=1, NumMonitors=2,
                                   BankPixelWidth=2, XMin=200, OutputWorkspace=name)
        AddSampleLog(Workspace=ws, LogName='run_number', LogText=str(run_number))

    def _create_event_workspace(self, run_number, prefix='', includeMonitors=True):
        name = prefix + str(run_number)
        CreateSampleWorkspace(WorkspaceType='Event',NumBanks=1, NumMonitors=3,
                              BankPixelWidth=2, XMin=200, OutputWorkspace=name)
        if includeMonitors:
            CropWorkspace(InputWorkspace=name, StartWorkspaceIndex=0, EndWorkspaceIndex=2,
                          OutputWorkspace=name + '_monitors')
            Rebin(InputWorkspace=name + '_monitors', Params='0,200,20000',
                  OutputWorkspace=name + '_monitors', PreserveEvents=False)
        CropWorkspace(InputWorkspace=name, StartWorkspaceIndex=3, EndWorkspaceIndex=4,
                      OutputWorkspace=name)
        AddSampleLog(Workspace=name, LogName='run_number', LogText=str(run_number))
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:00:00", Value=100)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:10:00", Value=100)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:20:00", Value=80)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:30:00", Value=80)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:40:00", Value=15)
        AddTimeSeriesLog(Workspace=name, Name="proton_charge", Time="2010-01-01T00:50:00", Value=100)

    def _create_workspace_group(self, run_number, number_of_items, prefix=''):
        group_name = prefix + str(run_number)
        child_names = list()
        for index in range(0, number_of_items):
            child_suffix = '_' + str(index+1)
            child_name = group_name + child_suffix
            self._create_workspace(run_number, prefix, child_suffix)
            child_names.append(child_name)
        GroupWorkspaces(InputWorkspaces=",".join(child_names), OutputWorkspace=group_name)

    def _check_history(self, ws, expected, unroll = True):
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

    def _assert_run_algorithm_succeeds(self, args, expected):
        """Run the algorithm with the given args and check it succeeds,
        and that the additional workspaces produced match the expected list.
        Clear these additional workspaces from the ADS"""
        alg = create_algorithm('ReflectometryISISLoadAndProcess', **args)
        assertRaisesNothing(self, alg.execute)
        actual = mtd.getObjectNames()
        self.assertEqual(set(actual), set(expected))

    def _assert_run_algorithm_fails(self, args):
        """Run the algorithm with the given args and check it fails to produce output"""
        alg = create_algorithm('ReflectometryISISLoadAndProcess', **args)
        assertRaisesNothing(self, alg.execute)
        self.assertEqual(mtd.doesExist('output'), False)

    def _assert_run_algorithm_throws(self, args = {}):
        """Run the algorithm with the given args and check it throws"""
        throws = False
        alg = create_algorithm('ReflectometryISISLoadAndProcess', **args)
        try:
            alg.execute()
        except:
            throws = True
        self.assertEqual(throws, True)

    def _assert_delta(self, value1, value2):
        self.assertEqual(round(value1, 6), round(value2, 6))

    def _clear(self, expected):
        for workspace in expected:
            mtd.remove(workspace)


if __name__ == '__main__':
    unittest.main()
