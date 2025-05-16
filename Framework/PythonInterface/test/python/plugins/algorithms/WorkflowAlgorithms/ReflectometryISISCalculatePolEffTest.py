# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff import (
    ReflectometryISISCalculatePolEff,
    _int_array_to_string,
    _PROP_DATA,
    _ALGS,
    _EFF_ALG_OUTPUT,
    _EFF_ALG_OUTPUT_DIAG,
    PropData,
)
from unittest.mock import patch, MagicMock, call
from dataclasses import dataclass
import re


class ReflectometryISISCalculatePolEffTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        # Needed to register algorithms to copy properties
        import mantid.simpleapi as sapi  # noqa

    def _set_up_alg(self, properties=None):
        alg = ReflectometryISISCalculatePolEff()
        alg.PyInit()

        if properties:
            for key, val in properties.items():
                alg.setProperty(_PROP_DATA[key].name, val)
        return alg

    def test_int_array_to_string_one_val(self):
        res = _int_array_to_string([5])
        self.assertEqual("5", res)

    def test_int_array_to_string_empty(self):
        res = _int_array_to_string([])
        self.assertEqual("", res)

    def test_int_array_to_string(self):
        res = _int_array_to_string([5, 6, 7, 8, 9, 10])
        self.assertEqual("5-10", res)

    def test_run_input_mandatory(self):
        with self.assertRaisesRegex(expected_exception=ValueError, expected_regex=r"A value must be entered for this parameter"):
            self._set_up_alg({"NON_MAG_INPUT_RUNS": []})

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._run_algorithm")
    def test_create_transmission_workspaces_mag(self, mock_run_alg):
        trans_ws_placeholder = "transmission_ws"
        mock_run_alg.return_value = trans_ws_placeholder
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs", "MAG_INPUT_RUNS": "test_mag_runs"}
        alg = self._set_up_alg(props)
        alg._initialize()
        x, y = alg._create_transmission_workspaces()
        self.assertEqual(trans_ws_placeholder, x)
        self.assertEqual(trans_ws_placeholder, y)
        self.assertTrue(mock_run_alg.call_count == 2)

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._run_algorithm")
    def test_create_transmission_workspaces_no_mag(self, mock_run_alg):
        trans_ws_placeholder = "transmission_ws"
        mock_run_alg.return_value = trans_ws_placeholder
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs"}
        alg = self._set_up_alg(props)
        alg._initialize()
        x, y = alg._create_transmission_workspaces()
        self.assertEqual(trans_ws_placeholder, x)
        self.assertEqual(None, y)
        mock_run_alg.assert_called_once()

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._run_algorithm")
    def test_calculate_wildes_efficiencies(self, mock_run_alg):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs"}
        alg = self._set_up_alg(props)
        alg._initialize()
        eff_arg_names_in = [x.name for x in _EFF_ALG_OUTPUT]
        eff_arg_alias_out = [x.alias for x in _EFF_ALG_OUTPUT]
        mock_run_alg.return_value = [x.name for x in _EFF_ALG_OUTPUT]
        out = alg._calculate_wildes_efficiencies(["trans_output"], [])
        self.assertEqual(dict(zip(eff_arg_alias_out, eff_arg_names_in)), out)
        mock_run_alg.assert_called_once()

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._run_algorithm")
    def test_calculate_wildes_efficiencies_diag(self, mock_run_alg):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs", "INCLUDE_DIAG_OUT": True}
        alg = self._set_up_alg(props)
        alg._initialize()
        eff_alg_output_with_diag = _EFF_ALG_OUTPUT + _EFF_ALG_OUTPUT_DIAG
        eff_arg_names_in = [x.name for x in eff_alg_output_with_diag]
        eff_arg_alias_out = [x.alias for x in eff_alg_output_with_diag]
        mock_run_alg.return_value = [x.name for x in eff_alg_output_with_diag]
        out = alg._calculate_wildes_efficiencies(["trans_output"], [])
        self.assertEqual(dict(zip(eff_arg_alias_out, eff_arg_names_in)), out)
        mock_run_alg.assert_called_once()

    def test_validate_processing_instructions(self):
        alg = self._set_up_alg(
            {"NON_MAG_INPUT_RUNS": ["test_non_mag"], "MAG_INPUT_RUNS": ["test_mag"], "TRANS_ROI": "10-20", "MAG_TRANS_ROI": "15-20"}
        )
        with self.assertRaisesRegex(
            expected_exception=ValueError,
            expected_regex=r"The number of spectra specified in both magnetic and"
            r" non-magnetic processing properties must be equal",
        ):
            alg._initialize()

    def test_populate_args_dict_mag(self):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag", "MAG_INPUT_RUNS": "test_mag"}
        alg = self._set_up_alg(props)
        eff_alg = _ALGS["EFF_ALG"]
        with patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff._PROP_DATA") as mock_prop_data:
            mock_prop_data.values.return_value = [
                PropData(name="NonMagInputRuns", alg=eff_alg, get_value=lambda x: [f"{x[0]}_edit"]),
                PropData(name="MagInputRuns", alg=eff_alg, alias="TEST_ALIAS", mag=True),
                PropData(name="NonAlgProp", alg="test_alg"),
            ]
            args = alg._populate_args_dict(eff_alg, True)
            self.assertEqual({"NonMagInputRuns": ["test_non_mag_edit"], "TEST_ALIAS": ["test_mag"]}, args)

    def test_populate_args_dict_non_mag(self):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag"}
        alg = self._set_up_alg(props)
        eff_alg = _ALGS["EFF_ALG"]
        with patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff._PROP_DATA") as mock_prop_data:
            mock_prop_data.values.return_value = [
                PropData(name="NonMagInputRuns", alg=eff_alg, get_value=lambda x: [f"{x[0]}_edit"]),
                PropData(name="MagInputRuns", alg=eff_alg, alias="TEST_ALIAS", mag=True),
                PropData(name="NonAlgProp", alg="test_alg"),
            ]
            args = alg._populate_args_dict(eff_alg, False)
            self.assertEqual({"NonMagInputRuns": ["test_non_mag_edit"]}, args)

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff.createChildAlgorithm")
    def test_run_algorithm(self, mock_create_child_alg):
        @dataclass
        class AlgResult:
            value: str

        props = {"NON_MAG_INPUT_RUNS": "test_non_mag"}
        alg = self._set_up_alg(props)
        eff_alg = _ALGS["EFF_ALG"]
        args = {"arg1": "arg1_val", "arg2": "arg2_val", "arg3": "arg3_val"}
        output_properties = ["out1", "out2"]
        child_alg = MagicMock()
        child_alg.getProperty.side_effect = lambda x: AlgResult(value=f"{x}_res")
        mock_create_child_alg.return_value = child_alg
        res = alg._run_algorithm(eff_alg, args, output_properties)
        mock_create_child_alg.assert_called_once_with(eff_alg, **args)
        self.assertEqual(["out1_res", "out2_res"], res)

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff.createChildAlgorithm")
    def test_run_algorithm_error(self, mock_create_child_alg):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag"}
        alg = self._set_up_alg(props)
        eff_alg = _ALGS["EFF_ALG"]
        args = {"arg1": "arg1_val"}
        output_properties = ["out1"]
        child_alg = MagicMock()
        child_alg.execute.side_effect = ValueError("test_error")
        mock_create_child_alg.return_value = child_alg
        with self.assertRaisesRegex(
            expected_exception=RuntimeError, expected_regex=re.compile(rf".*{eff_alg}.*{args}.*test_error.*", re.DOTALL)
        ):
            alg._run_algorithm(eff_alg, args, output_properties)

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._populate_args_dict")
    def test_generate_eff_args(self, mock_pop_args_dict):
        non_mag_ws = "test_non_mag"
        mock_pop_args_dict.return_value = {"test": "test_vals"}
        alg = self._set_up_alg()
        args = alg._generate_eff_args([non_mag_ws], [])
        self.assertTrue([non_mag_ws] in args.values())
        self.assertTrue("test_vals" in args.values())
        self.assertEqual(4, len(args))
        mock_pop_args_dict.assert_called_once_with(_ALGS["EFF_ALG"])

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._populate_args_dict")
    def test_generate_eff_args_mag(self, mock_pop_args_dict):
        non_mag_ws = "test_non_mag"
        mag_ws = "test_mag"
        mock_pop_args_dict.return_value = {"test": "test_vals"}
        alg = self._set_up_alg()
        args = alg._generate_eff_args([non_mag_ws], [mag_ws])
        self.assertTrue([non_mag_ws] in args.values())
        self.assertTrue([mag_ws] in args.values())
        self.assertTrue("test_vals" in args.values())
        self.assertEqual(7, len(args))
        mock_pop_args_dict.assert_called_once_with(_ALGS["EFF_ALG"])

    def test_set_output_properties(self):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs"}
        alg = self._set_up_alg(props)
        alg._initialize()
        test_join_ws = "join_ws"
        test_eff_output = {}
        with patch(
            "plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff.setProperty"
        ) as set_prop_mock:
            alg._set_output_properties(test_join_ws, test_eff_output)
        call_list = [call("OutputWorkspace", "join_ws")]
        self.assertEqual(call_list, set_prop_mock.call_args_list)

    def test_set_output_properties_diag(self):
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs", "INCLUDE_DIAG_OUT": True}
        alg = self._set_up_alg(props)
        alg._initialize()
        test_join_ws = "join_ws"
        test_eff_output = {"OutputPhi": "test_phi", "OutputRho": "test_rho", "OutputAlpha": "test_alpha"}
        with patch(
            "plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff.setProperty"
        ) as set_prop_mock:
            alg._set_output_properties(test_join_ws, test_eff_output)
        call_list = [call("OutputWorkspace", "join_ws")]
        call_list.extend([call(x, y) for x, y in test_eff_output.items()])
        self.assertEqual(call_list, set_prop_mock.call_args_list)

    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff._EFF_ALG_OUTPUT")
    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._set_output_properties")
    @patch("plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._run_algorithm")
    @patch(
        "plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._calculate_wildes_efficiencies"
    )
    @patch(
        "plugins.algorithms.WorkflowAlgorithms.ReflectometryISISCalculatePolEff.ReflectometryISISCalculatePolEff._create_transmission_workspaces"
    )
    def test_exec(self, mock_create_transmission_workspaces, mock_calc_wildes, mock_run_alg, mock_set_output, mock_eff_alg_output):
        @dataclass
        class EffAlgOutObj:
            alias: str

        non_mag_trans = "test_out_non_mag"
        mock_create_transmission_workspaces.return_value = (non_mag_trans, None)
        mock_eff_alg_output.__iter__.return_value = iter([EffAlgOutObj("test1_key"), EffAlgOutObj("test2_key")])
        eff_out = {"test1_key": "test1_val", "test2_key": "test2_val", "test4_key": "test4_val"}
        mock_calc_wildes.return_value = eff_out
        mock_run_alg.return_value = ["test_out"]
        props = {"NON_MAG_INPUT_RUNS": "test_non_mag_runs"}
        alg = self._set_up_alg(props)
        alg.PyExec()
        mock_create_transmission_workspaces.assert_called_once()
        mock_calc_wildes.assert_called_once_with(non_mag_trans, None)

        call_list = [call(_ALGS["JOIN_ALG"], {"test1_key": "test1_val", "test2_key": "test2_val"}, ["OutputWorkspace"])]
        self.assertEqual(call_list, mock_run_alg.call_args_list)

        mock_set_output.assert_called_once_with("test_out", eff_out)


if __name__ == "__main__":
    unittest.main()
