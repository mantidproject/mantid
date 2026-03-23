# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations
from mantid.simpleapi import CreateSingleValuedWorkspace, LoadCIF, CreateWorkspace, EvaluateFunction, Fit, CreateEmptyTableWorkspace
import numpy as np
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from mantid.api import FunctionFactory
from mantid.geometry import CrystalStructure, ReflectionGenerator, PointGroupFactory, PointGroup
from mantid.kernel import V3D, logger, UnitConversion, DeltaEModeType
from typing import Optional, Tuple, TYPE_CHECKING, Sequence
from scipy.optimize import least_squares
from plugins.algorithms.poldi_utils import simulate_2d_data, get_dspac_array_from_ws
from abc import ABC, abstractmethod


if TYPE_CHECKING:
    from mantid.dataobjects import Workspace2D, ITableWorkspace
    from scipy.optimize import OptimizeResult


class InstrumentParams:
    def __init__(self):
        self.p: np.ndarray = np.array([1.0, 0.0])
        self.labels = ("scale", "shift")
        self.default_isfree: np.ndarray = np.zeros_like(self.p, dtype=bool)

    def get_peak_centre(self, dpk: float) -> float:
        return self.p[0] * dpk + self.p[1]


class PeakProfile(ABC):
    def __init__(self):
        self.func_name: str
        self.labels: Tuple[str]
        self.p: np.ndarray
        self.default_isfree: np.ndarray

    @abstractmethod
    def get_mantid_peak_params(self, dpk: float):
        pass


class PVProfile(PeakProfile):
    def __init__(self):
        self.func_name = "PseudoVoigt"
        self.labels = ("sig0", "sig1", "sig2", "gam0", "gam1", "gam2", "gsize", "dst2", "zeta", "fsz")
        self.p: np.ndarray = np.array([5.0e-4, 1.0e-3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
        self.default_isfree: np.ndarray = np.array([1, 1, 0, 0, 0, 0, 0, 0, 0, 0], dtype=bool)

    def get_mantid_peak_params(self, dpk: float) -> dict:
        """
        TCH pseudo-voigt profile used in Fullprof (typically convoluted with back-to-back exponentials for TOF)
        [1] FullProf Manual (http://psi.ch/sites/default/files/import/sinq/dmc/ManualsEN/fullprof.pdf)
        [2] Using FullProf to analyze Time of Flight Neutron Powder Diffraction data (https://www.ill.eu/sites/fullprof/downloads/Docs/TOF_FullProf.pdf)
        """
        # calculate FWHM
        fwhm_g = np.sqrt(
            8
            * np.log(2)
            * (
                (self.p[0] ** 2)
                + ((self.p[1] ** 2) + self.p[-3] * ((1 - self.p[-2]) ** 2)) * dpk**2
                + ((self.p[2] ** 2) + self.p[-4]) * dpk**4
            )
        )  # Panel 6 in [2] Hg**2 / (8ln2)
        fwhm_l = self.p[3] + (self.p[4] + self.p[-2] * np.sqrt(8 * np.log(2) * self.p[-3])) * dpk + (self.p[5] + self.p[-1]) * dpk**2
        fwhm_pv = (
            fwhm_g**5
            + 2.69269 * (fwhm_g**4) * (fwhm_l)
            + 2.42843 * (fwhm_g**3) * (fwhm_l**2)
            + 4.47163 * (fwhm_g**2) * (fwhm_l**3)
            + 0.07842 * (fwhm_g) * (fwhm_l**4)
            + fwhm_l**5
        ) ** (1 / 5)  # Eq.3.16 in [1]
        # calculate mixing
        fwhm_ratio = fwhm_l / fwhm_pv
        mixing = 1.36603 * fwhm_ratio - 0.47719 * fwhm_ratio**2 + 0.11116 * fwhm_ratio**3  # Eq.3.17 in [1]
        return {"Mixing": mixing, "FWHM": fwhm_pv}


class GaussianProfile(PeakProfile):
    def __init__(self):
        self.func_name = "Gaussian"
        self.labels = ("sig0", "sig1", "sig2")
        self.p: np.ndarray = np.array([5.0e-4, 1.0e-3, 0.0])
        self.default_isfree: np.ndarray = np.array([1, 1, 0], dtype=bool)

    def get_mantid_peak_params(self, dpk: float) -> dict:
        """
        Gasusian profile from pseudo-voigt profile used in Fullprof with zeta=0
        Note dst2 and gsize perfectly corralated withsig1 and sig2 respectively
        [1] Using FullProf to analyze Time of Flight Neutron Powder Diffraction data (https://www.ill.eu/sites/fullprof/downloads/Docs/TOF_FullProf.pdf)
        """
        return {"Sigma": np.sqrt((self.p[0] ** 2) + (self.p[1] ** 2) * dpk**2 + (self.p[2] ** 2) * dpk**4)}  # Panel 6 in [1] with zeta=0


class BackToBackGauss(PeakProfile):
    def __init__(self):
        self.func_name = "BackToBackExponential"
        self.labels = ("sig0", "sig1", "sig2", "alpha_0", "alpha_1", "beta_0", "beta_1")
        self.p: np.ndarray = np.array([0, 9.06, 6.52, 0, 0.0968, 0.0216, 0.0123])  # for ENGIN-X North Bank
        self.default_isfree: np.ndarray = np.zeros_like(self.p, dtype=bool)

    def get_mantid_peak_params(self, dpk: float) -> dict:
        return {"S": self._calc_sigma(dpk), "A": self._calc_alpha(dpk), "B": self._calc_beta(dpk)}

    def _calc_sigma(self, dpk: float) -> float:
        return np.sqrt((self.p[0] ** 2) + (self.p[1] * dpk) ** 2 + (self.p[2] ** 2) * dpk**4)

    def _calc_alpha(self, dpk: float) -> float:
        return self.p[3] + self.p[4] / dpk

    def _calc_beta(self, dpk: float) -> float:
        return self.p[5] + self.p[6] / (dpk**4)


class Phase:
    def __init__(self, crystal_structure: CrystalStructure, hkls: Optional[np.ndarray] = None):
        self.unit_cell = crystal_structure.getUnitCell()
        # check initial unit cell compatible with spacegroup
        self.spgr = crystal_structure.getSpaceGroup()
        if not self.spgr.isAllowedUnitCell(self.unit_cell):
            raise ValueError("Unit cell not compatible with spacegroup")
        # add hkls - check if compatible wth spacegorup
        if hkls is not None:
            self.set_hkls(hkls)
        # set free parameters beased on lattice system
        self.param_names = np.array(["a", "b", "c", "alpha", "beta", "gamma"])
        self.alatt = self._get_alatt()
        ptgr = PointGroupFactory.createPointGroupFromSpaceGroup(self.spgr)
        match ptgr.getLatticeSystem():
            case PointGroup.LatticeSystem.Cubic:
                # a=b=c, alpha=beta=gamma=90
                self.ipars = [slice(0, 3)]  # a
            case PointGroup.LatticeSystem.Tetragonal | PointGroup.LatticeSystem.Hexagonal:
                # a=b!=c (tetrag has alpha=beta=gamma=90, hexag has alpha=beta=90, gamma= 120)
                self.ipars = [slice(0, 2), 2]  # a, c
            case PointGroup.LatticeSystem.Orthorhombic:
                # alpha=beta=gamma=90
                self.ipars = range(3)  # a, b, c
            case PointGroup.LatticeSystem.Rhombohedral:
                # a=b=c, alpha=beta=gamma
                self.ipars = [slice(0, 3), slice(3, 6)]  # a, alpha
            case PointGroup.LatticeSystem.Monoclinic:
                # a!=b!=c, alpha=gamma=90
                self.ipars = [0, 1, 2, 4]  # a, b, c, beta
            case _:  # Triclinic
                self.ipars = range(len(self.alatt))  # a, b, c, alpha, beta, gamma

    @classmethod
    def from_cif(cls, cif_file: str):
        ws = CreateSingleValuedWorkspace(StoreInADS=False, EnableLogging=False)
        LoadCIF(ws, cif_file, StoreInADS=False)
        return Phase(ws.sample().getCrystalStructure())

    @classmethod
    def from_alatt(cls, alatt: np.ndarray, spgr: str = "P 1", basis: str = ""):
        alatt_str = " ".join([str(par) for par in alatt])
        xtal = CrystalStructure(alatt_str, spgr, basis)
        return Phase(xtal)

    def _get_alatt(self) -> np.ndarray[float]:
        return np.array([getattr(self.unit_cell, method)() for method in self.param_names])

    def get_params(self) -> np.ndarray[float]:
        return np.array([self.alatt[ipar][0] if isinstance(ipar, slice) else self.alatt[ipar] for ipar in self.ipars])

    def get_param_names(self) -> np.ndarray[str]:
        return np.array([self.param_names[ipar][0] if isinstance(ipar, slice) else self.param_names[ipar] for ipar in self.ipars])

    def set_params(self, pars: np.ndarray[float]):
        for ipar, par in enumerate(pars):
            self.alatt[self.ipars[ipar]] = par
        self.unit_cell.set(*self.alatt)

    def set_hkls(self, hkls: Sequence[np.ndarray], do_sort: bool = True):
        self.hkls = []
        for hkl in hkls:
            hkl_vec = V3D(*hkl)
            if self.spgr.isAllowedReflection(hkl_vec):
                self.hkls.append(hkl_vec)
            else:
                logger.warning(f"Reflection {hkl} not allowed by spacegroup")
        if do_sort:
            # sort by descending d-spacing
            self.hkls = [self.hkls[ipk] for ipk in np.argsort(self.calc_dspacings())[::-1]]

    def set_hkls_from_dspac_limits(self, dmin: float, dmax: float):
        xtal = CrystalStructure(" ".join([str(par) for par in self.alatt]), self.spgr.getHMSymbol(), "")
        generator = ReflectionGenerator(xtal)
        self.set_hkls(generator.getUniqueHKLs(dmin, dmax))

    def calc_dspacings(self) -> np.ndarray[float]:
        return np.array([self.unit_cell.d(hkl) for hkl in self.hkls])

    def nhkls(self) -> int:
        return len(self.hkls)

    def nparams(self) -> int:
        return len(self.ipars)

    def merge_reflections(self, decimal_places=4):
        _, ihkls = np.unique((self.calc_dspacings() * (10**decimal_places)).astype(int), return_index=True)
        self.hkls = [self.hkls[ipk] for ipk in np.sort(ihkls)]

    def filter_hkls_to_ws_range(self, ws, lambda_min: float = 1.1, lambda_max: float = 5.0) -> "Phase":
        """Return a new Phase containing only HKLs whose d-spacing falls within the
        accessible range of *ws* (determined from its angular coverage and the given
        wavelength limits).  The Phase lattice and space-group are preserved."""
        dspacs_all = get_dspac_array_from_ws(ws, lambda_min=lambda_min, lambda_max=lambda_max)
        dmin, dmax = float(dspacs_all.min()), float(dspacs_all.max())
        new_phase = Phase(
            CrystalStructure(
                " ".join(str(p) for p in self.alatt),
                self.spgr.getHMSymbol(),
                "",
            )
        )
        accessible_hkls = [hkl for hkl in self.hkls if dmin <= self.unit_cell.d(hkl) <= dmax]
        if accessible_hkls:
            new_phase.set_hkls(accessible_hkls, do_sort=False)
        else:
            new_phase.hkls = []
        return new_phase

    def get_hkl_strings(self) -> Sequence[str]:
        return ["".join(str(int(round(v))) for v in [hkl.X(), hkl.Y(), hkl.Z()]) for hkl in self.hkls]


class MtdFuncMixin:
    """
    Methods used to interact with mantid composite function for unconstrained fits in PawleyPattern1D and
    PawleyPattern2DNoConstraints (although the getters may also be helpful for debugging PawleyPattern2D fits
    """

    def get_peak_centres(self) -> np.ndarray[float]:
        cen_par_name = self.comp_func[0].function.getCentreParameterName()
        return self.get_peak_params(cen_par_name)

    def get_peak_params(self, param_name: str) -> np.ndarray[float]:
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.getParameterValue(param_name) for f in self.comp_func if f.name not in bg_func_names])

    def get_peak_fwhm(self) -> np.ndarray[float]:
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.fwhm() for f in self.comp_func if f.name not in bg_func_names])

    def get_peak_intensities(self) -> np.ndarray[float]:
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.intensity() for f in self.comp_func if f.name not in bg_func_names])

    def set_mantid_peak_param_isfree(self, param_names: str | Sequence[str], isfree: bool = False):
        # relevant only for subsequent unconstrained fits - not used in Pawley fits
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        if isinstance(param_names, str):
            param_names = [param_names]  #  force ot be list with single element
        for func in self.comp_func:
            if func.name not in bg_func_names:
                for param_name in param_names:
                    if isfree:
                        func.free(param_name)
                    else:
                        func.fix(param_name)


class OutputTableMixin:
    """Mixin that provides ``create_output_table`` to any Pawley-fit class.

    Concrete classes must implement ``_iter_peak_rows`` which yields one row-dict
    per fitted peak.  The mixin also owns ``get_parameter_errors`` so both
    constrained and unconstrained fit classes share the same error computation.
    """

    @staticmethod
    def get_parameter_errors(res: "OptimizeResult", cond_thresh: float = 1e8) -> np.ndarray:
        hessian = res.jac.T @ res.jac
        cond_num = np.linalg.cond(hessian)
        if not np.isfinite(cond_num) or cond_num > cond_thresh:
            covar = np.linalg.pinv(hessian)
            logger.warning(f"Hessian is ill-conditioned (cond={cond_num:.2e}) using pseudoinverse")
        else:
            covar = np.linalg.inv(hessian)
        ndat, npar = res.jac.shape
        var = np.sum(res.fun**2) / (ndat - npar)
        return np.sqrt(np.diag(var * covar))

    def create_output_table(self, res: "OptimizeResult", output_workspace: str = None) -> "ITableWorkspace":
        """Create a TableWorkspace with one row per fitted peak.

        Columns: ``Workspace``, ``Spectrum``, ``HKL``, ``I``, ``I_err``,
        ``X0``, ``X0_err``, ``FWHM``, ``FWHM_err``.
        """
        param_errors = self.get_parameter_errors(res)
        isfree = self.get_isfree()
        free_idx_of_full = np.full(len(isfree), -1, dtype=int)
        free_idx_of_full[isfree] = np.arange(int(isfree.sum()))

        ws_name = output_workspace or f"{self.ws.name()}_pawley_table"
        tab = CreateEmptyTableWorkspace(OutputWorkspace=ws_name, EnableLogging=False)
        tab.addColumn(type="str", name="Workspace")
        tab.addColumn(type="int", name="Spectrum")
        tab.addColumn(type="str", name="HKL")
        tab.addColumn(type="double", name="I")
        tab.addColumn(type="double", name="I_err")
        tab.addColumn(type="double", name="X0")
        tab.addColumn(type="double", name="X0_err")
        tab.addColumn(type="double", name="FWHM")
        tab.addColumn(type="double", name="FWHM_err")

        if hasattr(self, "update_profile_function"):
            self.update_profile_function()
        for row in self._iter_peak_rows(param_errors, free_idx_of_full):
            tab.addRow(row)
        return tab

    def _iter_peak_rows(self, param_errors: np.ndarray, free_idx_of_full: np.ndarray):
        raise NotImplementedError()


class PawleyPatternBase(MtdFuncMixin, OutputTableMixin, ABC):
    def __init__(
        self,
        ws: Workspace2D,
        phases: Phase,
        profile: PeakProfile,
        bg_func: Optional[FunctionWrapper] = None,
        global_param_bound_frac: float = None,
        param_bound_frac: dict = None,
        param_bounds_abs_min: float = 1e-6,
        ispec: int = 0,
    ):
        self.ws = ws
        self.ispec = ispec
        self.phases = phases
        self.alatt_params = [phase.get_params() for phase in self.phases]
        self.alatt_isfree = [np.ones_like(alatt, dtype=bool) for alatt in self.alatt_params]
        self.profile = profile
        self.profile_params = [self.profile.p.copy() for _ in self.phases]
        self.profile_isfree = [self.profile.default_isfree.copy() for _ in self.phases]
        self.peak_func = FunctionFactory.Instance().createPeakFunction(self.profile.func_name)
        self.intens = [np.ones(len(phase.hkls), dtype=float) for phase in self.phases]
        self.intens_isfree = [np.ones_like(pars, dtype=bool) for pars in self.intens]
        self.inst = InstrumentParams()
        self.inst_params = self.inst.p.copy()
        self.inst_isfree = self.inst.default_isfree.copy()
        self.bg_params = []
        if bg_func is not None:
            self.bg_params = np.array([bg_func.function.getParamValue(ipar) for ipar in range(bg_func.nParams())])
        self.bg_isfree = np.ones_like(self.bg_params, dtype=bool)
        self.make_profile_function(bg_func)
        self.initial_params = None
        self.global_param_bound_frac = global_param_bound_frac
        self.param_bounds_abs_min = param_bounds_abs_min
        self.param_bound_frac = param_bound_frac

    def make_profile_function(self, bg_func: Optional[FunctionWrapper] = None):
        self.comp_func = CompositeFunctionWrapper()
        for phase in self.phases:
            for _ in range(phase.nhkls()):
                self.comp_func += FunctionWrapper(FunctionFactory.Instance().createPeakFunction(self.profile.func_name))
        if bg_func is not None:
            self.comp_func += bg_func
        self.comp_func.function.setAttributeValue("NumDeriv", True)

    def update_profile_function(self):
        self.profile.p = self.profile_params
        self.inst.p = self.inst_params
        istart = 0
        for iphase, phase in enumerate(self.phases):
            self.profile.p = self.profile_params[iphase]
            # set alatt for phase
            phase.set_params(self.alatt_params[iphase])
            dpks = self.inst.get_peak_centre(phase.calc_dspacings())  # apply scale and shift to calculated d
            pk_cens = self._get_peak_cens(dpks)  # could involve unit conversion etc.
            for ipk, dpk in enumerate(dpks):
                self.comp_func[istart + ipk].function.setCentre(pk_cens[ipk])
                for par_name, val in self.profile.get_mantid_peak_params(dpk).items():
                    self.comp_func[istart + ipk][par_name] = val
                self.comp_func[istart + ipk].function.setIntensity(self.intens[iphase][ipk])
            istart += phase.nhkls()
        if len(self.bg_params) > 0:
            [self.comp_func[len(self.comp_func) - 1].function.setParameter(ipar, par) for ipar, par in enumerate(self.bg_params)]

    def _get_peak_cens(self, dpks: np.ndarray[float]) -> np.ndarray[float]:
        return dpks

    def get_param_names(self) -> np.ndarray:
        """Return a name for every parameter in the same order as get_params()."""
        names = []
        for i, phase in enumerate(self.phases):
            phase_suffix = f"_ph{i}" if len(self.phases) > 1 else ""
            names.extend(n + phase_suffix for n in phase.get_param_names())
        for i, phase in enumerate(self.phases):
            phase_suffix = f"_ph{i}" if len(self.phases) > 1 else ""
            names.extend(s + phase_suffix for s in phase.get_hkl_strings())
        for i in range(len(self.phases)):
            phase_suffix = f"_ph{i}" if len(self.phases) > 1 else ""
            names.extend(lbl + phase_suffix for lbl in self.profile.labels)
        names.extend(self.inst.labels)
        if len(self.bg_params) > 0:
            bg_func = self.comp_func[len(self.comp_func) - 1]
            names.extend(bg_func.function.parameterName(j) for j in range(int(bg_func.nParams())))
        return np.array(names, dtype=str)

    def get_free_param_names(self) -> np.ndarray:
        return self.get_param_names()[self.get_isfree()]

    def get_params(self) -> np.ndarray[float]:
        return np.concatenate((*self.alatt_params, *self.intens, *self.profile_params, self.inst_params, self.bg_params))

    def get_free_params(self) -> np.ndarray[float]:
        return self.get_params()[self.get_isfree()]

    def get_isfree(self) -> np.ndarray[bool]:
        return np.concatenate((*self.alatt_isfree, *self.intens_isfree, *self.profile_isfree, self.inst_isfree, self.bg_isfree)).astype(
            bool
        )

    def set_params(self, params: np.ndarray[float]):
        # set alatt
        istart = 0
        for iphase, phase in enumerate(self.phases):
            npars = phase.nparams()
            self.alatt_params[iphase] = params[istart : istart + npars]
            istart = istart + npars
        # set peak parameters
        for iphase, phase in enumerate(self.phases):
            npars = self.intens[iphase].size
            self.intens[iphase] = params[istart : istart + npars]
            istart = istart + npars
        # set profile parameters
        for iphase, phase in enumerate(self.phases):
            npars = self.profile_params[iphase].size
            self.profile_params[iphase] = params[istart : istart + npars]
            istart = istart + npars
        # set instrument params
        iend = istart + len(self.inst_params)
        self.inst_params = params[istart:iend]
        # set global profile and bg_func params
        if len(self.bg_params) > 0:
            self.bg_params = params[iend:]

    def set_free_params(self, free_params: np.ndarray[float]):
        params = self.get_params()
        params[self.get_isfree()] = free_params
        self.set_params(params)

    def fit(self, **kwargs) -> OptimizeResult:
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        self.initial_params = self.get_free_params()
        if "bounds" not in kwargs:
            lb = np.full_like(self.initial_params, -np.inf)
            ub = np.full_like(self.initial_params, np.inf)
            if self.global_param_bound_frac is not None:
                margin = np.maximum(np.abs(self.initial_params) * self.global_param_bound_frac, self.param_bounds_abs_min)
                lb = self.initial_params - margin
                ub = self.initial_params + margin
            # apply per-parameter overrides (None entry → unconstrained for that param)
            if self.param_bound_frac:
                free_names = self.get_free_param_names()
                for name, frac in self.param_bound_frac.items():
                    mask = free_names == name
                    if not mask.any():
                        logger.warning(f"param_bound_frac: '{name}' not found in free parameters, ignoring.")
                        continue
                    if frac is None:
                        lb[mask] = -np.inf
                        ub[mask] = np.inf
                    else:
                        margin = np.maximum(np.abs(self.initial_params[mask]) * frac, self.param_bounds_abs_min)
                        lb[mask] = self.initial_params[mask] - margin
                        ub[mask] = self.initial_params[mask] + margin
            # always enforce non-negativity for intensity parameters
            n_alatt_free = sum(int(ai.sum()) for ai in self.alatt_isfree)
            n_intens_free = sum(int(ii.sum()) for ii in self.intens_isfree)
            lb[n_alatt_free : n_alatt_free + n_intens_free] = np.maximum(lb[n_alatt_free : n_alatt_free + n_intens_free], 0.0)
            kwargs["bounds"] = (lb, ub)
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters
        self.set_free_params(res.x)
        self.update_profile_function()
        return res

    def undo_fit(self):
        if self.initial_params is not None:
            self.set_free_params(self.initial_params)

    def _iter_peak_rows(self, param_errors: np.ndarray, free_idx_of_full: np.ndarray):
        n_alatt = sum(len(a) for a in self.alatt_params)
        n_intens_per_phase = [len(i) for i in self.intens]
        intens_start = n_alatt
        pk_offset = 0
        for iphase, phase in enumerate(self.phases):
            phase_intens_start = intens_start + sum(n_intens_per_phase[:iphase])
            for ipk, hkl in enumerate(phase.hkls):
                hkl_str = "".join(str(int(round(v))) for v in [hkl.X(), hkl.Y(), hkl.Z()])
                fidx = int(free_idx_of_full[phase_intens_start + ipk])
                pk_func = self.comp_func[pk_offset + ipk].function
                cen_par = pk_func.getCentreParameterName()
                yield {
                    "Workspace": self.ws.name(),
                    "Spectrum": self.ispec,
                    "HKL": hkl_str,
                    "I": float(self.intens[iphase][ipk]),
                    "I_err": float(param_errors[fidx]) if fidx >= 0 else float("nan"),
                    "X0": float(pk_func.getParameterValue(cen_par)),
                    "X0_err": float("nan"),
                    "FWHM": float(pk_func.fwhm()),
                    "FWHM_err": float("nan"),
                }
            pk_offset += phase.nhkls()

    @abstractmethod
    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        raise NotImplementedError()

    @abstractmethod
    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        raise NotImplementedError()


class PawleyPattern1D(PawleyPatternBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.xunit = self.ws.getAxis(0).getUnit().unitID()
        if self.xunit == "TOF":
            si = self.ws.spectrumInfo()
            if not si.hasDetectors(self.ispec):
                raise RuntimeError("Workspace has no detectors - cannot convert between TOF and d-spacing.")
        self.update_profile_function()

    def _get_peak_cens(self, dpks: np.ndarray[float]) -> np.ndarray[float]:
        if self.xunit == "TOF":
            return self._convert_dspac_to_tof(dpks)
        return dpks

    def _convert_dspac_to_tof(self, dpks: np.ndarray[float]) -> np.ndarray[float]:
        diff_consts = self.ws.spectrumInfo().diffractometerConstants(self.ispec)
        return np.asarray([UnitConversion.run("dSpacing", "TOF", dpk, 0, DeltaEModeType.Elastic, diff_consts) for dpk in dpks])

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws.readX(self.ispec))

    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        return self.ws.readY(self.ispec) - self.eval_profile(params)

    def get_eval_workspace(self, **kwargs):
        if "OutputWorkspace" not in kwargs:
            kwargs["OutputWorkspace"] = f"{self.ws.name()}_eval"
        return EvaluateFunction(Function=self.comp_func, InputWorkspace=self.ws, WorkspaceIndex=self.ispec, **kwargs)

    def fit_no_constraints(self, **kwargs):
        # select cost function
        default_kwargs = {
            "Minimizer": "Levenberg-Marquardt",
            "MaxIterations": 500,
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": True,
            "CreateOutput": True,
            "OutputCompositeMembers": False,
            "CostFunction": "Unweighted least squares",
        }
        kwargs = {**default_kwargs, **kwargs}
        res = Fit(Function=self.comp_func, InputWorkspace=self.ws, WorkspaceIndex=self.ispec, **kwargs)
        # upate parameters in comp_func (can't replace comp_func as result doesn't return individual IPEakFunctions)
        [self.comp_func.setParameter(ipar, res.Function.getParamValue(ipar)) for ipar in range(self.comp_func.nParams())]
        return res

    def estimate_initial_params(self):
        ws_eval = self.get_eval_workspace(StoreInADS=False)
        ppval = np.polyfit(ws_eval.readY(1), ws_eval.readY(0), 1)
        scale, bg = ppval
        for iphase in range(len(self.phases)):
            self.intens[iphase] = self.intens[iphase] * scale
            self.intens[iphase][self.intens[iphase] < 1e-8] = 1e-8  # arbitrary small number
        if len(self.bg_params) > 0:
            if np.allclose(self.bg_params, 0) and self.comp_func[len(self.comp_func) - 1].function.hasParameter("A0"):
                # try and set background (assume hasn't been previously fitted)
                ipar = self.comp_func[len(self.comp_func) - 1].function.getParameterIndex("A0")
                self.bg_params[ipar] = bg
        self.update_profile_function()

    def fit_background(self, **kwargs) -> OptimizeResult:
        if len(self.bg_params) == 0:
            return
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        res = least_squares(self._calc_robust_bg_resids, self.bg_params, **kwargs)
        # set bg params
        self._set_func_params(self.comp_func[len(self.comp_func) - 1], res.x)
        self.bg_params = res.x
        return res

    def get_bkg_ws(self, ws_name: str):
        bg_func = self.comp_func[len(self.comp_func) - 1]
        self._set_func_params(bg_func, self.bg_params)
        return EvaluateFunction(
            Function=bg_func, InputWorkspace=self.ws, WorkspaceIndex=self.ispec, OutputWorkspace=ws_name, EnableLogging=False
        )

    def _eval_bg_func(self, p: np.ndarray[float]) -> np.ndarray[float]:
        bg_func = self.comp_func[len(self.comp_func) - 1]
        self._set_func_params(bg_func, p)
        ws = EvaluateFunction(Function=bg_func, InputWorkspace=self.ws, WorkspaceIndex=self.ispec, StoreInADS=False, EnableLogging=False)
        return ws.readY(1).copy()

    @staticmethod
    def _set_func_params(func: FunctionWrapper, p: np.ndarray[float]):
        [func.function.setParameter(ipar, param) for ipar, param in enumerate(p)]

    def _calc_robust_bg_resids(self, p: np.ndarray[float]) -> np.ndarray[float]:
        # quadratic for negative residuals, scaled cauchy loss for positive (turnover to logarithmic at ~ 2 sigma)
        # note at x=0 cauchy loss has second-deriv == 2 (i.e. is quadratic)
        bg_calc = self._eval_bg_func(p)
        resids = (self.ws.readY(self.ispec) - bg_calc) / self.ws.readE(self.ispec)
        ipos = resids > 0
        resids[ipos] = np.sign(resids[ipos]) * 2 * np.sqrt(np.log(1 + (resids[ipos] / 2) ** 2))
        resids[~np.isfinite(resids)] = 0  # ignore empty bins
        return resids


class Poldi2DEvalMixin:
    """
    Methods relating to 2D POLDI workspace evaluation/simulation used in PawleyPattern2D and
    PawleyPattern2DNoConstraints classes
    """

    def eval_2d(self, params: np.ndarray[float]) -> Workspace2D:
        self.ws_1d.setY(0, self.eval_profile(params))
        ws_sim = simulate_2d_data(self.ws, self.ws_1d, output_workspace=f"{self.ws.name()}_sim", lambda_max=self.lambda_max)
        if self.apply_lorentz_correction:
            si = ws_sim.spectrumInfo()
            for ispec in range(ws_sim.getNumberHistograms()):
                ws_sim.setY(ispec, ws_sim.readY(ispec) * np.sin(si.twoTheta(ispec) / 2))
        if not self.global_scale:
            if self.scales is None:
                # First call: estimate a per-spectrum background from the lower tail of the
                # observed time series, then compute a scale from the background-subtracted
                # signal.  Using a data-derived background (rather than fitting one from the
                # simulation) means the scale only corrects for detector efficiency/geometry
                # and does NOT absorb any peak-intensity or texture information.
                # The per-spectrum bg is then locked alongside the scale for the rest of
                # optimisation, so the Jacobian remains dr/dI_k = -scale * d_ycalc/dI_k.
                self.scales = np.zeros(self.ws.getNumberHistograms())
                self.bgs = np.zeros_like(self.scales)
                for ispec in range(self.ws.getNumberHistograms()):
                    yobs = self.ws.readY(ispec)
                    ycalc = ws_sim.readY(ispec)
                    bg = float(np.percentile(yobs, self.bg_percentile))
                    yobs_bg_sub = yobs - bg
                    denom = float(np.dot(ycalc, ycalc))
                    scale = float(np.dot(ycalc, yobs_bg_sub)) / denom if denom > 0 else 1.0
                    self.scales[ispec] = scale
                    self.bgs[ispec] = bg
                    # include bg in ws_sim so eval_resids = yobs - ws_sim = (yobs-bg) - scale*ycalc
                    ws_sim.setY(ispec, scale * ycalc + bg)
            else:
                # Subsequent calls: both scale and bg are locked.
                for ispec in range(self.ws.getNumberHistograms()):
                    ws_sim.setY(ispec, self.scales[ispec] * ws_sim.readY(ispec) + self.bgs[ispec])
        return ws_sim

    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        ws_sim = self.eval_2d(params)
        return (self.ws.extractY() - ws_sim.extractY()).flat

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        return self.comp_func(self.ws_1d.readX(0))


class PawleyPattern2D(Poldi2DEvalMixin, PawleyPatternBase):
    def __init__(
        self,
        *args,
        global_scale: bool = True,
        lambda_max: float = 5.0,
        apply_lorentz_correction: bool = False,
        bg_percentile: float = 15.0,
        **kwargs,
    ):
        super().__init__(*args, **kwargs)
        self.lambda_max = lambda_max
        self.scales = None
        self.bgs = None
        self.apply_lorentz_correction = apply_lorentz_correction
        self.bg_percentile = bg_percentile
        self.set_global_scale(global_scale)
        # create workspace in d-spacing to contain diffraction pattern
        self.ws_1d = self.make_1d_ws(self.ws)
        self.update_profile_function()

    @staticmethod
    def make_1d_ws(ws: Workspace2D):
        dspacs = get_dspac_array_from_ws(ws)
        return CreateWorkspace(
            DataX=dspacs,
            DataY=np.zeros_like(dspacs),
            UnitX="dSpacing",
            YUnitLabel="Intensity (a.u.)",
            OutputWorkspace=f"{ws.name()}_pattern",
            EnableLogging=False,
        )

    def set_global_scale(self, global_scale: bool):
        self.global_scale = global_scale
        if not self.global_scale:
            # Per-spectrum scales (locked after initialisation of each fit() call) make all
            # peak intensities identifiable without needing to fix any individual intensity.
            # Fixing the most-intense peak anchors the whole scale to what may be the most
            # biased value from the 1D autocorrelation, so that practice is avoided here.
            # Zero and fix any global background — background is handled per-spectrum in eval_2d.
            if len(self.bg_params) > 0:
                self.bg_params[:] = 0
                self.bg_isfree[:] = False

    def set_params_from_pawley1d(
        self,
        pawley1d: PawleyPattern1D,
        global_param_bound_frac: float = None,
        param_bounds_abs_min: float = None,
        param_bound_frac: dict = None,
    ):
        if len(pawley1d.phases) != len(self.phases):
            logger.error("PawleyPattern1D object has a different number of phases.")
            return
        if pawley1d.profile.func_name == self.profile.func_name:
            self.profile_params = [p.copy() for p in pawley1d.profile_params]
        # Copy refined lattice parameters from the 1D fit so the 2D fit starts from
        # the globally-constrained lattice and cannot drift to unphysical values if a
        # peak is weakly constrained in a single sub-group.
        for iphase in range(len(self.phases)):
            self.alatt_params[iphase] = pawley1d.alatt_params[iphase].copy()
        intens_changed = False
        for iphase, phase in enumerate(self.phases):
            phase1d = pawley1d.phases[iphase]
            if phase.nhkls() == phase1d.nhkls():
                # identical set of HKLs — copy directly
                self.intens[iphase] = pawley1d.intens[iphase].copy()
                intens_changed = True
            elif phase.nhkls() < phase1d.nhkls():
                # 2D phase is a d-spacing-filtered subset: match each 2D HKL to the
                # closest 1D d-spacing and copy the corresponding intensity.
                dspacs_1d = phase1d.calc_dspacings()
                dspacs_2d = phase.calc_dspacings()
                new_intens = np.empty(phase.nhkls(), dtype=float)
                for ipk, dpk in enumerate(dspacs_2d):
                    idx = int(np.argmin(np.abs(dspacs_1d - dpk)))
                    new_intens[ipk] = pawley1d.intens[iphase][idx]
                self.intens[iphase] = new_intens
                intens_changed = True
        if intens_changed:
            self._estimate_intensities()
        if global_param_bound_frac is not None:
            self.global_param_bound_frac = global_param_bound_frac
        if param_bounds_abs_min is not None:
            self.param_bounds_abs_min = param_bounds_abs_min
        if param_bound_frac is not None:
            self.param_bound_frac = param_bound_frac

    def estimate_initial_params(self):
        self._estimate_intensities()

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws_1d.readX(0))

    def _estimate_intensities(self):
        # Compute per-spectrum scales at current intensities and use their median as a global
        # correction factor.  The median is robust to high-texture detectors and avoids the
        # background-dominated bias of a naive polyfit across all channels.
        self.scales = None  # ensure fresh per-spectrum scale computation
        _ = self.eval_2d(self.get_free_params())
        if self.scales is not None and len(self.scales) > 0:
            valid_scales = self.scales[self.scales > 0]
            global_scale = float(np.median(valid_scales)) if len(valid_scales) > 0 else 1.0
            for iphase in range(len(self.phases)):
                self.intens[iphase] *= global_scale
        self.scales = None  # reset so fit() recomputes scales at the updated intensities

    def create_no_constriants_fit(self, global_param_bound_frac: float = None, param_bounds_abs_min: float = 1e-6):
        return PawleyPattern2DNoConstraints(
            self.ws,
            self.comp_func,
            self.global_scale,
            self.lambda_max,
            global_param_bound_frac=global_param_bound_frac,
            param_bounds_abs_min=param_bounds_abs_min,
            apply_lorentz_correction=self.apply_lorentz_correction,
            bg_percentile=self.bg_percentile,
            phases=self.phases,
            ispec=self.ispec,
        )

    def fit(self, *args, **kwargs) -> OptimizeResult:
        # Reset scales so eval_2d recomputes them at the current parameter values,
        # then lock them for the duration of the optimisation.
        self.scales = None
        self.eval_2d(self.get_free_params())
        res = super().fit(*args, **kwargs)
        self.eval_2d(res.x)  # ensure 2D simulated workspace up to date
        return res


class PawleyPattern2DNoConstraints(MtdFuncMixin, Poldi2DEvalMixin, OutputTableMixin):
    def __init__(
        self,
        ws: Workspace2D,
        func: FunctionWrapper,
        global_scale: bool = True,
        lambda_max: float = 5.0,
        global_param_bound_frac: float = None,
        param_bounds_abs_min: float = 1e-6,
        apply_lorentz_correction: bool = False,
        bg_percentile: float = 15.0,
        phases: Optional[list] = None,
        ispec: int = 0,
    ):
        self.ws = ws
        self.ispec = ispec
        self.phases = phases
        self.ws_1d = PawleyPattern2D.make_1d_ws(self.ws)
        self.comp_func = func
        self.intens_par_name = self.get_peak_intensity_param_name()
        self.has_bg = self.comp_func[len(self.comp_func) - 1].name in FunctionFactory.Instance().getBackgroundFunctionNames()
        self.lambda_max = lambda_max
        self.scales = None
        self.bgs = None
        self.apply_lorentz_correction = apply_lorentz_correction
        self.bg_percentile = bg_percentile
        self.set_global_scale(global_scale)
        self.initial_params = None
        self.global_param_bound_frac = global_param_bound_frac
        self.param_bounds_abs_min = param_bounds_abs_min

    def get_peak_intensity_param_name(self):
        pk_func = FunctionFactory.Instance().createPeakFunction(self.comp_func[0].name)
        pk_func.setIntensity(1.0)
        return next(pk_func.getParamName(ipar) for ipar in range(pk_func.nParams()) if pk_func.isExplicitlySet(ipar))

    def get_npks(self) -> int:
        return len(self.comp_func) - 1 if self.has_bg else len(self.comp_func)

    def set_global_scale(self, global_scale: bool):
        self.global_scale = global_scale
        if not self.global_scale:
            # Per-spectrum scales (locked after initialisation of each fit() call) make all
            # peak intensities identifiable — no individual intensity needs to be fixed.
            # Zero and fix any global background.
            if self.has_bg:
                npks = self.get_npks()
                bg_func = self.comp_func[npks]
                [bg_func.function.setParameter(ipar, 0) for ipar in range(bg_func.nParams())]
                bg_func.function.fixAll()

    def get_params(self) -> np.ndarray[float]:
        return np.asarray([self.comp_func.getParameterValue(ipar) for ipar in range(self.comp_func.nParams())])

    def get_free_params(self) -> np.ndarray[float]:
        params = self.get_params()
        return params[self.get_isfree()]

    def get_isfree(self) -> np.ndarray[bool]:
        return np.asarray([not self.comp_func.isFixed(ipar) for ipar in range(self.comp_func.nParams())])

    def set_params(self, params: np.ndarray[float]):
        [self.comp_func.setParameter(ipar, val) for ipar, val in enumerate(params)]

    def set_free_params(self, free_params: np.ndarray[float]):
        params = self.get_params()
        params[self.get_isfree()] = free_params
        self.set_params(params)

    def _iter_peak_rows(self, param_errors: np.ndarray, free_idx_of_full: np.ndarray):
        name_to_full = {self.comp_func.parameterName(i): i for i in range(self.comp_func.nParams())}
        npks = self.get_npks()
        hkl_strings = self.phases[0].get_hkl_strings() if self.phases else None
        for ipk in range(npks):
            hkl_str = hkl_strings[ipk] if hkl_strings and ipk < len(hkl_strings) else str(ipk)
            pk_func = self.comp_func[ipk].function
            cen_par = pk_func.getCentreParameterName()

            def _err(par_name):
                full_idx = name_to_full.get(f"f{ipk}.{par_name}", -1)
                fidx = int(free_idx_of_full[full_idx]) if full_idx >= 0 else -1
                return float(param_errors[fidx]) if fidx >= 0 else float("nan")

            yield {
                "Workspace": self.ws.name(),
                "Spectrum": self.ispec,
                "HKL": hkl_str,
                "I": float(pk_func.intensity()),
                "I_err": _err(self.intens_par_name),
                "X0": float(pk_func.getParameterValue(cen_par)),
                "X0_err": _err(cen_par),
                "FWHM": float(pk_func.fwhm()),
                "FWHM_err": float("nan"),
            }

    def fit(self, **kwargs) -> OptimizeResult:
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        self.initial_params = self.get_free_params()
        if "bounds" not in kwargs:
            lb = np.full_like(self.initial_params, -np.inf)
            ub = np.full_like(self.initial_params, np.inf)
            if self.global_param_bound_frac is not None:
                margin = np.maximum(np.abs(self.initial_params) * self.global_param_bound_frac, self.param_bounds_abs_min)
                lb = self.initial_params - margin
                ub = self.initial_params + margin
            # enforce non-negativity for intensity parameters identified by name
            all_param_names = [self.comp_func.parameterName(ipar) for ipar in range(self.comp_func.nParams())]
            isfree = self.get_isfree()
            free_param_names = [name for name, free in zip(all_param_names, isfree) if free]
            is_intens_free = np.array([f".{self.intens_par_name}" in name or name == self.intens_par_name for name in free_param_names])
            lb[is_intens_free] = np.maximum(lb[is_intens_free], 0.0)
            kwargs["bounds"] = (lb, ub)
        # Pre-compute per-spectrum scales at the initial params then lock them.
        self.scales = None
        self.eval_2d(self.initial_params)
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters and ensure 2D simulated workspace up to date
        self.eval_2d(res.x)
        return res
