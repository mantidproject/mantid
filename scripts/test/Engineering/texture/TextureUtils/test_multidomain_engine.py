# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#
import unittest
from unittest.mock import patch, MagicMock, call
from os import path
import numpy as np
from Engineering.texture.TextureUtils.fitting_utils import fit_all_peaks
from Engineering.texture.TextureUtils.multidomain_engine import (
    get_initial_fit_function_and_kwargs_from_specs,
    rerun_fit_with_new_ws,
    _get_default_param_ties,
    calc_intens_and_sigma_arrays,
    _populate_multidomain_output_table,
)

texture_utils_path = "Engineering.texture.TextureUtils.fitting_utils"
multidomain_path = "Engineering.texture.TextureUtils.multidomain_engine"


def _make_param_table_mock(num_spec: int, params=("A", "B", "S", "X0")):
    names = [f"f{ispec}.f0.{p}" for ispec in range(num_spec) for p in params] + ["Cost Function"]
    vals = np.arange(1, len(names) + 1, dtype=float)  # just distinct values
    errs = np.full_like(vals, 0.1)
    param_ws = MagicMock()

    def _col(which):
        if which == "Name":
            return names
        if which == "Value":
            return vals
        return errs

    param_ws.column.side_effect = _col
    return param_ws, names, vals, errs


class MultidomainFitFunctionTests(unittest.TestCase):
    @patch(f"{multidomain_path}._tie_bkg")
    @patch(f"{multidomain_path}._make_composite")
    @patch(f"{multidomain_path}._estimate_intensity_background_and_centre")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{texture_utils_path}.DeltaEModeType")
    @patch(f"{multidomain_path}.FunctionFactory")
    @patch(f"{multidomain_path}.MultiDomainFunction")
    def test_get_initial_fit_function_and_kwargs_from_specs(
        self, mock_gen_mdf, mock_func_factory, mock_delta_e, mock_unit_conv, mock_estimate_intens, mock_make_comp, mock_tie_bkg
    ):
        # inputs

        mock_ws = MagicMock()
        mock_ws_tof = MagicMock()
        peak = 1.0
        x_window = (0.95, 1.05)
        x0_window = (0.99, 1.01)
        parameters_to_tie = ("A", "B")
        peak_func_name = "BackToBackExponential"
        bg_func_name = "LinearBackground"
        bkg_is_tied = True

        # mock intermediates

        # mock spectrumInfo
        mock_si = MagicMock()
        mock_si.size.return_value = 2  # two spectra
        diff_consts = MagicMock()
        mock_si.diffractometerConstants.return_value = diff_consts
        mock_ws.spectrumInfo.return_value = mock_si

        # mock functions and function factory
        base_peak_func, peak_func1, peak_func2 = MagicMock(), MagicMock(), MagicMock()

        comp_func1, comp_func2 = MagicMock(), MagicMock()

        mock_func_factory.createFunction.return_value = MagicMock()
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (base_peak_func, peak_func1, peak_func2)

        # mock unit conversion

        tof_peaks, tof_starts, tof_ends = (10, 11), (8, 9), (15, 16)  # not physically accurate TOF but easier to keep track of
        x0_lowers, x0_uppers = (0.99, 0.99), (1.01, 1.01)
        mock_delta_e.Elastic = "elastic"
        unit_conv_res1, unit_conv_res2 = zip(tof_peaks, tof_starts, tof_ends, x0_lowers, x0_uppers)
        mock_unit_conv.run.side_effect = unit_conv_res1 + unit_conv_res2

        # mock ws_tof data access

        istarts, iends = (2, 3), (100, 101)  # mock indices for the indices corresponding to an x value
        mock_ws_tof.yIndexOfX.side_effect = [istarts[0], iends[0], istarts[1], iends[1]]
        mock_ws_tof.name.return_value = "ws_tof"

        # mock estimate

        intensities, sigmas, bgs, x0s = (2.0, 4.0), (1.0, 1.0), (0.0, 0.0), (10, 11)

        mock_estimate_intens.side_effect = list(zip(intensities, sigmas, bgs, x0s))

        # mock mdf

        mock_mdf = MagicMock()
        mock_gen_mdf.return_value = mock_mdf
        mock_mdf.nDomains.return_value = 2

        mock_make_comp.side_effect = (comp_func1, comp_func2)

        # mock bkg tie

        mock_tie_bkg.return_value = (mock_mdf, [])

        # mock func processing

        base_peak_func.nParams.return_value = 5
        base_peak_func.getParamName.side_effect = ("A", "B", "I", "X0", "S", "A", "B", "I", "X0", "S")

        # exec

        out_func, out_kwargs, _ = get_initial_fit_function_and_kwargs_from_specs(
            mock_ws, mock_ws_tof, peak, x_window, x0_window, parameters_to_tie, peak_func_name, bg_func_name, bkg_is_tied
        )

        # assert

        base_peak_func.getWidthParameterName.assert_called_once()

        vals_to_convert = (1.0, 0.95, 1.05, 0.99, 1.01, 1.0, 0.95, 1.05, 0.99, 1.01)
        mock_unit_conv.run.assert_has_calls([call("dSpacing", "TOF", val, 0, mock_delta_e.Elastic, diff_consts) for val in vals_to_convert])

        mock_mdf.add.assert_has_calls([call(comp_func1), call(comp_func2)])
        mock_mdf.setDomainIndex.assert_has_calls([call(0, 0), call(1, 1)])
        mock_mdf.setMatrixWorkspace.assert_has_calls([call(mock_ws_tof, i, tof_starts[i], tof_ends[i]) for i in range(2)])

        mock_mdf.addTies.assert_called_once_with("f1.f0.A=f0.f0.A,f1.f0.B=f0.f0.B")

        # the function object itself is returned, not a string
        self.assertIs(out_func, mock_mdf)

        peak_func1.setParameter.assert_any_call("X0", x0s[0])
        peak_func1.setParameter.assert_any_call("I", intensities[0])
        peak_func1.setMatrixWorkspace.assert_called_once()

        expected_spec_kwargs = {
            "InputWorkspace": "ws_tof",
            "StartX": tof_starts[0],
            "EndX": tof_ends[0],
            "WorkspaceIndex": 0,
            "InputWorkspace_1": "ws_tof",
            "StartX_1": tof_starts[1],
            "EndX_1": tof_ends[1],
            "WorkspaceIndex_1": 1,
        }

        self.assertEqual(expected_spec_kwargs, out_kwargs)

    @patch(f"{multidomain_path}.Fit")
    @patch(f"{multidomain_path}._make_composite")
    @patch(f"{multidomain_path}.FunctionFactory")
    @patch(f"{multidomain_path}.MultiDomainFunction")
    def test_rerun_fit_with_new_ws(
        self,
        mock_gen_mdf,
        mock_func_factory,
        mock_make_comp,
        mock_fit,
    ):
        # inputs
        mdf = MagicMock()
        fit_kwargs = {"SomeKwarg": 123}
        md_fit_kwargs = {
            "InputWorkspace": "old_ws",
            "StartX": 10.0,
            "EndX": 20.0,
            "WorkspaceIndex": 0,
            "InputWorkspace_1": "old_ws",
            "StartX_1": 30.0,
            "EndX_1": 40.0,
            "WorkspaceIndex_1": 1,
        }
        new_ws = MagicMock()
        new_ws.name.return_value = "new_ws"

        x0_frac_move = 0.1
        iters = 50
        parameters_to_fix = ("A", "B")
        tie_background = True

        # mock existing mdf domains: two composite functions (peak + bg)
        mdf.nFunctions.return_value = 2

        peak0, peak1 = MagicMock(), MagicMock()
        bg0, bg1 = MagicMock(), MagicMock()

        peak0.name.return_value = "BackToBackExponential"
        peak1.name.return_value = "BackToBackExponential"

        # peak parameter vals
        peak0.getParameterValue.side_effect = lambda p: {"I": 0.5, "X0": 10.0, "A": 1, "B": 1, "S": 1}[p]
        peak1.getParameterValue.side_effect = lambda p: {"I": 4.0, "X0": 20.0, "A": 1, "B": 1, "S": 1}[p]

        # background params
        bg1.nParams.return_value = 2
        bg1.getParamName.side_effect = ("A0", "A1")

        comp0, comp1 = MagicMock(), MagicMock()
        comp0.__getitem__.side_effect = lambda i: [peak0, bg0][i]
        comp1.__getitem__.side_effect = lambda i: [peak1, bg1][i]
        mdf.__getitem__.side_effect = (comp0, comp1)

        # mock mdf
        new_func = MagicMock()
        mock_gen_mdf.return_value = new_func

        # mock func factory
        new_peak0, new_peak1 = MagicMock(), MagicMock()
        new_peak0.name.return_value = "BackToBackExponential"
        new_peak1.name.return_value = "BackToBackExponential"
        # mock nParams and getParamName for the non-IC path
        new_peak0.nParams.return_value = 5
        new_peak0.getParamName.side_effect = lambda i: ["I", "X0", "A", "B", "S"][i]
        new_peak1.nParams.return_value = 5
        new_peak1.getParamName.side_effect = lambda i: ["I", "X0", "A", "B", "S"][i]
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (new_peak0, new_peak1)

        # mock composite functions
        comp_out0, comp_out1 = MagicMock(), MagicMock()
        mock_make_comp.side_effect = (comp_out0, comp_out1)

        # mock fit
        fit_return = MagicMock()
        mock_fit.return_value = fit_return

        # exec
        out_fit, out_md_kwargs = rerun_fit_with_new_ws(
            mdf=mdf,
            fit_kwargs=fit_kwargs,
            md_fit_kwargs=md_fit_kwargs,
            new_ws=new_ws,
            x0_frac_move=x0_frac_move,
            iters=iters,
            parameters_to_fix=parameters_to_fix,
            tie_background=tie_background,
        )

        # assert
        self.assertEqual(out_md_kwargs["InputWorkspace"], "new_ws")
        self.assertEqual(out_md_kwargs["InputWorkspace_1"], "new_ws")

        mock_instance.createPeakFunction.assert_has_calls([call("BackToBackExponential"), call("BackToBackExponential")])

        # constraints updated around new values
        # note intens is max(I, 1)
        # I=0.5 -> max(0.5,1)=1, x0=10 then bounds should be 0.5<I<2 and 9<X0<11
        new_peak0.addConstraints.assert_has_calls(
            [call("0.5<I<2"), call("9.0<X0<11.0")],
        )
        # domain1: I=4 and x0=20 then bounds should be 2.0<I<8.0 and 18<X0<22
        new_peak1.addConstraints.assert_has_calls(
            [call("2.0<I<8.0"), call("18.0<X0<22.0")],
        )

        for p in parameters_to_fix:
            new_peak0.fixParameter.assert_any_call(p)
            new_peak1.fixParameter.assert_any_call(p)

        # now uses _make_composite and adds the composite directly
        new_func.add.assert_has_calls([call(comp_out0), call(comp_out1)])
        new_func.setDomainIndex.assert_has_calls([call(0, 0), call(1, 1)])

        # ties are added via addTies, not string concatenation
        new_func.addTies.assert_called_once_with("f1.f1.A0=f0.f1.A0,f1.f1.A1=f0.f1.A1")

        # Function object is passed directly (not as string)
        mock_fit.assert_called_once_with(
            Function=new_func,
            Output="fit_new_ws",
            MaxIterations=iters,
            **fit_kwargs,
            **out_md_kwargs,
        )

        # setMatrixWorkspace called on each new peak function
        new_peak0.setMatrixWorkspace.assert_called()
        new_peak1.setMatrixWorkspace.assert_called()

        # return values
        self.assertIs(out_fit, fit_return)
        self.assertIs(out_md_kwargs, md_fit_kwargs)

    @patch(f"{multidomain_path}.Fit")
    @patch(f"{multidomain_path}._make_composite")
    @patch(f"{multidomain_path}.FunctionFactory")
    @patch(f"{multidomain_path}.MultiDomainFunction")
    def test_rerun_fit_with_new_ws_fixes_shared_params_instead_of_tying(
        self,
        mock_gen_mdf,
        mock_func_factory,
        mock_make_comp,
        mock_fit,
    ):
        """The shared broadening params (parameters_to_tie) should be fixed per-domain at the value
        carried through from the previous fit, NOT tied across domains - this decouples the fit."""
        mdf = MagicMock()
        fit_kwargs = {"SomeKwarg": 123}
        md_fit_kwargs = {
            "InputWorkspace": "old_ws",
            "StartX": 10.0,
            "EndX": 20.0,
            "WorkspaceIndex": 0,
            "InputWorkspace_1": "old_ws",
            "StartX_1": 30.0,
            "EndX_1": 40.0,
            "WorkspaceIndex_1": 1,
        }
        new_ws = MagicMock()
        new_ws.name.return_value = "new_ws"

        mdf.nFunctions.return_value = 2

        # previous fit's domains carry the (tied) refined A, B values we expect to be re-asserted
        peak0, peak1 = MagicMock(), MagicMock()
        bg0, bg1 = MagicMock(), MagicMock()
        peak0.name.return_value = "BackToBackExponential"
        peak1.name.return_value = "BackToBackExponential"
        peak0.getParameterValue.side_effect = lambda p: {"I": 0.5, "X0": 10.0, "A": 1.5, "B": 2.5, "S": 1.0}[p]
        peak1.getParameterValue.side_effect = lambda p: {"I": 4.0, "X0": 20.0, "A": 1.5, "B": 2.5, "S": 1.0}[p]

        comp0, comp1 = MagicMock(), MagicMock()
        comp0.__getitem__.side_effect = lambda i: [peak0, bg0][i]
        comp1.__getitem__.side_effect = lambda i: [peak1, bg1][i]
        mdf.__getitem__.side_effect = (comp0, comp1)

        new_func = MagicMock()
        mock_gen_mdf.return_value = new_func

        new_peak0, new_peak1 = MagicMock(), MagicMock()
        new_peak0.name.return_value = "BackToBackExponential"
        new_peak1.name.return_value = "BackToBackExponential"
        for np_mock in (new_peak0, new_peak1):
            np_mock.nParams.return_value = 5
            np_mock.getParamName.side_effect = lambda i: ["I", "X0", "A", "B", "S"][i]
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.side_effect = (new_peak0, new_peak1)

        mock_make_comp.side_effect = (MagicMock(), MagicMock())
        mock_fit.return_value = MagicMock()

        rerun_fit_with_new_ws(
            mdf=mdf,
            fit_kwargs=fit_kwargs,
            md_fit_kwargs=md_fit_kwargs,
            new_ws=new_ws,
            x0_frac_move=0.1,
            iters=50,
            parameters_to_tie=("A", "B"),
            tie_background=False,
            is_final=False,
        )

        # each domain re-asserts the previous A, B value (survives setMatrixWorkspace) then fixes it
        for new_peak in (new_peak0, new_peak1):
            new_peak.setParameter.assert_any_call("A", 1.5)
            new_peak.setParameter.assert_any_call("B", 2.5)
            new_peak.fixParameter.assert_any_call("A")
            new_peak.fixParameter.assert_any_call("B")

        # the shared params must NOT be tied across domains (no f*.f0.* ties); with tie_background
        # off there are no cross-domain ties at all, so addTies is not called
        new_func.addTies.assert_not_called()

    @patch(f"{multidomain_path}.Fit")
    @patch(f"{multidomain_path}._make_composite")
    @patch(f"{multidomain_path}._get_default_param_ties")
    @patch(f"{multidomain_path}.FunctionFactory")
    @patch(f"{multidomain_path}.MultiDomainFunction")
    def test_rerun_fit_with_new_ws_last_fit_ic(
        self,
        mock_gen_mdf,
        mock_func_factory,
        mock_get_default_ties,
        mock_make_comp,
        mock_fit,
    ):
        mdf = MagicMock()
        fit_kwargs = {"SomeKwarg": 123}
        md_fit_kwargs = {
            "InputWorkspace": "old_ws",
            "StartX": 10.0,
            "EndX": 20.0,
            "WorkspaceIndex": 0,
        }
        new_ws = MagicMock()
        new_ws.name.return_value = "new_ws"

        mdf.nFunctions.return_value = 1

        peak0 = MagicMock()
        bg0 = MagicMock()
        peak0.name.return_value = "BackToBackExponential"
        peak0.getParameterValue.side_effect = lambda p: {"I": 5.0, "X0": 15.0}[p]

        comp0 = MagicMock()
        comp0.__getitem__.side_effect = lambda i: [peak0, bg0][i]
        mdf.__getitem__.side_effect = (comp0,)

        new_func = MagicMock()
        mock_gen_mdf.return_value = new_func

        new_peak = MagicMock()
        new_peak.name.return_value = "IkedaCarpenterPV"
        mock_instance = MagicMock()
        mock_func_factory.Instance.return_value = mock_instance
        mock_instance.createPeakFunction.return_value = new_peak

        mock_get_default_ties.return_value = ("Alpha0", "Alpha1", "Beta0", "Kappa")

        mock_make_comp.return_value = MagicMock()
        mock_fit.return_value = MagicMock()

        out_fit, _ = rerun_fit_with_new_ws(
            mdf=mdf,
            fit_kwargs=fit_kwargs,
            md_fit_kwargs=md_fit_kwargs,
            new_ws=new_ws,
            x0_frac_move=0.1,
            iters=50,
            is_final=True,
            last_fit_ic=True,
        )

        # should create IkedaCarpenterPV (not BackToBackExponential)
        mock_instance.createPeakFunction.assert_called_once_with("IkedaCarpenterPV")

        # should set X0 and I before setMatrixWorkspace
        new_peak.setParameter.assert_any_call("X0", 15.0)
        new_peak.setParameter.assert_any_call("I", 5.0)
        new_peak.setMatrixWorkspace.assert_called()

        # should get default IC ties since peak was not already IkedaCarpenter
        mock_get_default_ties.assert_called_once_with("IkedaCarpenterPV", None)

    @patch(f"{multidomain_path}.FunctionFactory")
    @patch(f"{multidomain_path}.MultiDomainFunction")
    def test_get_initial_fit_function_and_kwargs_from_specs_raises_on_invalid_tie_param(self, mock_mdf, mock_factory):
        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = 0  # skip per-spectrum loop
        ws.spectrumInfo.return_value = si
        ws_tof = MagicMock()

        # peak function with known available params
        peak_func = MagicMock()
        peak_func.nParams.return_value = 3
        peak_func.getParamName.side_effect = lambda i: ["I", "X0", "S"][i]
        peak_func.getWidthParameterName.return_value = "S"

        # factory wiring
        mock_factory.Instance().createPeakFunction.return_value = peak_func
        mock_factory.createFunction.return_value = MagicMock()

        # MDF minimal behavior
        mdf = MagicMock()
        mdf.nDomains.return_value = 0
        mock_mdf.return_value = mdf

        # invalid parameter name
        with self.assertRaises(ValueError) as ctx:
            get_initial_fit_function_and_kwargs_from_specs(
                ws=ws,
                ws_tof=ws_tof,
                peak=1.0,
                x_window=(0.9, 1.1),
                x0_window=(0.95, 1.05),
                parameters_to_tie=["NotAParam"],
                peak_func_name="BackToBackExponential",
                bg_func_name="LinearBackground",
                tie_bkg=False,
            )
        self.assertIn("Invalid parameter(s) to tie", str(ctx.exception))


class MultidomainHelperTests(unittest.TestCase):
    def test_get_default_param_ties_b2b(self):
        result = _get_default_param_ties("BackToBackExponential", None)
        self.assertEqual(result, ("A", "B"))

    def test_get_default_param_ties_ikeda_carpenter(self):
        result = _get_default_param_ties("IkedaCarpenterPV", None)
        self.assertEqual(result, ("Alpha0", "Alpha1", "Beta0", "Kappa"))

    def test_get_default_param_ties_preserves_explicit_ties(self):
        explicit = ("X0", "S")
        result = _get_default_param_ties("BackToBackExponential", explicit)
        self.assertIs(result, explicit)

    def test_get_default_param_ties_unknown_func_returns_none(self):
        result = _get_default_param_ties("Gaussian", None)
        self.assertIsNone(result)

    @patch(f"{multidomain_path}.calc_sigma_from_summation")
    @patch(f"{multidomain_path}.get_eval_ws")
    def test_calc_intens_and_sigma_arrays(self, mock_get_eval_ws, mock_calc_sigma):
        # set up a mock fit result with 2 domains
        func = MagicMock()
        comp0, comp1 = MagicMock(), MagicMock()
        comp0.getParameterValue.return_value = 10.0  # I for domain 0
        comp1.getParameterValue.return_value = 20.0  # I for domain 1
        func.__iter__ = MagicMock(return_value=iter([comp0, comp1]))
        func.nDomains.return_value = 2

        # mock workspace data
        ws0, ws1 = MagicMock(), MagicMock()
        ws0.readX.return_value = np.array([1, 2, 3])
        ws0.readE.return_value = np.array([0.1, 0.2, 0.3])
        ws0.readY.return_value = np.array([5, 6, 7])  # readY(3) for calc function
        ws1.readX.return_value = np.array([4, 5, 6])
        ws1.readE.return_value = np.array([0.4, 0.5, 0.6])
        ws1.readY.return_value = np.array([8, 9, 10])
        mock_get_eval_ws.side_effect = [ws0, ws1]

        mock_calc_sigma.side_effect = [(2.0, 1.5), (5.0, 4.0)]

        fit_result = {"Function": func, "OutputWorkspace": "fit_ws"}
        intens, sigma, i_over_sig, peak_limits = calc_intens_and_sigma_arrays(fit_result)

        np.testing.assert_array_equal(intens, [10.0, 20.0])
        np.testing.assert_array_equal(sigma, [2.0, 5.0])
        np.testing.assert_allclose(i_over_sig, [10.0 / 2.0, 20.0 / 5.0])
        np.testing.assert_array_equal(peak_limits, [1.5, 4.0])

    @patch(f"{multidomain_path}.calc_sigma_from_summation")
    @patch(f"{multidomain_path}.get_eval_ws")
    def test_calc_intens_and_sigma_arrays_zero_sigma(self, mock_get_eval_ws, mock_calc_sigma):
        func = MagicMock()
        comp0 = MagicMock()
        comp0.getParameterValue.return_value = 10.0
        func.__iter__ = MagicMock(return_value=iter([comp0]))
        func.nDomains.return_value = 1

        ws0 = MagicMock()
        ws0.readX.return_value = np.array([1, 2, 3])
        ws0.readE.return_value = np.array([0.1, 0.2, 0.3])
        ws0.readY.return_value = np.array([5, 6, 7])
        mock_get_eval_ws.return_value = ws0
        mock_calc_sigma.return_value = (0.0, None)

        fit_result = {"Function": func, "OutputWorkspace": "fit_ws"}
        _, _, i_over_sig, _ = calc_intens_and_sigma_arrays(fit_result)

        self.assertEqual(i_over_sig[0], 0.0)


class MultidomainOutputTableTests(unittest.TestCase):
    @patch(f"{multidomain_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{multidomain_path}.ADS")
    def test_populate_multidomain_output_table_columns_and_rows(self, mock_ads, mock_unitconv, mock_convert_toferr):
        # spectrum 0 fitted, spectrum 1 masked out; peak params are A and X0 (X0 reported in d-spacing)
        num_spec = 2
        param_ws, *_ = _make_param_table_mock(num_spec, params=("A", "X0"))
        mock_ads.retrieve.return_value = param_ws
        mock_unitconv.run.return_value = 1.5  # TOF -> d for X0
        mock_convert_toferr.return_value = 0.05

        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = num_spec
        ws.spectrumInfo.return_value = si

        out_tab = MagicMock()
        _populate_multidomain_output_table(
            out_tab,
            ws,
            "smooth_ws",
            intensity_estimates=[100.0, 200.0],
            fit_mask=np.array([True, False]),
            no_fit_value_dict=None,
            nan_replacement=None,
        )

        # parameter table for the last fitted ws was retrieved
        mock_ads.retrieve.assert_called_once_with("fit_smooth_ws_Parameters")

        # columns: wsindex, I_est, then a triple per peak parameter (A, X0)
        out_tab.addColumn.assert_any_call("int", "wsindex")
        out_tab.addColumn.assert_any_call("double", "I_est")
        for p in ("A", "X0"):
            out_tab.addColumn.assert_any_call("double", p)
            out_tab.addColumn.assert_any_call("double", f"{p}_err")
            out_tab.addColumn.assert_any_call("double", f"{p}/{p}_err")

        self.assertEqual(out_tab.addRow.call_count, num_spec)

        # fitted spectrum 0: [wsindex, I_est, A, A_err, A/A_err, X0(d), X0_err(d), X0/X0_err]
        row0 = out_tab.addRow.call_args_list[0][0][0]
        self.assertEqual(row0[0], 0)
        self.assertEqual(row0[1], 100.0)  # I_est
        self.assertEqual(row0[2], 1.0)  # A value (first value in the mocked table)
        self.assertAlmostEqual(row0[4], 1.0 / 0.1)  # A / A_err
        self.assertEqual(row0[5], 1.5)  # X0 converted to d-spacing
        self.assertEqual(row0[6], 0.05)  # X0 d-spacing error
        self.assertAlmostEqual(row0[7], 1.5 / 0.05)  # X0 / X0_err

        # masked spectrum 1: unfit convention -> infinite errors, zero ratios
        row1 = out_tab.addRow.call_args_list[1][0][0]
        self.assertEqual(row1[0], 1)
        self.assertEqual(row1[3], np.inf)  # A_err
        self.assertEqual(row1[4], 0.0)  # A / A_err
        self.assertEqual(row1[6], np.inf)  # X0_err
        self.assertEqual(row1[7], 0.0)  # X0 / X0_err


class MultidomainFitAllPeaksTests(unittest.TestCase):
    """End-to-end exercise of the multidomain engine via fit_all_peaks(engine="multidomain")."""

    @patch(f"{multidomain_path}.SaveNexus")
    @patch(f"{multidomain_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{multidomain_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.5, 5.0]), None),
    )
    @patch(f"{multidomain_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{multidomain_path}.rerun_fit_with_new_ws")
    @patch(f"{multidomain_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{multidomain_path}.Fit")
    @patch(f"{multidomain_path}.Rebunch")
    @patch(f"{multidomain_path}.fit_initial_summed_spectra")
    @patch(f"{multidomain_path}.ConvertUnits")
    @patch(f"{multidomain_path}._get_grouping_from_ws_log")
    @patch(f"{multidomain_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{multidomain_path}.ADS")
    def test_fit_all_peaks_basic_fit(
        self,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_convert_units,
        mock_fit_summed,
        mock_rebunch,
        mock_fit,
        mock_get_initial,
        mock_rerun_with_new,
        mock_unitconv,
        mock_convert_toferr_to_derr,
        _mock_calc_i_over_sig,
        mock_create_tab_ws,
        mock_save_nexus,
    ):
        mock_unitconv.run.return_value = 1.2345
        mock_convert_toferr_to_derr.return_value = 0.01
        mock_fit_summed.return_value = ([(0.95, 1.05)], [["ws_crop_rebin_peak0_ws0"]])

        wsname = "TEST123456_ws"
        prefix, run_number, group = "TEST", "123456", "TestGroup"
        peak, save_dir, num_spec = 1.0, "save", 2

        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = num_spec
        ws.spectrumInfo.return_value = si

        mock_convert_units.return_value = "ws_tof"
        mock_rebunch.side_effect = ["smooth_ws_3", "smooth_ws_2"]

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0])

        fit_obj = MagicMock()
        fit_obj.Function.function = "md_function"
        fit_obj.OutputWorkspace.name.return_value = "fit_out_ws"
        mock_fit.return_value = fit_obj

        fit_obj2 = MagicMock()
        fit_obj2.Function.function = "md_function_final"
        fit_obj2.OutputWorkspace.name.return_value = "fit_out_ws_final"
        mock_rerun_with_new.return_value = (fit_obj2, md_fit_kwargs)

        param_ws, *_ = _make_param_table_mock(num_spec, params=())
        mock_ads.retrieve.side_effect = [ws, param_ws]

        mock_get_run_prefix.return_value = (run_number, prefix)
        mock_get_group.return_value = group

        tab_ws = MagicMock()
        mock_create_tab_ws.return_value = tab_ws

        fit_all_peaks(
            wss=[wsname],
            peaks=[peak],
            peak_window=0.1,
            save_dir=save_dir,
            override_dir=True,
            engine="multidomain",
        )

        expected_tab_name = f"{prefix}{run_number}_{peak}_{group}_Fit_Parameters"
        expected_out_path = path.join(save_dir, f"{expected_tab_name}.nxs")
        mock_create_tab_ws.assert_called_once_with(OutputWorkspace=expected_tab_name)
        mock_save_nexus.assert_called_once_with(InputWorkspace=expected_tab_name, Filename=expected_out_path)

        mock_get_initial.assert_called_once_with(
            ws,
            "smooth_ws_3",
            peak,
            (peak - 0.1, peak + 0.1),
            (0.95, 1.05),
            ("A", "B"),
            "BackToBackExponential",
            "LinearBackground",
            False,
        )

        _, fit_call_kwargs = mock_fit.call_args
        self.assertEqual(fit_call_kwargs["Function"], "FUNC")
        self.assertEqual(fit_call_kwargs["Output"], "fit_smooth_ws_3")
        self.assertEqual(fit_call_kwargs["MaxIterations"], 50)
        self.assertEqual(fit_call_kwargs["CostFunction"], "Unweighted least squares")

        expected_kwargs = {
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": False,
            "CreateOutput": True,
            "OutputCompositeMembers": True,
            "Minimizer": "Levenberg-Marquardt",
            "CostFunction": "Unweighted least squares",
        }

        mock_rerun_with_new.assert_has_calls(
            [
                call("md_function", expected_kwargs, md_fit_kwargs, "smooth_ws_2", 0.02, 50, None, ("A", "B"), True, False, False),
                call("md_function_final", expected_kwargs, md_fit_kwargs, "ws_tof", 0.02, 50, None, ("A", "B"), True, True, False),
            ]
        )
        self.assertEqual(tab_ws.addRow.call_count, num_spec)

    @patch(f"{multidomain_path}.SaveNexus")
    @patch(f"{multidomain_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{multidomain_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.0, 3.0]), None),
    )
    @patch(f"{multidomain_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{multidomain_path}.rerun_fit_with_new_ws")
    @patch(f"{multidomain_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{multidomain_path}.Fit")
    @patch(f"{multidomain_path}.Rebunch")
    @patch(f"{multidomain_path}.fit_initial_summed_spectra")
    @patch(f"{multidomain_path}.ConvertUnits")
    @patch(f"{multidomain_path}._get_grouping_from_ws_log")
    @patch(f"{multidomain_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{multidomain_path}.ADS")
    def test_fit_all_peaks_multiple_wss_and_peaks(
        self,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_convert_units,
        mock_fit_summed,
        mock_rebunch,
        mock_fit,
        mock_get_initial,
        mock_rerun_with_new,
        mock_unitconv,
        mock_convert_toferr_to_derr,
        _mock_calc_i_over_sig,
        mock_create_tab_ws,
        mock_save_nexus,
    ):
        mock_unitconv.run.return_value = 1.2345
        mock_convert_toferr_to_derr.return_value = 0.01
        mock_fit_summed.return_value = ([(0.95, 1.05), (1.95, 2.05)], [["ws1_1.0", "ws2_1.0"], ["ws1_2.0", "ws2_2.0"]])

        wss = ["TEST000101_ws", "TEST000102_ws"]
        runs = ["000101", "000102"]
        prefix, group, peaks, num_spec = "TEST", "TestGroup", [1.0, 2.0], 2

        ws1, ws2 = MagicMock(), MagicMock()
        si1, si2 = MagicMock(), MagicMock()
        si1.size.return_value = num_spec
        si2.size.return_value = num_spec
        ws1.spectrumInfo.return_value = si1
        ws2.spectrumInfo.return_value = si2

        mock_convert_units.side_effect = ["ws1_tof_1", "ws1_tof_2", "ws2_tof_1", "ws2_tof_2"]
        mock_rebunch.side_effect = [
            "smooth_ws_3_1",
            "smooth_ws_2_1",
            "smooth_ws_3_2",
            "smooth_ws_2_2",
            "smooth_ws_3_1",
            "smooth_ws_2_1",
            "smooth_ws_3_2",
            "smooth_ws_2_2",
        ]

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0])

        fit_obj = MagicMock()
        fit_obj.Function.function = "md_function"
        fit_obj.OutputWorkspace.name.return_value = "fit_out_ws"
        mock_fit.return_value = fit_obj
        mock_rerun_with_new.return_value = (fit_obj, md_fit_kwargs)

        # param tables with only cost row
        param_ws, *_ = _make_param_table_mock(num_spec, params=())
        mock_ads.retrieve.side_effect = [ws1, param_ws, param_ws, ws2, param_ws, param_ws]

        mock_get_run_prefix.side_effect = [(runs[0], prefix), (runs[1], prefix)]
        mock_get_group.side_effect = [group, group]

        fit_all_peaks(
            wss=wss,
            peaks=peaks,
            peak_window=0.1,
            save_dir="save",
            override_dir=True,
            engine="multidomain",
        )

        expected_calls = len(wss) * len(peaks)
        self.assertEqual(mock_fit.call_count, expected_calls)
        self.assertEqual(mock_rerun_with_new.call_count, expected_calls * 2)
        self.assertEqual(mock_create_tab_ws.call_count, expected_calls)
        self.assertEqual(mock_save_nexus.call_count, expected_calls)

    @patch(f"{multidomain_path}.CloneWorkspace")
    @patch(f"{multidomain_path}.SaveNexus")
    @patch(f"{multidomain_path}.CreateEmptyTableWorkspace")
    @patch(
        f"{multidomain_path}.calc_intens_and_sigma_arrays",
        return_value=(None, None, np.array([3.5, 5.0]), None),
    )
    @patch(f"{multidomain_path}.convert_TOFerror_to_derror")
    @patch(f"{texture_utils_path}.UnitConversion")
    @patch(f"{multidomain_path}.rerun_fit_with_new_ws")
    @patch(f"{multidomain_path}.get_initial_fit_function_and_kwargs_from_specs")
    @patch(f"{multidomain_path}.Fit")
    @patch(f"{multidomain_path}.Rebunch")
    @patch(f"{multidomain_path}.fit_initial_summed_spectra")
    @patch(f"{multidomain_path}.ConvertUnits")
    @patch(f"{multidomain_path}._get_grouping_from_ws_log")
    @patch(f"{multidomain_path}._get_run_and_prefix_from_ws_log")
    @patch(f"{multidomain_path}.ADS")
    def test_fit_all_peaks_last_fit_ic_adds_extra_fit_ws(
        self,
        mock_ads,
        mock_get_run_prefix,
        mock_get_group,
        mock_convert_units,
        mock_fit_summed,
        mock_rebunch,
        mock_fit,
        mock_get_initial,
        mock_rerun_with_new,
        mock_unitconv,
        mock_convert_toferr_to_derr,
        _mock_calc_i_over_sig,
        mock_create_tab_ws,
        mock_save_nexus,
        mock_clone_ws,
    ):
        """When last_fit_ic=True with B2B peak func, an extra fit iteration should be added."""
        mock_unitconv.run.return_value = 1.2345
        mock_convert_toferr_to_derr.return_value = 0.01
        mock_fit_summed.return_value = ([(0.95, 1.05)], [["ws_crop_rebin_peak0_ws0"]])

        wsname = "TEST123456_ws"
        num_spec = 2

        ws = MagicMock()
        si = MagicMock()
        si.size.return_value = num_spec
        ws.spectrumInfo.return_value = si

        mock_convert_units.return_value = "ws_tof"
        mock_rebunch.side_effect = ["smooth_ws_3", "smooth_ws_2"]
        mock_clone_ws.return_value = "ws_tof_IkedaCarpenter"

        md_fit_kwargs = {"InputWorkspace": "ws_tof", "StartX": 0.9, "EndX": 1.1, "WorkspaceIndex": 0}
        mock_get_initial.return_value = ("FUNC", md_fit_kwargs, [10.0, 20.0])

        fit_obj = MagicMock()
        fit_obj.Function.function = "md_function"
        fit_obj.OutputWorkspace.name.return_value = "fit_out_ws"
        mock_fit.return_value = fit_obj
        mock_rerun_with_new.return_value = (fit_obj, md_fit_kwargs)

        param_ws = MagicMock()
        param_ws.column.side_effect = lambda which: {"Name": ["Cost Function"], "Value": [1.0], "Error": [0.0]}[which]
        mock_ads.retrieve.side_effect = [ws, param_ws]
        mock_get_run_prefix.return_value = ("123456", "TEST")
        mock_get_group.return_value = "TestGroup"
        mock_create_tab_ws.return_value = MagicMock()

        fit_all_peaks(
            wss=[wsname],
            peaks=[1.0],
            peak_window=0.1,
            save_dir="save",
            override_dir=True,
            last_fit_ic=True,
            peak_func_name="BackToBackExponential",
            engine="multidomain",
        )

        # with smooth_vals=(3,2) + final_fit_raw + last_fit_ic, there should be 4 rerun calls
        # (smooth_3 -> smooth_2 -> raw -> IC clone)
        self.assertEqual(mock_rerun_with_new.call_count, 3)

        # The last call should pass last_fit_ic=True and is_final=True
        last_call_args = mock_rerun_with_new.call_args_list[-1][0]
        self.assertTrue(last_call_args[-1])  # last_fit_ic
        self.assertTrue(last_call_args[-2])  # is_final


if __name__ == "__main__":
    unittest.main()
