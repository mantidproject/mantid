# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import os
import shutil
import numpy as np

# from unittest import mock
from unittest.mock import patch, MagicMock
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.model import \
    GSAS2Model
from mantid.api import FileFinder
from mantid.simpleapi import LoadNexus, CompareWorkspaces, ReplaceSpecialValues, mtd

model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.model"
output_sample_log_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.output_sample_logs"


def _try_delete(path):
    try:
        # Use this instead of os.remove as we could be passed a non-empty dir
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.remove(path)
    except OSError:
        print("Could not delete output file at: ", path)


class TestGSAS2Model(unittest.TestCase):
    def setUp(self):
        self.model = GSAS2Model()
        self.phase_file_path = FileFinder.getFullPath(os.path.join('EngDiff_gsas2_tab', 'gsas2_FE_GAMMA.cif'))
        self.lst_result_filepath = FileFinder.getFullPath(os.path.join('EngDiff_gsas2_tab', "gsas2_output.lst"))
        self.maxDiff = None
        self.model.user_save_directory = os.path.dirname(self.lst_result_filepath)
        self.model.project_name = "gsas2_output"
        # setup a mock workspace
        self.mock_inst = MagicMock()
        self.mock_inst.getFullName.return_value = 'instrument'
        mock_prop = MagicMock()
        mock_prop.value = 'bank 1'  # bank-id
        mock_log_data = [MagicMock(), MagicMock()]
        mock_log_data[0].name = "LogName"
        mock_log_data[1].name = "proton_charge"
        self.mock_run = MagicMock()
        self.mock_run.getProtonCharge.return_value = 1.0
        self.mock_run.getProperty.return_value = mock_prop
        self.mock_run.getLogData.return_value = mock_log_data
        self.mock_ws = MagicMock()
        self.mock_ws.getNumberHistograms.return_value = 1
        self.mock_ws.getRun.return_value = self.mock_run
        self.mock_ws.getInstrument.return_value = self.mock_inst
        self.mock_ws.getRunNumber.return_value = 1
        self.mock_ws.getTitle.return_value = 'title'
        mock_axis = MagicMock()
        mock_unit = MagicMock()
        self.mock_ws.getAxis.return_value = mock_axis
        mock_axis.getUnit.return_value = mock_unit
        mock_unit.caption.return_value = 'Time-of-flight'

    def tearDown(self) -> None:
        self.model.clear_input_components()
        mtd.clear()

    def _setup_model_log_workspaces(self):
        # grouped ws acts like a container/list of ws here
        self.model._sample_logs_workspace_group._log_workspaces = [MagicMock(), MagicMock()]
        self.model._sample_logs_workspace_group._log_workspaces[0].name.return_value = "run_info"
        self.model._sample_logs_workspace_group._log_workspaces[1].name.return_value = "LogName"

    def test_initial_validation(self):

        success = self.model.initial_validation("project_name", ['inst', 'phase', 'data'])
        self.assertTrue(success)
        no_project = self.model.initial_validation("project_name", [None, 'phase', 'data'])
        self.assertFalse(no_project)
        no_load = self.model.initial_validation(None, ['inst', None, 'data'])
        self.assertFalse(no_load)

    def test_further_validation(self):
        self.model.limits = []
        self.model.refinement_method = "Rietveld"
        self.model.mantid_pawley_reflections = []
        no_limits = self.model.further_validation()
        self.assertFalse(no_limits)

        self.model.limits = [18000, 50000]
        self.model.refinement_method = "Rietveld"
        self.model.mantid_pawley_reflections = []
        success = self.model.further_validation()
        self.assertTrue(success)

        self.model.limits = [18000, 50000]
        self.model.refinement_method = "Pawley"
        self.model.mantid_pawley_reflections = []
        no_reflections = self.model.further_validation()
        self.assertFalse(no_reflections)

        self.model.limits = [18000, 50000]
        self.model.refinement_method = "Pawley"
        self.model.mantid_pawley_reflections = ['valid_reflections', 1, 2, 3]
        success_2 = self.model.further_validation()
        self.assertTrue(success_2)

        self.model.limits = []
        self.model.refinement_method = "Pawley"
        self.model.mantid_pawley_reflections = ['valid_reflections', 1, 2, 3]
        no_limits_2 = self.model.further_validation()
        self.assertFalse(no_limits_2)

    def test_find_in_file(self):
        # read_space_group() is a specific wrapper for find_in_file()
        read_space_group_string = self.model.read_space_group(self.phase_file_path)
        self.assertEqual("F m 3 m", read_space_group_string)

    def test_insert_minus_before_first_digit(self):
        self.assertEqual(self.model.insert_minus_before_first_digit("F m 3 m"), "F m -3 m")
        self.assertEqual(self.model.insert_minus_before_first_digit("42m"), "-42m")
        self.assertEqual(self.model.insert_minus_before_first_digit("F d d d"), "F d d d")

    def test_read_basis(self):
        self.assertEqual(self.model.read_basis(self.phase_file_path), ['Fe 0.0 0.0 0.0 1.0 0.025'])
        self.assertEqual(self.model.read_basis("invalid_phase_file"), None)

    def test_choose_cell_lengths(self):
        self.assertEqual(self.model.choose_cell_lengths(self.phase_file_path), '3.6105 3.6105 3.6105')

        self.model.override_cell_length_string = '3.65'
        self.assertEqual(self.model.choose_cell_lengths("phase_file_not_used"), '3.65 3.65 3.65')

        self.model.override_cell_length_string = None
        self.assertEqual(self.model.choose_cell_lengths("invalid_phase_file"), None)

    def test_generate_reflections_from_space_group(self):
        self.model.dSpacing_min = 1.0
        mantid_reflections = self.model.generate_reflections_from_space_group(self.phase_file_path, "3.65 3.65 3.65")
        self.assertEqual(mantid_reflections, [[[1.0, 1.0, 1.0], 2.107328482542134, 8],
                                              [[2.0, 0.0, 0.0], 1.8250000000000002, 6],
                                              [[2.0, 2.0, 0.0], 1.2904698756654494, 12],
                                              [[3.0, 1.0, 1.0], 1.1005164077088374, 24],
                                              [[2.0, 2.0, 2.0], 1.053664241271067, 8]])

    def test_understand_data_structure(self):
        # note this gss-ExtendedHeader.gsa test file is not widely relevant for this interface
        gsa_file_path = FileFinder.getFullPath('gss-ExtendedHeader.gsa')
        self.model.data_files = [gsa_file_path]
        self.model.understand_data_structure()
        self.assertEqual(self.model.number_of_regions, 1)
        self.assertEqual(self.model.data_x_min, [41.7276698840232])
        self.assertEqual(self.model.data_x_max, [41.82791649202319])

    @patch(model_path + ".GSAS2Model.find_in_file")
    def test_crystal_params_from_instrument_split_on_spaces(self, mock_find_in_file):
        mock_find_in_file.return_value = '18306.98      2.99     14.44'
        self.assertEqual(self.model.get_crystal_params_from_instrument("unused_path"), [18324.41])

    @patch(model_path + ".GSAS2Model.find_in_file")
    def test_crystal_params_from_instrument_file_mismatch(self, mock_find_in_file):
        mock_find_in_file.return_value = None
        self.assertEqual(self.model.get_crystal_params_from_instrument("file_mismatch"), None)

    @patch(model_path + ".GSAS2Model.find_in_file")
    def test_crystal_params_from_instrument_split_on_tabs(self, mock_find_in_file):
        mock_find_in_file.return_value = "18000\t3.0\t14.0"
        self.assertEqual(self.model.get_crystal_params_from_instrument("mock_instrument"), [18017.0])

    @patch(model_path + ".GSAS2Model.find_in_file")
    def test_determine_tof_min(self, mock_find_in_file):
        # 2 instrument files
        mock_find_in_file.return_value = "18000\t3.0\t14.0"
        self.model.instrument_files = ["first", "second"]
        self.model.number_of_regions = 2
        self.assertEqual(self.model.get_crystal_params_from_instrument("mock_instrument"), [18017.0, 18017.0])

        # 1 instrument file
        self.model.instrument_files = ["first file containing 2 banks"]
        self.model.number_of_regions = 2
        self.assertEqual(self.model.get_crystal_params_from_instrument("mock_instrument"), [18017.0, 18017.0])

    @patch(model_path + ".GSAS2Model.determine_tof_min")
    def test_determine_x_limits(self, mock_tof_min):
        mock_tof_min.return_value = [17000, 19000]
        self.model.x_min = [18000, 18000]
        self.model.determine_x_limits()
        self.assertEqual(self.model.x_min, [18000, 19000])

    @patch(model_path + ".GSAS2Model.determine_tof_min")
    @patch(model_path + ".GSAS2Model.understand_data_structure")
    def test_validate_x_limits(self, mock_understand_data, mock_tof_min):
        self.model.number_of_regions = 1
        self.model.instrument_files = ["inst1", "inst2"]
        self.model.data_files = ["data1", "data2", "data3"]

        two_inst_three_histograms = self.model.validate_x_limits([[18000], [50000]])
        self.assertEqual(two_inst_three_histograms, None)

        self.model.instrument_files = ["inst1"]
        one_inst_three_histograms = self.model.validate_x_limits([[18000], [50000]])
        self.assertEqual(one_inst_three_histograms, True)
        self.assertEqual(self.model.limits, [[18000.0, 18000.0, 18000.0], [50000.0, 50000.0, 50000.0]])

        self.model.number_of_regions = 2
        self.model.instrument_files = ["inst1"]
        self.model.data_files = ["data1"]
        one_inst_one_hist_two_regions = self.model.validate_x_limits([[18000], [50000]])
        self.assertEqual(one_inst_one_hist_two_regions, True)
        self.assertEqual(self.model.limits, [[18000.0, 18000.0], [50000.0, 50000.0]])

        mock_tof_min.return_value = [19000]
        self.model.data_x_min = [18000]
        self.model.data_x_max = [40000]
        self.model.instrument_files = ["inst1"]
        self.model.data_files = ["data1"]
        no_user_limits = self.model.validate_x_limits(None)
        self.assertEqual(no_user_limits, True)
        self.assertEqual(self.model.limits, [[19000], [40000]])

    def test_read_gsas_lst(self):
        logged_lst_result = self.model.read_gsas_lst_and_print_wR(self.lst_result_filepath,
                                                                  ["ENGINX_305761_307521_all_banks_TOF.gss"],
                                                                  test=True)
        self.assertEqual(logged_lst_result, "Final refinement wR = 26.83%")

    def test_find_phase_names_in_lst(self):
        phase_names = self.model.find_phase_names_in_lst(self.lst_result_filepath)
        self.assertEqual(phase_names, ['Fe_gamma'])

    def test_report_on_outputs(self):
        test_directory = os.path.join(os.getcwd(), "GSASModelTesting")
        result_path = os.path.join(test_directory, f"{self.model.project_name}.lst")
        project_path = os.path.join(test_directory, f"{self.model.project_name}.gpx")
        os.makedirs(test_directory)
        try:
            # No output GSAS-II files
            self.model.err_call_gsas2 = "Errors from GSAS-II"
            self.model.temporary_save_directory = test_directory

            files_not_found = self.model.report_on_outputs(3.14)
            self.assertEqual(files_not_found, None)
            # Output GSAS-II files found
            with open(result_path, mode='w'):
                pass
            with open(project_path, mode='w'):
                pass
            self.model.temporary_save_directory = test_directory
            found_result_filepath, logged_success = self.model.report_on_outputs(3.14, test=True)
            self.assertEqual(found_result_filepath, result_path)
            self.assertEqual(logged_success, "\nGSAS-II call complete in 3.14 seconds.\n")
        finally:
            if os.path.exists(result_path):
                os.remove(result_path)
            if os.path.exists(project_path):
                os.remove(project_path)
            os.rmdir(test_directory)

    def test_check_for_output_file(self):
        project_name = "gsas2_project"
        test_directory = os.path.join(os.getcwd(), "GSASModelTesting")
        project_path = os.path.join(test_directory, f"{project_name}.gpx")
        os.makedirs(test_directory)
        try:
            # No output GSAS-II files
            self.model.err_call_gsas2 = "Detailed\nError\nMessage"
            self.model.temporary_save_directory = test_directory
            self.model.project_name = project_name
            logged_failure = self.model.check_for_output_file(".gpx", "project", test=True)
            expected_logged_failure = "GSAS-II call must have failed, as the output project file was not found." \
                                      + "\n\n" \
                                      + "\n---------------------\n---------------------\n" \
                                      + " Errors from GSAS-II " \
                                      + "\n---------------------\n---------------------\n" \
                                      + "Detailed\nError\nMessage" \
                                      + "\n---------------------\n---------------------\n" \
                                      + "\n\n"

            self.assertEqual(logged_failure, expected_logged_failure)

            # Output GSAS-II files found
            with open(project_path, mode='w'):
                pass
            self.model.temporary_save_directory = test_directory
            found_project_filepath = self.model.check_for_output_file(".gpx", "project file", test=True)
            self.assertEqual(found_project_filepath, project_path)

        finally:
            if os.path.exists(project_path):
                os.remove(project_path)
            os.rmdir(test_directory)

    @patch(model_path + ".output_settings.get_output_path")
    def test_organize_save_directories(self, mock_output_path):
        try:
            project_name = "project_name"
            self.model.project_name = project_name
            current_directory = os.getcwd()
            stem_save_dir = os.path.join(current_directory, "GSAS2")
            expected_temp_directory = os.path.join(stem_save_dir, "tmp_EngDiff_GSASII")
            mock_user_directory = os.path.join(stem_save_dir, project_name)
            mock_output_path.return_value = current_directory
            rb_num = "valid_rb_number"
            self.model.organize_save_directories(rb_num)
            self.assertEqual(self.model.user_save_directory, mock_user_directory)
            self.assertEqual(self.model.temporary_save_directory[:-20], expected_temp_directory)
            self.assertEqual(self.model.gsas2_save_dirs[0], os.path.join(current_directory, "GSAS2", ""))
            self.assertEqual(len(self.model.gsas2_save_dirs), 2)
            self.assertEqual(self.model.gsas2_save_dirs[1], os.path.join(current_directory, "User", rb_num, "GSAS2",
                                                                         project_name, ""))
        finally:
            _try_delete(stem_save_dir)

    def test_move_output_files_to_user_save_location(self):
        project_name = "project_name"
        file_name = f"{project_name}.gpx"
        current_directory = os.getcwd()
        stem_user_dir = os.path.join(current_directory, "User")
        rb_directory = os.path.join(stem_user_dir, "rb_num", "GSAS2",
                                    project_name, "")
        save_directory = os.path.join(current_directory, "GSAS2")
        try:
            self.model.gsas2_save_dirs = [save_directory, rb_directory]
            self.model.user_save_directory = os.path.join(save_directory, project_name)
            self.model.temporary_save_directory = os.path.join(current_directory, "GSAS2", "tmp_EngDiff_GSASII")
            project_path = os.path.join(self.model.temporary_save_directory, file_name)
            os.makedirs(self.model.temporary_save_directory)
            with open(project_path, mode='w'):
                pass
            save_message = self.model.move_output_files_to_user_save_location()
            self.assertEqual(save_message, f"\n\nOutput GSAS-II files saved in {self.model.user_save_directory}"
                             f" and in {rb_directory}")
        finally:
            _try_delete(self.model.user_save_directory)
            _try_delete(stem_user_dir)
            _try_delete(save_directory)

    def test_load_gsas_histogram(self):
        self.model.x_min = [20000]
        self.model.x_max = [21000]
        histogram_workspace = self.model.load_gsas_histogram(1)
        ReplaceSpecialValues(InputWorkspace=histogram_workspace, OutputWorkspace=histogram_workspace, NaNValue=0)
        expected_workspace = LoadNexus(FileFinder.getFullPath("gsas2_output_workspace_1.nxs"))
        ReplaceSpecialValues(InputWorkspace=expected_workspace, OutputWorkspace=expected_workspace, NaNValue=0)
        match_bool, _ = CompareWorkspaces(histogram_workspace, expected_workspace)
        self.assertTrue(match_bool)
        mtd.clear()

    def test_load_gsas2_reflections(self):
        self.model.clear_input_components()
        self.model.user_save_directory = os.path.dirname(FileFinder.getFullPath("gsas2_output_reflections_1_Fe_gamma.txt"))
        self.model.project_name = "gsas2_output"
        reflections = self.model.load_gsas_reflections(1, ["Fe_gamma"])
        expected_reflections = [np.array([38789.37179684, 33592.5813729,
                                          23753.54208634, 20257.08875516,
                                          19394.68589842])]
        # check all values in two numpy arrays are the same
        self.assertTrue((reflections[0] - expected_reflections[0]).all())

    @patch(model_path + ".GSAS2Model.find_phase_names_in_lst")
    def test_create_lattice_parameter_table(self, mock_find_phase_names):
        self.model.clear_input_components()
        mock_find_phase_names.return_value = ["Fe_gamma"]
        self.model.user_save_directory = os.path.dirname(FileFinder.getFullPath("gsas2_output.lst"))
        self.model.project_name = "gsas2_output"
        table_ws = self.model.create_lattice_parameter_table(test=True)
        expected_table_ws = LoadNexus(FileFinder.getFullPath("gsas2_output_table_workspace.nxs"))

        match_bool, _ = CompareWorkspaces(table_ws, expected_table_ws)
        self.assertTrue(match_bool)

    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.remove_all_log_rows")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.make_log_table")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.make_runinfo_table")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.create_log_workspace_group")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.add_log_to_table")
    @patch(output_sample_log_path + ".ADS")
    def test_update_logs_deleted(self, mock_ads, mock_add_log, mock_create_log_group, mock_make_runinfo,
                                 mock_make_log_table, mock_remove_all_log_rows):
        self.model._data_workspaces.add("name1", loaded_ws=self.mock_ws)
        self.model._sample_logs_workspace_group._log_workspaces = MagicMock()
        self.model._sample_logs_workspace_group._log_names = ['LogName1', 'LogName2']
        # simulate LogName2 and run_info tables being deleted
        mock_ads.doesExist = lambda ws_name: ws_name == 'LogName1'

        self.model._sample_logs_workspace_group.update_log_workspace_group(self.model._data_workspaces)
        mock_create_log_group.assert_not_called()
        mock_make_runinfo.assert_called_once()
        mock_make_log_table.assert_called_once_with('LogName2')
        self.model._sample_logs_workspace_group._log_workspaces.add.assert_any_call("run_info")
        self.model._sample_logs_workspace_group._log_workspaces.add.assert_any_call("LogName2")
        mock_add_log.assert_called_with("name1", self.mock_ws, 0)

    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.make_log_table")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.make_runinfo_table")
    @patch(output_sample_log_path + ".get_setting")
    @patch(output_sample_log_path + ".GroupWorkspaces")
    def test_create_log_workspace_group(self, mock_group, mock_get_setting, mock_make_runinfo, mock_make_log_table):
        log_names = ['LogName1', 'LogName2']
        mock_get_setting.return_value = ','.join(log_names)
        mock_group_ws = MagicMock()
        mock_group.return_value = mock_group_ws

        self.model._sample_logs_workspace_group.create_log_workspace_group()

        mock_group.assert_called_once()
        for log in log_names:
            mock_group_ws.add.assert_any_call(log)
        self.assertEqual(log_names, self.model._sample_logs_workspace_group._log_names)
        mock_make_runinfo.assert_called_once()
        self.assertEqual(len(log_names), mock_make_log_table.call_count)

    @patch(output_sample_log_path + ".write_table_row")
    @patch(output_sample_log_path + ".ADS")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.update_log_group_name")
    @patch(output_sample_log_path + ".AverageLogData")
    def test_add_log_to_table_already_averaged(self, mock_avglogs, mock_update_logname, mock_ads, mock_writerow):
        self._setup_model_log_workspaces()
        mock_ads.retrieve = lambda ws_name: [ws for ws in self.model._sample_logs_workspace_group._log_workspaces
                                             if ws.name() == ws_name][0]
        self.model._sample_logs_workspace_group._log_values = {"name1": {"LogName": [2, 1]}}
        self.model._sample_logs_workspace_group._log_names = ["LogName"]

        log_workspaces = self.model._sample_logs_workspace_group._log_workspaces
        self.model._sample_logs_workspace_group.add_log_to_table("name1", self.mock_ws, 3)
        mock_writerow.assert_any_call(log_workspaces[0], ['instrument', 1, 'bank 1', 1.0, 'title'], 3)
        mock_writerow.assert_any_call(log_workspaces[1], [2, 1], 3)
        mock_avglogs.assert_not_called()
        mock_update_logname.assert_called_once()

    @patch(output_sample_log_path + ".write_table_row")
    @patch(output_sample_log_path + ".ADS")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.update_log_group_name")
    @patch(output_sample_log_path + ".AverageLogData")
    def test_add_log_to_table_not_already_averaged(self, mock_avglogs, mock_update_logname, mock_ads, mock_writerow):
        self._setup_model_log_workspaces()
        mock_ads.retrieve = lambda ws_name: [ws for ws in self.model._sample_logs_workspace_group._log_workspaces
                                             if ws.name() == ws_name][0]
        self.model._sample_logs_workspace_group._log_values = {"name1": {}}
        self.model._sample_logs_workspace_group._log_names = ["LogName"]
        mock_avglogs.return_value = [1.0, 1.0]

        self.model._sample_logs_workspace_group.add_log_to_table("name1", self.mock_ws, 3)

        self.assertEqual(self.model._sample_logs_workspace_group._log_values["name1"]["LogName"], [1.0, 1.0])
        mock_writerow.assert_any_call(self.model._sample_logs_workspace_group._log_workspaces[1], [1.0, 1.0], 3)
        mock_avglogs.assert_called_with("name1", LogName="LogName", FixZero=False)
        mock_update_logname.assert_called_once()

    @patch(output_sample_log_path + ".write_table_row")
    @patch(output_sample_log_path + ".ADS")
    @patch(output_sample_log_path + ".SampleLogsGroupWorkspace.update_log_group_name")
    @patch(output_sample_log_path + ".AverageLogData")
    def test_add_log_to_table_not_existing_in_ws(self, mock_avglogs, mock_update_logname, mock_ads, mock_writerow):
        self._setup_model_log_workspaces()
        mock_ads.retrieve = lambda ws_name: [ws for ws in self.model._sample_logs_workspace_group._log_workspaces
                                             if ws.name() == ws_name][0]
        self.model._sample_logs_workspace_group._log_values = {"name1": {}}
        self.model._sample_logs_workspace_group._log_names = ["LogName"]
        self.mock_run.getLogData.return_value = [self.mock_run.getLogData()[1]]  # only proton_charge

        self.model._sample_logs_workspace_group.add_log_to_table("name1", self.mock_ws, 3)

        self.assertTrue(all(np.isnan(self.model._sample_logs_workspace_group._log_values["name1"]["LogName"])))
        self.assertTrue(len(self.model._sample_logs_workspace_group._log_values["name1"]["LogName"]), 2)
        mock_avglogs.assert_not_called()
        mock_update_logname.assert_called_once()


if __name__ == '__main__':
    unittest.main()
