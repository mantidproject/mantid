# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import Load, DeleteWorkspace, CreateWorkspace, CompareWorkspaces, GroupWorkspaces
import numpy as np
from mantid.utils.pip import package_installed
from BayesQuasi2 import BayesQuasi2

from unittest import mock


if package_installed("quickBayes"):
    from quickBayes.fitting.fit_engine import FitEngine
    from quickBayes.workflow.QlData import QLData
    from quickBayes.functions.qldata_function import QlDataFunction

    SAMPLE_NAME = "__BayesStretchTest_Sample"
    RES_NAME = "__BayesStretchTest_Resolution"

    def add_log_mock(workspace, sample_logs, data_ws):
        return workspace

    class BayesQuasi2Test(unittest.TestCase):
        """
        These tests are for checking the quickBayes
        lib is used correctly. The results from the
        lib are assumed to be correct (its fully tested).

        Going to test each method in isolation.
        """

        def setUp(self):
            self._res_ws = Load(Filename="irs26173_graphite002_res.nxs", OutputWorkspace=RES_NAME)
            self._sample_ws = Load(Filename="irs26176_graphite002_red.nxs", OutputWorkspace=SAMPLE_NAME)
            self._alg = BayesQuasi2()
            self._alg.initialize()
            self._N_hist = 1

        def tearDown(self):
            """
            Remove workspaces from ADS.
            """
            DeleteWorkspace(self._sample_ws)
            DeleteWorkspace(self._res_ws)

        def point_mock(self, name):
            if "res" in name:
                return self._res_ws, self._N_hist
            else:
                return self._sample_ws, self._N_hist

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

        def test_make_fit_ws(self):
            ws = CreateWorkspace([1, 2], [3, 4])
            ws_list = [self._sample_ws, self._res_ws]
            engine = mock.MagicMock(autospec=FitEngine, return_value=ws)
            self._alg.create_ws = mock.Mock()
            engine._x_data = [1, 2, 3]
            engine._y_data = [4, 5, 6]
            engine._e_data = [0.2, 0.1, 0.1]
            engine.get_fit_values.return_value = [1, 2.1, 3], [4.1, 4.9, 6], [0.1, 0.1, 0.1], [0.1, -0.1, 0], [0, 0, 0]

            output = self._alg.make_fit_ws(engine, 2, ws_list, "unit", "test")
            CompareWorkspaces(Workspace1=ws, Workspace2=output[0], CheckAllData=True)

            self._alg.create_ws.assert_called_once()
            self.assert_mock_called_with(
                self._alg.create_ws,
                N_calls=1,
                call_number=1,
                OutputWorkspace="test_workspace",
                DataX=np.array([1.0, 2.0, 3.0, 1.0, 2.1, 3.0, 1.0, 2.1, 3.0, 1.0, 2.1, 3.0, 1.0, 2.1, 3.0]),
                DataY=np.array([4, 5, 6, 4.1, 4.9, 6, 0.1, -0.1, 0, 4.1, 4.9, 6, 0.1, -0.1, 0]),
                NSpec=5,
                UnitX="unit",
                YUnitLabel="",
                VerticalAxisUnit="Text",
                VerticalAxisValues=["data", "fit 1", "diff 1", "fit 2", "diff 2"],
                DataE=np.array([0.2, 0.1, 0.1, 0.1, 0.1, 0.1, 0.0, 0.0, 0.0, 0.1, 0.1, 0.1, 0.0, 0.0, 0.0]),
            )
            DeleteWorkspace(ws)
            DeleteWorkspace("compare_msgs")

        def test_add_to_make_fit_ws(self):
            ws = CreateWorkspace([1, 2], [3, 4])
            ws_list = [self._sample_ws, self._res_ws]
            engine = mock.MagicMock(autospec=FitEngine, return_value=ws)
            self._alg.create_ws = mock.Mock()
            engine._x_data = [1, 2, 3]
            engine._y_data = [4, 5, 6]
            engine._e_data = [0.2, 0.1, 0.1]
            engine.get_fit_values.return_value = [1, 2.1, 3], [4.1, 4.9, 6], [0.1, 0.1, 0.1], [0.1, -0.1, 0], [0, 0, 0]

            output = self._alg.make_fit_ws(engine, 2, ws_list, "unit", "test")

            ws2 = CreateWorkspace([4, 2], [1, 3])
            self._alg.create_ws.return_value = ws2
            output = self._alg.make_fit_ws(engine, 2, ws_list, "unit", "test")

            CompareWorkspaces(Workspace1=ws, Workspace2=output[0], CheckAllData=True)
            CompareWorkspaces(Workspace1=ws2, Workspace2=output[1], CheckAllData=True)

            DeleteWorkspace(ws)
            DeleteWorkspace("compare_msgs")

        def test_make_results(self):
            results = {"a": [1, 2], "b": [2, 3], "c": [3, 4], "loglikelihood": [-1, -2]}
            errors = {"a": [0.1, 0.2], "b": [0.2, 0.1], "c": [0.1, 0.2]}
            x_data = [11, 12]
            self._alg.create_ws = mock.Mock()
            result_name, prob_name = self._alg.make_results(results, errors, x_data, "unit", 2, "data", "prob")

            self.assertEqual(result_name, "data")
            self.assertEqual(prob_name, "prob")

            self.assert_mock_called_with(
                self._alg.create_ws,
                N_calls=2,
                call_number=1,
                OutputWorkspace="data",
                DataX=np.array([11, 12]),
                DataY=np.array([[1, 2], [2, 3], [3, 4]]),
                NSpec=3,
                UnitX="unit",
                YUnitLabel="",
                VerticalAxisUnit="Text",
                VerticalAxisValues=["a", "b", "c"],
                DataE=np.array([[0.1, 0.2], [0.2, 0.1], [0.1, 0.2]]),
            )

            self.assert_mock_called_with(
                self._alg.create_ws,
                N_calls=2,
                call_number=2,
                OutputWorkspace="prob",
                DataX=np.array([11, 12]),
                DataY=np.array([-1, -2]),
                NSpec=2,
                UnitX="unit",
                YUnitLabel="",
                VerticalAxisUnit="Text",
                VerticalAxisValues=["1 feature(s)", "2 feature(s)"],
            )

        def test_calculate(self):
            func_mock = mock.Mock()
            engine_mock = mock.Mock()

            self._alg.get_background_function = mock.Mock(return_value=func_mock)

            self._alg.setProperty("Emax", 0.3)
            self._alg.setProperty("Emin", -0.3)
            self._alg.setProperty("Elastic", True)
            self._alg.setProperty("Background", "Linear")
            self._alg.setProperty("SampleWorkspace", self._sample_ws)

            self._alg.make_fit_ws = mock.Mock(return_value=["unit", "test"])  # use side effect to change outputs

            method = mock.MagicMock(autospec=QLData)
            method.preprocess_data.return_value = [1, 2], [3, 4]
            result_mock = mock.PropertyMock(return_value=({"a": 1}, {"b": 2}))
            type(method).get_parameters_and_errors = result_mock
            type(method).fit_engine = mock.PropertyMock(return_value=engine_mock)
            mock_method = mock.Mock(return_value=method)

            function = mock.MagicMock(autospec=QlDataFunction)
            function.get_bounds.return_value = [-1, -2], [10, 20]
            function.read_from_report.return_value = [6, 7]
            function.get_guess.return_value = [0, 4]
            mock_function = mock.Mock(return_value=function)

            ws_list, results, errors, logs = self._alg.calculate(
                self._sample_ws, mock.Mock(), [self._res_ws, self._sample_ws], 2, 3, mock_method, mock_function
            )
            self.assertEqual(2, len(ws_list))
            self.assertEqual("unit", ws_list[0])
            self.assertEqual("test", ws_list[1])

            self.assertEqual({"a": 1}, results)
            self.assertEqual({"b": 2}, errors)

            self.assertEqual(4, len(logs))
            self.assertEqual("background", logs[0][0])
            self.assertEqual("Linear", logs[0][1])
            self.assertEqual("elastic_peak", logs[1][0])
            self.assertEqual(True, logs[1][1])
            self.assertEqual("energy_min", logs[2][0])
            self.assertEqual(-0.3, logs[2][1])
            self.assertEqual("energy_max", logs[3][0])
            self.assertEqual(0.3, logs[3][1])

            self._alg.get_background_function.assert_called_once_with("Linear")
            self.assertEqual(mock_method.call_count, 2)

            def get_ws(spec):
                return {"x": self._sample_ws.readX(spec), "y": self._sample_ws.readY(spec), "e": self._sample_ws.readE(spec)}

            sample = get_ws(0)
            self.assert_mock_called_with(
                method.preprocess_data,
                N_calls=2,
                call_number=1,
                x_data=sample["x"],
                y_data=sample["y"],
                e_data=sample["e"],
                start_x=-0.3,
                end_x=0.3,
                res=self._res_ws,
            )
            sample = get_ws(1)
            self.assert_mock_called_with(
                method.preprocess_data,
                N_calls=2,
                call_number=2,
                x_data=sample["x"],
                y_data=sample["y"],
                e_data=sample["e"],
                start_x=-0.3,
                end_x=0.3,
                res=self._sample_ws,
            )

            self.assert_mock_called_with(
                mock_function,
                N_calls=2,
                call_number=1,
                bg_function=func_mock,
                elastic_peak=True,
                r_x=[1, 2],
                r_y=[3, 4],
                start_x=-0.3,
                end_x=0.3,
            )
            self.assert_mock_called_with(
                mock_function,
                N_calls=2,
                call_number=2,
                bg_function=func_mock,
                elastic_peak=True,
                r_x=[1, 2],
                r_y=[3, 4],
                start_x=-0.3,
                end_x=0.3,
            )

            self.assertEqual(function.get_bounds.call_count, 2)
            self.assert_mock_called_with(method.set_scipy_engine, N_calls=2, call_number=1, guess=[0, 4], lower=[-1, -2], upper=[10, 20])
            self.assert_mock_called_with(method.set_scipy_engine, N_calls=2, call_number=2, guess=[0, 4], lower=[-1, -2], upper=[10, 20])

            self.assert_mock_called_with(method.execute, N_calls=2, call_number=1, max_num_features=3, func=function, params=[0, 4])
            self.assert_mock_called_with(
                method.execute, N_calls=2, call_number=2, max_num_features=3, func=function, params=[6, 7]
            )  # updated start params

            self.assert_mock_called_with(function.read_from_report, N_calls=2, call_number=1, report_dict={"a": 1}, N=1, index=-1)
            self.assert_mock_called_with(function.read_from_report, N_calls=2, call_number=2, report_dict={"a": 1}, N=1, index=-1)

            self.assert_mock_called_with(
                self._alg.make_fit_ws,
                N_calls=2,
                call_number=1,
                engine=engine_mock,
                max_features=3,
                ws_list=[],
                x_unit="DeltaE",
                name="__BayesStretchTest_Sample_QL_0_",
            )
            self.assert_mock_called_with(
                self._alg.make_fit_ws,
                N_calls=2,
                call_number=2,
                engine=engine_mock,
                max_features=3,
                ws_list=["unit", "test"],
                x_unit="DeltaE",
                name="__BayesStretchTest_Sample_QL_1_",
            )

        def exec_setup(self, fit_ws, results, probs):
            self._alg.setProperty("Emax", 0.3)
            self._alg.setProperty("Emin", -0.3)
            self._alg.setProperty("Elastic", True)
            self._alg.setProperty("Background", "Linear")
            self._alg.setProperty("SampleWorkspace", self._sample_ws.name())
            self._alg.setProperty("ResolutionWorkspace", self._res_ws)
            self._alg.setProperty("Program", "QL")
            self._alg.setProperty("OutputWorkspaceFit", "fits")
            self._alg.setProperty("OutputWorkspaceResult", "results")
            self._alg.setProperty("OutputWorkspaceProb", "prob")

            self._alg.point_data = mock.Mock(side_effect=self.point_mock)
            self._alg.duplicate_res = mock.Mock(return_value=[1, 1, 1])
            self._alg.unique_res = mock.Mock(return_value=[2, 1, 3])
            self._alg.calculate = mock.Mock(return_value=(["ws"], {"a": 1}, {"a": 0.1}, [("log", "sample")]))

            self._alg.group_ws = mock.Mock(return_value=fit_ws)

            self._alg.add_sample_logs = mock.Mock(side_effect=add_log_mock)
            self._alg.make_results = mock.Mock(return_value=(results.name(), probs.name()))

        @mock.patch("BayesQuasi2.get_two_theta_and_q")
        @mock.patch("BayesQuasi2.Progress")
        def test_pyexec_QSe(self, prog_mock, get_Q_mock):
            progress_mock = mock.Mock()
            prog_mock.return_value = progress_mock

            tmp = CreateWorkspace([1, 2], [3, 4])
            fit_ws = GroupWorkspaces([tmp])
            results = CreateWorkspace([5, 6], [7, 8])
            probs = CreateWorkspace([9, 10], [11, 12])

            function_mock = mock.Mock()
            method_mock = mock.Mock()
            self._alg.QlStretchedExp = mock.Mock(return_value=method_mock)
            self._alg.QSEFunction = mock.Mock(return_value=function_mock)

            self.exec_setup(fit_ws, results, probs)
            self._alg.setProperty("Program", "QSe")

            get_Q_mock.return_value = [1, 2, 3]
            self._N_hist = 1

            self._alg.execute()

            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=1, name="__BayesStretchTest_Sample")
            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=2, name="__BayesStretchTest_Resolution")
            self.assertEqual(self._alg.duplicate_res.call_count, 1)
            self.assertEqual(self._alg.unique_res.call_count, 0)

            self._alg.calculate.assert_called_once_with(
                sample_ws=self._sample_ws,
                report_progress=progress_mock,
                res_list=[1, 1, 1],
                N=1,
                max_num_peaks=1,
                method=method_mock,
                function=function_mock,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=1,
                workspace=fit_ws,
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution"), ("fit_program", "QSe")],
                data_ws=self._sample_ws,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=2,
                workspace=results.name(),
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution"), ("fit_program", "QSe")],
                data_ws=self._sample_ws,
            )
            self._alg.make_results.assert_called_once_with(
                results={"a": 1},
                results_errors={"a": 0.1},
                x_data=2,
                x_unit="MomentumTransfer",
                max_features=1,
                name_params="results",
                name_prob="prob",
            )

        @mock.patch("BayesQuasi2.get_two_theta_and_q")
        @mock.patch("BayesQuasi2.Progress")
        def test_pyexec_QL(self, prog_mock, get_Q_mock):
            progress_mock = mock.Mock()
            prog_mock.return_value = progress_mock

            tmp = CreateWorkspace([1, 2], [3, 4])
            fit_ws = GroupWorkspaces([tmp])
            results = CreateWorkspace([5, 6], [7, 8])
            probs = CreateWorkspace([9, 10], [11, 12])

            function_mock = mock.Mock()
            method_mock = mock.Mock()
            self._alg.QLData = mock.Mock(return_value=method_mock)
            self._alg.QlDataFunction = mock.Mock(return_value=function_mock)

            self.exec_setup(fit_ws, results, probs)

            get_Q_mock.return_value = [1, 2, 3]

            self._alg.execute()

            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=1, name="__BayesStretchTest_Sample")
            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=2, name="__BayesStretchTest_Resolution")
            self.assertEqual(self._alg.duplicate_res.call_count, 1)
            self.assertEqual(self._alg.unique_res.call_count, 0)

            self._alg.calculate.assert_called_once_with(
                sample_ws=self._sample_ws,
                report_progress=progress_mock,
                res_list=[1, 1, 1],
                N=1,
                max_num_peaks=3,
                method=method_mock,
                function=function_mock,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=1,
                workspace=fit_ws,
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution"), ("fit_program", "QLr")],
                data_ws=self._sample_ws,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=2,
                workspace=results.name(),
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution"), ("fit_program", "QLr")],
                data_ws=self._sample_ws,
            )
            self._alg.make_results.assert_called_once_with(
                results={"a": 1},
                results_errors={"a": 0.1},
                x_data=2,
                x_unit="MomentumTransfer",
                max_features=3,
                name_params="results",
                name_prob="prob",
            )

        @mock.patch("BayesQuasi2.get_two_theta_and_q")
        @mock.patch("BayesQuasi2.Progress")
        def test_pyexec_QL_unique(self, prog_mock, get_Q_mock):
            progress_mock = mock.Mock()
            prog_mock.return_value = progress_mock

            tmp = CreateWorkspace([1, 2], [3, 4])
            fit_ws = GroupWorkspaces([tmp])
            results = CreateWorkspace([5, 6], [7, 8])
            probs = CreateWorkspace([9, 10], [11, 12])

            function_mock = mock.Mock()
            method_mock = mock.Mock()
            self._alg.QLData = mock.Mock(return_value=method_mock)
            self._alg.QlDataFunction = mock.Mock(return_value=function_mock)
            self.exec_setup(fit_ws, results, probs)

            get_Q_mock.return_value = [1, 2, 3]
            self._N_hist = 2

            self._alg.execute()

            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=1, name="__BayesStretchTest_Sample")
            self.assert_mock_called_with(self._alg.point_data, N_calls=2, call_number=2, name="__BayesStretchTest_Resolution")
            self.assertEqual(self._alg.duplicate_res.call_count, 0)
            self.assertEqual(self._alg.unique_res.call_count, 1)

            self._alg.calculate.assert_called_once_with(
                sample_ws=self._sample_ws,
                report_progress=progress_mock,
                res_list=[2, 1, 3],
                N=2,
                max_num_peaks=3,
                method=method_mock,
                function=function_mock,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=1,
                workspace=fit_ws,
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution"), ("fit_program", "QLd")],
                data_ws=self._sample_ws,
            )
            self.assert_mock_called_with(
                self._alg.add_sample_logs,
                N_calls=2,
                call_number=2,
                workspace=results.name(),
                sample_logs=[("log", "sample"), ("res_workspace", "__BayesStretchTest_Resolution"), ("fit_program", "QLd")],
                data_ws=self._sample_ws,
            )
            self._alg.make_results.assert_called_once_with(
                results={"a": 1},
                results_errors={"a": 0.1},
                x_data=2,
                x_unit="MomentumTransfer",
                max_features=3,
                name_params="results",
                name_prob="prob",
            )

    if __name__ == "__main__":
        unittest.main()
