# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest

from plugins.algorithms.WorkflowAlgorithms.SANSISISPolarizationCorrections import (
    SANSISISPolarizationCorrections,
    InstrumentConfig,
    WsInfo,
    MAIN_PROPERTIES,
    _get_value_from_dict,
    _add_or_replace,
    _clean_temporary_ws,
)
from mantid.kernel import ConfigService
from mantid.api import AnalysisDataService as ads
from mantid.simpleapi import CreateWorkspace, GroupWorkspaces, SaveNexusProcessed

from collections import namedtuple
from dataclasses import asdict
from tempfile import TemporaryDirectory, NamedTemporaryFile
from testhelpers import assertRaisesNothing
from unittest.mock import patch, MagicMock, call


class SANSISISPolarizationCorrectionsTest(unittest.TestCase):
    _ALG_PATCH_PATH = "plugins.algorithms.WorkflowAlgorithms.SANSISISPolarizationCorrections.SANSISISPolarizationCorrections"
    _GROUP_NAME = "group"
    _WS_NAME = "test"
    _EFF_NAMES = ["analyzer_eff", "flipper_eff", "flipper_a_eff", "pol_fit_table", "polarizer_eff"]
    _RED_TYPE = "Calibration"

    @classmethod
    def setUpClass(cls):
        cls.user_file = NamedTemporaryFile(mode="r+", suffix=".toml", delete=False)
        ConfigService["MantidOptions.InvisibleWorkspaces"] = "1"
        cls._MANDATORY_PROPS_CAL = {
            MAIN_PROPERTIES.reduction: cls._RED_TYPE,
            MAIN_PROPERTIES.transmission: "1",
            MAIN_PROPERTIES.direct: "1",
            MAIN_PROPERTIES.cell: "1",
            MAIN_PROPERTIES.user: cls.user_file.name,
        }

    @classmethod
    def tearDownClass(cls):
        if os.path.exists(cls.user_file.name):
            cls.user_file.close()
            os.remove(cls.user_file.name)

    def setUp(self):
        self.inst = InstrumentConfig()

    def tearDown(self):
        ads.clear()

    def test_get_value_from_dict(self):
        dict_user = asdict(self.inst)

        fake_key = "not_in_dict"
        real_key = "name"

        # wrong key mandatory
        self.assertRaises(KeyError, _get_value_from_dict, dict_user, [fake_key], "")
        # value from dict
        self.assertEqual(_get_value_from_dict(dict_user, [real_key], ""), "LARMOR")
        # value from default_value param
        self.assertEqual(_get_value_from_dict(dict_user, [fake_key], "LARMOR", False), "LARMOR")
        # can retrieve from nested key
        dict_user.update({"name": {"nested_name": "LARMOR"}})
        self.assertEqual(_get_value_from_dict(dict_user, [real_key, "nested_name"], ""), "LARMOR")

    def test_add_or_replace_ads_deletes_group_members_when_replacing_groups(self):
        names1 = self._create_test_group(return_member_names=True)
        ws = self._create_test_workspace()
        _add_or_replace("result", ws)
        self.assertNotIn(names1, ads.getObjectNames())

    def test_clean_temporary_ws(self):
        # Deletes workspaces with temp prefix and any additional we require
        temp_prefix = "__polsans_"
        names = []
        for i in range(2):
            names.append(f"{temp_prefix}_{i}")
            self._create_test_workspace(out_name=names[-1])
        member_names = self._create_test_group(group_name=self._GROUP_NAME, return_member_names=True)
        names.extend([self._GROUP_NAME, *member_names])

        self.assertEqual(names, ads.getObjectNames())
        _clean_temporary_ws(self._GROUP_NAME)
        self.assertEqual(0, len(ads.getObjectNames()))

    def test_mandatory_runs_for_different_reduction_types(self):
        # Format: [reduction_type, scattering_runs, transmission_runs, direct_run, depolarized_run]
        error_cases = [
            ["Correction", [*[] * 4]],  # Only scattering is needed.
            ["Calibration", "", "", "1", "1"],  # Missing scattering is OK for calibration only
            ["Calibration", "", "1", "", "1"],
            ["Calibration", "", "1", "1", ""],
            ["CalibrationAndCorrection", "", "1", "1", "1"],  # Nothing can be missing for complete reduction
            ["CalibrationAndCorrection", "1", "", "1", "1"],
            ["CalibrationAndCorrection", "1", "1", "", "1"],
            ["CalibrationAndCorrection", "1", "1", "1", ""],
        ]

        for test_case in error_cases:
            with self.subTest(test_case=test_case):
                prop_names = MAIN_PROPERTIES[:4]
                args = dict(self._MANDATORY_PROPS_CAL, ReductionType=test_case[0], **{k: v for k, v in zip(prop_names, test_case[1:])})
                alg = self._setup_algorithm(args)
                self.assertRaisesRegex(RuntimeError, f"This property is necessary for reduction of type {test_case[0]}", alg.execute)

    def test_create_run_list_updates_aux_ws_with_suffix_and_prefix(self):
        alg = self._setup_algorithm()
        alg.aux_ws = dict()
        alg.suffix = "post"
        prefix = "pre"
        run_list = ["1", "2", "3"]
        expected_names = [f"{prefix}_{r}" for r in run_list]

        result_names = alg._create_run_list(run_list, prefix)

        self.assertEqual(result_names, expected_names)
        self.assertEqual([v.ads_name for v in alg.aux_ws.values()], expected_names)

    def test_create_run_list_does_not_add_new_key_for_protected_name(self):
        alg = self._setup_algorithm()
        alg.aux_ws = dict()
        prefix = "direct"  # It's in AUX_WS_BASENAME list
        run_list = ["222"]

        result_names = alg._create_run_list(run_list, prefix)
        self.assertEqual(result_names, ["direct_222"])
        self.assertTrue("direct" in alg.aux_ws)

    def test_filter_ads_names_selects_from_multiple_suffices(self):
        names = ["pre_1_", "pre_1_t", "pre1", "pre2", "pre_2", "hola"]
        alg = self._setup_algorithm()
        alg.aux_ws = {name: WsInfo(ads_name=name) for name in names}

        filtered_names = alg._filter_ads_names(["pre_1", "pre_2"])
        self.assertEqual(filtered_names, [*names[0:2], names[4]])

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_load_runs_raises_runtime_error_for_non_groups_if_not_direct_run(self, mock_alg_child):
        self._create_test_workspace(out_name=self._WS_NAME)
        alg = self._setup_algorithm()

        self.assertRaisesRegex(
            RuntimeError, "Run run_number is not compatible with the polarization reduction", alg._load_run, "run_number", self._WS_NAME
        )
        mock_alg_child.assert_called_once()

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_load_runs_accepts_a_non_group_for_direct_runs(self, mock_alg_child):
        self._create_test_workspace(out_name="direct")
        alg = self._setup_algorithm()

        assertRaisesNothing(self, alg._load_run, "run_number", "direct")
        mock_alg_child.assert_called_once()

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_ungroup_on_average_workspace_method(self, mock_alg_child):
        group_names = self._create_test_group(return_member_names=True)
        alg = self._setup_algorithm()

        call_args = [
            ["UnGroupWorkspace", {"InputWorkspace": self._GROUP_NAME}],
            ["Mean", {"Workspaces": ",".join(group_names), "OutputWorkspace": "Out"}],
            ["DeleteWorkspaces", {"WorkspaceList": group_names}],
        ]
        calls = [call(arg[0], **arg[1]) for arg in call_args]
        alg._average_workspaces([self._GROUP_NAME], "Out", ungroup=True)

        mock_alg_child.assert_has_calls(calls)

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_move_instrument_not_called_if_parameters_are_zero(self, mock_child_alg):
        alg = self._setup_algorithm()
        alg.instrument = self.inst
        alg._move_instrument(self._WS_NAME)
        mock_child_alg.assert_not_called()

    def test_load_ws_from_path_throws_runtime_error_if_missing_paths(self):
        alg = self._setup_algorithm()
        alg.aux_ws = {key: WsInfo(key, "") for key in self._EFF_NAMES}
        self.assertRaisesRegex(
            RuntimeError, "Missing path for file or ws with name: analyzer_eff", alg._load_workspace_from_path, self._EFF_NAMES
        )

    def test_load_ws_from_paths_in_filesystem(self):
        """We create a temporary folder and store mock workspaces to represent the efficiency files, all files are deleted
        once this test is over"""
        alg = self._setup_algorithm()
        ws = self._create_test_workspace()
        alg.aux_ws = dict()
        with TemporaryDirectory() as temp_dir:
            for key in self._EFF_NAMES:
                path = os.path.join(temp_dir, f"{key}_file.nxs")
                alg.aux_ws[key] = WsInfo(ads_name=f"{key}_file_test", path=path)
                SaveNexusProcessed(InputWorkspace=ws, Filename=path)

            assertRaisesNothing(self, alg._load_workspace_from_path, self._EFF_NAMES)
            self.assertTrue(all([ads.doesExist(f"{key}_file_test") for key in self._EFF_NAMES]))

    @patch(f"{_ALG_PATCH_PATH}.load")
    def test_load_ws_from_paths_doesnt_try_to_load_if_ws_on_ads(self, mock_data_processor_load):
        alg = self._setup_algorithm()
        alg.aux_ws = dict()
        for key in self._EFF_NAMES:
            test_name = f"{key}_test"
            self._create_test_workspace(out_name=test_name)
            alg.aux_ws[key] = WsInfo(test_name, key)

        assertRaisesNothing(self, alg._load_workspace_from_path, self._EFF_NAMES)
        mock_data_processor_load.assert_not_called()

    @patch(f"{_ALG_PATCH_PATH}._load_run")
    @patch(f"{_ALG_PATCH_PATH}._prepare_workspace")
    def test_load_and_process_runs_doesnt_do_anything_if_ws_on_ads(self, mock_prepare, mock_load):
        alg = self._setup_algorithm()
        alg.aux_ws = dict()
        for key in self._EFF_NAMES:
            test_name = f"test_{key}"
            self._create_test_workspace(out_name=test_name)
            alg.aux_ws[key] = WsInfo(test_name, key)

        assertRaisesNothing(self, alg._load_and_process_runs, self._EFF_NAMES, "test", *[True] * 4)
        mock_load.assert_not_called()
        mock_prepare.assert_not_called()

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_assert_spin_alg_not_called_if_assert_spin_states_property_false(self, mock_load_child_alg):
        self._create_test_group()
        alg = self._setup_algorithm()
        alg.instrument = self.inst

        alg.instrument.spin_state_str = "00,01,10,11"
        alg.assert_spin = False
        assertRaisesNothing(self, alg._check_spin_states, self._GROUP_NAME)
        mock_load_child_alg.assert_not_called()

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_check_spin_state_raises_error_if_not_assert_spin_and_group_size_different_than_spin_states(self, mock_load_child_alg):
        self._create_test_group()
        alg = self._setup_algorithm()
        alg.instrument = self.inst

        alg.instrument.spin_state_str = "00,11"
        alg.assert_spin = False
        self.assertRaisesRegex(
            RuntimeError,
            f"The number of periods in {self._GROUP_NAME} differs from the expected: 2",
            alg._check_spin_states,
            self._GROUP_NAME,
        )
        mock_load_child_alg.assert_not_called()

    @patch(f"{_ALG_PATCH_PATH}._average_workspaces")
    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_process_efficiencies_calls_correctly_for_every_flipper_configuration(self, mock_child, mock_average):
        alg = self._setup_algorithm()
        alg.aux_ws = {key: WsInfo(key, "") for key in self._EFF_NAMES}
        alg.instrument = self.inst
        alg.instrument.spin_state_str = "00,01,10,11"

        alg.progress = MagicMock()
        alg._calculate_helium_analyzer_efficiency_and_parameters = MagicMock()
        alg._create_run_list = MagicMock(return_value=["test_1"])

        for has_2nd_flipper, no_calls in zip([False, True], [2, 3]):
            with self.subTest(has_2nd_flipper=has_2nd_flipper, no_calls=no_calls):
                alg.has_2nd_flipper = has_2nd_flipper
                alg._process_efficiencies(["1"])
                self.assertEqual(mock_child.call_count, no_calls)
                self.assertEqual(mock_average.call_count, no_calls + 1)
                mock_child.reset_mock()
                mock_average.reset_mock()

    @patch(f"{_ALG_PATCH_PATH}._load_child_algorithm")
    def test_check_spin_state_raises_error_for_wrong_spin_assertion(self, mock_load_child_alg):
        alg = self._setup_algorithm()
        alg.instrument = self.inst
        alg.instrument.spin_state_str = "00,11"

        alg.assert_spin = True
        mock_load_child_alg.return_value = False
        self.assertRaisesRegex(
            RuntimeError,
            f"The spin configuration for workspace {self._GROUP_NAME} differs from the expected: {alg.instrument.spin_state_str}",
            alg._check_spin_states,
            self._GROUP_NAME,
        )
        mock_load_child_alg.assert_called_once()

    @patch(f"{_ALG_PATCH_PATH}.createChildAlgorithm")
    def test_load_child_algorithm_when_output_param_is_not_set(self, mock_create_child_alg):
        alg_name = "TestAlg"
        alg = self._setup_algorithm()
        kwargs = {"InputWS": "in_1", "OutputWS": "out_1"}

        child_alg = MagicMock()
        mock_create_child_alg.return_value = child_alg

        result = alg._load_child_algorithm(alg_name, **kwargs)

        mock_create_child_alg.assert_called_once_with(alg_name, **kwargs)
        child_alg.setAlwaysStoreInADS.assert_called_once_with(True)
        self.assertEqual(result, None)

    @patch(f"{_ALG_PATCH_PATH}.createChildAlgorithm")
    def test_load_child_algorithm_when_output_param_set(self, mock_create_child_alg):
        alg_name = "TestAlg"
        alg = self._setup_algorithm()
        kwargs = {"InputWS": "in_1", "OutputWS": "out_1"}
        result_nt = namedtuple("Result", "value")

        child_alg = MagicMock()
        child_alg.getProperty.side_effect = lambda x: result_nt(value=x)
        mock_create_child_alg.return_value = child_alg

        result_alg = alg._load_child_algorithm(alg_name, output="OutputWS", **kwargs)

        mock_create_child_alg.assert_called_once_with(alg_name, **kwargs)
        child_alg.setAlwaysStoreInADS.assert_called_once_with(False)
        self.assertEqual(result_alg, "OutputWS")

    @patch(f"{_ALG_PATCH_PATH}.createChildAlgorithm")
    def test_load_child_algorithm_error(self, mock_create_child_alg):
        alg_name = "TestAlg"
        error_msg = "error message"
        alg = self._setup_algorithm()

        child_alg = MagicMock()
        child_alg.execute.side_effect = RuntimeError(error_msg)
        mock_create_child_alg.return_value = child_alg

        with self.assertRaisesRegex(RuntimeError, f"Error in execution of child algorithm {alg_name} : {error_msg}"):
            alg._load_child_algorithm(alg_name)

    @patch(f"{_ALG_PATCH_PATH}._init_reduction_settings")
    @patch(f"{_ALG_PATCH_PATH}._polarization_calibration")
    def test_context_manager_deletes_partial_ads_names_on_error(self, mock_calibration, mock_init):
        alg = self._setup_algorithm()
        # add test workspaces to ads
        prefix = "__polsans_"
        names = []
        for i in range(2):
            names.append(f"{prefix}_{i}")
            self._create_test_workspace(out_name=names[-1])
        member_names = self._create_test_group(group_name=self._GROUP_NAME, return_member_names=True)
        names.extend([self._GROUP_NAME, *member_names])
        alg.instrument = self.inst
        alg.aux_ws = {name: WsInfo(ads_name=name) for name in names}

        mock_calibration.side_effect = RuntimeError("test")

        self.assertEqual(names, ads.getObjectNames())
        alg.execute()
        mock_init.assert_called_once()
        mock_calibration.assert_called_once()
        self.assertEqual(0, len(ads.getObjectNames()))

    @patch(f"{_ALG_PATCH_PATH}._init_reduction_settings")
    @patch(f"{_ALG_PATCH_PATH}._polarization_calibration")
    def test_context_manager_does_not_deletes_partial_ads_names_on_error_if_delete_partial_property_set_to_false(
        self, mock_calibration, mock_init
    ):
        alg = self._setup_algorithm()
        # add test workspaces to ads
        prefix = "__polsans_"
        names = []
        for i in range(2):
            names.append(f"{prefix}_{i}")
            self._create_test_workspace(out_name=names[-1])
        member_names = self._create_test_group(group_name=self._GROUP_NAME, return_member_names=True)
        names.extend([self._GROUP_NAME, *member_names])
        names.sort()
        alg.instrument = self.inst
        alg.delete_partial = False
        alg.aux_ws = {name: WsInfo(ads_name=name) for name in names}

        mock_calibration.side_effect = RuntimeError("test")

        self.assertEqual(names, ads.getObjectNames())
        alg.execute()
        mock_init.assert_called_once()
        mock_calibration.assert_called_once()
        self.assertEqual(len(names), len(ads.getObjectNames()))

    @patch(f"{_ALG_PATCH_PATH}._init_reduction_settings")
    @patch(f"{_ALG_PATCH_PATH}._polarization_calibration")
    def test_context_manager_restores_config(self, mock_calibration, mock_init):
        def set_config(input_dict):
            for k, v in input_dict.items():
                ConfigService[k] = v

        # We change the config service dict to check that it is restored at the end to whatever the value is here
        test_config_dict = {
            "default.facility": "test",
            "datasearch.searcharchive": "test",
            "default.instrument": "test",
            "MantidOptions.InvisibleWorkspaces": "test",
        }
        backup_config = {k: ConfigService[k] for k in test_config_dict.keys()}

        set_config(test_config_dict)
        alg = self._setup_algorithm()
        alg.instrument = self.inst
        alg.aux_ws = {}

        alg.execute()
        mock_init.assert_called_once()
        mock_calibration.assert_called_once()
        for k in test_config_dict.keys():
            self.assertEqual(ConfigService[k], test_config_dict[k])

        set_config(backup_config)

    def _create_test_group(self, prefix=_WS_NAME, group_name=_GROUP_NAME, size=4, return_member_names=False):
        names = []
        for i in range(size):
            names.append(f"{prefix}_{i}")
            self._create_test_workspace(out_name=names[-1])
        GroupWorkspaces(names, OutputWorkspace=group_name)
        if return_member_names:
            return names

    def _create_test_workspace(self, x=None, y=None, out_name=None, **kwargs):
        x = [0, 1] if x is None else x
        y = [1, 1] if y is None else y
        ws = CreateWorkspace(DataX=x, DataY=y, StoreInADS=False, **kwargs)
        if out_name:
            ads.addOrReplace(out_name, ws)
            return
        return ws

    def _setup_algorithm(self, properties=None):
        properties = self._MANDATORY_PROPS_CAL if properties is None else properties

        alg = SANSISISPolarizationCorrections()
        alg.initialize()
        alg.reduction_type = properties[MAIN_PROPERTIES.reduction] if MAIN_PROPERTIES.reduction in properties else self._RED_TYPE
        for k, v in properties.items():
            alg.setProperty(k, v)
        return alg


if __name__ == "__main__":
    unittest.main()
