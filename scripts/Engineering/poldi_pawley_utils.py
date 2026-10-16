# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""POLDI-specific Pawley fitting utilities.

The 2D pattern classes, which simulate a POLDI 2D detector image from a 1D profile.  The
instrument-agnostic machinery they build on lives in pawley_utils.
"""

from __future__ import annotations
from mantid.simpleapi import CreateWorkspace
import numpy as np
from mantid.fitfunctions import FunctionWrapper
from mantid.api import FunctionFactory
from mantid.kernel import logger
from typing import TYPE_CHECKING, Optional
from scipy.optimize import least_squares
from plugins.algorithms.poldi_utils import simulate_2d_data, get_dspac_array_from_ws
from plugins.algorithms.diffraction_utils import get_live_spectra
from Engineering.pawley_utils import (
    PawleyFitStrategy,
    calculate_scales_and_bg,
    BoundsMixin,
    MtdFuncMixin,
    OutputTableMixin,
    PawleyPattern1D,
    PawleyPatternBase,
)

if TYPE_CHECKING:
    from mantid.dataobjects import Workspace2D
    from scipy.optimize import OptimizeResult


class Poldi2DEvalMixin:
    """
    Methods relating to 2D POLDI workspace evaluation/simulation used in PawleyPattern2D and
    PawleyPattern2DNoConstraints classes
    """

    def _reset_scales_and_bgs(self):
        """Reset locked per-spectrum scale/background values to neutral defaults."""
        n_hist = self.ws.getNumberHistograms()
        self.scales = np.ones(n_hist, dtype=float)
        self.bgs = np.zeros(n_hist, dtype=float)

    def _ensure_scales_and_bgs(self):
        if (
            getattr(self, "scales", None) is None
            or getattr(self, "bgs", None) is None
            or len(self.scales) != self.ws.getNumberHistograms()
            or len(self.bgs) != self.ws.getNumberHistograms()
        ):
            self._reset_scales_and_bgs()

    def _simulate_2d(self, params: np.ndarray[float]) -> Workspace2D:
        self.ws_1d.setSharedY(0, self.eval_profile(params))
        ws_sim = simulate_2d_data(self.ws, self.ws_1d, output_workspace=f"{self.ws.name()}_sim", lambda_max=self.lambda_max)
        if self.apply_lorentz_correction:
            si = ws_sim.spectrumInfo()
            for ispec in range(ws_sim.getNumberHistograms()):
                ws_sim.setSharedY(ispec, ws_sim.y(ispec) * np.sin(si.twoTheta(ispec) / 2))
        return ws_sim

    def _apply_scales_and_bg(self, ws_sim: Workspace2D):
        self._ensure_scales_and_bgs()
        for ispec in range(self.ws.getNumberHistograms()):
            ws_sim.setSharedY(ispec, self.scales[ispec] * ws_sim.y(ispec) + self.bgs[ispec])

    def _reestimate_scales(self, params: np.ndarray[float] | None = None) -> tuple[np.ndarray, np.ndarray]:
        """Re-estimate and lock per-spectrum scale/background values.

        In global-scale mode no per-spectrum correction is applied, so the
        locked values are reset to neutral defaults and returned.
        """
        if params is None:
            params = self.get_free_params()
        if self.global_scale:
            self._reset_scales_and_bgs()
            return self.scales, self.bgs

        ws_sim = self._simulate_2d(params)
        live = get_live_spectra(self.ws)
        # masked spectra are left at the neutral defaults - the lstsq is degenerate for an all-zero spectrum
        self._reset_scales_and_bgs()
        self.scales[live], self.bgs[live] = calculate_scales_and_bg(self.ws.extractY()[live], ws_sim.extractY()[live])
        return self.scales, self.bgs

    def eval_2d(self, params: np.ndarray[float]) -> Workspace2D:
        ws_sim = self._simulate_2d(params)
        if not self.global_scale:
            self._apply_scales_and_bg(ws_sim)
        return ws_sim

    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        ws_sim = self.eval_2d(params)
        live = get_live_spectra(self.ws)
        return (self.ws.extractY()[live] - ws_sim.extractY()[live]).flat

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        return self.comp_func(self.ws_1d.x(0))


class PawleyPattern2D(Poldi2DEvalMixin, PawleyPatternBase):
    def __init__(
        self,
        *args,
        global_scale: bool = True,
        lambda_max: float = 5.0,
        apply_lorentz_correction: bool = False,
        **kwargs,
    ):
        super().__init__(*args, **kwargs)
        self.lambda_max = lambda_max
        self.apply_lorentz_correction = apply_lorentz_correction
        self.set_global_scale(global_scale)
        # create workspace in d-spacing to contain diffraction pattern
        self.ws_1d = self.make_1d_ws(self.ws)
        self.update_profile_function()

    @staticmethod
    def make_1d_ws(ws: Workspace2D) -> Workspace2D:
        dspacs = get_dspac_array_from_ws(ws)
        return CreateWorkspace(
            DataX=dspacs,
            DataY=np.zeros_like(dspacs),
            UnitX="dSpacing",
            YUnitLabel="Intensity (a.u.)",
            OutputWorkspace=f"{ws.name()}_pattern",
            EnableLogging=False,
        )

    def set_global_scale(self, global_scale: bool) -> None:
        self.global_scale = global_scale
        self._reset_scales_and_bgs()
        if not self.global_scale:
            # Per-spectrum scales (locked after initialisation of each fit() call) make all
            # peak intensities identifiable without needing to fix any individual intensity.
            # Zero and fix any global background — background is handled per-spectrum in eval_2d.
            if len(self.bg_params) > 0:
                self.bg_params[:] = 0
                self.bg_isfree[:] = False

    def set_params_from_pawley1d(
        self,
        pawley1d: PawleyPattern1D,
    ) -> None:
        if len(pawley1d.phases) != len(self.phases):
            logger.error("PawleyPattern1D object has a different number of phases.")
            return
        if pawley1d.profile.func_name == self.profile.func_name:
            self.profile_params = [p.copy() for p in pawley1d.profile_params]
        intens_changed = False
        for iphase, phase in enumerate(self.phases):
            phase1d = pawley1d.phases[iphase]

            # check the phase lattice parameters can be copied
            if phase.has_the_same_parameters_as(phase1d):
                self.alatt_params[iphase] = pawley1d.alatt_params[iphase].copy()
            else:
                logger.error("Lattice parameterisation of phases on Pawley1D don't match phases on Pawley2D")
                return

            if phase.nhkls() == phase1d.nhkls():
                # identical set of HKLs — copy directly
                self.intens[iphase] = pawley1d.intens[iphase].copy()
                intens_changed = True
            elif phase.nhkls() < phase1d.nhkls():
                # 2D phase is a d-spacing-filtered subset: match each 2D HKL to the
                # corresponding 1D HKL by integer (h,k,l) and copy the intensity.
                hkl_to_idx = {}
                for idx, hkl in enumerate(phase1d.hkls):
                    key = phase.hkl_as_key(hkl)
                    hkl_to_idx[key] = idx
                new_intens = np.empty(phase.nhkls(), dtype=float)
                for ipk, hkl in enumerate(phase.hkls):
                    key = phase.hkl_as_key(hkl)
                    new_intens[ipk] = pawley1d.intens[iphase][hkl_to_idx[key]]
                self.intens[iphase] = new_intens
                intens_changed = True
        if intens_changed:
            self._estimate_intensities()

    def estimate_initial_params(self) -> None:
        self._estimate_intensities()

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws_1d.x(0))

    def _estimate_intensities(self) -> None:
        # Compute per-spectrum scales at current intensities and use their median as a global
        # correction factor.
        self._reestimate_scales(self.get_free_params())
        if len(self.scales) > 0:
            # masked spectra sit at the neutral scale of 1 so must be excluded from the median
            valid_scales = self.scales[get_live_spectra(self.ws) & (self.scales > 0)]
            global_scale = float(np.median(valid_scales)) if len(valid_scales) > 0 else 1.0
            for iphase in range(len(self.phases)):
                self.intens[iphase] *= global_scale
        self._reset_scales_and_bgs()  # fit() will explicitly re-estimate after intensities change

    def create_no_constraints_fit(self, param_bounds_abs_min: float = 1e-6) -> PawleyPattern2DNoConstraints:
        return PawleyPattern2DNoConstraints(
            self.ws,
            self.comp_func,
            self.global_scale,
            self.lambda_max,
            param_bounds_abs_min=param_bounds_abs_min,
            apply_lorentz_correction=self.apply_lorentz_correction,
            phases=self.phases,
        )

    @staticmethod
    def _parse_fit_strategy(strategy: PawleyFitStrategy | str) -> PawleyFitStrategy:
        if isinstance(strategy, PawleyFitStrategy):
            return strategy
        try:
            return PawleyFitStrategy(str(strategy).lower())
        except ValueError as exc:
            valid = ", ".join(s.value for s in PawleyFitStrategy)
            raise ValueError(f"Unknown fit strategy '{strategy}'. Options: {valid}.") from exc

    def fit(self, *args, **kwargs) -> OptimizeResult:
        """Fit the 2D Pawley pattern with a single scale/background estimation.

        Per-spectrum scale/background values are estimated once from the
        initial parameters, then locked for the least-squares optimisation.
        Use `fit_with_strategy` for higher-level iterative strategies.
        """
        strategy_kwargs = {"fit_strategy", "strategy", "max_scale_iter", "scale_rtol"}.intersection(kwargs)
        if strategy_kwargs:
            names = ", ".join(sorted(strategy_kwargs))
            raise TypeError(
                f"PawleyPattern2D.fit() only runs a single fit and does not accept strategy options ({names}). "
                "Use fit_with_strategy(PawleyFitStrategy.ALTERNATING, ...) or fit_alternating(...) instead."
            )
        self._reestimate_scales(self.get_free_params())
        res = super().fit(*args, **kwargs)
        self.set_free_params(res.x)
        self.eval_2d(res.x)  # ensure 2D simulated workspace up to date
        return res

    def fit_with_strategy(
        self,
        strategy: PawleyFitStrategy | str = PawleyFitStrategy.SINGLE,
        *args,
        max_scale_iter: int = 2,
        scale_rtol: float = 1e-3,
        **kwargs,
    ) -> OptimizeResult:
        """Fit the 2D Pawley pattern using a named higher-level strategy.

        Parameters
        ----------
        strategy : PawleyFitStrategy or str
            ``PawleyFitStrategy.SINGLE`` calls :meth:`fit`.
            ``PawleyFitStrategy.ALTERNATING`` calls :meth:`fit_alternating`.
        max_scale_iter : int
            Maximum number of scale/background re-estimation outer iterations
            for ``PawleyFitStrategy.ALTERNATING``.
        scale_rtol : float
            Relative tolerance on the cost function between successive outer
            iterations for ``PawleyFitStrategy.ALTERNATING``.
        """
        strategy = self._parse_fit_strategy(strategy)
        if strategy == PawleyFitStrategy.SINGLE:
            return self.fit(*args, **kwargs)
        if strategy == PawleyFitStrategy.ALTERNATING:
            return self.fit_alternating(*args, max_scale_iter=max_scale_iter, scale_rtol=scale_rtol, **kwargs)
        raise AssertionError(f"Unhandled fit strategy: {strategy}")

    def fit_alternating(self, *args, max_scale_iter: int = 2, scale_rtol: float = 1e-3, **kwargs) -> OptimizeResult:
        """Alternate per-spectrum scale/background estimation and Pawley fitting."""
        if max_scale_iter < 1:
            raise ValueError("max_scale_iter must be at least 1")
        res = None
        prev_cost = None
        for _ in range(max_scale_iter):
            self._reestimate_scales(self.get_free_params())
            res = super().fit(*args, **kwargs)
            self.set_free_params(res.x)
            if prev_cost is not None and abs(prev_cost - res.cost) / max(prev_cost, 1e-12) < scale_rtol:
                break
            prev_cost = res.cost
        self.eval_2d(res.x)  # ensure 2D simulated workspace up to date
        return res


class PawleyPattern2DNoConstraints(BoundsMixin, MtdFuncMixin, Poldi2DEvalMixin, OutputTableMixin):
    def __init__(
        self,
        ws: Workspace2D,
        func: FunctionWrapper,
        global_scale: bool = True,
        lambda_max: float = 5.0,
        param_bounds_abs_min: float = 1e-6,
        apply_lorentz_correction: bool = False,
        phases: Optional[list] = None,
    ):
        self.ws = ws
        self.phases = phases
        self.ws_1d = PawleyPattern2D.make_1d_ws(self.ws)
        self.comp_func = func
        self.intens_par_name = self.get_peak_intensity_param_name()
        self.has_bg = self.comp_func[len(self.comp_func) - 1].name in FunctionFactory.Instance().getBackgroundFunctionNames()
        self.lambda_max = lambda_max
        self.apply_lorentz_correction = apply_lorentz_correction
        self.set_global_scale(global_scale)
        self.initial_params = None
        self.param_errors = np.full(self.comp_func.nParams(), np.nan)
        self.param_bounds_abs_min = param_bounds_abs_min
        self._init_bounds()

    def get_peak_intensity_param_name(self) -> str:
        pk_func = FunctionFactory.Instance().createPeakFunction(self.comp_func[0].name)
        pk_func.setIntensity(1.0)
        return next(pk_func.getParamName(ipar) for ipar in range(pk_func.nParams()) if pk_func.isExplicitlySet(ipar))

    def get_npks(self) -> int:
        return len(self.comp_func) - 1 if self.has_bg else len(self.comp_func)

    def set_global_scale(self, global_scale: bool) -> None:
        self.global_scale = global_scale
        self._reset_scales_and_bgs()
        if not self.global_scale:
            # Per-spectrum scales (locked after initialisation of each fit() call) make all
            # peak intensities identifiable — no individual intensity needs to be fixed.
            # Zero and fix any global background.
            if self.has_bg:
                npks = self.get_npks()
                bg_func = self.comp_func[npks]
                for ipar in range(bg_func.nParams()):
                    bg_func.function.setParameter(ipar, 0)
                bg_func.function.fixAll()

    def get_param_names(self) -> np.ndarray:
        return np.array([self.comp_func.parameterName(i) for i in range(self.comp_func.nParams())])

    def force_non_negative_intensities(self):
        """Enforce a lower bound of zero on all intensity parameters."""
        intens_names = [n for n in self.get_param_names() if f".{self.intens_par_name}" in n or n == self.intens_par_name]
        self.set_non_negative(intens_names)

    def get_params(self) -> np.ndarray[float]:
        return np.asarray([self.comp_func.getParameterValue(ipar) for ipar in range(self.comp_func.nParams())])

    def get_free_params(self) -> np.ndarray[float]:
        params = self.get_params()
        return params[self.get_isfree()]

    def get_isfree(self) -> np.ndarray[bool]:
        return np.asarray([not self.comp_func.isFixed(ipar) for ipar in range(self.comp_func.nParams())])

    def set_params(self, params: np.ndarray[float]):
        for ipar, val in enumerate(params):
            self.comp_func.setParameter(ipar, val)

    def set_free_params(self, free_params: np.ndarray[float]):
        params = self.get_params()
        params[self.get_isfree()] = free_params
        self.set_params(params)

    def _get_param_error_by_name(self, name: str) -> float:
        """Look up the stored error for a composite-function parameter by its full name."""
        for i in range(self.comp_func.nParams()):
            if self.comp_func.parameterName(i) == name:
                return float(self.param_errors[i])
        return float("nan")

    def _populate_table(self, tab, iphase: int):
        if self.phases:
            pk_start = sum(len(self.phases[i].hkls) for i in range(iphase))
            npks = len(self.phases[iphase].hkls)
            hkl_strings = self.phases[iphase].get_hkl_strings()
        else:
            pk_start = 0
            npks = self.get_npks()
            hkl_strings = None
        for ipk in range(npks):
            abs_ipk = pk_start + ipk
            hkl_str = hkl_strings[ipk] if hkl_strings and ipk < len(hkl_strings) else str(abs_ipk)
            pk_func = self.comp_func[abs_ipk].function
            cen_par = pk_func.getCentreParameterName()
            tab.addRow(
                {
                    "Workspace": self.ws.name(),
                    "HKL": hkl_str,
                    "I": float(pk_func.intensity()),
                    "I_err": self._get_param_error_by_name(f"f{abs_ipk}.{self.intens_par_name}"),
                    "X0": float(pk_func.getParameterValue(cen_par)),
                    "X0_err": self._get_param_error_by_name(f"f{abs_ipk}.{cen_par}"),
                    "FWHM": float(pk_func.fwhm()),
                    "FWHM_err": float("nan"),
                }
            )

    def fit(self, **kwargs) -> OptimizeResult:
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        self.initial_params = self.get_free_params()
        if "bounds" not in kwargs:
            kwargs["bounds"] = self.get_bounds()
        # Pre-compute per-spectrum scales at the initial params then lock them.
        self._reestimate_scales(self.initial_params)
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters, errors, and ensure 2D simulated workspace up to date
        self.param_errors = np.full(self.comp_func.nParams(), np.nan)
        self.param_errors[self.get_isfree()] = self.get_parameter_errors(res)
        self.eval_2d(res.x)
        return res
