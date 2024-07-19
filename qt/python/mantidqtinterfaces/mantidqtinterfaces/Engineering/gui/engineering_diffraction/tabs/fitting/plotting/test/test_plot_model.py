# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import patch, call
from numpy import isnan, nan

from mantid import FunctionFactory
from mantid.kernel import UnitParams, UnitParametersMap
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting import plot_model
from mantid.api import CompositeFunction
from mantid.simpleapi import CreateEmptyTableWorkspace, GroupWorkspaces

plot_model_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.fitting.plotting.plot_model"


class FittingPlotModelTest(unittest.TestCase):
    def setUp(self):
        self.model = plot_model.FittingPlotModel()

        self.mock_inst = mock.MagicMock()
        self.mock_inst.getFullName.return_value = "instrument"

        mock_prop = mock.MagicMock()
        mock_prop.value = "bank 1"  # bank-id
        mock_log_data = [mock.MagicMock(), mock.MagicMock()]
        mock_log_data[0].name = "LogName"
        mock_log_data[1].name = "proton_charge"

        self.mock_run = mock.MagicMock()
        self.mock_run.getProtonCharge.return_value = 1.0
        self.mock_run.getProperty.return_value = mock_prop
        self.mock_run.getLogData.return_value = mock_log_data

        self.mock_ws = mock.MagicMock()
        self.mock_ws.getNumberHistograms.return_value = 1
        self.mock_ws.getRun.return_value = self.mock_run
        self.mock_ws.getInstrument.return_value = self.mock_inst
        self.mock_ws.getRunNumber.return_value = 1
        self.mock_ws.getTitle.return_value = "title"
        mock_axis = mock.MagicMock()
        self.mock_ws.getAxis.return_value = mock_axis

    def test_adding_workspace_to_plot(self):
        self.assertEqual(set(), self.model.plotted_workspaces)
        ax = mock.MagicMock()

        self.model.add_workspace_to_plot("mocked_ws", ax, {"linestyle": "x"})

        self.assertEqual({"mocked_ws"}, self.model.plotted_workspaces)
        ax.plot.assert_called_once_with("mocked_ws", linestyle="x")

    def test_removing_single_tracked_workspace_from_plot(self):
        self.model.plotted_workspaces.add("mocked_ws")
        ax = mock.MagicMock()

        self.model.remove_workspace_from_plot("mocked_ws", ax)

        self.assertEqual(set(), self.model.plotted_workspaces)
        ax.remove_workspace_artists.assert_called_once_with("mocked_ws")

    def test_removing_not_tracked_workspace_from_plot(self):
        self.model.plotted_workspaces.add("mocked_ws")
        ax = mock.MagicMock()

        self.model.remove_workspace_from_plot("whatever", ax)

        self.assertEqual({"mocked_ws"}, self.model.plotted_workspaces)
        ax.remove_workspace_artists.assert_not_called()

    def _get_fwhm_values(self, func_str, func_index):
        comp_func = FunctionFactory.createInitialized(func_str)
        peak_func_names = FunctionFactory.getPeakFunctionNames()
        if isinstance(comp_func, CompositeFunction):
            peak_funcs = [func for func in comp_func if func.name() in peak_func_names]
            func = peak_funcs[func_index]
        else:
            func = comp_func
        peak_func = FunctionFactory.Instance().createPeakFunction(func.name())
        for param_index in range(0, func.nParams()):
            peak_func.setParameter(param_index, func.getParameterValue(param_index))
        return [peak_func.fwhm(), 0.0]

    def test_removing_all_workspaces_from_plot(self):
        self.model.plotted_workspaces.update({"mocked_ws", "mock_ws_2"})
        ax = mock.MagicMock()

        self.model.remove_all_workspaces_from_plot(ax)

        self.assertEqual(set(), self.model.plotted_workspaces)
        self.assertEqual(1, ax.cla.call_count)

    def _setup_update_fit_test(self, mock_table_values, mock_ads, mock_get_diffs, func_str, peak_center_params=None):
        if peak_center_params is None:
            peak_center_params = []
        mock_table = mock.MagicMock()
        mock_table.toDict.return_value = mock_table_values
        mock_ads.retrieve.return_value = mock_table
        difc = 10000
        params = UnitParametersMap()
        params[UnitParams.difc] = difc
        mock_get_diffs.return_value = params
        fitprop = {
            "properties": {
                "InputWorkspace": "name1",
                "Output": "name1",
                "StartX": 50000,
                "EndX": 52000,
                "Function": func_str,
                "ConvolveMembers": True,
                "OutputCompositeMembers": True,
            },
            "status": "success",
            "peak_centre_params": peak_center_params,
            "version": 1,
        }
        return fitprop

    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    @patch(plot_model_path + ".FittingPlotModel.create_fit_tables")
    @patch(plot_model_path + ".ADS")
    def test_update_fit(self, mock_ads, mock_create_fit_tables, mock_get_diffs):
        mock_loaded_ws_list = mock.MagicMock()
        mock_active_ws_list = mock.MagicMock()
        mock_log_ws_name = mock.MagicMock()
        mock_table_return_values = {
            "Name": ["f0.Height", "f0.PeakCentre", "f0.Sigma", "f1.Height", "f1.PeakCentre", "f1.Sigma", "Cost function value"],
            "Value": [11.0, 40000.0, 54.0, 10.0, 30000.0, 51.0, 1.0],
            "Error": [1.0, 10.0, 2.0, 1.0, 10.0, 2.0, 0.0],
        }
        func_str = "name=Gaussian,Height=11,PeakCentre=40000,Sigma=54;name=Gaussian,Height=10,PeakCentre=30000,Sigma=51"
        fitprop = self._setup_update_fit_test(mock_table_return_values, mock_ads, mock_get_diffs, func_str, ["Gaussian_PeakCentre"])
        self.model.update_fit([fitprop], mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)

        self.assertEqual(self.model._fit_results["name1"]["model"], func_str)
        self.assertEqual(
            self.model._fit_results["name1"]["results"],
            {
                "Gaussian_Height": [[11.0, 1.0], [10.0, 1.0]],
                "Gaussian_PeakCentre": [[40000.0, 10.0], [30000.0, 10.0]],
                "Gaussian_PeakCentre_dSpacing": [[4.0, 1.0e-3], [3.0, 1.0e-3]],
                "Gaussian_Sigma": [[54.0, 2.0], [51.0, 2.0]],
                "Gaussian_fwhm": [self._get_fwhm_values(func_str, 0), self._get_fwhm_values(func_str, 1)],
            },
        )
        mock_create_fit_tables.assert_called_once_with(mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)
        self.assertEqual(mock_get_diffs.call_count, 4)  # twice for each peak

    @patch(plot_model_path + ".FittingPlotModel._convert_TOFerror_to_derror")
    @patch(plot_model_path + ".FittingPlotModel._convert_TOF_to_d")
    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    @patch(plot_model_path + ".FittingPlotModel.create_fit_tables")
    @patch(plot_model_path + ".ADS")
    def test_update_fit_with_single_user_function(
        self, mock_ads, mock_create_fit_tables, mock_get_diffs, mock_conv_tof_to_d, mock_conv_tof_e
    ):
        mock_loaded_ws_list = mock.MagicMock()
        mock_active_ws_list = mock.MagicMock()
        mock_log_ws_name = mock.MagicMock()
        mock_conv_tof_to_d.return_value = 1.0
        mock_conv_tof_e.return_value = 0.5
        mock_table_return_values = {
            "Name": ["A0", "Phi", "Nu", "Cost function value"],
            "Value": [1.3, 0.01, 0.1, 1.7],
            "Error": [0.5, 0.4, 4.5, 0.0],
        }
        func_str = "name=Bessel,A0=-1.03469,Phi=-0.017227,Nu=0.100225"
        fitprop = self._setup_update_fit_test(mock_table_return_values, mock_ads, mock_get_diffs, func_str)

        self.model.update_fit([fitprop], mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)

        self.assertEqual(self.model._fit_results["name1"]["model"], func_str)
        self.assertEqual(
            self.model._fit_results["name1"]["results"],
            {"Bessel_A0": [[1.3, 0.5]], "Bessel_Phi": [[0.01, 0.4]], "Bessel_Nu": [[0.1, 4.5]]},
        )

    @patch(plot_model_path + ".FittingPlotModel._convert_TOFerror_to_derror")
    @patch(plot_model_path + ".FittingPlotModel._convert_TOF_to_d")
    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    @patch(plot_model_path + ".FittingPlotModel.create_fit_tables")
    @patch(plot_model_path + ".ADS")
    def test_update_fit_with_single_peak_function(
        self, mock_ads, mock_create_fit_tables, mock_get_diffs, mock_conv_tof_to_d, mock_conv_tof_e
    ):
        mock_loaded_ws_list = mock.MagicMock()
        mock_active_ws_list = mock.MagicMock()
        mock_log_ws_name = mock.MagicMock()
        mock_conv_tof_to_d.return_value = 1.0
        mock_conv_tof_e.return_value = 0.5

        # Below return values are set to 'nan' to show that values for calculating fwhm are taken from func_str in the fitprop
        mock_table_return_values = {
            "Name": ["Height", "Centre", "Radius", "Cost function value"],
            "Value": [nan, nan, nan, nan],
            "Error": [nan, nan, nan, nan],
        }
        mock_conv_tof_to_d.return_value = 1.0
        mock_conv_tof_e.return_value = 0.5
        func_str = "name=ElasticIsoRotDiff,Q=0.3,Height=20.2136,Centre=23666.0,Radius=0.98"
        fitprop = self._setup_update_fit_test(mock_table_return_values, mock_ads, mock_get_diffs, func_str, ["ElasticIsoRotDiff_Centre"])

        self.model.update_fit([fitprop], mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)

        self.assertEqual(self.model._fit_results["name1"]["model"], func_str)
        self.assertEqual(self.model._fit_results["name1"]["results"]["ElasticIsoRotDiff_fwhm"], [self._get_fwhm_values(func_str, 0)])

    @patch(plot_model_path + ".FittingPlotModel._convert_TOFerror_to_derror")
    @patch(plot_model_path + ".FittingPlotModel._convert_TOF_to_d")
    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    @patch(plot_model_path + ".FittingPlotModel.create_fit_tables")
    @patch(plot_model_path + ".ADS")
    def test_update_fit_with_composite_peak_and_user_function(
        self, mock_ads, mock_create_fit_tables, mock_get_diffs, mock_conv_tof_to_d, mock_conv_tof_e
    ):
        mock_loaded_ws_list = mock.MagicMock()
        mock_active_ws_list = mock.MagicMock()
        mock_log_ws_name = mock.MagicMock()
        mock_table_return_values = {
            "Name": [
                "f0.Height",
                "f0.Centre",
                "f0.Radius",
                "f1.I",
                "f1.A",
                "f1.B",
                "f1.X0",
                "f1.S",
                "f2.A0",
                "f2.Phi",
                "f2.Nu",
                "Cost function value",
            ],
            "Value": [16.0, 38613.0, 0.65, 1600.0, 0.06, 0.02, 23695.6, 14.1, 1.3, 0.01, 0.1, 1.7],
            "Error": [nan, nan, 0.0, 25.75, 0.0, 0.0, 0.65, 0.945, 0.5, 0.4, 4.5, 0.0],
        }
        mock_conv_tof_to_d.return_value = 1.0
        mock_conv_tof_e.return_value = 0.5
        func_str = (
            "name=ElasticIsoRotDiff,Q=0.3,Height=20.2136,Centre=23666.0,Radius=0.98;"
            "name=BackToBackExponential,I=1600.0,A=0.06,B=0.02,X0=23695.6,S=14.1,ties=(A=0.06,B=0.02);"
            "name=Bessel,A0=-1.03469,Phi=-0.017227,Nu=0.100225"
        )
        fitprop = self._setup_update_fit_test(
            mock_table_return_values,
            mock_ads,
            mock_get_diffs,
            func_str,
            peak_center_params=["ElasticIsoRotDiff_Centre", "BackToBackExponential_X0"],
        )
        self.model.update_fit([fitprop], mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)

        self.assertEqual(self.model._fit_results["name1"]["model"], func_str)
        self.assertEqual(
            self.model._fit_results["name1"]["results"],
            {
                "ElasticIsoRotDiff_Height": [[16.0, nan]],
                "ElasticIsoRotDiff_Centre": [[38613.0, nan]],
                "ElasticIsoRotDiff_Centre_dSpacing": [[1.0, 0.5]],
                "ElasticIsoRotDiff_Radius": [[0.65, 0.0]],
                "ElasticIsoRotDiff_fwhm": [self._get_fwhm_values(func_str, 0)],
                "BackToBackExponential_I": [[1600.0, 25.75]],
                "BackToBackExponential_A": [[0.06, 0.0]],
                "BackToBackExponential_B": [[0.02, 0.0]],
                "BackToBackExponential_X0": [[23695.6, 0.65]],
                "BackToBackExponential_X0_dSpacing": [[1.0, 0.5]],
                "BackToBackExponential_S": [[14.1, 0.945]],
                "BackToBackExponential_fwhm": [self._get_fwhm_values(func_str, 1)],
                "Bessel_A0": [[1.3, 0.5]],
                "Bessel_Phi": [[0.01, 0.4]],
                "Bessel_Nu": [[0.1, 4.5]],
            },
        )

    @patch(plot_model_path + ".FittingPlotModel._convert_TOFerror_to_derror")
    @patch(plot_model_path + ".FittingPlotModel._convert_TOF_to_d")
    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    @patch(plot_model_path + ".GroupWorkspaces")
    @patch(plot_model_path + ".CreateWorkspace")
    @patch(plot_model_path + ".ADS")
    def test_update_fit_with_func_having_fwhm_params(
        self, mock_ads, mock_create_ws, mock_group_ws, mock_get_diffs, mock_conv_tof_to_d, mock_conv_tof_e
    ):
        mock_loaded_ws_list = mock.MagicMock()
        mock_loaded_ws_list.__len__.return_value = 1
        mock_active_ws_list = mock.MagicMock()
        mock_log_ws_name = mock.MagicMock()
        mock_log_ws_name.split.return_value = "log_workspace"
        mock_table_return_values = {
            "Name": [
                "f0.Mixing",
                "f0.Intensity",
                "f0.PeakCentre",
                "f0.FWHM",
                "f1.Mixing",
                "f1.Intensity",
                "f1.PeakCentre",
                "f1.FWHM",
                "Cost function value",
            ],
            "Value": [0.999, 101.3, 25074.1, 98.7982, 1.0, 0.834, 25074.1, 0.939437, 2.2599485221142213],
            "Error": [0.0338, 2.0421, 0.7936, 1.7157, 2.23e-05, nan, nan, nan, 0.0],
        }
        mock_conv_tof_to_d.return_value = 1.0
        mock_conv_tof_e.return_value = 0.5
        func_str = (
            "name=PseudoVoigt,Mixing=0.999,Intensity=101.3,PeakCentre=25074.1,FWHM=98.7982;"
            "name=PseudoVoigt,Mixing=1,Intensity=0.834,PeakCentre=25074.1,FWHM=0.939437"
        )
        fitprop = self._setup_update_fit_test(
            mock_table_return_values,
            mock_ads,
            mock_get_diffs,
            func_str,
            peak_center_params=["PseudoVoigt_PeakCentre", "PseudoVoigt_PeakCentre"],
        )
        self.model.update_fit([fitprop], mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)

        self.assertEqual(self.model._fit_results["name1"]["model"], func_str)
        self.assertEqual(
            self.model._fit_results["name1"]["results"],
            {
                "PseudoVoigt_Mixing": [[0.999, 0.0338], [1.0, 2.23e-05]],
                "PseudoVoigt_Intensity": [[101.3, 2.0421], [0.834, nan]],
                "PseudoVoigt_PeakCentre": [[25074.1, 0.7936], [25074.1, nan]],
                "PseudoVoigt_PeakCentre_dSpacing": [[1.0, 0.5], [1.0, 0.5]],
                "PseudoVoigt_FWHM": [[98.7982, 1.7157], [0.939437, nan]],
            },
        )
        mock_create_ws.assert_has_calls(
            calls=[
                call(OutputWorkspace="PseudoVoigt_Mixing", DataX=[1, 2], DataY=[1, 2], NSpec=1),
                call(OutputWorkspace="PseudoVoigt_PeakCentre_dSpacing", DataX=[1, 2], DataY=[1, 2], NSpec=1),
                call(OutputWorkspace="PseudoVoigt_PeakCentre", DataX=[1, 2], DataY=[1, 2], NSpec=1),
                call(OutputWorkspace="PseudoVoigt_Intensity", DataX=[1, 2], DataY=[1, 2], NSpec=1),
                call(OutputWorkspace="PseudoVoigt_FWHM", DataX=[1, 2], DataY=[1, 2], NSpec=1),
            ],
            any_order=True,
        )
        mock_group_ws.assert_called_once()

    @patch("mantid.api.FunctionFactoryImpl.createPeakFunction")
    @patch(plot_model_path + ".FittingPlotModel._convert_TOFerror_to_derror")
    @patch(plot_model_path + ".FittingPlotModel._convert_TOF_to_d")
    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    @patch(plot_model_path + ".FittingPlotModel.create_fit_tables")
    @patch(plot_model_path + ".ADS")
    def test_update_fit_fwhm_independent_from_params_table(
        self, mock_ads, mock_create_fit_tables, mock_get_diffs, mock_conv_tof_to_d, mock_conv_tof_e, mock_create_peak_func
    ):
        mock_peak_func = mock.MagicMock()
        mock_create_peak_func.return_value = mock_peak_func
        mock_loaded_ws_list = mock.MagicMock()
        mock_active_ws_list = mock.MagicMock()
        mock_log_ws_name = mock.MagicMock()
        mock_table_return_values = {
            "Name": [
                "Height",
                "Centre",
                "Radius",
                "Cost function value",
            ],
            "Value": [nan] * 4,
            "Error": [nan] * 4,
        }
        mock_conv_tof_to_d.return_value = 1.0
        mock_conv_tof_e.return_value = 0.5
        func_str = "name=ElasticIsoRotDiff,Q=0.3,Height=20.2136,Centre=23666.0,Radius=0.98"
        fitprop = self._setup_update_fit_test(
            mock_table_return_values,
            mock_ads,
            mock_get_diffs,
            func_str,
            peak_center_params=["ElasticIsoRotDiff_Centre"],
        )
        self.model.update_fit([fitprop], mock_loaded_ws_list, mock_active_ws_list, mock_log_ws_name)

        # Q is a non-fitting parameter. So not expecting a call for that
        mock_peak_func.setParameter.assert_has_calls(calls=[call(0, 20.2136), call(1, 23666.0), call(2, 0.98)])
        mock_peak_func.fwhm.assert_called_once()

    def setup_test_create_fit_tables(self, mock_create_ws, mock_create_table, mock_groupws):
        loaded_ws_list = ["name1", "name2"]
        active_ws_list = ["name1", "name2_bgsub"]
        log_workspaces_name = "some_log"

        mock_ws_list = [mock.MagicMock(), mock.MagicMock(), mock.MagicMock(), mock.MagicMock()]
        mock_create_ws.side_effect = mock_ws_list
        mock_create_table.return_value = mock.MagicMock()
        mock_groupws.side_effect = lambda wslist, OutputWorkspace: wslist
        # setup fit results
        # self.model._data_workspaces.add("name1", loaded_ws=self.mock_ws)
        # self.model._data_workspaces.add(
        #     "name2", loaded_ws=self.mock_ws, bgsub_ws=self.mock_ws, bgsub_ws_name="name2_bgsub", bg_params=[True]
        # )
        # self.model._log_workspaces = mock.MagicMock()
        # self.model._log_workspaces.name.return_value = "some_log"

        func_str = "name=Gaussian,Height=11,PeakCentre=40000,Sigma=54;name=Gaussian,Height=10,PeakCentre=30000,Sigma=51"
        self.model._fit_results = dict()
        self.model._fit_results["name1"] = {
            "model": func_str,
            "status": "success",
            "results": {
                "Gaussian_Height": [[11.0, 1.0], [10.0, 1.0]],
                "Gaussian_PeakCentre": [[40000.0, 10.0], [30000.0, 10.0]],
                "Gaussian_PeakCentre_dSpacing": [[4.0, 1.0e-3], [3.0, 1.0e-3]],
                "Gaussian_Sigma": [[54.0, 2.0], [51.0, 2.0]],
            },
            "costFunction": 1.0,
        }
        return mock_ws_list, mock_create_table, mock_create_ws, loaded_ws_list, active_ws_list, log_workspaces_name

    @patch(plot_model_path + ".write_table_row")
    @patch(plot_model_path + ".GroupWorkspaces")
    @patch(plot_model_path + ".CreateEmptyTableWorkspace")
    @patch(plot_model_path + ".CreateWorkspace")
    def test_create_fit_tables(self, mock_create_ws, mock_create_table, mock_groupws, mock_writerow):
        (
            mock_ws_list,
            mock_create_table,
            mock_create_ws,
            loaded_ws_list,
            active_ws_list,
            log_workspaces_name,
        ) = self.setup_test_create_fit_tables(mock_create_ws, mock_create_table, mock_groupws)
        self.model.create_fit_tables(loaded_ws_list, active_ws_list, log_workspaces_name)

        # test the table stores the correct function strings (empty string if no function present)
        mock_writerow.assert_any_call(
            mock_create_table.return_value,
            [
                "name1",
                self.model._fit_results["name1"]["costFunction"],
                self.model._fit_results["name1"]["status"],
                self.model._fit_results["name1"]["model"],
            ],
            0,
        )
        mock_writerow.assert_any_call(mock_create_table.return_value, ["", nan, ""], 1)  # name2 has no entry
        # check the matrix workspaces corresponding to the fit parameters
        # Gaussian has 3 params plus centre converted to dSpacing
        ws_names = [mock_create_ws.mock_calls[iws][2]["OutputWorkspace"] for iws in range(0, 4)]
        self.assertEqual(sorted(ws_names), sorted(self.model._fit_results["name1"]["results"].keys()))
        # check the first call to setY and setE for one of the parameters
        for im, m in enumerate(self.model._fit_workspaces[:-2]):
            for iws, ws in enumerate(loaded_ws_list):
                _, argsY, _ = m.setY.mock_calls[iws]
                _, argsE, _ = m.setE.mock_calls[iws]
                self.assertEqual([argsY[0], argsE[0]], [iws, iws])
                if ws in self.model._fit_results:
                    self.assertTrue(all(argsY[1] == [x[0] for x in self.model._fit_results["name1"]["results"][ws_names[im]]]))
                    self.assertTrue(all(argsE[1] == [x[1] for x in self.model._fit_results["name1"]["results"][ws_names[im]]]))
                else:
                    self.assertTrue(all(isnan(argsY[1])))
                    self.assertTrue(all(isnan(argsE[1])))

    @patch(plot_model_path + ".write_table_row")
    @patch(plot_model_path + ".GroupWorkspaces")
    @patch(plot_model_path + ".CreateEmptyTableWorkspace")
    @patch(plot_model_path + ".CreateWorkspace")
    def test_create_fit_tables_different_funcs(self, mock_create_ws, mock_create_table, mock_groupws, mock_writerow):
        (
            mock_ws_list,
            mock_create_table,
            mock_create_ws,
            loaded_ws_list,
            active_ws_list,
            log_workspaces_name,
        ) = self.setup_test_create_fit_tables(mock_create_ws, mock_create_table, mock_groupws)
        mock_ws_list.append(mock.MagicMock())  # adding an additional parameter into model for name2
        func_str2 = self.model._fit_results["name1"]["model"] + ";name=FlatBackground,A0=1"
        self.model._fit_results["name2_bgsub"] = {
            "model": func_str2,
            "status": "success",
            "results": dict(self.model._fit_results["name1"]["results"], FlatBackground_A0=[[1.0, 0.1]]),
            "costFunction": 2.0,
        }
        self.model.create_fit_tables(loaded_ws_list, active_ws_list, log_workspaces_name)

        # test the workspaces were created and added to fit_workspaces
        self.assertEqual(self.model._fit_workspaces, mock_ws_list + [mock_create_table.return_value])
        # test the table stores the correct function strings (empty string if no function present)
        mock_writerow.assert_any_call(
            mock_create_table.return_value,
            [
                "name1",
                self.model._fit_results["name1"]["costFunction"],
                self.model._fit_results["name1"]["status"],
                self.model._fit_results["name1"]["model"],
            ],
            0,
        )
        mock_writerow.assert_any_call(
            mock_create_table.return_value,
            [
                "name2_bgsub",
                self.model._fit_results["name2_bgsub"]["costFunction"],
                self.model._fit_results["name1"]["status"],
                self.model._fit_results["name2_bgsub"]["model"],
            ],
            1,
        )
        # check the matrix workspaces corresponding to the fit parameters
        # 4 unique params plus the peak centre converted to dSpacing
        ws_names = [mock_create_ws.mock_calls[iws][2]["OutputWorkspace"] for iws in range(0, 5)]
        # get list of all unique params across both models
        param_names = list(
            set(list(self.model._fit_results["name1"]["results"].keys()) + list(self.model._fit_results["name2_bgsub"]["results"].keys()))
        )
        # test only table for unique parameter
        self.assertEqual(sorted(ws_names), sorted(param_names))

    @patch(plot_model_path + ".FittingPlotModel._get_diff_constants")
    def test_convert_centres_and_error_from_TOF_to_d(self, mock_get_diffs):
        params = UnitParametersMap()
        params[UnitParams.difc] = 18000
        mock_get_diffs.return_value = params
        tof = 40000
        tof_error = 5
        d = self.model._convert_TOF_to_d(tof, "ws_name")
        d_error = self.model._convert_TOFerror_to_derror(tof_error, d, "ws_name")

        self.assertAlmostEqual(tof / d, 18000, delta=1e-10)
        self.assertAlmostEqual(d_error / d, tof_error / tof, delta=1e-10)

    def _get_sample_findpeaksconvolve_group_ws(self):
        peak_centers = CreateEmptyTableWorkspace(OutputWorkspace="PeakCentre")
        peak_centers.addColumn("int", "SpecIndex")
        peak_centers.addColumn("double", "PeakCentre_0")
        peak_centers.addColumn("double", "PeakCentre_1")
        peak_centers.addColumn("double", "PeakCentre_2")
        peak_centers.addRow([1, 0.09, 0.15, 0.30])

        peak_y = CreateEmptyTableWorkspace(OutputWorkspace="PeakYPosition")
        peak_y.addColumn("int", "SpecIndex")
        peak_y.addColumn("double", "PeakYPosition_0")
        peak_y.addColumn("double", "PeakYPosition_1")
        peak_y.addColumn("double", "PeakYPosition_2")
        peak_y.addRow([1, 798, 800, 984])

        i_over_sigma = CreateEmptyTableWorkspace(OutputWorkspace="PeakIOverSigma")
        i_over_sigma.addColumn("int", "SpecIndex")
        i_over_sigma.addColumn("double", "PeakIOverSigma_0")
        i_over_sigma.addColumn("double", "PeakIOverSigma_1")
        i_over_sigma.addColumn("double", "PeakIOverSigma_2")
        i_over_sigma.addRow([1, 12.5, 13.2, 15.2])
        return GroupWorkspaces([peak_centers, peak_y, i_over_sigma], OutputWorkspace="test")

    @mock.patch(plot_model_path + ".FunctionWrapper")
    @mock.patch(plot_model_path + ".FunctionFactory")
    @mock.patch(plot_model_path + ".FindPeaksConvolve")
    @mock.patch(plot_model_path + ".logger.error")
    @mock.patch(plot_model_path + ".ADS")
    def test_run_find_peaks_convolve_existing_ws_1(self, mock_ads, mock_error, mock_find_peaks_convolve, mock_func_fac, mock_func_wrapper):
        mock_ads.doesExist.return_value = True
        mock_ws = mock.MagicMock()
        mock_ads.retrieve.return_value = mock_ws
        mock_find_peaks_convolve.return_value = self._get_sample_findpeaksconvolve_group_ws()
        mock_peak_func = mock.MagicMock()
        mock_func_fac.Instance().createPeakFunction.return_value = mock_peak_func

        ret_str = self.model.run_find_peaks_convolve("ws_1", "BackToBackExponential", (0.08, 0.10))
        self.assertNotEqual(ret_str, None)
        mock_peak_func.setCentre.assert_called_once_with(0.09)
        mock_peak_func.setHeight.assert_called_once_with(798)
        mock_peak_func.setMatrixWorkspace.assert_called_once_with(mock_ws, 0, 0, 0)
        mock_error.assert_not_called()

    @mock.patch(plot_model_path + ".FunctionWrapper")
    @mock.patch(plot_model_path + ".FunctionFactory")
    @mock.patch(plot_model_path + ".FindPeaksConvolve")
    @mock.patch(plot_model_path + ".logger.error")
    @mock.patch(plot_model_path + ".ADS")
    def test_run_find_peaks_convolve_existing_ws_2(self, mock_ads, mock_error, mock_find_peaks_convolve, mock_func_fac, mock_func_wrapper):
        mock_ads.doesExist.return_value = True
        mock_ws = mock.MagicMock()
        mock_ads.retrieve.return_value = mock_ws
        mock_find_peaks_convolve.return_value = self._get_sample_findpeaksconvolve_group_ws()
        mock_peak_func = mock.MagicMock()
        mock_func_fac.Instance().createPeakFunction.return_value = mock_peak_func

        ret_str = self.model.run_find_peaks_convolve("ws_1", "BackToBackExponential", (0.15, 0.30))
        self.assertNotEqual(ret_str, None)
        mock_peak_func.setCentre.assert_has_calls([call(0.15), call(0.30)])
        mock_peak_func.setHeight.assert_has_calls([call(800), call(984)])
        mock_peak_func.setMatrixWorkspace.assert_has_calls([call(mock_ws, 0, 0, 0), call(mock_ws, 0, 0, 0)])
        mock_error.assert_not_called()

    @mock.patch(plot_model_path + ".logger.error")
    @mock.patch(plot_model_path + ".ADS")
    def test_run_find_peaks_convolve_non_existing_ws(self, mock_ads, mock_error):
        mock_ads.doesExist.return_value = False
        ret_str = self.model.run_find_peaks_convolve("non_existing_ws", "BackToBackExponential", (10, 20))
        self.assertEqual(ret_str, None)
        mock_error.assert_called()


if __name__ == "__main__":
    unittest.main()
