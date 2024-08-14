# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import Load, CreateWorkspace, CompareWorkspaces, GroupWorkspaces
import numpy as np
from mantid import AnalysisDataService
from mantid.utils.pip import package_installed
from BayesStretch2 import BayesStretch2

from unittest import mock


if package_installed("quickBayes"):
    from quickBayes.functions.qse_fixed import QSEFixFunction
    from quickBayes.workflow.qse_search import QSEGridSearch

    SAMPLE_NAME = "__BayesStretchTest_Sample"
    RES_NAME = "__BayesStretchTest_Resolution"

    def add_log_mock(workspace, sample_logs, data_ws):
        return workspace

    class BayesStretch2Test(unittest.TestCase):
        """
        These tests are for checking the quickBayes
        lib is used correctly. The results from the
        lib are assumed to be correct (its fully tested).

        Going to test each method in isolation.
        """

        @classmethod
        def setUpClass(cls):
            cls._res_ws = Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace=RES_NAME)
            cls._sample_ws = Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace=SAMPLE_NAME)
            cls._N_hist = 1

        def setUp(self):
            self._alg = BayesStretch2()
            self._alg.initialize()

        @classmethod
        def tearDownClass(cls):
            """
            Remove workspaces from ADS.
            """
            AnalysisDataService.clear()

        def assert_mock_called_with(self, mock_object, N_calls, call_number, **kargs):
            self.assertEqual(N_calls, mock_object.call_count)

            called_with = mock_object.call_args_list[call_number - 1][1]  # dict of {keyword: value}
            self.assertEqual(len(kargs), len(called_with))
            for j, keyword in enumerate(kargs):
                expected = kargs[keyword]
                if isinstance(expected, np.ndarray):
                    np.testing.assert_array_equal(expected, called_with[keyword])
                else:
                    self.assertEqual(expected, called_with[keyword])

        def point_mock(self, name):
            if "res" in name:
                return self._res_ws, self._N_hist
            else:
                return self._sample_ws, self._N_hist

        def test_make_contour(self):
            x = np.linspace(0.01, 0.1, 3)
            y = np.linspace(0.8, 1.0, 3)
            X, Y = np.meshgrid(x, y)

            Z = np.sin(X) - np.cos(Y)

            ws_str = self._alg.make_contour(X, Y, Z, 0, "test")
            ws = AnalysisDataService.retrieve(ws_str)
            expect = [[-0.687, -0.642, -0.597], [-0.612, -0.567, -0.522], [-0.530, -0.485, -0.440]]

            self.assertEqual(ws.getNumberHistograms(), 3)
            for j in range(ws.getNumberHistograms()):
                np.testing.assert_almost_equal(ws.readX(j), x, 3)
                np.testing.assert_almost_equal(ws.readY(j), expect[j], 3)

            label = ["beta", "FWHM"]
            axis_values = [x, y]
            for i in range(ws.axes()):
                axis = ws.getAxis(i)
                unit = axis.getUnit()
                self.assertEqual(unit.caption(), label[i])
                np.testing.assert_equal(axis.extractValues(), axis_values[i])

        def test_make_slice_ws(self):
            x = np.linspace(0.8, 1.0, 3)
            y1 = -((x - 0.9) ** 2) + 0.01
            y2 = -((x - 0.86) ** 2) + 0.01

            Q = [0.4, 0.8]
            slices = [(x, y1), (x, y2)]

            ws_str = self._alg.make_slice_ws(slices, Q, "MomentumTransfer", "test")

            ws = AnalysisDataService.retrieve(ws_str)
            expect = [[0, 1e-2, 0.0], [0.006, 0.008, -0.01]]

            self.assertEqual(ws.getNumberHistograms(), 2)
            for j in range(ws.getNumberHistograms()):
                np.testing.assert_almost_equal(ws.readX(j), x, 3)
                np.testing.assert_almost_equal(ws.readY(j), expect[j], 3)

            label = ["q", ""]
            axis_values = [x, [str(val) for val in Q]]
            for i in range(ws.axes()):
                axis = ws.getAxis(i)
                unit = axis.getUnit()
                self.assertEqual(unit.caption(), label[i])
                np.testing.assert_equal(axis.extractValues(), axis_values[i])

        def test_make_results(self):
            def slice(slice_list, x_data, x_unit, name):
                return name

            def set_label(ws_str, label, unit):
                return ws_str

            self._alg.make_slice_ws = mock.Mock(side_effect=slice)
            self._alg.group_ws = mock.Mock(return_value="group")
            self._alg.set_label = mock.Mock(side_effect=set_label)

            beta_list = [(1, 2), (3, 4)]
            FWHM_list = [(5, 6), (7, 8)]
            x_data = [9]

            self._alg.make_results(beta_list, FWHM_list, x_data, "MomentumTransfer", "test")
            self._alg.group_ws.assert_called_once_with(["test_Stretch_Beta", "test_Stretch_FWHM"], "test")

            self.assert_mock_called_with(
                self._alg.make_slice_ws,
                N_calls=2,
                call_number=1,
                slice_list=beta_list,
                x_data=x_data,
                x_unit="MomentumTransfer",
                name="test_Stretch_Beta",
            )
            self.assert_mock_called_with(
                self._alg.make_slice_ws,
                N_calls=2,
                call_number=2,
                slice_list=FWHM_list,
                x_data=x_data,
                x_unit="FWHM",
                name="test_Stretch_FWHM",
            )
            self.assert_mock_called_with(self._alg.set_label, N_calls=2, call_number=2, ws_str="test_Stretch_Beta", label="beta", unit="")
            self.assert_mock_called_with(self._alg.set_label, N_calls=2, call_number=1, ws_str="test_Stretch_FWHM", label="FWHM", unit="eV")

        def test_do_one_spec(self):
            data = {
                "sample": self._sample_ws,
                "start x": -0.3,
                "end x": 0.3,
                "res_list": [self._res_ws],
                "beta start": 0.8,
                "beta end": 1.0,
                "N_beta": 10,
                "FWHM start": 0.01,
                "FWHM end": 0.2,
                "N_FWHM": 20,
                "BG": "Linear",
                "elastic": True,
                "name": "test",
            }

            self._alg.QSEGridSearch = mock.Mock()
            self._alg.QSEFixFunction = mock.Mock()

            method_mock = mock.MagicMock(autospec=QSEGridSearch)
            method_mock.preprocess_data.return_value = ([7, 8, 9], [4, 5, 6])
            method_mock.execute.return_value = ([1, 2, 3], [4, 5, 6])
            method_mock.get_grid = mock.PropertyMock(return_value=[[1, 2, 3], [4, 5, 6], [7, 8, 9]])
            method_mock.get_x_axis.values = mock.PropertyMock(return_value=[0.8, 0.9, 1.0])
            method_mock.get_y_axis.values = mock.PropertyMock(return_value=[0.01, 0.05, 0.1])
            method_mock.get_slices.return_value = ([3, 2, 1], [6, 5, 4])
            self._alg.QSEGridSearch.return_value = method_mock

            self._alg.make_contour = mock.Mock(return_value="contour_ws")

            mock_function = mock.MagicMock(autospec=QSEFixFunction)
            mock_function.get_guess.return_value = [11, 12]
            mock_function.get_bounds.return_value = ([1, 2], [21, 22])
            self._alg.QSEFixFunction.return_value = mock_function

            contour, beta, FWHM = self._alg.do_one_spec(0, data)

            self.assert_mock_called_with(
                method_mock.preprocess_data,
                N_calls=1,
                call_number=1,
                x_data=self._sample_ws.readX(0),
                y_data=self._sample_ws.readY(0),
                e_data=self._sample_ws.readE(0),
                start_x=-0.3,
                end_x=0.3,
                res=self._res_ws,
            )

            self.assert_mock_called_with(method_mock.set_x_axis, N_calls=1, call_number=1, start=0.8, end=1.0, N=10, label="beta")

            self.assert_mock_called_with(method_mock.set_y_axis, N_calls=1, call_number=1, start=0.01, end=0.2, N=20, label="FWHM")

            self._alg.QSEFixFunction.assert_called_once_with(
                bg_function="Linear", elastic_peak=True, r_x=[7, 8, 9], r_y=[4, 5, 6], start_x=-0.3, end_x=0.3
            )

            mock_function.add_single_SE.assert_called_once_with()
            mock_function.set_delta_bounds.assert_called_once_with(lower=[0, -0.5], upper=[200, 0.5])

            method_mock.set_scipy_engine(guess=[11, 12], lower=[1, 2], upper=[21, 22])
            method_mock.execute.assert_called_once_with(func=mock_function)
            method_mock.get_slices.assert_called_once_with()

            self.assertEqual(contour, "contour_ws")
            self.assertEqual(beta, (method_mock.get_x_axis.values, [3, 2, 1]))
            self.assertEqual(FWHM, (method_mock.get_y_axis.values, [6, 5, 4]))

        def exec_setup(self, fit_ws, results):
            self._alg.setProperty("Emax", 0.3)
            self._alg.setProperty("Emin", -0.3)
            self._alg.setProperty("Elastic", True)
            self._alg.setProperty("Background", "Linear")
            self._alg.setProperty("SampleWorkspace", self._sample_ws.name())
            self._alg.setProperty("ResolutionWorkspace", self._res_ws)
            self._alg.setProperty("OUTPUTWORKSPACEFIT", "out")
            self._alg.setProperty("OutputWorkspaceContour", "contour")

            self._alg.point_data = mock.Mock(side_effect=self.point_mock)
            self._alg.duplicate_res = mock.Mock(return_value=[1, 1, 1])
            self._alg.unique_res = mock.Mock(return_value=[2, 1, 3])
            self._alg.calculate = mock.Mock(return_value=(["ws"], [1], [2], [("log", "sample")]))
            self._alg.group_ws = mock.Mock(return_value=fit_ws)
            self._alg.add_sample_logs = mock.Mock(side_effect=add_log_mock)
            self._alg.make_results = mock.Mock(return_value=results.name())

        @mock.patch("BayesStretch2.get_two_theta_and_q")
        @mock.patch("BayesStretch2.Progress")
        def test_pyexec_QL_unique(self, prog_mock, get_Q_mock):
            progress_mock = mock.Mock()
            prog_mock.return_value = progress_mock

            tmp = CreateWorkspace([1, 2], [3, 4])
            fit_ws = GroupWorkspaces([tmp])
            results = CreateWorkspace([5, 6], [7, 8])
            results2 = GroupWorkspaces([results])
            self.exec_setup(fit_ws, results2)

            get_Q_mock.return_value = [1, 2, 3]
            self._N_hist = 2

            self._alg.execute()

            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=1, name="__BayesStretchTest_Sample")
            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=2, name="__BayesStretchTest_Resolution")
            self.assertEqual(self._alg.duplicate_res.call_count, 0)
            self.assertEqual(self._alg.unique_res.call_count, 1)

            # calculate
            self._alg.calculate.assert_called_once_with(sample_ws=self._sample_ws, report_progress=progress_mock, res_list=[2, 1, 3], N=2)
            # add sample logs
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=1,
                workspace=fit_ws,
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution")],
                data_ws=self._sample_ws,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=2,
                workspace=results2.name(),
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution")],
                data_ws=self._sample_ws,
            )
            # make results
            self._alg.make_results.assert_called_once_with(beta_list=[1], FWHM_list=[2], x_data=2, x_unit="MomentumTransfer", name="out")

        @mock.patch("BayesStretch2.get_two_theta_and_q")
        @mock.patch("BayesStretch2.Progress")
        def test_pyexec_QL_duplicate(self, prog_mock, get_Q_mock):
            progress_mock = mock.Mock()
            prog_mock.return_value = progress_mock

            tmp = CreateWorkspace([1, 2], [3, 4])
            fit_ws = GroupWorkspaces([tmp])
            results = CreateWorkspace([5, 6], [7, 8])
            results2 = GroupWorkspaces([results])
            self.exec_setup(fit_ws, results2)

            get_Q_mock.return_value = [1, 2, 3]
            self._N_hist = 1

            self._alg.execute()

            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=1, name="__BayesStretchTest_Sample")
            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=2, name="__BayesStretchTest_Resolution")
            self.assertEqual(self._alg.duplicate_res.call_count, 1)
            self.assertEqual(self._alg.unique_res.call_count, 0)

            # calculate
            self._alg.calculate.assert_called_once_with(sample_ws=self._sample_ws, report_progress=progress_mock, res_list=[1, 1, 1], N=1)
            # add sample logs
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=1,
                workspace=fit_ws,
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution")],
                data_ws=self._sample_ws,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=2,
                workspace=results2.name(),
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution")],
                data_ws=self._sample_ws,
            )
            # make results
            self._alg.make_results.assert_called_once_with(beta_list=[1], FWHM_list=[2], x_data=2, x_unit="MomentumTransfer", name="out")

    if __name__ == "__main__":
        unittest.main()
