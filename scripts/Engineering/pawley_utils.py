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
from collections.abc import Iterable


if TYPE_CHECKING:
    from mantid.dataobjects import Workspace2D
    from scipy.optimize import OptimizeResult

_MIN_SCALE = 1e-6  # floor for per-spectrum scale factors (must be positive)


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
    def __init__(
        self,
        crystal_structure: CrystalStructure,
        hkls: Optional[np.ndarray] = None,
        name: Optional[str] = None,
        hkl_str_delimiter: str = "",
    ):
        self.unit_cell = crystal_structure.getUnitCell()
        self.name = name
        self.hkl_str_delimiter = hkl_str_delimiter
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
    def from_cif(cls, cif_file: str, name: Optional[str] = None, hkl_delimiter: str = ","):
        ws = CreateSingleValuedWorkspace(StoreInADS=False, EnableLogging=False)
        LoadCIF(ws, cif_file, StoreInADS=False)
        return Phase(ws.sample().getCrystalStructure(), name=name, hkl_str_delimiter=hkl_delimiter)

    @classmethod
    def from_alatt(cls, alatt: np.ndarray, spgr: str = "P 1", basis: str = "", name: Optional[str] = None, hkl_delimiter: str = ""):
        alatt_str = " ".join([str(par) for par in alatt])
        xtal = CrystalStructure(alatt_str, spgr, basis)
        return Phase(xtal, name=name, hkl_str_delimiter=hkl_delimiter)

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

    @staticmethod
    def hkl_as_key(hkl) -> tuple:
        return (int(round(hkl.X())), int(round(hkl.Y())), int(round(hkl.Z())))

    def nhkls(self) -> int:
        return len(self.hkls)

    def nparams(self) -> int:
        return len(self.ipars)

    def merge_reflections(self, decimal_places=4):
        _, ihkls = np.unique((self.calc_dspacings() * (10**decimal_places)).astype(int), return_index=True)
        self.hkls = [self.hkls[ipk] for ipk in np.sort(ihkls)]

    def filter_hkls_to_ws_range(self, ws, lambda_min: float = 1.1, lambda_max: float = 5.0) -> Phase:
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
            ),
            name=self.name,
        )
        accessible_hkls = [hkl for hkl in self.hkls if dmin <= self.unit_cell.d(hkl) <= dmax]
        if accessible_hkls:
            new_phase.set_hkls(accessible_hkls, do_sort=False)
        else:
            new_phase.hkls = []
        return new_phase

    def get_hkl_strings(self) -> Sequence[str]:
        return [self.hkl_str_delimiter.join(str(c) for c in self.hkl_as_key(hkl)) for hkl in self.hkls]

    def hkl_string_to_indices(self, hkl_str: str, delimiter: Optional[str] = None) -> Optional[Sequence[int]]:
        # if no delimiter is given uses the one set on the phase
        delim = delimiter or self.hkl_str_delimiter
        # if delimeter is "" we can use the string as the iterable
        iterable = hkl_str.split(delim) if delim else hkl_str
        err_msg = f"Can't convert input string ('{hkl_str}') to H,K, and L indices. Delimiter being used is: '{delim}'"
        try:
            hkls = [int(x) for x in iterable]
            if len(hkls) == 3:
                return hkls
            else:
                logger.error(err_msg)
                return
        except:
            logger.error(err_msg)
            return

    def get_phase_name(self) -> Optional[str]:
        return self.name

    def set_phase_name(self, phase_name: str) -> None:
        self.name = phase_name

    def has_the_same_parameters_as(self, other):
        return set(self.get_param_names()) == set(other.get_param_names())


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

    def get_peak_param_errors(self, param_name: str) -> np.ndarray[float]:
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.getError(param_name) for f in self.comp_func if f.name not in bg_func_names])

    def get_peak_centre_errors(self) -> np.ndarray[float]:
        cen_par_name = self.comp_func[0].function.getCentreParameterName()
        return self.get_peak_param_errors(cen_par_name)

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


class OutputTableMixin(ABC):
    """Mixin that provides ``create_output_tables`` to any Pawley-fit class.

    The mixin creates and names the tables, while concrete classes control their
    schema (``_get_table_columns``) and content (``_populate_table``).  The mixin
    also owns ``get_parameter_errors`` so both constrained and unconstrained fit
    classes share the same error computation.
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

    def _create_table(self, ws_name: str):
        """Create an empty TableWorkspace with the columns returned by ``_get_table_columns``."""
        tab = CreateEmptyTableWorkspace(OutputWorkspace=ws_name, EnableLogging=False)
        for col_type, col_name in self._get_table_columns():
            tab.addColumn(type=col_type, name=col_name)
        return tab

    def _get_table_columns(self) -> list[Tuple[str, str]]:
        """Return ``(type, name)`` pairs for the output table columns.

        Subclasses may override to add or remove columns (e.g. ``Spectrum``).
        """
        return [
            ("str", "Workspace"),
            ("str", "HKL"),
            ("double", "I"),
            ("double", "I_err"),
            ("double", "X0"),
            ("double", "X0_err"),
            ("double", "FWHM"),
            ("double", "FWHM_err"),
        ]

    def create_output_tables(
        self,
        output_workspace: str = None,
    ) -> list:
        """Create one TableWorkspace per phase, each with one row per fitted peak.

        Reads peak data and errors from the object state (set by ``fit()``).

        Parameters
        ----------
        output_workspace : str, optional
            Base name for output workspaces.  Each table is named
            ``<base>_<phase_name>`` (or ``<base>_phase<i>`` when no names
            are provided).
        """
        if hasattr(self, "update_profile_function"):
            self.update_profile_function()

        base_name = output_workspace or f"{self.ws.name()}_pawley_table"
        nphases = len(self.phases) if self.phases else 1

        tables = []
        for iphase in range(nphases):
            phase_name = self.phases[iphase].get_phase_name() if self.phases else None
            suffix = phase_name or f"phase{iphase}"
            tab = self._create_table(f"{base_name}_{suffix}")
            self._populate_table(tab, iphase)
            tables.append(tab)
        return tables

    @abstractmethod
    def _populate_table(self, tab, iphase: int):
        """Add rows to *tab* for the given phase. Subclasses must implement this."""


class BoundsMixin(ABC):
    """Mixin providing flexible parameter bounds.

    Requires subclasses to provide: ``get_param_names()``, ``get_free_params()``,
    ``get_isfree()``, and a ``param_bounds_abs_min`` attribute.
    """

    param_bounds_abs_min = None

    @abstractmethod
    def get_param_names(self) -> np.ndarray:
        pass

    @abstractmethod
    def get_params(self) -> np.ndarray:
        pass

    @abstractmethod
    def get_free_params(self) -> np.ndarray:
        pass

    @abstractmethod
    def get_isfree(self) -> np.ndarray[bool]:
        pass

    def _init_bounds(self):
        self._param_bounds: dict[str, Tuple[float, float]] = {}

    # --- helpers that compute (lb, ub) for a single parameter value --------

    def _fractional_bounds(self, p: float, frac: float) -> Tuple[float, float]:
        margin = max(abs(p) * frac, self.param_bounds_abs_min)
        return p - margin, p + margin

    def _log_bounds(self, p: float, factor: float) -> Tuple[float, float]:
        if p > 0:
            return p / factor, p * factor
        elif p < 0:
            return p * factor, p / factor
        return -self.param_bounds_abs_min, self.param_bounds_abs_min

    @staticmethod
    def _absolute_bounds(lb: float, ub: float) -> Tuple[float, float]:
        return (lb if lb is not None else -np.inf), (ub if ub is not None else np.inf)

    @staticmethod
    def repeat_values(obj, n: int, obj_name: str) -> Optional[Iterable]:
        if not isinstance(obj, Iterable):
            return [obj] * n
        if len(obj) == n:
            return obj
        raise ValueError(f"Length of sequence provided for {obj_name} ({len(obj)}) does not match number of parameter names provided ({n})")

    def set_bounds(
        self,
        names: Optional[str | Sequence[str]] = None,
        mode: str = "fractional",
        values: float | Sequence[float] = None,
        lb: float = None,
        ub: float = None,
    ):
        """Set bounds for specific parameters by name.

        Bounds are computed immediately from the current parameter values and
        stored as ``(lb, ub)`` pairs.

        Parameters
        ----------
        names : str or sequence of str
            Parameter names (as returned by ``get_param_names``).
        mode : ["fractional", "log", "absolute", None]
            "fractional" - symmetric bounds: p +/- max(|p| * value, abs_min)
            "log" - multiplicative bounds: [p / value, p * value]
            "absolute" - explicit bounds given by *lb* and *ub*
            None - remove bounds (unconstrained)
        values : float, optional
            Fractions (for "fractional") or factors (for "log").
        lb, ub : float, optional
            Lower / upper bound (for "absolute").
        """
        if not names:
            return
        if isinstance(names, str):
            names = [names]
        all_names = self.get_param_names()
        all_params = self.get_params()
        name_to_value = dict(zip(all_names, all_params))
        n_params = len(names)

        values = self.repeat_values(values, n_params, "values")
        lb = self.repeat_values(lb, n_params, "lb")
        ub = self.repeat_values(ub, n_params, "ub")

        for i, name in enumerate(names):
            if name not in name_to_value:
                logger.warning(f"set_bounds: '{name}' not found in parameters, ignoring.")
                continue
            match mode:
                case None:
                    self._param_bounds.pop(name, None)
                case "fractional":
                    self._param_bounds[name] = self._fractional_bounds(name_to_value[name], values[i])
                case "log":
                    self._param_bounds[name] = self._log_bounds(name_to_value[name], values[i])
                case "absolute":
                    self._param_bounds[name] = self._absolute_bounds(lb[i], ub[i])
                case _:
                    logger.error(f"Mode: '{mode}' is not a valid option. Options are: ['fractional', 'log', 'absolute', None]")

    def set_bounds_all(self, mode: str = "fractional", value: float = None):
        """Set the same bound mode on every parameter."""
        self.set_bounds(list(self.get_param_names()), mode=mode, values=value)

    def clear_bounds(self, names: str | Sequence[str] = None):
        """Remove bounds for *names*, or all if *names* is ``None``."""
        if names is None:
            self._param_bounds.clear()
        else:
            if isinstance(names, str):
                names = [names]
            for name in names:
                self._param_bounds.pop(name, None)

    def get_bounds(self) -> Tuple[np.ndarray, np.ndarray]:
        """Assemble ``(lb, ub)`` arrays for the free parameters from stored bounds."""
        isfree = self.get_isfree()
        free_names = self.get_param_names()[isfree]
        n = len(free_names)

        lo = np.full(n, -np.inf)
        hi = np.full(n, np.inf)

        for i, name in enumerate(free_names):
            bounds = self._param_bounds.get(name)
            if bounds is not None:
                lo[i], hi[i] = bounds

        return lo, hi

    def set_non_negative(self, names: str | Sequence[str]):
        """Enforce a lower bound of zero on the named parameters.

        If the parameter already has bounds, the lower bound is raised to zero.
        If it has no bounds, bounds of [0, +inf] are set.
        """
        if isinstance(names, str):
            names = [names]
        for name in names:
            existing = self._param_bounds.get(name)
            if existing is not None:
                self._param_bounds[name] = (max(existing[0], 0.0), existing[1])
            else:
                self._param_bounds[name] = (0.0, np.inf)


class PawleyPatternBase(BoundsMixin, MtdFuncMixin, OutputTableMixin, ABC):
    def __init__(
        self,
        ws: Workspace2D,
        phases: Phase,
        profile: PeakProfile,
        bg_func: Optional[FunctionWrapper] = None,
        param_bounds_abs_min: float = 1e-6,
    ):
        self.ws = ws
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
        self.param_errors = np.full_like(self.get_params(), np.nan)
        self.param_bounds_abs_min = param_bounds_abs_min
        self._init_bounds()

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
            for ipar, par in enumerate(self.bg_params):
                self.comp_func[len(self.comp_func) - 1].function.setParameter(ipar, par)

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

    def get_intens_param_names(self) -> list[str]:
        """Return the names of all intensity parameters across phases."""
        names = []
        for i, phase in enumerate(self.phases):
            phase_suffix = f"_ph{i}" if len(self.phases) > 1 else ""
            names.extend(s + phase_suffix for s in phase.get_hkl_strings())
        return names

    def force_non_negative_intensities(self):
        """Enforce a lower bound of zero on all intensity parameters."""
        self.set_non_negative(self.get_intens_param_names())

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
            kwargs["bounds"] = self.get_bounds()
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters and errors
        self.set_free_params(res.x)
        self._set_errors_from_result(res)
        self.update_profile_function()
        return res

    def _set_errors_from_result(self, res: "OptimizeResult"):
        free_errors = self.get_parameter_errors(res)
        self.param_errors = np.full_like(self.get_params(), np.nan)
        self.param_errors[self.get_isfree()] = free_errors

    def undo_fit(self):
        if self.initial_params is not None:
            self.set_free_params(self.initial_params)

    def _get_intens_error(self, iphase: int) -> np.ndarray:
        """Return the intensity errors for a single phase from self.param_errors"""
        n_alatt = sum(a.size for a in self.alatt_params)
        offset = n_alatt + sum(self.intens[i].size for i in range(iphase))
        return self.param_errors[offset : offset + self.intens[iphase].size]

    def _populate_table(self, tab, iphase: int):
        phase = self.phases[iphase]
        pk_offset = sum(self.phases[i].nhkls() for i in range(iphase))
        intens_errors = self._get_intens_error(iphase)
        hkl_strings = phase.get_hkl_strings()
        for ipk in range(phase.nhkls()):
            pk_func = self.comp_func[pk_offset + ipk].function
            tab.addRow(self._make_peak_row(iphase, ipk, hkl_strings[ipk], pk_func, intens_errors[ipk]))

    def _make_peak_row(self, iphase, ipk, hkl_str, pk_func, intens_err):
        cen_par = pk_func.getCentreParameterName()
        return {
            "Workspace": self.ws.name(),
            "HKL": hkl_str,
            "I": float(self.intens[iphase][ipk]),
            "I_err": float(intens_err),
            "X0": float(pk_func.getParameterValue(cen_par)),
            "X0_err": float("nan"),
            "FWHM": float(pk_func.fwhm()),
            "FWHM_err": float("nan"),
        }

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
        self.set_ispec(0)
        self.update_profile_function()

    def set_ispec(self, ispec: int):
        self.ispec = ispec
        if self.xunit != "dSpacing":
            si = self.ws.spectrumInfo()
            if not si.hasDetectors(self.ispec):
                raise RuntimeError("Workspace has no detectors - may not be able to convert to d-spacing.")

    def _get_table_columns(self) -> list[Tuple[str, str]]:
        return [
            ("str", "Workspace"),
            ("int", "Spectrum"),
            ("str", "HKL"),
            ("double", "I"),
            ("double", "I_err"),
            ("double", "X0"),
            ("double", "X0_err"),
            ("double", "FWHM"),
            ("double", "FWHM_err"),
        ]

    def _make_peak_row(self, iphase, ipk, hkl_str, pk_func, intens_err):
        row = super()._make_peak_row(iphase, ipk, hkl_str, pk_func, intens_err)
        row["Spectrum"] = self.ispec
        return row

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
        # update parameters in comp_func (can't replace comp_func as result doesn't return individual IPeakFunctions)
        for ipar in range(self.comp_func.nParams()):
            self.comp_func.setParameter(ipar, res.Function.getParamValue(ipar))
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
        for ipar, param in enumerate(p):
            func.function.setParameter(ipar, param)

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
                # First call (or after reset by the outer loop): jointly estimate the
                # per-spectrum scale and background via linear regression on the current ycalc.
                # Both are then locked for the duration of the inner Pawley optimisation
                self.scales = np.zeros(self.ws.getNumberHistograms())
                self.bgs = np.zeros_like(self.scales)
                ones = np.ones_like(ws_sim.readY(0))
                for ispec in range(self.ws.getNumberHistograms()):
                    yobs = self.ws.readY(ispec)
                    ycalc = ws_sim.readY(ispec)
                    A = np.column_stack([ycalc, ones])
                    result, _, _, _ = np.linalg.lstsq(A, yobs, rcond=None)
                    scale = float(max(result[0], _MIN_SCALE))  # physically non-negative
                    bg = float(result[1])
                    self.scales[ispec] = scale
                    self.bgs[ispec] = bg
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
        **kwargs,
    ):
        super().__init__(*args, **kwargs)
        self.lambda_max = lambda_max
        self.scales = None
        self.bgs = None
        self.apply_lorentz_correction = apply_lorentz_correction
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
            # Zero and fix any global background — background is handled per-spectrum in eval_2d.
            if len(self.bg_params) > 0:
                self.bg_params[:] = 0
                self.bg_isfree[:] = False

    def set_params_from_pawley1d(
        self,
        pawley1d: PawleyPattern1D,
    ):
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

    def estimate_initial_params(self):
        self._estimate_intensities()

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws_1d.readX(0))

    def _estimate_intensities(self):
        # Compute per-spectrum scales at current intensities and use their median as a global
        # correction factor.
        self.scales = None  # ensure fresh per-spectrum scale computation
        _ = self.eval_2d(self.get_free_params())
        if self.scales is not None and len(self.scales) > 0:
            valid_scales = self.scales[self.scales > 0]
            global_scale = float(np.median(valid_scales)) if len(valid_scales) > 0 else 1.0
            for iphase in range(len(self.phases)):
                self.intens[iphase] *= global_scale
        self.scales = None  # reset so fit() recomputes scales at the updated intensities

    def create_no_constraints_fit(self, param_bounds_abs_min: float = 1e-6):
        return PawleyPattern2DNoConstraints(
            self.ws,
            self.comp_func,
            self.global_scale,
            self.lambda_max,
            param_bounds_abs_min=param_bounds_abs_min,
            apply_lorentz_correction=self.apply_lorentz_correction,
            phases=self.phases,
        )

    def fit(
        self,
        *args,
        fit_strategy: str = "alternating",
        max_scale_iter: int = 2,
        scale_rtol: float = 1e-3,
        **kwargs,
    ) -> OptimizeResult:
        """Fit the 2D Pawley pattern.

        Parameters
        ----------
        fit_strategy : str
            ``"alternating"`` — alternate between per-spectrum scale/background
            estimation (via linear regression) and Bragg-intensity optimisation.
            ``"single"`` — compute per-spectrum scales once, then run a single
            least-squares optimisation (no outer iteration).
        max_scale_iter : int
            Maximum number of outer iterations (only used by ``"alternating"``).
        scale_rtol : float
            Relative tolerance on the cost function between successive outer
            iterations.  The loop breaks early when the fractional change in
            cost drops below this threshold (only used by ``"alternating"``).
        """
        if fit_strategy == "alternating":
            res = self._fit_alternating(*args, max_scale_iter=max_scale_iter, scale_rtol=scale_rtol, **kwargs)
        elif fit_strategy == "single":
            self.scales = None
            self.eval_2d(self.get_free_params())
            res = super().fit(*args, **kwargs)
            self.set_free_params(res.x)
        else:
            raise ValueError(f"Unknown fit_strategy '{fit_strategy}'. Options: 'alternating', 'single'.")
        self.eval_2d(res.x)  # ensure 2D simulated workspace up to date
        return res

    def _fit_alternating(self, *args, max_scale_iter: int = 2, scale_rtol: float = 1e-3, **kwargs) -> OptimizeResult:
        res = None
        prev_cost = None
        for _ in range(max_scale_iter):
            self.scales = None  # force scale+bg re-estimation from current params
            self.eval_2d(self.get_free_params())
            res = super().fit(*args, **kwargs)
            self.set_free_params(res.x)
            if prev_cost is not None and abs(prev_cost - res.cost) / max(prev_cost, 1e-12) < scale_rtol:
                break
            prev_cost = res.cost
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
        self.scales = None
        self.bgs = None
        self.apply_lorentz_correction = apply_lorentz_correction
        self.set_global_scale(global_scale)
        self.initial_params = None
        self.param_errors = np.full(self.comp_func.nParams(), np.nan)
        self.param_bounds_abs_min = param_bounds_abs_min
        self._init_bounds()

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
        self.scales = None
        self.eval_2d(self.initial_params)
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters, errors, and ensure 2D simulated workspace up to date
        self.param_errors = np.full(self.comp_func.nParams(), np.nan)
        self.param_errors[self.get_isfree()] = self.get_parameter_errors(res)
        self.eval_2d(res.x)
        return res
